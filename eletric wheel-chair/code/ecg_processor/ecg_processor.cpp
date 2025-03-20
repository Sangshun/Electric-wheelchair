#include "ecg_processor.hpp"
#include <iostream>
#include <cstring>

// EnhancedFilter Implementation
EnhancedFilter::EnhancedFilter(const std::vector<double>& b, const std::vector<double>& a)
    : b_coeff(b), a_coeff(a), x(5, 0.0), y(5, 0.0) {}

std::vector<double> EnhancedFilter::process(const std::vector<double>& input) {
    std::vector<double> output;
    output.reserve(input.size());
    static double baseline = 0.0;

    for (auto sample : input) {
        baseline = baseline_alpha * baseline + (1 - baseline_alpha) * sample;
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

// AdvancedHRCalculator Implementation
AdvancedHRCalculator::AdvancedHRCalculator() 
    : last_valid_hr(0.0), qrs_window(200), min_interval(300), noise_count(0) {}

void AdvancedHRCalculator::update_r_peak() {
    const auto now = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> lock(data_mutex);

    if (!last_peak_time.time_since_epoch().count()) {
        last_peak_time = now;
        return;
    }

    const auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_peak_time);
    if (interval < min_interval) {
        if (++noise_count > 3) reset_state();
        return;
    }

    noise_count = 0;
    const double new_hr = 60000.0 / interval.count();
    
    if (new_hr < 30.0 || new_hr > 220.0) return;

    hr_buffer.push_back(new_hr);
    if (hr_buffer.size() > 7) hr_buffer.pop_front();

    auto temp = hr_buffer;
    std::sort(temp.begin(), temp.end());
    last_valid_hr = temp[temp.size()/2];
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

// StableECGProcessor Implementation
StableECGProcessor::StableECGProcessor()
    : filter({{0.0034, 0.0, -0.0068, 0.0, 0.0034},
             {1.0, -3.6789, 5.1797, -3.3058, 0.8060}}),
      active(false),
      threshold_low(0.0),
      threshold_high(0.0) {}

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
    if (processor.joinable()) {
        processor.join();
    }
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
    while (active.load()) {
        auto data = fetch_data();
        if (data.empty()) continue;

        auto filtered = filter.process(data);
        
        // Update analysis window
        window.insert(window.end(), filtered.begin(), filtered.end());
        if (window.size() > SAMPLE_RATE * WINDOW_SECONDS) {
            window.erase(window.begin(),
                       window.begin() + (window.size() - SAMPLE_RATE * WINDOW_SECONDS));
        }

        update_thresholds(window);
        detect_r_peaks(filtered);
    }
}

std::vector<double> StableECGProcessor::fetch_data() {
    std::unique_lock<std::mutex> lock(buffer_mutex);
    data_ready.wait(lock, [this](){ return !circular_buffer.empty() || !active.load(); });

    std::vector<double> data;
    if (!circular_buffer.empty()) {
        data.swap(circular_buffer);
    }
    return data;
}

void StableECGProcessor::update_thresholds(const std::vector<double>& window) {
    static double noise_peak = 0.0;
    static double signal_peak = 0.0;
    
    const double current_max = *std::max_element(window.begin(), window.end());
    
    noise_peak = 0.875 * noise_peak + 0.125 * current_max;
    signal_peak = 0.125 * signal_peak + 0.875 * current_max;
    
    threshold_low = noise_peak + 0.25 * (signal_peak - noise_peak);
    threshold_high = 0.5 * threshold_low + 0.5 * signal_peak;
}

void StableECGProcessor::detect_r_peaks(const std::vector<double>& data) {
    bool above_threshold = false;
    
    for (size_t i = 1; i < data.size() - 1; ++i) {
        const double slope = data[i] - data[i-1];
        
        if (data[i] > threshold_high && 
            slope > 0.5 * threshold_high &&
            data[i] > data[i-1] &&
            data[i] > data[i+1]) 
        {
            if (!above_threshold) {
                calculator.update_r_peak();
                above_threshold = true;
            }
        } else if (data[i] < threshold_low) {
            above_threshold = false;
        }
    }
}

ReliableSerialReader::ReliableSerialReader(const std::string& port, StableECGProcessor& proc)
    : processor(proc), active(false), byte_pos(0), current_sample(0) {
    if ((fd = ::open(port.c_str(), O_RDWR | O_NOCTTY)) < 0) {
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
    if (reader.joinable()) {
        reader.join();
    }
    if (fd != -1) {
        ::close(fd);
        fd = -1;
    }
}

void ReliableSerialReader::reading_loop() {
    uint8_t buffer[BUFFER_SIZE];
    std::vector<int16_t> sample_cache;

    while (active.load()) {
        ssize_t n = ::read(fd, buffer, BUFFER_SIZE);
        if (n < 0) {
            if (errno == EAGAIN) continue;
            std::cerr << "Read error: " << strerror(errno) << std::endl;
            break;
        }

        for (int i = 0; i < n; ++i) {
            if (byte_pos == 0) {
                current_sample = buffer[i];
                byte_pos = 1;
            } else {
                current_sample |= buffer[i] << 8;
                sample_cache.push_back(current_sample);
                byte_pos = 0;
            }
        }

        if (!sample_cache.empty()) {
            std::vector<double> samples;
            samples.reserve(sample_cache.size());
            for (auto s : sample_cache) {
                samples.push_back(s * 0.0025);
            }
            processor.add_samples(samples);
            sample_cache.clear();
        }
    }
}
