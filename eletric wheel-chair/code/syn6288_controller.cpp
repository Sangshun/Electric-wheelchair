#include "syn6288_controller.hpp"
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>

using namespace std;
using namespace std::chrono_literals;


TTSController::TTSController(const string& device, int baud_rate) {
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


    tty.c_cflag &= ~PARENB;        
    tty.c_cflag &= ~CSTOPB;        
    tty.c_cflag &= ~CSIZE;         
    tty.c_cflag |= CS8;            
    tty.c_cflag &= ~CRTSCTS;       
    tty.c_cflag |= CREAD | CLOCAL; 

    tty.c_lflag &= ~ICANON;        
    tty.c_lflag &= ~ECHO;          
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;          

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); 
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);

    tty.c_oflag &= ~OPOST;         
    tty.c_cc[VMIN] = 0;            
    tty.c_cc[VTIME] = 5;           


    cfsetispeed(&tty, baud_rate);
    cfsetospeed(&tty, baud_rate);

    if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
        throw system_error(errno, generic_category(), "tcsetattr failed");
    }

    tcflush(m_fd, TCIOFLUSH);
}


void TTSController::wake_up() {

    static const vector<uint8_t> WAKE_CMD{0xFD, 0x00, 0x02, 0x01, 0x00, 0x02};
    send_command(WAKE_CMD);
    this_thread::sleep_for(100ms); 
}

void TTSController::play_text(
    const string& text,
    uint8_t encoding,
    unsigned int wait_ms
) {
    if (encoding != 0x00 && encoding != 0x04) {
        throw invalid_argument("Unsupported encoding format");
    }

    auto cmd = build_command(0x01, text, encoding);
    send_command(cmd);
    this_thread::sleep_for(chrono::milliseconds(wait_ms));
}


vector<uint8_t> TTSController::build_command(
    uint8_t cmd_type,
    const string& payload,
    uint8_t encoding
) const {
    vector<uint8_t> cmd;

    
    cmd.push_back(0xFD);

    
    uint16_t data_len = 1 + 1 + payload.size();
    cmd.push_back(static_cast<uint8_t>((data_len >> 8) & 0xFF)); 
    cmd.push_back(static_cast<uint8_t>(data_len & 0xFF));        

    
    cmd.push_back(cmd_type);

   
    cmd.push_back(encoding);

   
    for (char c : payload) {
        cmd.push_back(static_cast<uint8_t>(c));
    }

 
    cmd.push_back(calculate_checksum(cmd));

    return cmd;
}


uint8_t TTSController::calculate_checksum(const vector<uint8_t>& data) const {
    uint8_t checksum = 0;
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
            if (errno == EINTR) { 
                continue;
            }
            throw system_error(error_code(errno, generic_category()), "Write failed");
        }
        total_written += written;
    }

    
    if (tcdrain(m_fd) == -1) {
        throw system_error(error_code(errno, generic_category()), "Sync failed");
    }

   
    std::cout << "Successfully sent: ";
    for (uint8_t byte : command) {
        printf("%02X ", byte);
    }
    std::cout << std::endl;
}
