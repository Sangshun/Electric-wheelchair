#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <gpiod.h>
#include <signal.h>  // Add signal handling

#define TRIG_PIN 23
#define ECHO_PIN 24
#define MAX_DISTANCE_CM 400
#define TIMEOUT_US 38000

// Global flag for signal handling
volatile sig_atomic_t keep_running = 1;

long getMicrotime() {
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    return currentTime.tv_sec * 1000000 + currentTime.tv_usec;
}

// Signal handler for clean exit
void sigint_handler(int sig) {
    keep_running = 0;
}

int main() {
    struct gpiod_chip *chip;
    struct gpiod_line *trig_line, *echo_line;
    int err;

    // Register signal handler
    signal(SIGINT, sigint_handler);

    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) {
        perror("Failed to open GPIO chip");
        return EXIT_FAILURE;
    }

    trig_line = gpiod_chip_get_line(chip, TRIG_PIN);
    echo_line = gpiod_chip_get_line(chip, ECHO_PIN);
    if (!trig_line || !echo_line) {
        perror("Failed to get GPIO lines");
        gpiod_chip_close(chip);
        return EXIT_FAILURE;
    }

    err = gpiod_line_request_output(trig_line, "trig", 0);
    if (err < 0) {
        perror("Failed to configure Trig");
        gpiod_chip_close(chip);
        return EXIT_FAILURE;
    }

    err = gpiod_line_request_input(echo_line, "echo");
    if (err < 0) {
        perror("Failed to configure Echo");
        gpiod_chip_close(chip);
        return EXIT_FAILURE;
    }

    while (keep_running) {  // Changed to controlled loop
        gpiod_line_set_value(trig_line, 1);
        usleep(10);
        gpiod_line_set_value(trig_line, 0);

        long start_time, end_time;
        int timeout = 0;

        long timeout_start = getMicrotime();
        while (gpiod_line_get_value(echo_line) == 0) {
            if (getMicrotime() - timeout_start > TIMEOUT_US) {
                timeout = 1;
                break;
            }
            if (!keep_running) break;  // Allow early exit
        }
        if (timeout || !keep_running) continue;
        start_time = getMicrotime();

        timeout_start = getMicrotime();
        while (gpiod_line_get_value(echo_line) == 1) {
            if (getMicrotime() - timeout_start > TIMEOUT_US) {
                timeout = 1;
                break;
            }
            if (!keep_running) break;  // Allow early exit
        }
        if (timeout || !keep_running) continue;
        end_time = getMicrotime();

        long duration = end_time - start_time;
        float distance = (duration * 0.0343) / 2;

        if (distance <= MAX_DISTANCE_CM) {
            printf("Distance: %.2f cm\n", distance);
        } else {
            printf("Out of range\n");
        }

        usleep(200000);
    }

    // Cleanup resources properly
    printf("\nCleaning up GPIO resources...\n");
    gpiod_line_set_value(trig_line, 0);
    gpiod_line_release(trig_line);
    gpiod_line_release(echo_line);
    gpiod_chip_close(chip);
    
    printf("Program terminated safely.\n");
    return EXIT_SUCCESS;
}