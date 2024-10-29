#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

// STK500 constants
#define STK_OK               0x10
#define STK_ENTER_PROGMODE   0x50
#define STK_LOAD_ADDRESS     0x55
#define STK_PROG_PAGE        0x64
#define STK_LEAVE_PROGMODE   0x51

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
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 5;

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        perror("tcsetattr error");
        close(serial_port);
        exit(1);
    }
    return serial_port;
}

void stk_send(int serial, unsigned char *cmd, int len) {
    write(serial, cmd, len);
    unsigned char response;
    read(serial, &response, 1);
    if (response != STK_OK) {
        fprintf(stderr, "STK500 command failed with response: 0x%02X\n", response);
        exit(1);
    }
}

void stk500_program(int serial, const char* hex_file_path) {
    FILE* hex_file = fopen(hex_file_path, "r");
    if (!hex_file) {
        perror("Error opening hex file");
        exit(1);
    }

    unsigned char enter_prog[] = {STK_ENTER_PROGMODE, STK_OK};
    stk_send(serial, enter_prog, sizeof(enter_prog));

    char line[256];
    while (fgets(line, sizeof(line), hex_file)) {
        if (line[0] != ':') continue;  // Invalid line
        int byte_count, address, record_type;
        sscanf(line + 1, "%02x%04x%02x", &byte_count, &address, &record_type);

        if (record_type == 1) break;  // End of file record

        unsigned char load_address[] = {STK_LOAD_ADDRESS, (address >> 8) & 0xFF, address & 0xFF, STK_OK};
        stk_send(serial, load_address, sizeof(load_address));

        unsigned char data[byte_count];
        for (int i = 0; i < byte_count; i++) {
            sscanf(line + 9 + (i * 2), "%02x", &data[i]);
        }

        unsigned char prog_page[4 + byte_count];
        prog_page[0] = STK_PROG_PAGE;
        prog_page[1] = (byte_count >> 8) & 0xFF;
        prog_page[2] = byte_count & 0xFF;
        prog_page[3] = 'F';  // Flash memory
        memcpy(&prog_page[4], data, byte_count);
        stk_send(serial, prog_page, sizeof(prog_page));
    }

    unsigned char leave_prog[] = {STK_LEAVE_PROGMODE, STK_OK};
    stk_send(serial, leave_prog, sizeof(leave_prog));

    fclose(hex_file);
    printf("Programming complete.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <serial_port> <hex_file>\n", argv[0]);
        exit(1);
    }

    int serial = open_serial(argv[1]);
    stk500_program(serial, argv[2]);
    close(serial);
    return 0;
}
