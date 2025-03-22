#include "ecg_processor.hpp"
#include <iostream>
#include <iomanip>
#include <atomic>
#include <mutex>
#include <sstream>
#include <string>
#include <chrono>
#include <thread>


std::atomic<double> g_latest_ecg_value{0.0};


class InterceptBuf : public std::streambuf {
public:
    
    InterceptBuf(std::streambuf* orig) : orig_buf(orig) {}

protected:
    
    virtual int overflow(int c) override {
        std::lock_guard<std::mutex> lock(mtx);
        if (c != EOF) {
            char ch = static_cast<char>(c);
            buffer.push_back(ch);
            if (ch == '\n') {
                processLine(buffer);
                buffer.clear();
            }
            return orig_buf->sputc(c);
        }
        return c;
    }

   
    virtual std::streamsize xsputn(const char* s, std::streamsize count) override {
        std::lock_guard<std::mutex> lock(mtx);
        std::string str(s, count);
        buffer.append(str);
        size_t pos = 0;
        
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos + 1);
            processLine(line);
            buffer.erase(0, pos + 1);
        }
        return orig_buf->sputn(s, count);
    }

    
    void processLine(const std::string& line) {
        if (line.find("Converted ECG Value:") != std::string::npos) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                std::istringstream iss(line.substr(pos + 1));
                double value;
                if (iss >> value) {
                    g_latest_ecg_value.store(value);
                }
            }
        }
    }

private:
    std::streambuf* orig_buf;  
    std::string buffer;        
    std::mutex mtx;           
};

int main() {
    try {
       
        InterceptBuf interceptor(std::cerr.rdbuf());
        std::streambuf* oldCerrBuf = std::cerr.rdbuf(&interceptor);

      
        StableECGProcessor processor;
        ReliableSerialReader reader("/dev/ttyUSB0", processor);

        processor.start();
        reader.start();

      
        auto last_display = std::chrono::steady_clock::now();
        while (true) {
            auto now = std::chrono::steady_clock::now();
            if (now - last_display >= std::chrono::seconds(2)) {
                double ecg = g_latest_ecg_value.load();
                std::cout << "\rConverted ECG Value: " << std::fixed << std::setprecision(1)
                          << ecg << "    " << std::flush;
                last_display = now;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        reader.stop();
        processor.stop();

        
        std::cerr.rdbuf(oldCerrBuf);
    } catch (const std::exception& e) {
        std::cerr << "\nFatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
