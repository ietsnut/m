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

// Constants for ArduinoISP protocol
#define STK_OK      0x10
#define STK_FAILED  0x11
#define STK_INSYNC  0x14
#define STK_NOSYNC  0x15
#define CRC_EOP     0x20

// Timeout values (in microseconds)
#define INIT_DELAY_US 2000000    
#define STABILIZE_DELAY_US 50000  
#define READ_TIMEOUT_US 500000    
#define SYNC_ATTEMPTS 3           

#define MAX_PORTS 64
#define PORT_NAME_LENGTH 32

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

// Configure port based on platform detection
int configure_port(const char *port) {
    if (IsWindows()) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "cmd.exe /c mode %s: BAUD=19200 PARITY=N DATA=8 STOP=1", port);
        return system(cmd);
    }
    // For other platforms, configuration is done in open_port
    return 0;
}

// Open and configure port based on platform
int open_port(const char *port) {
    if (IsWindows()) {
        if (configure_port(port) != 0) {
            return -1;
        }
        return open(port, O_RDWR | O_NONBLOCK);
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

        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
            close(fd);
            return -1;
        }

        return fd;
    }
}

// Get list of available ports based on platform
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

int sync_with_programmer(int fd) {
    unsigned char cmd[] = {'0', CRC_EOP};
    unsigned char response[2];
    
    for (int attempt = 1; attempt <= SYNC_ATTEMPTS; attempt++) {
        if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
            continue;
        }
        
        usleep(STABILIZE_DELAY_US);
        
        int bytes_read = read_with_timeout(fd, response, sizeof(response), READ_TIMEOUT_US);
        if (bytes_read == 2 && response[0] == STK_INSYNC && response[1] == STK_OK) {
            return 1;
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
    if (!sync_with_programmer(fd)) {
        return 0;
    }
    return get_programmer_version(fd);
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
        
        if (check_arduinoisp(fd)) {
            printf("Success: ArduinoISP found on %s!\n", ports[i]);
            close(fd);
            return 0;
        } else {
            printf("No ArduinoISP on %s\n", ports[i]);
        }
        
        close(fd);
    }
    
    printf("\nNo ArduinoISP programmer found on any port\n");
    return 1;
}

