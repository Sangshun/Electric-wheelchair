#include "ecg_processor.hpp"
#include "reliable_serial_reader.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <csignal>

volatile std::sig_atomic_t g_running = 1;

void signalHandler(int signum) {
    g_running = 0;
}

int main() {
    std::signal(SIGINT, signalHandler);
    try {
        // Create ECG processor and serial reader (ECG module connected to /dev/ttyUSB0)
        StableECGProcessor processor;
        ReliableSerialReader reader("/dev/ttyUSB0", processor);

        // Start ECG processing and serial reading
        processor.start();
        reader.start();

        std::cout << "ECG module started. Press Ctrl+C to exit." << std::endl;
        auto last_display = std::chrono::steady_clock::now();

        // Main loop: update and display heart rate every 500ms
        while (g_running) {
            auto now = std::chrono::steady_clock::now();
            if (now - last_display >= std::chrono::milliseconds(500)) {
                std::cout << "\rCurrent heart rate: " << std::fixed << std::setprecision(1)
                          << processor.current_hr() << " BPM" << std::flush;
                last_display = now;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        reader.stop();
        processor.stop();
    } catch (const std::exception& e) {
        std::cerr << "\nFatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << std::endl;
    return EXIT_SUCCESS;
}
