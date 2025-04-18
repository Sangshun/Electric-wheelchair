#include "syn6288_controller.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    try {
        // Create a TTSController instance for "/dev/serial0" at 9600 baud.
        TTSController tts("/dev/serial0", B9600);

        std::cout << "Waking up TTS module..." << std::endl;
        tts.wake_up();

        // Wait briefly to allow the module to stabilize.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::cout << "Playing text..." << std::endl;
        // Play the text "Hello, world!" with default encoding (0x00) and wait 1000ms after playing.
        tts.play_text("Hello, world!", 0x00, 1000);

        std::cout << "TTS test complete." << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error in TTS test: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
