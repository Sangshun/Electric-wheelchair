#include "GPIOButton.hpp"
#include <iostream>
#include <chrono>
#include <thread>
// Initialize GPIO button
GPIOButton::GPIOButton(unsigned gpio,
                       const std::function<void()>& shortCallback,
                       const std::function<void()>& longCallback)
    : gpio_(gpio), shortCallback_(shortCallback), longCallback_(longCallback),
      running_(false), chip_("gpiochip0"), line_(chip_.get_line(gpio)),
      debounceDelay_(std::chrono::milliseconds(100)),     // 100ms debounce window
      longPressThreshold_(std::chrono::milliseconds(500))   // 500ms threshold for long press
{
    // No additional initialization required
}
// Stop button polling and releases the GPIO line
GPIOButton::~GPIOButton() {
    stop();
    line_.release();
}
// Start monitoring the button press events
bool GPIOButton::start() {
    try {
        // Request the GPIO line for both falling and rising edge events
        line_.request({ "gpio-button", gpiod::line_request::EVENT_BOTH_EDGES, 0 });
    } catch (const std::exception &e) {
        std::cerr << "Failed to request GPIO " << gpio_ << ": " << e.what() << std::endl;
        return false;
    }
    running_ = true;
    pollThread_ = std::thread(&GPIOButton::pollButton, this);  // Start a separate thread to continuously monitor button events
    return true;
}
// Stop button event monitoring
void GPIOButton::stop() {
    running_ = false;
    if (pollThread_.joinable())
        pollThread_.join();
    line_.release();
}
// Thread function to continuously monitor button events
void GPIOButton::pollButton() {
    std::chrono::milliseconds timeout(100);  // Event wait timeout
    std::chrono::steady_clock::time_point pressTime;  // Store button press time

    while (running_) {
        bool eventOccurred = false;
        try {
            eventOccurred = line_.event_wait(timeout);  // Wait for a GPIO event
        } catch (const std::exception &e) {
            std::cerr << "Error waiting for event on GPIO " << gpio_ << ": " << e.what() << std::endl;
            break;
        }

        if (eventOccurred) {
            gpiod::line_event event;
            try {
                event = line_.event_read();
            } catch (const std::exception &e) {
                std::cerr << "Error reading event on GPIO " << gpio_ << ": " << e.what() << std::endl;
                continue;
            }
            // Detect button press and release
            if (event.event_type == gpiod::line_event::FALLING_EDGE) {  // Button is pressed: record the timestamp
                pressTime = std::chrono::steady_clock::now();
            } else if (event.event_type == gpiod::line_event::RISING_EDGE) {  // Button is released and calculate press duration
                auto releaseTime = std::chrono::steady_clock::now();
                auto pressDuration = std::chrono::duration_cast<std::chrono::milliseconds>(releaseTime - pressTime);
                if (pressDuration < debounceDelay_) {
                    continue;  // Ignore noise
                }
                if (pressDuration >= longPressThreshold_) {
                    longCallback_();  // Long press detected
                } else {
                    shortCallback_();  // Short press detected
                }
            }
        }
    }
}
