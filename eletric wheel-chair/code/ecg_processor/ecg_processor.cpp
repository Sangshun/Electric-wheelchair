#include "ecg_processor.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <stdexcept>
#include <deque>
#include <vector>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cctype>


EnhancedFilter::EnhancedFilter(const std::vector<double>& b, const std::vector<double>& a)
    : b_coeff(b), a_coeff(a), x(5, 0.0), y(5, 0.0) {}

std::vector<double> EnhancedFilter::process(const std::vector<double>& input) {
    std::vector<double> output;
    output.reserve(input.size());
    static double baseline = 0.0;

    for (auto sample : input) {
        
        baseline = 0.995 * baseline + 0.005 * sample;
        sample -= baseline;

       
        x = {sample, x[0], x[1], x[2], x[3]};
        
        y = {0.0, y[0], y[1], y[2], y[3]};

        
        y[0] = b_coeff[0]*x[0] + b_coeff[1]*x[1] + b_coeff[2]*x[2] +
               b_coeff[3]*x[3] + b_coeff[4]*x[4] -
               a_coeff[1]*y[1] - a_coeff[2]*y[2] -
               a_coeff[3]*y[3] - a_coeff[4]*y[4];

        output.push_back(y[0]);
    }
    return output;
}


AdvancedHRCalculator::AdvancedHRCalculator()
    : last_valid_hr(0.0), qrs_window(200), min_interval(300), noise_count(0) {}


void AdvancedHRCalculator::update_r_peak() {
    const auto now = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> lock(data_mutex);

    if (!last_peak_time.time_since_epoch().count()) {
        last_peak_time = now;
        return;
    }

    auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_peak_time);

    
    if (interval.count() < 500) {  
        std::cerr << "? Ignoring unrealistically fast beat (interval: " << interval.count() << " ms)" << std::endl;
        return;
    }

    double new_hr = 60000.0 / interval.count();

    
    if (last_valid_hr > 0 && std::abs(new_hr - last_valid_hr) > 20) {
        std::cerr << "?? Sudden HR jump detected (from " << last_valid_hr 
                  << " to " << new_hr << "), ignoring." << std::endl;
        return;
    }

  
    std::cerr << "? Calculated BPM: " << new_hr << std::endl;

    if (new_hr >= 30.0 && new_hr <= 220.0) {
        hr_buffer.push_back(new_hr);
        if (hr_buffer.size() > 7) {
            hr_buffer.pop_front();
        }

        auto temp = hr_buffer;
        std::sort(temp.begin(), temp.end());
        last_valid_hr = temp[temp.size() / 2];  
    }

    last_peak_time = now;
}

double AdvancedHRCalculator::get_heart_rate() const {
    std::lock_guard<std::mutex> lock(data_mutex);
    return last_valid_hr;
}

void AdvancedHRCalculator::reset_state() {
    std::lock_guard<std::mutex> lock(data_mutex);
    hr_buffer.clear();
    last_peak_time = std::chrono::system_clock::time_point{};
    noise_count = 0;
}


StableECGProcessor::StableECGProcessor()
    : filter({{0.0034, 0.0, -0.0068, 0.0, 0.0034},
              {1.0, -3.6789, 5.1797, -3.3058, 0.8060}}),
      active(false),
      threshold_low(0.0),
      threshold_high(0.0),
      noise_peak(0.0),
      signal_peak(0.0),
      last_peak_index(0)
{}

StableECGProcessor::~StableECGProcessor() {
    stop();
}

void StableECGProcessor::start() {
    active.store(true);
    processor = std::thread(&StableECGProcessor::processing_loop, this);
}

void StableECGProcessor::stop() {
    active.store(false);
    data_ready.notify_all();
    if (processor.joinable())
        processor.join();
}

void StableECGProcessor::add_samples(const std::vector<double>& samples) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    circular_buffer.insert(circular_buffer.end(), samples.begin(), samples.end());
    data_ready.notify_one();
}

double StableECGProcessor::current_hr() const {
    return calculator.get_heart_rate();
}

void StableECGProcessor::processing_loop() {
    std::vector<double> window;
    int print_counter = 0;

    while (active.load()) {
        auto data = fetch_data();
        if (data.empty()) continue;

        
        if (print_counter % 20 == 0) {
            std::cerr << "Raw data: ";
            for (size_t i = 0; i < data.size() && i < 5; ++i) {
                std::cerr << data[i] << " ";
            }
            std::cerr << std::endl;
        }

        auto filtered = filter.process(data);

       
       if (print_counter % 20 == 0) {
            std::cerr << "Filtered ECG Data: ";
            for (size_t i = 0; i < filtered.size() && i < 5; ++i) {
                std::cerr << filtered[i] << " ";
            }
            std::cerr << std::endl;
        }


        ++print_counter;

        window.insert(window.end(), filtered.begin(), filtered.end());
        if (window.size() > SAMPLE_RATE * WINDOW_SECONDS) {
            window.erase(window.begin(), window.begin() + (window.size() - SAMPLE_RATE * WINDOW_SECONDS));
        }

        update_thresholds(window);
        detect_r_peaks(filtered);
    }
}


std::vector<double> StableECGProcessor::fetch_data() {
    std::unique_lock<std::mutex> lock(buffer_mutex);
    data_ready.wait(lock, [this]() { return !circular_buffer.empty() || !active.load(); });
    std::vector<double> data;
    if (!circular_buffer.empty()) {
        data.swap(circular_buffer);
    }
    return data;
}

void StableECGProcessor::update_thresholds(const std::vector<double>& window) {
    double current_max = *std::max_element(window.begin(), window.end());
    
    noise_peak = 0.95 * noise_peak + 0.05 * current_max;
    signal_peak = 0.3 * signal_peak + 0.7 * current_max;

    threshold_low = noise_peak + 0.35 * (signal_peak - noise_peak);
    threshold_high = 0.6 * signal_peak; 
  
    std::cerr << "Thresholds - Low: " << threshold_low
              << " | High: " << threshold_high
              << " | Noise Peak: " << noise_peak
              << " | Signal Peak: " << signal_peak
              << std::endl;
}

void StableECGProcessor::detect_r_peaks(const std::vector<double>& data) {
    static size_t last_peak_index = 0;
    const size_t min_distance = 30;  

    for (size_t i = 1; i < data.size() - 1; ++i) {
        double slope = data[i] - data[i - 1];

        if (i > last_peak_index + min_distance &&  
            data[i] > threshold_high &&  
            slope > 0.3 * threshold_high) 
        {      
            std::cerr << "?? R-Peak Detected! Index: " << i
                      << " | Value: " << data[i] << std::endl;
            
            calculator.update_r_peak();
            last_peak_index = i;  
        }
    }
}


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
            if (errno == EAGAIN) continue;
            std::cerr << "Read error: " << strerror(errno) << std::endl;
            break;
        }
        if (n == 0) continue;

        
        std::cerr << "Read " << n << " bytes: ";
        for (ssize_t i = 0; i < n; i++) {
            std::cerr << std::hex << (int)read_buffer[i] << " ";
        }
        std::cerr << std::endl;

        
        line_buffer.append(read_buffer, n);

        size_t pos;
        while ((pos = line_buffer.find('\n')) != std::string::npos) {
            std::string line = line_buffer.substr(0, pos);
            line_buffer.erase(0, pos + 1);

           
            if (!line.empty() && line.back() == '\r') 
                line.pop_back();
            line.erase(0, line.find_first_not_of(" \t\r\n"));  

            
            if (line.empty()) continue;

            
            std::cerr << "Extracted Line: [" << line << "]" << std::endl;

            
            std::istringstream iss(line);
            std::string token;
            std::vector<std::string> tokens;

            while (std::getline(iss, token, ',')) {
                if (!token.empty()) {  
                    tokens.push_back(token);
                }
            }

            
            std::cerr << "Extracted Tokens: ";
            for (const auto &tok : tokens) {
                std::cerr << "[" << tok << "] ";
            }
            std::cerr << std::endl;

            
            if (tokens.size() >= 3) {
                std::string ecgToken = tokens[2];  

               
                std::cerr << "Extracted ECG Token: [" << ecgToken << "]" << std::endl;

                try {
                    double value = std::stod(ecgToken);
                    value *= 0.7;  

                    
                    std::cerr << "Converted ECG Value: " << value << std::endl;

                    processor.add_samples({value});
                } catch (const std::exception& e) {
                    std::cerr << "? Failed to convert token: [" << ecgToken 
                              << "] - " << e.what() << std::endl;
                }
            } else {
                std::cerr << "?? Skipping line (not enough valid tokens)" << std::endl;
            }
        }
    }
}
