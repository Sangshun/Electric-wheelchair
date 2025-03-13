#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <chrono>
#include <algorithm>
#include <numeric>  

#define SAMPLE_RATE 250
#define BUFFER_SIZE 1024
#define THRESHOLD 0.6

using namespace std;
using namespace std::chrono;

class ECGProcessor {
public:
    ECGProcessor() : heart_rate(0) {}  

   
    const vector<double> b = {0.0034, 0.0, -0.0068, 0.0, 0.0034};
    const vector<double> a = {1.0, -3.6789, 5.1797, -3.3058, 0.8060};
    
    vector<double> filter(const vector<double>& input) {
        vector<double> output(input.size(), 0.0);
        static vector<double> x(5, 0.0), y(5, 0.0);
        
        for(size_t n=0; n<input.size(); n++) {
            x = {input[n], x[0], x[1], x[2], x[3]};
            y = {0.0, y[0], y[1], y[2], y[3]};

            y[0] = b[0]*x[0] + b[1]*x[1] + b[2]*x[2] + 
                   b[3]*x[3] + b[4]*x[4] -
                   a[1]*y[1] - a[2]*y[2] - 
                   a[3]*y[3] - a[4]*y[4];
            
            output[n] = y[0];
        }
        return output;
    }

    void process(const vector<double>& ecg_data) {
        auto filtered = filter(ecg_data);
        
        static vector<double> window;
        window.insert(window.end(), filtered.begin(), filtered.end());
        
        if(window.size() > SAMPLE_RATE*2) {
            window.erase(window.begin(), window.begin()+(window.size()-SAMPLE_RATE*2));
        }

        
        double rms = sqrt(accumulate(window.begin(), window.end(), 0.0, 
            [](double a, double b){return a + b*b;}) / window.size());
        double threshold = THRESHOLD * rms;

        for(size_t i=1; i<filtered.size()-1; i++) {
            if(filtered[i] > threshold && 
               filtered[i] > filtered[i-1] && 
               filtered[i] > filtered[i+1]) 
            {
                auto now = system_clock::now();
                if(last_r_peak != time_point<system_clock>()) { 
                    auto interval = duration_cast<milliseconds>(now - last_r_peak);
                    heart_rate = 60000.0 / interval.count();
                }
                last_r_peak = now;
                break;
            }
        }
    }

    double get_heart_rate() const { return heart_rate; }

private:
    time_point<system_clock> last_r_peak{};  
    double heart_rate;
};


    close(fd);
    return 0;
}
