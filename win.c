#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

#define STK_OK 0x10
#define STK_FAILED 0x11
#define STK_UNKNOWN 0x12
#define STK_NODEVICE 0x13
#define STK_INSYNC 0x14
#define STK_NOSYNC 0x15
#define CRC_EOP 0x20

// STK500 commands we'll use
#define STK_GET_SYNC 0x30
#define STK_SET_DEVICE 0x42
#define STK_ENTER_PROGMODE 0x50
#define STK_LOAD_ADDRESS 0x55
#define STK_PROG_PAGE 0x64
#define STK_LEAVE_PROGMODE 0x51

int open_serial(const char *port) {
    int fd = open(port, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        fprintf(stderr, "Error opening %s: %s\n", port, strerror(errno));
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(fd, &tty) != 0) {
        fprintf(stderr, "Error from tcgetattr: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;

    tty.c_cc[VTIME] = 10;  // 1 second timeout
    tty.c_cc[VMIN] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Error from tcsetattr: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

int send_command(int fd, uint8_t cmd, uint8_t *data, int len, uint8_t *response, int resp_len) {
    uint8_t sync_cmd[] = {cmd};
    write(fd, sync_cmd, 1);
    
    if (data && len > 0) {
        write(fd, data, len);
    }
    
    uint8_t eop[] = {CRC_EOP};
    write(fd, eop, 1);

    if (response && resp_len > 0) {
        int n = read(fd, response, resp_len);
        if (n != resp_len) {
            fprintf(stderr, "Error: Expected %d bytes, got %d\n", resp_len, n);
            return -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <serial port> <hex file>\n", argv[0]);
        return 1;
    }

    int fd = open_serial(argv[1]);
    if (fd < 0) return 1;

    printf("Synchronizing...\n");
    uint8_t response[2];
    for (int i = 0; i < 5; i++) {
        if (send_command(fd, STK_GET_SYNC, NULL, 0, response, 2) == 0) {
            if (response[0] == STK_INSYNC && response[1] == STK_OK) {
                printf("Synchronized!\n");
                break;
            }
        }
        usleep(50000);
        if (i == 4) {
            fprintf(stderr, "Failed to synchronize with programmer\n");
            close(fd);
            return 1;
        }
    }

    // Set device parameters for ATtiny85
    printf("Setting device parameters for ATtiny85...\n");
    uint8_t device_params[] = {
        0x1E, // devicecode: ATtiny85
        0x00, // revision
        0x00, // progtype
        0x01, // parmode
        0x01, // polling
        0x01, // selftimed
        0x01, // lockbytes
        0x03, // fusebytes (3 fuse bytes for ATtiny85)
        0xFF, // flashpollval1
        0xFF, // flashpollval2
        0xFF, // eeprompollval1
        0xFF, // eeprompollval2
        0x00, // pagesizehigh
        0x20, // pagesizelow (32 bytes page size for ATtiny85)
        0x00, // eepromsizehigh
        0x80, // eepromsizelow (512 bytes EEPROM)
        0x00, // flashsize4
        0x00, // flashsize3
        0x20, // flashsize2
        0x00  // flashsize1 (8K flash = 0x2000 bytes)
    };
    
    if (send_command(fd, STK_SET_DEVICE, device_params, sizeof(device_params), response, 2) < 0) {
        fprintf(stderr, "Failed to set device parameters\n");
        close(fd);
        return 1;
    }

    printf("Entering programming mode...\n");
    if (send_command(fd, STK_ENTER_PROGMODE, NULL, 0, response, 2) < 0) {
        fprintf(stderr, "Failed to enter programming mode\n");
        close(fd);
        return 1;
    }

    FILE *hex_file = fopen(argv[2], "rb");
    if (!hex_file) {
        fprintf(stderr, "Error opening hex file: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    char line[256];
    uint16_t address = 0;
    uint8_t page_buffer[32];  // ATtiny85 has 32-byte pages
    int page_size = 32;       // ATtiny85 page size
    
    printf("Programming flash...\n");
    while (fgets(line, sizeof(line), hex_file)) {
        if (line[0] != ':') continue;
        
        int len, addr, type;
        sscanf(line + 1, "%02x%04x%02x", &len, &addr, &type);
        
        if (type == 0) {
            uint8_t addr_data[] = {addr & 0xFF, (addr >> 8) & 0xFF};
            if (send_command(fd, STK_LOAD_ADDRESS, addr_data, 2, response, 2) < 0) {
                fprintf(stderr, "Failed to set address\n");
                break;
            }
            
            uint8_t prog_data[page_size + 4];
            prog_data[0] = 0x00;
            prog_data[1] = (page_size >> 8) & 0xFF;
            prog_data[2] = page_size & 0xFF;
            prog_data[3] = 'F';
            
            for (int i = 0; i < len; i++) {
                int byte;
                sscanf(line + 9 + (i * 2), "%02x", &byte);
                prog_data[4 + i] = byte;
            }
            
            if (send_command(fd, STK_PROG_PAGE, prog_data, len + 4, response, 2) < 0) {
                fprintf(stderr, "Failed to program page\n");
                break;
            }
            
            printf(".");
            fflush(stdout);
        }
    }
    printf("\nProgramming complete!\n");
    
    fclose(hex_file);

    printf("Leaving programming mode...\n");
    if (send_command(fd, STK_LEAVE_PROGMODE, NULL, 0, response, 2) < 0) {
        fprintf(stderr, "Failed to leave programming mode\n");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}