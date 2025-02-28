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
                if(last_r_peak != time_point<system_clock>()) {  // �޸ıȽϷ�ʽ
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

int main() {
    const char* port = "/dev/ttyUSB0";
    int fd = open(port, O_RDWR | O_NOCTTY);
    
    if(fd == -1) {
        cerr << "Error opening serial port" << endl;
        return 1;
    }

    struct termios tty;
    tcgetattr(fd, &tty);
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tcsetattr(fd, TCSANOW, &tty);
    //
    //system("libcamera-vid -t 0 --width 1920 --height 1080 "
    // "--codec yuv420 --inline | ffmpeg -i - -f v4l2 /dev/video0");

    //system("cd /home/pi/Desktop && sudo ./syn6288_test");
    //
    ECGProcessor processor;
    vector<double> ecg_buffer;
    
    while(true) {
        uint8_t raw_data[BUFFER_SIZE];
        int n = read(fd, raw_data, BUFFER_SIZE);
        
        for(int i=0; i<n; i+=2) {
            if(i+1 >= n) break;
            int16_t sample = (raw_data[i+1] << 8) | raw_data[i];
            ecg_buffer.push_back(sample * 0.0025);
        }

        if(ecg_buffer.size() >= SAMPLE_RATE) {
            processor.process(ecg_buffer);
            ecg_buffer.clear();
            cout << "the heart rating is : " << processor.get_heart_rate() << " BPM" << endl;
        }

        usleep(1000);
    }

    close(fd);
    return 0;
}