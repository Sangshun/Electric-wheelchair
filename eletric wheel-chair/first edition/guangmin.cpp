#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <gpiod.h>

#define DO_PIN 16  
volatile sig_atomic_t exit_flag = 0;

void sig_handler(int signo) {
    if (signo == SIGINT) {
        exit_flag = 1;
        printf("\nExiting...\n");
    }
}

int main() {
    struct gpiod_chip *chip;
    struct gpiod_line *do_line;

    signal(SIGINT, sig_handler);
    chip = gpiod_chip_open_by_name("gpiochip0");
    
    if (!chip) {
        perror("Error opening GPIO chip");
        return EXIT_FAILURE;
    }

    do_line = gpiod_chip_get_line(chip, DO_PIN);
    if (!do_line || gpiod_line_request_input(do_line, "light-sensor") < 0) {
        perror("GPIO16 config failed");
        gpiod_chip_close(chip);
        return EXIT_FAILURE;
    }

    printf("Monitoring light via GPIO16...\n");
    while (!exit_flag) {
        int val = gpiod_line_get_value(do_line);
        printf("Status: %s\n", val ? "Dark" : "Light");
        usleep(500000);
    }

    gpiod_line_release(do_line);
    gpiod_chip_close(chip);
    printf("GPIO16 released.\n");
    return EXIT_SUCCESS;
}