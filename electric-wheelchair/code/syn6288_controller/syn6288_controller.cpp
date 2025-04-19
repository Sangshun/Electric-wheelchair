#include "syn6288_controller.hpp"
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include <vector>
#include <cstdio>
#include <stdexcept>
#include <system_error>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

using namespace std;
using namespace std::chrono_literals;

TTSController::TTSController(const std::string& device, int baud_rate) {
    m_fd = open(device.c_str(), O_RDWR | O_NOCTTY);
    if (m_fd == -1) {
        throw system_error(errno, generic_category(), "Open UART failed");
    }
    try {
        configure_serial(baud_rate);
    } catch (...) {
        close(m_fd);
        throw;
    }
}

TTSController::~TTSController() noexcept {
    if (m_fd != -1) {
        close(m_fd);
        m_fd = -1;
    }
}

void TTSController::configure_serial(int baud_rate) {
    termios tty{};
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(m_fd, &tty) != 0) {
        throw system_error(errno, generic_category(), "tcgetattr failed");
    }

    // Set 8N1 and disable flow control
    tty.c_cflag &= ~PARENB;        // No parity
    tty.c_cflag &= ~CSTOPB;        // 1 stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;            // 8 data bits
    tty.c_cflag &= ~CRTSCTS;       // No hardware flow control
    tty.c_cflag |= CLOCAL | CREAD; // Enable receiver

    tty.c_lflag &= ~ICANON; // Disable canonical mode
    tty.c_lflag &= ~ECHO;   // Disable echo
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;   // Disable interpretation of INTR, QUIT, SUSP

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // No software flow control
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);

    tty.c_oflag &= ~OPOST; // Raw output
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;   // 0.5 second timeout

    cfsetispeed(&tty, baud_rate);
    cfsetospeed(&tty, baud_rate);

    if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
        throw system_error(errno, generic_category(), "tcsetattr failed");
    }

    tcflush(m_fd, TCIOFLUSH);
}

void TTSController::wake_up() {
    // Wake-up command per manual:
    // Example wake-up command: FD + Length (0x000B) + Command (0x01) + Parameter (0x00) + Payload (0x02) + Checksum.
    static const vector<uint8_t> WAKE_CMD{0xFD, 0x00, 0x0B, 0x01, 0x00, 0x02};
    send_command(WAKE_CMD);
    // Wait at least 16 ms per manual; here we wait 100 ms.
    this_thread::sleep_for(100ms);
}

void TTSController::play_text(const string& text, uint8_t encoding, unsigned int wait_ms) {
    if (encoding != 0x00 && encoding != 0x04) {
        throw invalid_argument("Unsupported encoding format");
    }
    auto cmd = build_command(0x01, text, encoding);
    send_command(cmd);
    this_thread::sleep_for(chrono::milliseconds(wait_ms));
}

vector<uint8_t> TTSController::build_command(uint8_t cmd_type, const string& payload, uint8_t encoding) const {
    vector<uint8_t> cmd;
    // Frame header
    cmd.push_back(0xFD);
    
    // Prepare payload: append null terminator if not present.
    string payload_to_send = payload;
    if (payload.empty() || payload.back() != '\0') {
        payload_to_send.push_back('\0');
    }
    
    // Data length = command code (1) + parameter (1) + payload length + checksum (1)
    uint16_t data_len = 1 + 1 + payload_to_send.size() + 1;
    // Push length: high byte first, then low byte.
    cmd.push_back(static_cast<uint8_t>((data_len >> 8) & 0xFF));
    cmd.push_back(static_cast<uint8_t>(data_len & 0xFF));
    
    // Command code
    cmd.push_back(cmd_type);
    // Command parameter (encoding)
    cmd.push_back(encoding);
    
    // Append payload (text)
    for (char c : payload_to_send) {
        cmd.push_back(static_cast<uint8_t>(c));
    }
    
    // Append XOR checksum: from first length byte to the last payload byte.
    cmd.push_back(calculate_checksum(cmd));
    
    return cmd;
}

uint8_t TTSController::calculate_checksum(const vector<uint8_t>& data) const {
    uint8_t checksum = 0;
    // XOR from data[1] (first length byte) to the last element (excluding the checksum itself)
    for (size_t i = 1; i < data.size(); ++i) {
        checksum ^= data[i];
    }
    return checksum;
}

void TTSController::send_command(const vector<uint8_t>& command) {
    if (m_fd == -1) {
        throw runtime_error("UART not initialized");
    }
    std::cout << "Attempt to send HEX: ";
    for (uint8_t byte : command) {
        printf("%02X ", byte);
    }
    std::cout << std::endl;
    
    ssize_t total_written = 0;
    const size_t total_size = command.size();
    const uint8_t* data_ptr = command.data();
    while (total_written < static_cast<ssize_t>(total_size)) {
        ssize_t written = write(m_fd, data_ptr + total_written, total_size - total_written);
        if (written == -1) {
            if (errno == EINTR) continue;
            throw system_error(errno, generic_category(), "Write failed");
        }
        total_written += written;
    }
    if (tcdrain(m_fd) == -1) {
        throw system_error(errno, generic_category(), "Sync failed");
    }
    std::cout << "Successfully sent: ";
    for (uint8_t byte : command) {
        printf("%02X ", byte);
    }
    std::cout << std::endl;
}
