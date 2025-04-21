#include <boost/asio.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace std::chrono;

// This function runs an HTTP server that serves dummy sensor data.
// It provides two endpoints:
//   "/"     : HTML page with JavaScript for real-time updates
//   "/data" : JSON data with dummy sensor values
void run_sensor_http_server() {
    try {
        io_context io;
        // Bind to port 8000 on all interfaces
        tcp::endpoint endpoint(tcp::v4(), 8000);
        tcp::acceptor acceptor(io, endpoint);
        // Set non-blocking mode for polling
        acceptor.non_blocking(true);
        std::cout << "Dummy sensor HTTP server started on port 8000" << std::endl;
        
        while (true) {
            tcp::socket socket(io);
            boost::system::error_code ec;
            acceptor.accept(socket, ec);
            if (ec) {
                if (ec == error::would_block || ec == error::try_again) {
                    std::this_thread::sleep_for(milliseconds(100));
                    continue;
                } else {
                    std::cerr << "Accept error: " << ec.message() << std::endl;
                    continue;
                }
            }
            
            // Read HTTP request header (basic parsing)
            boost::asio::streambuf request;
            read_until(socket, request, "\r\n\r\n", ec);
            std::istream request_stream(&request);
            std::string request_line;
            std::getline(request_stream, request_line);
            if (!request_line.empty() && request_line.back() == '\r') {
                request_line.pop_back();
            }
            
            std::string response;
            if (request_line.find("GET /data") != std::string::npos) {
                // Return JSON with dummy sensor values
                std::stringstream json;
                json << "{";
                json << "\"light\":\"Dark\",";
                json << "\"ultrasonic\":\"100cm\",";
                json << "\"pir\":\"No motion\",";
                json << "\"ecg\":0.0";
                json << "}";
                
                std::stringstream response_stream;
                response_stream << "HTTP/1.1 200 OK\r\n";
                response_stream << "Content-Type: application/json\r\n";
                response_stream << "Content-Length: " << json.str().size() << "\r\n";
                response_stream << "\r\n";
                response_stream << json.str();
                response = response_stream.str();
            } else {
                // Return HTML page with JavaScript to auto-update every second
                std::string html = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Dummy Sensor Dashboard</title>
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
  <h1>Dummy Sensor Dashboard</h1>
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
            
            write(socket, buffer(response), ec);
            socket.close();
        }
    } catch (std::exception &e) {
        std::cerr << "HTTP server exception: " << e.what() << std::endl;
    }
}

int main() {
    run_sensor_http_server();
    return 0;
}
