#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

// Constants for port and hex file
#define SERIAL_PORT "/dev/cu.usbmodem1101"
#define HEX_FILE "blink.hex"

// STK500 constants
#define STK_OK              0x10
#define STK_FAILED          0x11
#define STK_INSYNC          0x14
#define STK_NOSYNC          0x15
#define STK_GET_SYNC        0x30
#define STK_ENTER_PROGMODE  0x50
#define STK_LOAD_ADDRESS    0x55
#define STK_PROG_PAGE       0x64
#define STK_LEAVE_PROGMODE  0x51
#define CRC_EOP             0x20

int open_serial(const char* port) {
    int serial_port = open(port, O_RDWR);
    if (serial_port < 0) {
        perror("Unable to open serial port");
        exit(1);
    }

    struct termios tty;
    if (tcgetattr(serial_port, &tty) != 0) {
        perror("tcgetattr error");
        close(serial_port);
        exit(1);
    }

    cfsetispeed(&tty, B19200);
    cfsetospeed(&tty, B19200);
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ISIG;
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN] = 0;    // Changed to non-blocking
    tty.c_cc[VTIME] = 100; // 10 second timeout

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        perror("tcsetattr error");
        close(serial_port);
        exit(1);
    }
    return serial_port;
}

int get_sync(int serial) {
    unsigned char sync_cmd[] = {STK_GET_SYNC, CRC_EOP};
    unsigned char response[2];
    int retry_count = 0;
    
    while (retry_count < 3) {
        // Flush any existing data
        tcflush(serial, TCIOFLUSH);
        
        // Send sync command
        write(serial, sync_cmd, sizeof(sync_cmd));
        usleep(100000); // Wait 100ms
        
        // Read response
        int bytes_read = read(serial, response, 2);
        if (bytes_read == 2 && response[0] == STK_INSYNC && response[1] == STK_OK) {
            return 0;
        }
        
        retry_count++;
        usleep(200000); // Wait 200ms before retry
    }
    return -1;
}

void stk_send(int serial, unsigned char *cmd, int len) {
    // Add CRC_EOP to the end of every command
    write(serial, cmd, len);
    write(serial, &(unsigned char){CRC_EOP}, 1);
    
    // Read two bytes: INSYNC and OK
    unsigned char response[2];
    read(serial, response, 2);
    
    if (response[0] != STK_INSYNC || response[1] != STK_OK) {
        fprintf(stderr, "STK500 command failed with response: 0x%02X 0x%02X\n", 
                response[0], response[1]);
        exit(1);
    }
}

void stk500_program(int serial, const char* hex_file_path) {
    // First establish sync
    printf("Establishing sync with programmer...\n");
    if (get_sync(serial) != 0) {
        fprintf(stderr, "Failed to sync with programmer\n");
        exit(1);
    }
    printf("Sync established\n");

    FILE* hex_file = fopen(hex_file_path, "r");
    if (!hex_file) {
        perror("Error opening hex file");
        exit(1);
    }

    printf("Entering programming mode...\n");
    unsigned char enter_prog[] = {STK_ENTER_PROGMODE};
    stk_send(serial, enter_prog, sizeof(enter_prog));

    char line[256];
    int total_bytes = 0;
    printf("Programming flash memory...\n");
    
    while (fgets(line, sizeof(line), hex_file)) {
        if (line[0] != ':') continue;  // Invalid line
        
        int byte_count, address, record_type;
        sscanf(line + 1, "%02x%04x%02x", &byte_count, &address, &record_type);

        if (record_type == 1) break;  // End of file record

        unsigned char load_address[] = {STK_LOAD_ADDRESS, (address >> 8) & 0xFF, address & 0xFF};
        stk_send(serial, load_address, sizeof(load_address));

        unsigned char data[byte_count];
        for (int i = 0; i < byte_count; i++) {
            int value;
            sscanf(line + 9 + (i * 2), "%02x", &value);
            data[i] = (unsigned char)value;
        }

        unsigned char prog_page[4 + byte_count];
        prog_page[0] = STK_PROG_PAGE;
        prog_page[1] = (byte_count >> 8) & 0xFF;
        prog_page[2] = byte_count & 0xFF;
        prog_page[3] = 'F';  // Flash memory
        memcpy(&prog_page[4], data, byte_count);
        stk_send(serial, prog_page, sizeof(prog_page));
        
        total_bytes += byte_count;
        printf("Wrote %d bytes at address 0x%04X\r", byte_count, address);
        fflush(stdout);
    }

    printf("\nTotal bytes programmed: %d\n", total_bytes);
    printf("Leaving programming mode...\n");
    unsigned char leave_prog[] = {STK_LEAVE_PROGMODE};
    stk_send(serial, leave_prog, sizeof(leave_prog));

    fclose(hex_file);
    printf("Programming complete!\n");
}

int main() {
    printf("Opening serial port %s...\n", SERIAL_PORT);
    int serial = open_serial(SERIAL_PORT);
    stk500_program(serial, HEX_FILE);
    close(serial);
    return 0;
}