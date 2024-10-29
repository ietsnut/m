#include <cosmo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <dirent.h>
#include <termios.h>
#include "libc/dce.h"

// STK500v1 constants
#define STK_START           0x1B
#define STK_GET_SYNC       'A'
#define STK_GET_SIGN_ON    'B'
#define STK_SET_PARAMETER  'A'
#define STK_GET_PARAMETER  'B'
#define STK_SET_DEVICE     'E'
#define STK_ENTER_PROGMODE 'P'
#define STK_LEAVE_PROGMODE 'Q'
#define STK_CHIP_ERASE     'R'
#define STK_CHECK_AUTOINC  'S'
#define STK_LOAD_ADDRESS   'U'
#define STK_PROG_FLASH     'V'
#define STK_PROG_DATA      'W'
#define STK_PROG_FUSE      'X'
#define STK_PROG_LOCK      'Y'
#define STK_READ_FLASH     'Z'
#define STK_READ_DATA      '['
#define STK_READ_FUSE      '\\'
#define STK_READ_LOCK      ']'
#define STK_READ_PAGE      '^'
#define STK_READ_SIGN      '_'
#define STK_UNIVERSAL      'V'
#define STK_SW_MAJOR       '`'
#define STK_SW_MINOR       'a'

#define STK_OK              0x10
#define STK_FAILED          0x11
#define STK_UNKNOWN         0x12
#define STK_NODEVICE        0x13
#define STK_INSYNC          0x14
#define STK_NOSYNC          0x15

#define CRC_EOP             0x20

// Intel HEX record types
#define DATA_RECORD         0x00
#define END_OF_FILE         0x01
#define EXT_SEGMENT_ADDR    0x02
#define START_SEGMENT_ADDR  0x03
#define EXT_LINEAR_ADDR     0x04
#define START_LINEAR_ADDR   0x05

// ATtiny85 specific
#define ATTINY85_SIGNATURE_0 0x1E
#define ATTINY85_SIGNATURE_1 0x93
#define ATTINY85_SIGNATURE_2 0x0B

// Timeouts and delays
#define INIT_DELAY_US       2000000
#define STABILIZE_DELAY_US  50000
#define READ_TIMEOUT_US     500000
#define SYNC_ATTEMPTS       3
#define PAGE_SIZE           64  // ATtiny85 page size

// Buffer sizes
#define MAX_PORTS           64
#define PORT_NAME_LENGTH    32
#define HEX_LINE_LENGTH     256
#define RESPONSE_BUFFER     275

typedef struct {
    unsigned char data[PAGE_SIZE];
    unsigned int address;
    size_t length;
} Page;

int wait_for_data(int fd, int timeout_us) {
    fd_set readfds;
    struct timeval tv;
    
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    
    tv.tv_sec = timeout_us / 1000000;
    tv.tv_usec = timeout_us % 1000000;
    
    return select(fd + 1, &readfds, NULL, NULL, &tv);
}

int read_with_timeout(int fd, unsigned char *buf, size_t size, int timeout_us) {
    int result = wait_for_data(fd, timeout_us);
    if (result > 0) {
        return read(fd, buf, size);
    }
    return result;
}

int configure_port(const char *port) {
    if (IsWindows()) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), 
            "cmd.exe /c mode %s: BAUD=19200 PARITY=N DATA=8 STOP=1 dtr=on rts=on", port);
        return system(cmd);
    }
    return 0;
}

int open_port(const char *port) {
    if (IsWindows()) {
        if (configure_port(port) != 0) {
            return -1;
        }
        
        // Open port with both read and write access
        int fd = open(port, O_RDWR | O_NONBLOCK);
        if (fd < 0) return -1;

        // Set DTR and RTS using Windows API
        HANDLE hComm = (HANDLE)_get_osfhandle(fd);
        if (hComm == INVALID_HANDLE_VALUE) {
            close(fd);
            return -1;
        }

        // Assert DTR and RTS
        EscapeCommFunction(hComm, SETDTR);
        EscapeCommFunction(hComm, SETRTS);
        
        // Reset pulse
        usleep(250000);  // 250ms delay
        EscapeCommFunction(hComm, CLRDTR);
        usleep(50000);   // 50ms delay
        EscapeCommFunction(hComm, SETDTR);

        return fd;
    } else {
        int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0) return -1;

        struct termios tty;
        memset(&tty, 0, sizeof(tty));
        if (tcgetattr(fd, &tty) != 0) {
            close(fd);
            return -1;
        }

        cfsetispeed(&tty, B19200);
        cfsetospeed(&tty, B19200);
        tty.c_cflag |= (CS8 | CLOCAL | CREAD);
        tty.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);
        tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_oflag &= ~OPOST;

        // Set DTR and RTS on POSIX systems
        int bits;
        ioctl(fd, TIOCMGET, &bits);
        bits |= TIOCM_DTR | TIOCM_RTS;
        ioctl(fd, TIOCMSET, &bits);

        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
            close(fd);
            return -1;
        }

        // Reset pulse
        bits &= ~TIOCM_DTR;
        ioctl(fd, TIOCMSET, &bits);
        usleep(50000);  // 50ms delay
        bits |= TIOCM_DTR;
        ioctl(fd, TIOCMSET, &bits);

        return fd;
    }
}

int get_available_ports(char ports[][PORT_NAME_LENGTH]) {
    int count = 0;

    if (IsWindows()) {
        // Windows: Check COM1 through COM20
        for (int i = 1; i <= 20 && count < MAX_PORTS; i++) {
            snprintf(ports[count], PORT_NAME_LENGTH, "COM%d", i);
            int fd = open(ports[count], O_RDWR | O_NONBLOCK);
            if (fd >= 0) {
                close(fd);
                count++;
            }
        }
    } else if (IsXnu()) {
        // macOS: Check /dev/cu.* devices
        DIR *dir = opendir("/dev");
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL && count < MAX_PORTS) {
                if (strncmp(entry->d_name, "cu.", 3) == 0) {
                    snprintf(ports[count], PORT_NAME_LENGTH, "/dev/%s", entry->d_name);
                    count++;
                }
            }
            closedir(dir);
        }
    } else if (IsLinux()) {
        // Linux: Check /dev/ttyUSB* and /dev/ttyACM*
        DIR *dir = opendir("/dev");
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL && count < MAX_PORTS) {
                if (strncmp(entry->d_name, "ttyUSB", 6) == 0 ||
                    strncmp(entry->d_name, "ttyACM", 6) == 0) {
                    snprintf(ports[count], PORT_NAME_LENGTH, "/dev/%s", entry->d_name);
                    count++;
                }
            }
            closedir(dir);
        }
    } else if (IsBsd()) {
        // BSD: Check /dev/cuaU* devices
        DIR *dir = opendir("/dev");
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL && count < MAX_PORTS) {
                if (strncmp(entry->d_name, "cuaU", 4) == 0) {
                    snprintf(ports[count], PORT_NAME_LENGTH, "/dev/%s", entry->d_name);
                    count++;
                }
            }
            closedir(dir);
        }
    }

    return count;
}

int send_command(int fd, unsigned char *cmd, size_t cmd_len, unsigned char *response, size_t resp_len) {
    if (write(fd, cmd, cmd_len) != cmd_len) {
        return 0;
    }
    
    usleep(STABILIZE_DELAY_US);
    
    int bytes_read = read_with_timeout(fd, response, resp_len, READ_TIMEOUT_US);
    if (bytes_read < 2 || response[0] != STK_INSYNC || response[bytes_read-1] != STK_OK) {
        return 0;
    }
    
    return bytes_read;
}

int sync_programmer(int fd) {
    // Add manual sync sequence
    if (IsWindows()) {
        HANDLE hComm = (HANDLE)_get_osfhandle(fd);
        EscapeCommFunction(hComm, CLRDTR);
        usleep(50000);
        EscapeCommFunction(hComm, SETDTR);
    } else {
        int bits;
        ioctl(fd, TIOCMGET, &bits);
        bits &= ~TIOCM_DTR;
        ioctl(fd, TIOCMSET, &bits);
        usleep(50000);
        bits |= TIOCM_DTR;
        ioctl(fd, TIOCMSET, &bits);
    }
    
    // Clear any pending data
    unsigned char buf[128];
    while (read_with_timeout(fd, buf, sizeof(buf), 100000) > 0) {
        // Discard data
    }
    
    unsigned char cmd[] = {STK_GET_SYNC, CRC_EOP};
    unsigned char response[2];
    
    for (int i = 0; i < SYNC_ATTEMPTS; i++) {
        if (write(fd, cmd, sizeof(cmd)) == sizeof(cmd)) {
            usleep(STABILIZE_DELAY_US);
            int bytes_read = read_with_timeout(fd, response, sizeof(response), READ_TIMEOUT_US);
            if (bytes_read == 2 && response[0] == STK_INSYNC && response[1] == STK_OK) {
                return 1;
            }
        }
        usleep(STABILIZE_DELAY_US);
    }
    return 0;
}

int get_programmer_version(int fd) {
    unsigned char cmd[] = {'1', CRC_EOP};
    unsigned char response[7];  // STK_INSYNC + "AVR ISP" + STK_OK
    
    if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
        return 0;
    }
    
    usleep(STABILIZE_DELAY_US);
    
    int bytes_read = read_with_timeout(fd, response, sizeof(response), READ_TIMEOUT_US);
    return (bytes_read >= 2 && response[0] == STK_INSYNC);
}

int check_arduinoisp(int fd) {
    if (!sync_programmer(fd)) {  // Changed from sync_with_programmer to sync_programmer
        return 0;
    }
    return get_programmer_version(fd);
}

int set_device_parameters(int fd) {
    // ATtiny85 programming parameters
    unsigned char set_device_cmd[] = {
        STK_SET_DEVICE, 
        0x14,  // device descriptor length
        0x42,  // ATTINY85 device code
        0x86,  // revision
        0x00,  // prog type
        0x01,  // parallelmode
        0x01,  // polling
        0x01,  // self-timed
        0x01,  // lock bytes
        0x03,  // fuse bytes
        0xFF,  // flash poll value 1
        0xFF,  // flash poll value 2
        0x00,  // eeprom poll value 1
        0xFF,  // eeprom poll value 2
        0xFF,  // page size
        0x00,  // eeprom page size
        0x00, 0x80,  // flash size
        0x00, 0x04,  // eeprom size
        CRC_EOP
    };
    
    unsigned char response[2];
    return send_command(fd, set_device_cmd, sizeof(set_device_cmd), response, sizeof(response));
}

int wait_for_response(int fd) {
    unsigned char response[2];
    int bytes_read = read_with_timeout(fd, response, 2, READ_TIMEOUT_US);
    return (bytes_read == 2 && response[0] == STK_INSYNC && response[1] == STK_OK);
}

int enter_program_mode(int fd) {
    unsigned char cmd[] = {STK_ENTER_PROGMODE, CRC_EOP};
    unsigned char response[2];
    return send_command(fd, cmd, sizeof(cmd), response, sizeof(response));
}

int leave_program_mode(int fd) {
    unsigned char cmd[] = {STK_LEAVE_PROGMODE, CRC_EOP};
    unsigned char response[2];
    return send_command(fd, cmd, sizeof(cmd), response, sizeof(response));
}

int universal_command(int fd, unsigned char cmd0, unsigned char cmd1, unsigned char cmd2, unsigned char cmd3) {
    unsigned char cmd[] = {STK_UNIVERSAL, cmd0, cmd1, cmd2, cmd3, CRC_EOP};
    unsigned char response[3];
    return send_command(fd, cmd, sizeof(cmd), response, sizeof(response));
}

int check_signature(int fd) {
    unsigned char sig0, sig1, sig2;
    
    // Read signature bytes
    universal_command(fd, 0x30, 0x00, 0x00, 0x00);
    universal_command(fd, 0x30, 0x00, 0x01, 0x00);
    universal_command(fd, 0x30, 0x00, 0x02, 0x00);
    
    return (sig0 == ATTINY85_SIGNATURE_0 &&
            sig1 == ATTINY85_SIGNATURE_1 &&
            sig2 == ATTINY85_SIGNATURE_2);
}

int load_address(int fd, unsigned int addr) {
    unsigned char cmd[] = {
        STK_LOAD_ADDRESS,
        (unsigned char)(addr & 0xFF),
        (unsigned char)((addr >> 8) & 0xFF),
        CRC_EOP
    };
    unsigned char response[2];
    return send_command(fd, cmd, sizeof(cmd), response, sizeof(response));
}

int program_page(int fd, Page *page) {
    unsigned char cmd[PAGE_SIZE + 5];
    cmd[0] = STK_PROG_FLASH;
    cmd[1] = (page->length >> 8) & 0xFF;
    cmd[2] = page->length & 0xFF;
    cmd[3] = 'F';  // Flash memory
    memcpy(&cmd[4], page->data, page->length);
    cmd[page->length + 4] = CRC_EOP;

    unsigned char response[2];
    return send_command(fd, cmd, page->length + 5, response, sizeof(response));
}

int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

int parse_hex_byte(const char *str) {
    int high = hex_char_to_int(str[0]);
    int low = hex_char_to_int(str[1]);
    if (high < 0 || low < 0) return -1;
    return (high << 4) | low;
}

int process_hex_line(const char *line, Page *current_page, unsigned int *base_addr) {
    if (line[0] != ':') return 0;
    
    int length = parse_hex_byte(line + 1);
    if (length < 0) return 0;
    
    int addr = (parse_hex_byte(line + 3) << 8) | parse_hex_byte(line + 5);
    int record_type = parse_hex_byte(line + 7);
    
    switch (record_type) {
        case DATA_RECORD: {
            unsigned int full_addr = (*base_addr | addr) / 2;  // Convert byte address to word address
            
            if (current_page->length == 0) {
                current_page->address = full_addr;
            }
            
            for (int i = 0; i < length; i++) {
                int byte = parse_hex_byte(line + 9 + (i * 2));
                if (byte < 0) return 0;
                current_page->data[current_page->length++] = byte;
                
                if (current_page->length >= PAGE_SIZE) {
                    return 2;  // Page full
                }
            }
            return 1;
        }
        
        case EXT_LINEAR_ADDR: {
            *base_addr = (parse_hex_byte(line + 9) << 24) | (parse_hex_byte(line + 11) << 16);
            return 1;
        }
        
        case END_OF_FILE:
            return current_page->length > 0 ? 2 : 1;
            
        default:
            return 1;  // Skip other record types
    }
}

int upload_hex_file(int fd, const char *filename) {
    if (!sync_programmer(fd)) {
        printf("Failed to sync with programmer\n");
        return 0;
    }
    
    if (!set_device_parameters(fd)) {
        printf("Failed to set device parameters\n");
        return 0;
    }
    
    if (!enter_program_mode(fd)) {
        printf("Failed to enter programming mode\n");
        return 0;
    }
    
    if (!check_signature(fd)) {
        printf("Device signature mismatch - expected ATtiny85\n");
        leave_program_mode(fd);
        return 0;
    }
    
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Failed to open hex file: %s\n", strerror(errno));
        leave_program_mode(fd);
        return 0;
    }
    
    char line[HEX_LINE_LENGTH];
    Page current_page = {0};
    unsigned int base_addr = 0;
    int total_bytes = 0;
    
    // [Rest of the hex file processing remains the same]
    while (fgets(line, sizeof(line), fp)) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = 0;
        newline = strchr(line, '\r');
        if (newline) *newline = 0;
        
        int result = process_hex_line(line, &current_page, &base_addr);
        if (!result) {
            printf("Error processing hex file line: %s\n", line);
            fclose(fp);
            leave_program_mode(fd);
            return 0;
        }
        
        if (result == 2 || feof(fp)) {
            if (!load_address(fd, current_page.address)) {
                printf("Failed to load address 0x%04X\n", current_page.address);
                fclose(fp);
                leave_program_mode(fd);
                return 0;
            }
            
            if (!program_page(fd, &current_page)) {
                printf("Failed to program page at address 0x%04X\n", current_page.address);
                fclose(fp);
                leave_program_mode(fd);
                return 0;
            }
            
            total_bytes += current_page.length;
            printf("Programmed %d bytes at address 0x%04X\n", current_page.length, current_page.address);
            
            current_page.length = 0;
        }
    }
    
    printf("Successfully programmed %d bytes total\n", total_bytes);
    fclose(fp);
    return leave_program_mode(fd);
}

int main() {
    char ports[MAX_PORTS][PORT_NAME_LENGTH];
    int port_count = get_available_ports(ports);
    
    printf("Found %d potential serial ports\n", port_count);
    
    for (int i = 0; i < port_count; i++) {
        printf("\nTrying port %s...\n", ports[i]);
        
        int fd = open_port(ports[i]);
        if (fd < 0) {
            printf("Failed to open %s: %s\n", ports[i], strerror(errno));
            continue;
        }
        
        printf("Opened %s successfully\n", ports[i]);
        usleep(INIT_DELAY_US);
        
        printf("Attempting to upload blink.hex using STK500v1 protocol...\n");
        if (upload_hex_file(fd, "blink.hex")) {
            printf("Upload completed successfully!\n");
            close(fd);
            return 0;
        } else {
            printf("Upload failed\n");
            close(fd);
        }
    }
    
    printf("\nFailed to upload hex file on any port\n");
    return 1;
}