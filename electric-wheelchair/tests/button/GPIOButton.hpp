#ifndef GPIOBUTTON_HPP
#define GPIOBUTTON_HPP

#include <gpiod.hpp>
#include <functional>
#include <thread>
#include <atomic>
#include <chrono>

/**
 * @brief GPIOButton class encapsulates GPIO button detection.
 *
 * This class requests a GPIO line as an event input and distinguishes short presses from long presses,
 * calling the corresponding callback.
 */
class GPIOButton {
public:
    /**
     * @brief Constructor.
     * @param gpio The GPIO number used for the button.
     * @param shortCallback Function to call on a short press.
     * @param longCallback Function to call on a long press.
     */
    GPIOButton(unsigned gpio,
               const std::function<void()>& shortCallback,
               const std::function<void()>& longCallback);
    ~GPIOButton();

    // Start polling for button events
    bool start();
    // Stop polling for button events
    void stop();

private:
    unsigned gpio_;
    std::function<void()> shortCallback_;
    std::function<void()> longCallback_;
    std::atomic<bool> running_;
    std::thread pollThread_;
    gpiod::chip chip_;
    gpiod::line line_;
    std::chrono::milliseconds debounceDelay_;
    std::chrono::milliseconds longPressThreshold_;

    // Thread function that polls for button events and measures press duration
    void pollButton();
};

#endif // GPIOBUTTON_HPP
