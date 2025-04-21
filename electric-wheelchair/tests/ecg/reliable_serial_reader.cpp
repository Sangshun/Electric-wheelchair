#include "reliable_serial_reader.hpp"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <iostream>

ReliableSerialReader::ReliableSerialReader(const std::string& port, StableECGProcessor& proc)
    : processor(proc), active(false)
{
    fd = ::open(port.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0) {
        throw std::runtime_error("Failed to open serial port: " + std::string(strerror(errno)));
    }
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) {
        ::close(fd);
        throw std::runtime_error("Failed to get serial attributes: " + std::string(strerror(errno)));
    }
    cfmakeraw(&tty);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;
    cfsetspeed(&tty, B115200);
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        ::close(fd);
        throw std::runtime_error("Failed to set serial attributes: " + std::string(strerror(errno)));
    }
}

ReliableSerialReader::~ReliableSerialReader() {
    stop();
    if (fd != -1) {
        ::close(fd);
        fd = -1;
    }
}

void ReliableSerialReader::start() {
    active.store(true);
    reader = std::thread(&ReliableSerialReader::reading_loop, this);
}

void ReliableSerialReader::stop() {
    active.store(false);
    if (reader.joinable())
        reader.join();
    if (fd != -1) {
        ::close(fd);
        fd = -1;
    }
}

void ReliableSerialReader::reading_loop() {
    char read_buffer[BUFFER_SIZE];
    std::string line_buffer;
    while (active.load()) {
        ssize_t n = ::read(fd, read_buffer, BUFFER_SIZE);
        if (n < 0) {
            if (errno == EAGAIN)
                continue;
            break;
        }
        if (n == 0)
            continue;
        // Debug prints removed.
        line_buffer.append(read_buffer, n);
        size_t pos;
        while ((pos = line_buffer.find('\n')) != std::string::npos) {
            std::string line = line_buffer.substr(0, pos);
            line_buffer.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            if (line.empty())
                continue;
            std::istringstream iss(line);
            std::string token;
            std::vector<std::string> tokens;
            while (std::getline(iss, token, ',')) {
                if (!token.empty())
                    tokens.push_back(token);
            }
            if (tokens.size() >= 3) {
                std::string ecgToken = tokens[2];
                try {
                    double value = std::stod(ecgToken);
                    value *= 0.7;
                    processor.add_samples({value});
                } catch (const std::exception& e) {
                    // Conversion errors ignored.
                }
            }
        }
    }
}
