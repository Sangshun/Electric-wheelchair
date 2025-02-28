#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <gpiod.h>
#include <unistd.h>

#define SENSOR_PIN 25  
#define DEBOUNCE_US 100000  

volatile sig_atomic_t exit_flag = 0;


void sig_handler(int signo) {
    if (signo == SIGINT) {
        exit_flag = 1;
        printf("\nExiting...\n");
    }
}

int main() {
    struct gpiod_chip *chip;
    struct gpiod_line *sensor_line;
    int last_state = -1;

    
    signal(SIGINT, sig_handler);

    
    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) {
        perror("Error opening GPIO chip");
        return EXIT_FAILURE;
    }

    
    sensor_line = gpiod_chip_get_line(chip, SENSOR_PIN);
    if (!sensor_line || gpiod_line_request_input(sensor_line, "PIR-Sensor") < 0) {
        perror("GPIO25 config failed");
        gpiod_chip_close(chip);
        return EXIT_FAILURE;
    }

    printf("HC-SR501 Monitoring Started (GPIO25)\n");
    
    while (!exit_flag) {
        int current_state = gpiod_line_get_value(sensor_line);
        
        
        if (current_state != last_state) {
            last_state = current_state;
            
            if (current_state == 1) {
                printf("Motion Detected!\n");
            } else {
                printf("No Motion\n");
            }
            
            
            usleep(DEBOUNCE_US);
        }
        
        usleep(10000);  
    }

    
    gpiod_line_release(sensor_line);
    gpiod_chip_close(chip);
    printf("GPIO25 released.\n");
    return EXIT_SUCCESS;
}