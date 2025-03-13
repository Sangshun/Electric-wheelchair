#ifndef ECG_PROCESSOR_HPP
#define ECG_PROCESSOR_HPP

#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <thread>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <iomanip>

#define SAMPLE_RATE 250
#define BUFFER_SIZE 1024
#define WINDOW_SECONDS 3

class EnhancedFilter {
public:
    EnhancedFilter(const std::vector<double>& b, const std::vector<double>& a);
    std::vector<double> process(const std::vector<double>& input);

private:
    const std::vector<double> b_coeff, a_coeff;
    std::vector<double> x, y;
    const double baseline_alpha = 0.99;
};

class AdvancedHRCalculator {
public:
    AdvancedHRCalculator();
    void update_r_peak();
    double get_heart_rate() const;
    void reset_state();

private:
    mutable std::mutex data_mutex;
    std::deque<double> hr_buffer;
    std::chrono::system_clock::time_point last_peak_time;
    double last_valid_hr;
    std::chrono::milliseconds qrs_window;
    std::chrono::milliseconds min_interval;
    std::atomic<int> noise_count;
};

class StableECGProcessor {
public:
    StableECGProcessor();
    ~StableECGProcessor();
    void start();
    void stop();
    void add_samples(const std::vector<double>& samples);
    double current_hr() const;

private:
    void processing_loop();
    std::vector<double> fetch_data();
    void update_thresholds(const std::vector<double>& window);
    void detect_r_peaks(const std::vector<double>& data);

    EnhancedFilter filter;
    AdvancedHRCalculator calculator;
    std::vector<double> circular_buffer;
    mutable std::mutex buffer_mutex;
    std::condition_variable data_ready;
    std::atomic<bool> active;
    std::thread processor;
    double threshold_low;
    double threshold_high;
};

class ReliableSerialReader {
public:
    ReliableSerialReader(const std::string& port, StableECGProcessor& proc);
    ~ReliableSerialReader();
    void start();
    void stop();

private:
    void reading_loop();

    StableECGProcessor& processor;
    std::atomic<bool> active;
    std::thread reader;
    int fd;
    int byte_pos;
    int16_t current_sample;
};

#endif
