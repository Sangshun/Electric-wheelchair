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
