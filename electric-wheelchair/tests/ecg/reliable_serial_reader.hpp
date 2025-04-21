#ifndef RELIABLE_SERIAL_READER_HPP
#define RELIABLE_SERIAL_READER_HPP

#include <string>
#include <thread>
#include <atomic>
#include "ecg_processor.hpp"  // Defines StableECGProcessor

class ReliableSerialReader {
public:
    static const int BUFFER_SIZE = 256;
    ReliableSerialReader(const std::string& port, StableECGProcessor& proc);
    ~ReliableSerialReader();
    void start();
    void stop();
private:
    void reading_loop();
    StableECGProcessor& processor;
    std::atomic<bool> active;
    int fd;
    std::thread reader;
};

#endif // RELIABLE_SERIAL_READER_HPP
