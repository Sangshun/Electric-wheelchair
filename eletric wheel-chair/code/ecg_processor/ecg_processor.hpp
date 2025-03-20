#ifndef ECG_PROCESSOR_HPP
#define ECG_PROCESSOR_HPP

#include <vector>
#include <deque>
#include <chrono>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <stdexcept>
#include <string>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>

constexpr double BASELINE_ALPHA = 0.99;      
constexpr int SAMPLE_RATE = 1000;            
constexpr int WINDOW_SECONDS = 5;           
constexpr int BUFFER_SIZE = 256;   

class EnhancedFilter {
public:
    EnhancedFilter(const std::vector<double>& b, const std::vector<double>& a);
    std::vector<double> process(const std::vector<double>& input);
private:
    std::vector<double> b_coeff;
    std::vector<double> a_coeff;
    std::vector<double> x;
    std::vector<double> y;
};

class AdvancedHRCalculator {
public:
    AdvancedHRCalculator();
    void update_r_peak();
    double get_heart_rate() const;
    void reset_state();
private:
    mutable std::mutex data_mutex;
    std::chrono::system_clock::time_point last_peak_time;
    std::deque<double> hr_buffer;
    double last_valid_hr;
    int noise_count;
    const int qrs_window;    // Window size for QRS detection (in ms)
    const int min_interval;  // Minimum interval between peaks (in ms)
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
    std::atomic<bool> active;
    std::thread processor;
    std::mutex buffer_mutex;
    std::condition_variable data_ready;
    std::vector<double> circular_buffer;
    
    // Members for adaptive thresholding
    double noise_peak;
    double signal_peak;
    // To prevent duplicate detection of the same R peak (refractory mechanism)
    size_t last_peak_index;
    
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
    int fd;
    std::thread reader;
};

#endif 
