#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>

#define MAX_PATH 1024
#define BUFFER_SIZE 256
#define SYNC_ATTEMPTS 3

int set_interface_attribs(int fd, int speed) {
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) return -1;
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 5;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    if (tcsetattr(fd, TCSANOW, &tty) != 0) return -1;
    return 0;
}

int check_arduinoisp(const char *port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) return 0;

    if (set_interface_attribs(fd, B19200) != 0) {
        close(fd);
        return 0;
    }

    // Wait for device to initialize
    usleep(100000);  // 100ms delay

    unsigned char cmd[] = {0x30, 0x20};
    unsigned char resp[2];
    fd_set readfds;
    struct timeval timeout;

    for (int attempt = 0; attempt < SYNC_ATTEMPTS; attempt++) {
        if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
            close(fd);
            return 0;
        }

        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        if (select(fd + 1, &readfds, NULL, NULL, &timeout) > 0) {
            if (read(fd, resp, sizeof(resp)) == 2 && resp[0] == 0x14 && resp[1] == 0x10) {
                close(fd);
                return 1;
            }
        }

        // Wait a bit before next attempt
        usleep(50000);  // 50ms delay
    }

    close(fd);
    return 0;
}

int main() {
    DIR *d;
    struct dirent *dir;
    char path[MAX_PATH];

    d = opendir("/dev");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strncmp(dir->d_name, "cu.", 3) == 0) {
                snprintf(path, sizeof(path), "/dev/%s", dir->d_name);
                printf("Checking %s at 19200 baud...\n", path);
                
                if (check_arduinoisp(path)) {
                    printf("ArduinoISP found on %s at 19200 baud\n", path);
                    closedir(d);
                    return 0;
                }
            }
        }
        closedir(d);
    }

    printf("ArduinoISP not found on any port at 19200 baud.\n");
    return 1;
}