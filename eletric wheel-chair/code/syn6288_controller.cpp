#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

// **Raspberry Pi hardware serial port**
#define UART_DEV "/dev/ttyAMA0"

// **SYN6288 wake-up command**
const unsigned char WAKE_CMD[] = { 0xFD, 0x00, 0x02, 0x01, 0x00, 0x02 };

// **SYN6288 command to play "liu shao xi huan ji lao shi"**
const unsigned char PLAY_CMD[] = {
    0xFD, 0x00, 0x19, 0x01, 0x00,  
    0x6C, 0x69, 0x75, 0x20, 0x73, 0x68, 0x61, 0x6F, 0x20,  // "liu shao"
    0x78, 0x69, 0x20, 0x68, 0x75, 0x61, 0x6E, 0x20,        // "xi huan"
    0x6A, 0x69, 0x20, 0x6C, 0x61, 0x6F, 0x20, 0x73, 0x68, 0x69,  // "ji lao shi"
    0xC5  // **Checksum**
};

// **Open UART serial port**
int open_uart() {
    int uart_fd = open(UART_DEV, O_RDWR | O_NOCTTY);
    if (uart_fd == -1) {
        perror("Error opening UART");
        return -1;
    }

    struct termios options;
    tcgetattr(uart_fd, &options);
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);
    options.c_cflag = CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart_fd, TCIFLUSH);
    tcsetattr(uart_fd, TCSANOW, &options);

    return uart_fd;
}

// **Send SYN6288 command**
void send_syn6288_command(int uart_fd) {
    // **Step 1: Send wake-up command (initialize SYN6288)**
    //write(uart_fd, WAKE_CMD, sizeof(WAKE_CMD));
    //usleep(100000);  // 100ms delay

    // **Step 2: Send playback command**
    write(uart_fd, PLAY_CMD, sizeof(PLAY_CMD));
    printf("Sent speech command: 'liu shao xi huan ji lao shi'\n");

    // **Step 3: Wait for playback to finish**
    sleep(3);
}

int main() {
    int uart_fd = open_uart();
    if (uart_fd == -1) return EXIT_FAILURE;

    send_syn6288_command(uart_fd);

    // **Close serial port**
    // close(uart_fd);
    printf("Speech playback completed. Closing UART.\n");

    return EXIT_SUCCESS;
}


