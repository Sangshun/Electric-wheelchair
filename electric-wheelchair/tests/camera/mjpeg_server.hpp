#ifndef MJPEG_SERVER_HPP
#define MJPEG_SERVER_HPP

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <memory>
#include <cstdint>
#include <string>

#include <boost/asio.hpp>
#include <libcamera/libcamera.h>
#include <jpeglib.h>

class MJPEGServer {
public:
    // Constructor: port defaults to 8080.
    MJPEGServer(unsigned short port = 8080);
    ~MJPEGServer();

    // Start the server. Returns true on success.
    bool start();
    // Stop the server gracefully.
    void stop();

    // Static signal handler for SIGINT (Ctrl+C).
    static void signal_handler(int signal);

private:
    // Global pointer to the running instance (for signal handling).
    //static MJPEGServer* instance_;
    // Global running flag.
    static std::atomic<bool> running_;

    // Camera management members.
    libcamera::CameraManager* cameraManager_;
    std::shared_ptr<libcamera::Camera> camera_;
    std::unique_ptr<libcamera::CameraConfiguration> config_;
    std::unique_ptr<libcamera::FrameBufferAllocator> allocator_;
    std::vector<std::unique_ptr<libcamera::Request>> requests_;
    std::mutex cameraMutex_;
    std::condition_variable requestCV_;

    // Boost.Asio server members.
    boost::asio::io_context io_;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
    unsigned short port_;
    std::thread serverThread_;

    // Helper method to start asynchronous accept.
    void start_accept();

    // Server methods.
    void run_server();
    void handle_client(boost::asio::ip::tcp::socket socket);

    // Image processing helper functions.
    uint16_t get_pixel(const uint16_t* bayer, int r, int c, int width, int height, int rawStride, int shift);
    std::vector<uint8_t> demosaic_malvar(const uint16_t* bayer, int width, int height, int rawStride, int shift);
    std::vector<unsigned char> encode_jpeg(const libcamera::FrameBuffer* buffer);

    // Camera initialization.
    bool init_camera();
};

#endif // MJPEG_SERVER_HPP
