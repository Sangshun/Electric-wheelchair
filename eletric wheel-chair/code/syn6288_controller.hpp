#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <system_error>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

class TTSController {
public:

    explicit TTSController(
        const std::string& device = "/dev/ttyAMA0",
        int baud_rate = B9600
    );
    

    TTSController(const TTSController&) = delete;
    TTSController& operator=(const TTSController&) = delete;


    ~TTSController() noexcept;


    void wake_up();


    void play_text(
        const std::string& text,
        uint8_t encoding = 0x04,
        unsigned int wait_ms = 3000
    );

private:

    int m_fd = -1;


    void send_command(const std::vector<uint8_t>& command);
    std::vector<uint8_t> build_command(
        uint8_t cmd_type,
        const std::string& payload,
        uint8_t encoding
    ) const;
    uint8_t calculate_checksum(const std::vector<uint8_t>& data) const;
    void configure_serial(int baud_rate);
};
