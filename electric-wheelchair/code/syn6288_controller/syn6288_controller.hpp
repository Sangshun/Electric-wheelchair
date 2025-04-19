#ifndef SYN6288_CONTROLLER_HPP
#define SYN6288_CONTROLLER_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <termios.h>

class TTSController {
public:
    // Constructor: device path (e.g., "/dev/serial0") and baud rate (e.g., B9600)
    TTSController(const std::string& device, int baud_rate);
    ~TTSController() noexcept;

    // Wake up the module (must wait at least 16ms after wake-up per manual)
    void wake_up();

    // Play text. 'encoding': 0x00 for ASCII, 0x04 for GB2312.
    // 'wait_ms' is the delay after sending the command.
    void play_text(const std::string& text, uint8_t encoding, unsigned int wait_ms);

private:
    // Configure the serial port with the specified baud rate.
    void configure_serial(int baud_rate);
    // Build the command frame: header + 2-byte length + command code + parameter + payload + checksum.
    std::vector<uint8_t> build_command(uint8_t cmd_type, const std::string& payload, uint8_t encoding) const;
    // Calculate the XOR checksum from the length field (first length byte) to the last payload byte.
    uint8_t calculate_checksum(const std::vector<uint8_t>& data) const;
    // Send the command frame to the serial port.
    void send_command(const std::vector<uint8_t>& command);

    int m_fd;
};

#endif // SYN6288_CONTROLLER_HPP
