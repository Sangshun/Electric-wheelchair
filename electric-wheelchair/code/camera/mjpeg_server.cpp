#include "mjpeg_server.hpp"
#include <boost/asio.hpp>
#include <jpeglib.h>
#include <sys/mman.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <csignal>

// For brevity
using namespace boost::asio;
using namespace boost::asio::ip;

// Define static members.
//MJPEGServer* MJPEGServer::instance_ = nullptr;
std::atomic<bool> MJPEGServer::running_{true};

//------------------------------------------------------------------------------
// Static signal handler definition.
//void MJPEGServer::signal_handler(int signal) {
//    if (signal == SIGINT) {
//        std::cout << "\nSIGINT received, stopping MJPEG server..." << std::endl;
//        if (instance_) {
//            instance_->stop();
 //       }
//        running_.store(false);
//    }
//}

//------------------------------------------------------------------------------
// Image Processing Helper Functions
// Boundary clipping: If the row or column is out of range, "clip" to the nearest legal value
uint16_t MJPEGServer::get_pixel(const uint16_t* bayer, int r, int c, int width, int height, int rawStride, int shift) {
    if (r < 0)
        r = 0;
    if (r >= height)
        r = height - 1;
    if (c < 0)
        c = 0;
    if (c >= width)
        c = width - 1;
    return bayer[r * rawStride + c] >> shift; // Access the original data and shift right by shift bits (processing 10/16 bit data)
}
// Use the Malvar demosaicing algorithm to convert the Bayer format to RGB
std::vector<uint8_t> MJPEGServer::demosaic_malvar(const uint16_t* bayer, int width, int height, int rawStride, int shift) {// Gain coefficient for each channel to increase color saturation
    const double R_gain = 5.0;
    const double G_gain = 4.5;
    const double B_gain = 4.8;
// Allocate RGB buffer: 3 bytes per pixel
    std::vector<uint8_t> rgb(width * height * 3, 0);
    for (int r = 0; r < height; r++) { // Iterate through each pixel
        for (int c = 0; c < width; c++) {
            double R = 0.0, G = 0.0, B = 0.0;// Calculate R/G/B according to different coordinates of RGGB mode
            if ((r % 2 == 1) && (c % 2 == 0)) { // Determine the pixel position of the Bayer format
                R = get_pixel(bayer, r, c, width, height, rawStride, shift); // Read the red component of the current pixel from the Bayer data
                double sum1 = get_pixel(bayer, r, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 1, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c, width, height, rawStride, shift);
                double sum2 = get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 2, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 2, c, width, height, rawStride, shift);
                G = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sum1 - sum2);// Calculate adjacent green pixel interpolation
                double diag = get_pixel(bayer, r - 1, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 1, c + 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c + 1, width, height, rawStride, shift);
                double sumB2 = get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r + 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c + 2, width, height, rawStride, shift);
                B = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * diag - sumB2);// Calculate adjacent blue pixel interpolation
            } else if ((r % 2 == 0) && (c % 2 == 1)) { 
				    B = get_pixel(bayer, r, c, width, height, rawStride, shift);// Blue pixel position
                double sum1 = get_pixel(bayer, r, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 1, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c, width, height, rawStride, shift);
                double sum2 = get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 2, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 2, c, width, height, rawStride, shift);
                G = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sum1 - sum2);// Calculate green interpolation
                double diag = get_pixel(bayer, r - 1, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 1, c + 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c + 1, width, height, rawStride, shift);
                double sumR2 = get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r + 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c + 2, width, height, rawStride, shift);
                R = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * diag - sumR2);// Calculate red interpolation
            } else if ((r % 2 == 0) && (c % 2 == 0)) { 
                G = get_pixel(bayer, r, c, width, height, rawStride, shift);// Green pixel (G1) position
                double sumR = get_pixel(bayer, r - 1, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c, width, height, rawStride, shift);
                double sumB = get_pixel(bayer, r, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 1, width, height, rawStride, shift);
                double sumR2 = get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r + 2, c, width, height, rawStride, shift);
                double sumB2 = get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c + 2, width, height, rawStride, shift); 
                R = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sumR - sumR2);// Calculate red interpolation
                B = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sumB - sumB2);// Calculate blue interpolation
            } else {
                G = get_pixel(bayer, r, c, width, height, rawStride, shift);// Another green pixel (G2) position, same as above
                double sumR = get_pixel(bayer, r, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 1, width, height, rawStride, shift);
                double sumB = get_pixel(bayer, r - 1, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c, width, height, rawStride, shift);
                double sumR2 = get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c + 2, width, height, rawStride, shift);
                double sumB2 = get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r + 2, c, width, height, rawStride, shift);// Interpolation calculation R and B
                R = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sumR - sumR2);
                B = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sumB - sumB2);
            }
            R *= R_gain;
            G *= G_gain;
            B *= B_gain;// Apply gain to increase saturation
            auto clamp_scale = [](double val) -> uint8_t {
                if (val < 0) val = 0;
                if (val > 1023) val = 1023;
                return static_cast<uint8_t>((val * 255 + 511) / 1023);
            };// Map the range of 0~1023 to 0~255 and round it up
            int index = (r * width + c) * 3;// Calculate the starting position of the pixel in the RGB one-dimensional array
            rgb[index]     = clamp_scale(R);
            rgb[index + 1] = clamp_scale(G);
            rgb[index + 2] = clamp_scale(B);
        }
    }
    return rgb;
}

std::vector<unsigned char> MJPEGServer::encode_jpeg(const libcamera::FrameBuffer* buffer) {// Compress the RGB buffer to JPEG format and return a byte array
    const auto& planes = buffer->planes();
    const auto& streamConfig = config_->at(0);
    const unsigned int width = streamConfig.size.width;
    const unsigned int height = streamConfig.size.height;

    std::vector<uint8_t> rgbBuffer;// Only supports SGBRG10 / SGBRG16 raw Bayer format
    if (streamConfig.pixelFormat == libcamera::formats::SGBRG10 ||
        streamConfig.pixelFormat == libcamera::formats::SGBRG16) {
        if (planes.size() != 1) {// Make sure there is only one plane, otherwise the logic cannot handle it
            throw std::runtime_error("Unexpected plane count for raw Bayer format");
        }
        void* data = mmap(nullptr, planes[0].length, PROT_READ, MAP_SHARED, planes[0].fd.get(), 0);// Memory map raw data
        if (data == MAP_FAILED) {
            throw std::runtime_error("mmap failed: " + std::string(strerror(errno)));
        }
        const uint16_t* bayer = reinterpret_cast<const uint16_t*>(data);// reinterpret as 16-bit unsigned
        int rawStride = streamConfig.stride / 2; // Calculate the number of pixels per row (stride is in bytes, divided by 2 to get the number of pixels)
        int shift = (streamConfig.pixelFormat == libcamera::formats::SGBRG16) ? 6 : 0;// If it is a 16-bit format, you need to shift right 6 bits to match the 10-bit precision
        rgbBuffer = demosaic_malvar(bayer, width, height, rawStride, shift);// De-mosaic, generate RGB buffer
        munmap(data, planes[0].length);// Unmap the memory
    } else {
        throw std::runtime_error("This example only supports SGBRG10/SGBRG16 raw Bayer format");
    }
    // Set up libjpeg compression structure
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    unsigned char* jpegData = nullptr;// Output to memory buffer
    unsigned long jpegSize = 0;
    jpeg_mem_dest(&cinfo, &jpegData, &jpegSize);
    // Configure output image parameters
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;//RGB three channels
    cinfo.in_color_space = JCS_RGB;// Input color space
    jpeg_set_defaults(&cinfo);//Default compression settings
    jpeg_set_quality(&cinfo, 80, TRUE);// Quality level 80
    jpeg_start_compress(&cinfo, TRUE);
    // Write JPEG data line by line
    JSAMPROW rowPointer[1];
    while (cinfo.next_scanline < cinfo.image_height) {
        rowPointer[0] = &rgbBuffer[cinfo.next_scanline * width * 3];
        jpeg_write_scanlines(&cinfo, rowPointer, 1);
    }
    // Complete compression and cleanup
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    // Copy the compressed data to std::vector and release the memory allocated by libjpeg
    std::vector<unsigned char> jpegBuffer(jpegData, jpegData + jpegSize);
    free(jpegData);
    return jpegBuffer;
}

//------------------------------------------------------------------------------
// Asynchronous Accept Helper



//------------------------------------------------------------------------------
// Server Methods

void MJPEGServer::handle_client(tcp::socket socket) {
    try {// Send HTTP headers indicating a multipart MJPEG stream
        const std::string header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
        boost::system::error_code ec;
        write(socket, buffer(header), ec);
        if (ec) {
            std::cerr << "Initial write error: " << ec.message() << std::endl;
            return;
        }
        auto last_frame = std::chrono::steady_clock::now();// Last frame sent time, used to limit the frame rate (about 30 FPS)
        while (socket.is_open() && running_.load()) {// Loop to send each frame
            std::unique_lock<std::mutex> lock(cameraMutex_);
            auto now = std::chrono::steady_clock::now();// Control frame rate: minimum interval 33ms
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame);
            if (elapsed < std::chrono::milliseconds(33))
                std::this_thread::sleep_for(std::chrono::milliseconds(33) - elapsed);
            last_frame = now;
            bool frame_ready = requestCV_.wait_for(lock, std::chrono::seconds(1), [&] {// Wait for a new frame to be ready or timeout
                return std::any_of(requests_.begin(), requests_.end(), [](const auto& req) {// Check if any request is completed
                    return req->status() == libcamera::Request::RequestComplete;
                });
            });
            if (!frame_ready)
                continue;
            // Run flag check
            if (!running_.load())
                break;
            for (auto& request : requests_) {// Traverse all completed requests, encode and send
                if (request->status() != libcamera::Request::RequestComplete)
                    continue;
                if (!socket.is_open())
                    break;
                try {
                    auto jpeg_buffer = encode_jpeg(request->buffers().begin()->second);// Encode frame data as JPEG
                    std::string part_header = // Construct segment header
                        "--frame\r\n"
                        "Content-Type: image/jpeg\r\n"
                        "Content-Length: " + std::to_string(jpeg_buffer.size()) + "\r\n\r\n";
                    std::vector<const_buffer> buffers;// Aggregate header and image data
                    buffers.push_back(buffer(part_header));
                    buffers.push_back(buffer(jpeg_buffer));
                    buffers.push_back(buffer("\r\n"));
                    write(socket, buffers, ec);// Asynchronous write
                    if (ec) {
                        std::cerr << "Write error: " << ec.message() << std::endl;
                        socket.close();
                        break;
                    }
                    request->reuse(libcamera::Request::ReuseFlag::ReuseBuffers);// Reuse the buffer and continue with the next request
                    if (camera_->queueRequest(request.get()) < 0)
                        throw std::runtime_error("Requeue failed");
                } catch (const std::exception& e) {
                    std::cerr << "Processing error: " << e.what() << std::endl;
                    try {// When an error occurs, try to requeue the request
                        request->reuse(libcamera::Request::ReuseFlag::ReuseBuffers);
                        camera_->queueRequest(request.get());
                    } catch (...) {}
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Client handler error: " << e.what() << std::endl;
    }
    if (socket.is_open()) {// Close the socket and clean up
        boost::system::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);
        socket.close(ec);
    }
}


// Camera initialization and configuration
bool MJPEGServer::init_camera() {
    cameraManager_ = new libcamera::CameraManager();// Create CameraManager and start it
    if (cameraManager_->start() != 0) {
        std::cerr << "Camera manager initialization failed" << std::endl;
        return false;
    }
    if (cameraManager_->cameras().empty()) {// Check if there is an available camera
        std::cerr << "No cameras detected" << std::endl;
        cameraManager_->stop();
        delete cameraManager_;
        cameraManager_ = nullptr;
        return false;
    }
    {
        std::unique_lock<std::mutex> lock(cameraMutex_);
        camera_ = cameraManager_->cameras()[0];// Get the first camera
        std::cout << "Initializing camera: " << camera_->id() << std::endl;
        if (camera_->acquire() != 0) {// Request and get the camera
            std::cerr << "Camera acquisition failed" << std::endl;
            return false;
        }
        config_ = camera_->generateConfiguration({libcamera::StreamRole::Viewfinder});// Generate default configuration (Viewfinder)
        if (!config_) {
            std::cerr << "Configuration generation failed" << std::endl;
            return false;
        }
        auto& streamConfig = config_->at(0);// Modify pixel format and resolution
        streamConfig.pixelFormat = libcamera::formats::SGBRG10;
        streamConfig.size = libcamera::Size(640, 480);
        if (config_->validate() == libcamera::CameraConfiguration::Invalid) {// Verify configuration validity
            std::cerr << "Invalid camera configuration" << std::endl;
            return false;
        }
        if (camera_->configure(config_.get()) < 0) {// Application configuration
            std::cerr << "Camera configuration failed" << std::endl;
            return false;
        }
        allocator_ = std::make_unique<libcamera::FrameBufferAllocator>(camera_);// Allocate frame buffer
        if (allocator_->allocate(config_->at(0).stream()) < 0) {
            std::cerr << "Buffer allocation failed" << std::endl;
            return false;
        }
        libcamera::ControlList controls;// Set frame rate control (30 FPS fixed interval)
        controls.set(libcamera::controls::FrameDurationLimits,
                     libcamera::Span<const int64_t, 2>({33333, 33333}));
        for (auto& buffer : allocator_->buffers(config_->at(0).stream())) {// Create a Request for each buffer and add it to the queue
            auto request = camera_->createRequest();
            if (!request)
                continue;
            if (request->addBuffer(config_->at(0).stream(), buffer.get()) == 0) {
                requests_.push_back(std::move(request));
            }
        }
        if (camera_->start() || requests_.empty()) {// Start the camera, if it fails or there is no request, return false
            std::cerr << "Camera startup failed" << std::endl;
            return false;
        }
         for (auto& request : requests_) {// Put all requests into the shooting queue
            camera_->queueRequest(request.get());
        }
        camera_->requestCompleted.connect(camera_.get(), [this](libcamera::Request* request) {//When each request is completed, notify the waiting thread
            requestCV_.notify_all();
        });
    }
    return true;
}
//Constructor and destructor
MJPEGServer::MJPEGServer(unsigned short port)
    : cameraManager_(nullptr), port_(port) {
    // Set the global instance pointer for signal handling.
    //instance_ = this;
    //std::signal(SIGINT, MJPEGServer::signal_handler);
}

MJPEGServer::~MJPEGServer() {
    stop();// Stop the server
    if (camera_) {
        camera_->stop();
        camera_->release();
    }// Stop and release the camera
    if (cameraManager_) {
        cameraManager_->stop();
        delete cameraManager_;
    }// Stop and release CameraManager
}
// Start and stop the server
bool MJPEGServer::start() {
    if (!init_camera())// Initialize the camera and return false if failed
        return false;
    running_.store(true);// Flag set to run
    // Start the server thread which will run the asynchronous accept loop.
    serverThread_ = std::thread(&MJPEGServer::run_server, this);
    return true;
}

void MJPEGServer::run_server() {
    try {
        acceptor_ = std::make_unique<tcp::acceptor>(io_, tcp::endpoint(tcp::v4(), port_));// Create a TCP receiver to listen on the specified port
        std::cout << "MJPEG server running on port " << port_ << std::endl;
        // Start asynchronous accept loop.
        start_accept();
        // Run the io_context. This will return when io_.stop() is called.
        io_.run();
    } catch (const std::exception& e) {
        std::cerr << "Server exception: " << e.what() << std::endl;
    }
}

void MJPEGServer::start_accept() {
    auto socket = std::make_shared<tcp::socket>(io_);// Create a new socket (managed by shared_ptr)
    acceptor_->async_accept(*socket, [this, socket](const boost::system::error_code &ec) {
        if (!ec && running_.load()) {//Accept the callback executed after completion
            std::thread(&MJPEGServer::handle_client, this, std::move(*socket)).detach();
        }
        if (running_.load()) {// If still running, continue to accept the next connection
            start_accept(); 
        }
    });
}

void MJPEGServer::stop() {
    running_.store(false);// Turn off the running flag
    if (acceptor_) {
        boost::system::error_code ec;
        acceptor_->close(ec);
    }
    io_.stop();// Stop io_context
    if (serverThread_.joinable())// Wait for the server thread to exit
        serverThread_.join();
}
