#include "MotorController.hpp"
#include "GPIOButton.hpp"
#include "LightSensor.hpp"
#include "UltrasonicSensor.hpp"
#include "pir_sensor.hpp"
#include "ecg_processor.hpp"
#include "syn6288_controller.hpp"  // TTS module header
#include <iostream>
#include <iomanip>
#include <csignal>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <sstream>
#include <string>
#include <boost/asio.hpp>
#include <unistd.h>       // fork, execl, usleep
#include <sys/types.h>    // pid_t
#include <signal.h>       // kill, SIGTERM

using namespace std::chrono;  // For convenient use of steady_clock, milliseconds, seconds, duration_cast, etc.

// Global termination flag
volatile std::sig_atomic_t g_running = 1;

// Global sensor state variables
std::atomic<bool> g_lightState{false};       // true = Dark, false = Light
std::atomic<bool> g_motionDetected{false};   // true = Motion detected, false = No motion

// Global variable to store the latest converted ECG value extracted from debug output
std::atomic<double> g_latest_ecg_value{0.0};

// Global ultrasonic sensor data for webpage display and associated mutex for protection
std::mutex g_sensor_mutex;
std::string g_ultrasonic_str = "Unknown";

// Global variable to store the mjpeg server process ID.
// When not running, its value is -1.
pid_t mjpeg_pid = -1;

// Intercept buffer to capture lines from std::cerr containing "Converted ECG Value:" and update g_latest_ecg_value
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
        // Search for lines containing "Converted ECG Value:" and update g_latest_ecg_value
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
// Signal handler function to capture SIGINT signal
void signalHandler(int signum) {
    g_running = 0;
}

// Simple HTTP server thread function using Boost.Asio
// It serves two endpoints:
//   "/"     : HTML page with JavaScript for real-time sensor updates
//   "/data" : JSON sensor data
void run_http_server() {
    try {
        boost::asio::io_context io_context;
        // Bind to the specified IP address and port
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("172.20.10.3"), 8000);
        boost::asio::ip::tcp::acceptor acceptor(io_context, endpoint);
        // Set non-blocking mode for polling the g_running flag
        acceptor.non_blocking(true);
        std::cout << "HTTP server started on 172.20.10.3:8000" << std::endl;
        
        while (g_running) {
            boost::asio::ip::tcp::socket socket(io_context);
            boost::system::error_code ec;
            acceptor.accept(socket, ec);
            if (ec) {
                if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again) {
                    std::this_thread::sleep_for(milliseconds(100));
                    continue;
                } else {
                    std::cerr << "Accept error: " << ec.message() << std::endl;
                    continue;
                }
            }
            
            // Read HTTP request header (basic parsing)
            boost::asio::streambuf request;
            boost::asio::read_until(socket, request, "\r\n\r\n", ec);
            std::istream request_stream(&request);
            std::string request_line;
            std::getline(request_stream, request_line);
            if (!request_line.empty() && request_line.back() == '\r') {
                request_line.pop_back();
            }
            
            std::string response;
            if (request_line.find("GET /data") != std::string::npos) {
                // JSON endpoint: prepare JSON response with sensor data
                std::stringstream json;
                json << "{";
                json << "\"light\":\"" << (g_lightState.load() ? "Dark" : "Light") << "\",";
                {
                    std::lock_guard<std::mutex> lock(g_sensor_mutex);
                    json << "\"ultrasonic\":\"" << g_ultrasonic_str << "\",";
                }
                json << "\"pir\":\"" << (g_motionDetected.load() ? "Motion detected" : "No motion") << "\",";
                double ecg_val = g_latest_ecg_value.load();
                json << "\"ecg\":" << std::fixed << std::setprecision(1) << ecg_val;
                json << "}";
                
                std::stringstream response_stream;
                response_stream << "HTTP/1.1 200 OK\r\n";
                response_stream << "Content-Type: application/json\r\n";
                response_stream << "Content-Length: " << json.str().size() << "\r\n";
                response_stream << "\r\n";
                response_stream << json.str();
                response = response_stream.str();
            } else {
                // HTML page endpoint: serve the HTML page with JavaScript for auto-refresh
                std::string html = R"(
                <!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Sensor Dashboard</title>
  <script>
    function updateData() {
      fetch('/data')
      .then(response => response.json())
      .then(data => {
         document.getElementById("light").innerText = data.light;
         document.getElementById("ultrasonic").innerText = data.ultrasonic;
         document.getElementById("pir").innerText = data.pir;
         document.getElementById("ecg").innerText = data.ecg.toFixed(1);
      })
      .catch(err => console.error(err));
    }
    setInterval(updateData, 1000);
    window.onload = updateData;
  </script>
</head>
<body>
  <h1>Sensor Dashboard</h1>
  <p>Light: <span id="light"></span></p>
  <p>Ultrasonic: <span id="ultrasonic"></span></p>
  <p>PIR: <span id="pir"></span></p>
  <p>ECG: <span id="ecg"></span></p>
</body>
</html>
)";
                std::stringstream response_stream;
                response_stream << "HTTP/1.1 200 OK\r\n";
                response_stream << "Content-Type: text/html\r\n";
                response_stream << "Content-Length: " << html.size() << "\r\n";
                response_stream << "\r\n";
                response_stream << html;
                response = response_stream.str();
            }
            
            boost::asio::write(socket, boost::asio::buffer(response), ec);
        }
    } catch (std::exception& e) {
        std::cerr << "HTTP server exception: " << e.what() << std::endl;
    }
}

int main() {
    std::signal(SIGINT, signalHandler);

    // Install custom intercept buffer into std::cerr
    InterceptBuf interceptor(std::cerr.rdbuf());
    std::streambuf* oldCerrBuf = std::cerr.rdbuf(&interceptor);

    // Start HTTP server thread
    std::thread http_thread(run_http_server);
     try {
        // Initialize TTS controller (using "/dev/serial0", corresponding to GPIO14/15, with baud rate 9600)
        TTSController tts("/dev/serial0", B9600);

        // Initialize MotorController (controls motor via GPIO17 and GPIO27)
        MotorController motor;

        // Initialize GPIOButton on GPIO5
        // Short press: If motor is STOP, set to FORWARD; if already FORWARD, then stop the motor.
        // Long press: If motor is STOP, set to BACKWARD, then wake up, play "BACKWARD" text and launch the reverse camera.
        //           If motor is BACKWARD, stop the motor and kill the reverse camera process.
        GPIOButton button5(5,
            // Short press callback (FORWARD)
            [&motor, &tts]() {
                MotorState state = motor.get_state();
                if (state == MotorState::STOP) {
                    motor.set_state(MotorState::FORWARD);
                    std::cout << "Motor set to FORWARD" << std::endl;
                    tts.wake_up();
                    tts.play_text("FORWARD", 0x00, 100);
                    tts.play_text("FORWARD", 0x00, 100);
                } else if (state == MotorState::FORWARD) {
                    motor.set_state(MotorState::STOP);
                    std::cout << "Motor FORWARD stopped" << std::endl;
                } else if (state == MotorState::BACKWARD) {
                    std::cout << "Cannot set FORWARD: Motor is in BACKWARD state. Stop it first." << std::endl;
                }
            },
            // Long press callback (BACKWARD and reverse camera control)
            [&motor, &tts]() {
    MotorState state = motor.get_state();
    if (state == MotorState::STOP) {
        motor.set_state(MotorState::BACKWARD);
        std::cout << "Motor set to BACKWARD" << std::endl;
        tts.wake_up();
        tts.play_text("BACKWARD", 0x00, 100);
        tts.play_text("BACKWARD", 0x00, 100);
        // Launch the reverse camera server in the background using system()
        int ret = system("/home/pi/Desktop/final/motor/mjpeg_server &");
        if (ret != 0) {
            std::cerr << "Failed to launch reverse camera server using system()" << std::endl;
        }
    } else if (state == MotorState::BACKWARD) {
        motor.set_state(MotorState::STOP);
        std::cout << "Motor BACKWARD stopped" << std::endl;
        // Kill the reverse camera server process using pkill
        int ret = system("pkill mjpeg_server");
        if (ret != 0) {
            std::cerr << "Failed to kill reverse camera server using system()" << std::endl;
        }
    } else if (state == MotorState::FORWARD) {
        std::cout << "Cannot set BACKWARD: Motor is in FORWARD state. Stop it first." << std::endl;
    }
}
        );
        if (!button5.start()) {
            std::cerr << "Failed to start button event polling on GPIO5" << std::endl;
            return 1;
        }

        // Initialize LightSensor on GPIO16 and set callback to update g_lightState
        LightSensor lightSensor(16, [](bool state) {
            g_lightState.store(state, std::memory_order_release);
        });
        lightSensor.start();

        // Initialize UltrasonicSensor (Trig: GPIO23, Echo: GPIO24)
        UltrasonicSensor ultrasonicSensor(23, 24);

        // Initialize PIR sensor on GPIO25 (using chip "gpiochip0")
        PIRController pirSensor("gpiochip0", 25, [](bool motion) {
            g_motionDetected.store(motion, std::memory_order_release);
        });
        pirSensor.start(200);

        // Initialize ECG processor and start serial reading (ECG module at /dev/ttyUSB0)
        StableECGProcessor ecgProcessor;
        ReliableSerialReader ecgReader("/dev/ttyUSB0", ecgProcessor);
        ecgProcessor.start();
        ecgReader.start();

        std::cout << "All modules started:" << std::endl;
        std::cout << "  - Motor control via button on GPIO5" << std::endl;
        std::cout << "  - Light sensor on GPIO16" << std::endl;
        std::cout << "  - Ultrasonic sensor on GPIO23/24" << std::endl;
        std::cout << "  - PIR sensor on GPIO25" << std::endl;
        std::cout << "  - ECG processor reading from /dev/ttyUSB0" << std::endl;
        std::cout << "  - TTS (SYN6288) on /dev/serial0 at 9600bps" << std::endl;
        std::cout << "Press Ctrl+C to exit." << std::endl;

        // Timing variables for displaying sensor data
        auto last_light_print = steady_clock::now();
        bool last_light = g_lightState.load(std::memory_order_acquire);
        auto last_ultrasonic = steady_clock::now();
        auto last_pir_print = steady_clock::now();
        bool last_pir = g_motionDetected.load(std::memory_order_acquire);
        auto last_ecg_disp = steady_clock::now();
         while (g_running) {
            std::this_thread::sleep_for(milliseconds(100));
            auto now = steady_clock::now();

            // Light sensor: print every 10 seconds or when state changes
            bool current_light = g_lightState.load(std::memory_order_acquire);
            if (current_light != last_light ||
                duration_cast<seconds>(now - last_light_print).count() >= 10) {
                std::cout << "Light sensor state: " << (current_light ? "Dark" : "Light") << std::endl;
                last_light_print = now;
                last_light = current_light;
            }
             // Ultrasonic sensor: read and print once per second, and update global variable for webpage use
            if (duration_cast<seconds>(now - last_ultrasonic).count() >= 1) {
                std::string distance_str = ultrasonicSensor.get_distance_str();
                std::cout << "Ultrasonic sensor: " << distance_str << std::endl;
                {
                    std::lock_guard<std::mutex> lock(g_sensor_mutex);
                    g_ultrasonic_str = distance_str;
                }
                last_ultrasonic = now;
            }

            // PIR sensor: print when state changes or every second
            bool current_pir = g_motionDetected.load(std::memory_order_acquire);
            if (current_pir != last_pir ||
                duration_cast<seconds>(now - last_pir_print).count() >= 1) {
                std::cout << "PIR sensor state: " << (current_pir ? "Motion detected" : "No motion") << std::endl;
                last_pir_print = now;
                last_pir = current_pir;
            }

            // Every 3 seconds, display the latest ECG converted value captured from debug output
            if (duration_cast<seconds>(now - last_ecg_disp).count() >= 3) {
                double ecg_val = g_latest_ecg_value.load();
                std::cout << "Latest Converted ECG Value: " << std::fixed << std::setprecision(1)
                          << ecg_val << " (converted value)" << std::endl;
                last_ecg_disp = now;
            }
        }
        std::cout << std::endl;
        button5.stop();
        lightSensor.stop();
        pirSensor.stop();
        ecgReader.stop();
        ecgProcessor.stop();

        // Restore std::cerr buffer
        std::cerr.rdbuf(oldCerrBuf);
    } catch (const std::exception& e) {
        std::cerr << "\nFatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // Wait for HTTP server thread to finish
    if (http_thread.joinable())
        http_thread.join();

    return EXIT_SUCCESS;
}
