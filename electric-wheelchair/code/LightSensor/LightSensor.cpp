#include "LightSensor.hpp"
#include <cstdlib>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <cstring>

// define the static exit_flag_ variable
std::atomic<bool> LightSensor::exit_flag_{false};

LightSensor::LightSensor(int gpio_pin, Callback callback)
    : gpio_pin_(gpio_pin), callback_(std::move(callback)), running_(false),
      chip_(nullptr), line_(nullptr)
{
    // remove the following line to avoid installing a separate SIGINT handler:
    // std::signal(SIGINT, LightSensor::signal_handler);
    initialize_gpio();// initialize GPIO related settings
}

LightSensor::~LightSensor() {
    stop();// stop the event monitoring thread
    if (line_) gpiod_line_release(line_);// release GPIO pins
    if (chip_) gpiod_chip_close(chip_);// turn off the GPIO chip
}

void LightSensor::signal_handler(int signal) {   // optional SIGINT signal processing function, used to set the exit flag
    if (signal == SIGINT) {
        exit_flag_.store(true, std::memory_order_release);
        std::cout << "\nExit signal received" << std::endl;
    }
}
// initialize the GPIO chip and pins, and request dual edge events
void LightSensor::initialize_gpio() { // turn on the GPIO chip
    chip_ = gpiod_chip_open_by_name("gpiochip0");
    if (!chip_) {
        throw std::runtime_error("Failed to open GPIO chip: gpiochip0");
    }
    line_ = gpiod_chip_get_line(chip_, gpio_pin_);// get the specified GPIO pin
    if (!line_) {
        gpiod_chip_close(chip_);
        throw std::runtime_error("Failed to get GPIO line " + std::to_string(gpio_pin_));
    }
    struct gpiod_line_request_config config = {};//configure the request parameters to detect rising and falling edge events and enable the internal pull-up resistor
    config.consumer = "light-sensor";
    config.request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;// detect both rising and falling edges
    config.flags = GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;  // enable internal pull-up
    if (gpiod_line_request(line_, &config, 0) < 0) { // request pin event monitoring; the third parameter is the default value
        gpiod_line_release(line_);
        gpiod_chip_close(chip_);
        throw std::runtime_error("Failed to configure GPIO events on pin " + std::to_string(gpio_pin_));
    }
}

void LightSensor::start() { // start the event monitoring thread
    if (running_) return; // if it is already running, return directly
    running_ = true;// set the run flag
    event_thread_ = std::thread(&LightSensor::event_monitor, this);
}
// stop event monitoring and wait for the thread to end
void LightSensor::stop() {
    running_ = false;// clear the running flag
    if (event_thread_.joinable()) {
        event_thread_.join();// wait for the thread to exit
    }
}
// event monitoring main loop: combination of polling and event waiting
void LightSensor::event_monitor() {
    struct gpiod_line_event event;
    
    while (running_) {
        // instead of printing the state here, just call the callback
        bool state = gpiod_line_get_value(line_);
        // call the callback only if you want to update the global variable; remove printing:
        callback_(state);
        
        // sleep in short intervals so the thread checks the exit flag frequently
        const int intervals = 5;
        for (int i = 0; i < intervals && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // use a short timeout to wait for events
        struct timespec timeout = {0, 100 * 1000000};  // 100ms
        int ret = gpiod_line_event_wait(line_, &timeout);
        if (ret > 0) { // an event occurs, read the event and call the callback again
            if (gpiod_line_event_read(line_, &event) == 0) {
                state = gpiod_line_get_value(line_);
                callback_(state);
            }
        } else if (ret < 0) { // if an error occurs while waiting for an event, print the error and exit the loop
            std::cerr << "Event wait error" << std::endl;
            break;
        }
    }
}
