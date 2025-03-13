#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#define UART_DEV "/dev/ttyAMA0"

// **SYN6288 Commands**
const unsigned char SPEED_CMD[] = { 0xFD, 0x00, 0x02, 0x02, 0x05, 0x08 };  // Set speech speed
const unsigned char BG_MUSIC_CMD[] = { 0xFD, 0x00, 0x02, 0x07, 0x06, 0x0A }; // Enable background music

// **Speech Command
unsigned char PLAY_CMD[] = {
    0xFD, 0x00, 0x0C, 0x01, 0x01,  
    0xC1, 0xF5, 0xC9, 0xD9, 0xCF, 0xA2, 0xBB, 0xB6, 0xBC, 0xC7, 0xC0, 0xCF,
    0x00  // Placeholder for checksum
};

// **Calculate checksum**
void update_checksum() {
    unsigned int sum = 0;
    for (int i = 2; i < sizeof(PLAY_CMD) - 1; i++) {
        sum += PLAY_CMD[i];
    }
    PLAY_CMD[sizeof(PLAY_CMD) - 1] = (sum & 0xFF);
}

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
    update_checksum();  // Update speech command checksum

    // **Step 1: Set Speech Speed**
    //write(uart_fd, SPEED_CMD, sizeof(SPEED_CMD));
    //usleep(100000);

    // **Step 2: Enable Background Music**
    //write(uart_fd, BG_MUSIC_CMD, sizeof(BG_MUSIC_CMD));
    //usleep(100000);

    // **Step 3: Send Speech Command**
    write(uart_fd, PLAY_CMD, sizeof(PLAY_CMD));
    printf("Sent speech command\n");

    // **Step 4: Wait for playback**
    sleep(3);
}

int main() {
    int uart_fd = open_uart();
    if (uart_fd == -1) return EXIT_FAILURE;

    send_syn6288_command(uart_fd);

    // **Close serial port**
    //close(uart_fd);
    printf("Speech playback completed. Closing UART.\n");

    return EXIT_SUCCESS;
}
