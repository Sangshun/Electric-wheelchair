#include "ecg_processor.hpp"
#include <iostream>
#include <iomanip>
#include <atomic>
#include <mutex>
#include <sstream>
#include <string>
#include <chrono>
#include <thread>

// Global to capture when we’ve seen at least one reading
static std::atomic<bool> got_value{false};
// And the latest value itself
static std::atomic<double> g_latest_ecg_value{0.0};

class InterceptBuf : public std::streambuf {
public:
    InterceptBuf(std::streambuf* orig) : orig_buf(orig) {}

protected:
    int overflow(int c) override {
        std::lock_guard<std::mutex> lock(mtx);
        if (c != EOF) {
            char ch = static_cast<char>(c);
            buffer.push_back(ch);
            if (ch == '\n') {
                processLine(buffer);
                buffer.clear();
            }
            return orig_buf->sputc(ch);
        }
        return c;
    }

    std::streamsize xsputn(const char* s, std::streamsize count) override {
        std::lock_guard<std::mutex> lock(mtx);
        buffer.append(s, count);
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            processLine(buffer.substr(0, pos + 1));
            buffer.erase(0, pos + 1);
        }
        return orig_buf->sputn(s, count);
    }

    void processLine(const std::string& line) {
        auto idx = line.find("Converted ECG Value:");
        if (idx != std::string::npos) {
            auto colon = line.find(':', idx);
            if (colon != std::string::npos) {
                std::istringstream iss(line.substr(colon + 1));
                double v;
                if (iss >> v) {
                    g_latest_ecg_value.store(v);
                    got_value.store(true);
                }
            }
        }
    }

private:
    std::streambuf* orig_buf;
    std::string     buffer;
    std::mutex      mtx;
};

int main() {
    // Redirect stderr through our InterceptBuf
    InterceptBuf interceptor(std::cerr.rdbuf());
    std::streambuf* oldErr = std::cerr.rdbuf(&interceptor);

    StableECGProcessor processor;
    ReliableSerialReader reader("/dev/ttyUSB0", processor);

    processor.start();
    reader.start();

    // Wait up to 5?s (was 3?s) for the first value
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!got_value.load() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!got_value.load()) {
        std::cerr << "No ECG value received within timeout\n";
        reader.stop();
        processor.stop();
        std::cerr.rdbuf(oldErr);
        return 1;
    }

    // Print the value once on stdout
    double v = g_latest_ecg_value.load();
    std::cout << "Converted ECG Value: " << std::fixed << std::setprecision(1)
              << v << std::endl;

    reader.stop();
    processor.stop();
    std::cerr.rdbuf(oldErr);
    return 0;
}
