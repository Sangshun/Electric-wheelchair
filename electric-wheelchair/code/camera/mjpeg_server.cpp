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


// Static inline function to get pixel values ​​in a Bayer image
static inline uint16_t get_pixel(const uint16_t* bayer, int r, int c, int width, int height, int rawStride, int shift) {
    // If the row number is less than 0, set it to 0
    if (r < 0)
        r = 0;
    // If the row number is greater than or equal to the image height, it is set to the image height minus 1
    if (r >= height)
        r = height - 1;
    // If the column number is less than 0, it is set to 0
    if (c < 0)
        c = 0;
    // If the column number is greater than or equal to the image width, it is set to the image width minus 1
    if (c >= width)
        c = width - 1;
    // Returns the pixel value at the specified position in the Bayer image, shifted right by shift bits.
    return bayer[r * rawStride + c] >> shift;
}


std::vector<uint8_t> demosaic_malvar(const uint16_t* bayer, int width, int height, int rawStride, int shift) {

    // Define the gain of the red, green and blue channels
    const double R_gain = 5.0;
    const double G_gain = 4.5;
    const double B_gain = 4.8;


    // Define a vector to store RGB images, with an initial value of 0
    std::vector<uint8_t> rgb(width * height * 3, 0);


    // Iterate over each pixel of the image
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            double R = 0.0, G = 0.0, B = 0.0;

            if ((r % 2 == 1) && (c % 2 == 0)) {

                // Get the value of the current pixel
                R = get_pixel(bayer, r, c, width, height, rawStride, shift);
                // Calculate the sum of surrounding pixels
                double sum1 = get_pixel(bayer, r, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 1, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c, width, height, rawStride, shift);
                // Calculate the sum of the squares of surrounding pixels
                double sum2 = get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 2, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 2, c, width, height, rawStride, shift);
                // Calculate the value of the green pixel
                G = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sum1 - sum2);
                // Calculate the sum of diagonal pixels
                double diag = get_pixel(bayer, r - 1, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 1, c + 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c + 1, width, height, rawStride, shift);
                // Calculate the sum of the squares of blue pixels
                double sumB2 = get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r + 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c + 2, width, height, rawStride, shift);
                // 计算蓝色像素的值
                B = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * diag - sumB2);
            } else if ((r % 2 == 0) && (c % 2 == 1)) {

                // Get the value of the current pixel
                B = get_pixel(bayer, r, c, width, height, rawStride, shift);
                // Calculate the sum of surrounding pixels
                double sum1 = get_pixel(bayer, r, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 1, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c, width, height, rawStride, shift);
                // Calculate the sum of the squares of surrounding pixels
                double sum2 = get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 2, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 2, c, width, height, rawStride, shift);
                // Calculate the value of the green pixel
                G = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sum1 - sum2);
                //Calculate the sum of diagonal pixels
                double diag = get_pixel(bayer, r - 1, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 1, c + 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c + 1, width, height, rawStride, shift);
                // Calculate the sum of the squares of red pixels
                double sumR2 = get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r + 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c + 2, width, height, rawStride, shift);
                // Calculate the value of the red pixel
                R = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * diag - sumR2);
            } else if ((r % 2 == 0) && (c % 2 == 0)) {
            
                // Get the value of the current pixel
                G = get_pixel(bayer, r, c, width, height, rawStride, shift);
                // Calculate the sum of red pixels
                double sumR = get_pixel(bayer, r - 1, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c, width, height, rawStride, shift);
                // Calculate the sum of blue pixels
                double sumB = get_pixel(bayer, r, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 1, width, height, rawStride, shift);
                // Calculate the sum of the squares of red pixels
                double sumR2 = get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r + 2, c, width, height, rawStride, shift);
                // Calculate the sum of the squares of blue pixels
                double sumB2 = get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c + 2, width, height, rawStride, shift);
                // Calculate the value of the red pixel
                R = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sumR - sumR2);
                // Calculate the value of the blue pixel
                B = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sumB - sumB2);
            } else { 
                G = get_pixel(bayer, r, c, width, height, rawStride, shift);
                // Calculate the sum of red pixels
double sumR = get_pixel(bayer, r, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 1, width, height, rawStride, shift);
                // Calculate the sum of blue pixels
                double sumB = get_pixel(bayer, r - 1, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c, width, height, rawStride, shift);
                // Calculate the sum of the squares of red pixels
                double sumR2 = get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c + 2, width, height, rawStride, shift);
                // Calculate the sum of the squares of blue pixels
                double sumB2 = get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r + 2, c, width, height, rawStride, shift);
                // Calculate the value of the red pixel
                R = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sumR - sumR2);
                // Calculate the value of the blue pixel
                B = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sumB - sumB2);
            }

            // Adjust the gain of the red, green and blue channels
            R *= R_gain;
            G *= G_gain;
            B *= B_gain;
            

            // Define a function to limit the value of double type to between 0 and 255 and convert it to uint8_t type
            auto clamp_scale = [](double val) -> uint8_t {
                if (val < 0) val = 0;
                if (val > 1023) val = 1023;
                return static_cast<uint8_t>((val * 255 + 511) / 1023);
            };
            // Calculate the index of the current pixel in the RGB image
            int index = (r * width + c) * 3;
            // Store the calculated values ​​of the red, green, and blue channels into the RGB image
            rgb[index]     = clamp_scale(R);
            rgb[index + 1] = clamp_scale(G);
            rgb[index + 2] = clamp_scale(B);
        }
    }
    return rgb;
}


std::vector<unsigned char> encode_jpeg(const libcamera::FrameBuffer* buffer) {
    // Get the plane of the frame buffer
    const auto& planes = buffer->planes();
    // Get Stream Configuration
    const auto& streamConfig = config->at(0);
    // Get the width and height of an image
    const unsigned int width = streamConfig.size.width;
    const unsigned int height = streamConfig.size.height;

    // Create an RGB buffer
    std::vector<uint8_t> rgbBuffer;

    if (streamConfig.pixelFormat == libcamera::formats::SGBRG10 ||
        streamConfig.pixelFormat == libcamera::formats::SGBRG16) {
        // If the number of planes is not equal to 1, an exception is thrown
        if (planes.size() != 1) {
            throw std::runtime_error("Unexpected plane count for raw Bayer format");
        }
        // Use mmap to map flat data into memory
        void* data = mmap(nullptr, planes[0].length, PROT_READ, MAP_SHARED, planes[0].fd.get(), 0);
        // If the mapping fails, an exception is thrown
        if (data == MAP_FAILED) {
            throw std::runtime_error("mmap failed: " + std::string(strerror(errno)));
        }

        // Convert data to uint16_t pointer
        const uint16_t* bayer = reinterpret_cast<const uint16_t*>(data);
        int rawStride = streamConfig.stride / 2; // Convert byte stride to pixel count

        int shift = (streamConfig.pixelFormat == libcamera::formats::SGBRG16) ? 6 : 0;
        rgbBuffer = demosaic_malvar(bayer, width, height, rawStride, shift);
        munmap(data, planes[0].length);
 } else {
        throw std::runtime_error("This example only supports SGBRG10/SGBRG16 raw Bayer format");
    }

    //Initialize the compression library
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    unsigned char* jpegData = nullptr;
    unsigned long jpegSize = 0;
    jpeg_mem_dest(&cinfo, &jpegData, &jpegSize);

    //Start the compression process
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 80, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    //Write image data line by line to a JPEG file
    JSAMPROW rowPointer[1];
    while (cinfo.next_scanline < cinfo.image_height) {
        rowPointer[0] = &rgbBuffer[cinfo.next_scanline * width * 3];
        jpeg_write_scanlines(&cinfo, rowPointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    std::vector<unsigned char> jpegBuffer(jpegData, jpegData + jpegSize);
    free(jpegData);
    return jpegBuffer;
}
//------------------------------------------------------------------------------
// Asynchronous Accept Helper



//------------------------------------------------------------------------------
// Server Methods

void handle_client(tcp::socket socket) {
    try {
        // Send HTTP header information
        const std::string header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";

        boost::system::error_code ec;
        // Send HTTP header information to the client
        write(socket, buffer(header), ec);
        if (ec) {
            std::cerr << "Initial write error: " << ec.message() << std::endl;
            return;
        }

        // Get the current time and store it
        auto last_frame = std::chrono::steady_clock::now();
        while (socket.is_open()) {
            // Lock
            std::unique_lock<std::mutex> lock(cameraMutex);
            // Get the current time
            auto now = std::chrono::steady_clock::now();
            // Calculate time difference
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame);
            // If the time difference is less than 33 milliseconds, sleep
            if (elapsed < std::chrono::milliseconds(33))
                std::this_thread::sleep_for(std::chrono::milliseconds(33) - elapsed);
            // Update last frame time
            last_frame = now;

            // Waiting for the request to complete
            bool frame_ready = requestCV.wait_for(lock, std::chrono::seconds(1), [&] {
                return std::any_of(requests.begin(), requests.end(), [](const auto& req) {
                    return req->status() == libcamera::Request::RequestComplete;
                });
            });
            if (!frame_ready)
                continue;
 for (auto& request : requests) {
                // If the request status is not completed, skip
                if (request->status() != libcamera::Request::RequestComplete)
                    continue;
                // If the socket is closed, exit the loop
                if (!socket.is_open())
                    break;
                try {
                    // Encoding JPEG images
                    auto jpeg_buffer = encode_jpeg(request->buffers().begin()->second);
                    // Constructing the header information of a JPEG image
                    std::string part_header =
                        "--frame\r\n"
                        "Content-Type: image/jpeg\r\n"
                        "Content-Length: " + std::to_string(jpeg_buffer.size()) + "\r\n\r\n";

                    // Construct the data to be sent
                    std::vector<const_buffer> buffers;
                    buffers.push_back(buffer(part_header));
                    buffers.push_back(buffer(jpeg_buffer));
                    buffers.push_back(buffer("\r\n"));

                    // Sending data to the client
                    write(socket, buffers, ec);
                    if (ec) {
                        std::cerr << "Write error: " << ec.message() << std::endl;
                        socket.close();
                        break;
                    }
                    // Reusing request buffers
                    request->reuse(libcamera::Request::ReuseFlag::ReuseBuffers);
                    // Requeue Request
                    if (camera->queueRequest(request.get()) < 0)
                        throw std::runtime_error("Requeue failed");
                } catch (const std::exception& e) {
                    std::cerr << "Processing error: " << e.what() << std::endl;
                    try {
                        // Reusing request buffers
                        request->reuse(libcamera::Request::ReuseFlag::ReuseBuffers);
                        // Requeue Request
                        camera->queueRequest(request.get());
                    } catch (...) {}
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Client handler error: " << e.what() << std::endl;
    }

    // If the socket is still open, close it
    if (socket.is_open()) {
        boost::system::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);
        socket.close(ec);
    }
}

bool MJPEGServer::init_camera() {
    cameraManager_ = new libcamera::CameraManager();
    if (cameraManager_->start() != 0) {
        std::cerr << "Camera manager initialization failed" << std::endl;
        return false;
    }
    if (cameraManager_->cameras().empty()) {
        std::cerr << "No cameras detected" << std::endl;
        cameraManager_->stop();
        delete cameraManager_;
        cameraManager_ = nullptr;
        return false;
    }
    {
        std::unique_lock<std::mutex> lock(cameraMutex_);
        camera_ = cameraManager_->cameras()[0];
        std::cout << "Initializing camera: " << camera_->id() << std::endl;
        if (camera_->acquire() != 0) {
            std::cerr << "Camera acquisition failed" << std::endl;
            return false;
        }
        config_ = camera_->generateConfiguration({libcamera::StreamRole::Viewfinder});
        if (!config_) {
            std::cerr << "Configuration generation failed" << std::endl;
            return false;
        }
        auto& streamConfig = config_->at(0);
        streamConfig.pixelFormat = libcamera::formats::SGBRG10;
        streamConfig.size = libcamera::Size(640, 480);
        if (config_->validate() == libcamera::CameraConfiguration::Invalid) {
            std::cerr << "Invalid camera configuration" << std::endl;
            return false;
        }
        if (camera_->configure(config_.get()) < 0) {
            std::cerr << "Camera configuration failed" << std::endl;
            return false;
        }
        allocator_ = std::make_unique<libcamera::FrameBufferAllocator>(camera_);
        if (allocator_->allocate(config_->at(0).stream()) < 0) {
            std::cerr << "Buffer allocation failed" << std::endl;
            return false;
        }
        libcamera::ControlList controls;
        controls.set(libcamera::controls::FrameDurationLimits,
                     libcamera::Span<const int64_t, 2>({33333, 33333}));
        for (auto& buffer : allocator_->buffers(config_->at(0).stream())) {
            auto request = camera_->createRequest();
            if (!request)
                continue;
            if (request->addBuffer(config_->at(0).stream(), buffer.get()) == 0) {
                requests_.push_back(std::move(request));
            }
        }
        if (camera_->start() || requests_.empty()) {
            std::cerr << "Camera startup failed" << std::endl;
            return false;
        }
         for (auto& request : requests_) {
            camera_->queueRequest(request.get());
        }
        camera_->requestCompleted.connect(camera_.get(), [this](libcamera::Request* request) {
            requestCV_.notify_all();
        });
    }
    return true;
}

MJPEGServer::MJPEGServer(unsigned short port)
    : cameraManager_(nullptr), port_(port) {
    // Set the global instance pointer for signal handling.
    //instance_ = this;
    //std::signal(SIGINT, MJPEGServer::signal_handler);
}

MJPEGServer::~MJPEGServer() {
    stop();
    if (camera_) {
        camera_->stop();
        camera_->release();
    }
    if (cameraManager_) {
        cameraManager_->stop();
        delete cameraManager_;
    }
}

bool MJPEGServer::start() {
    if (!init_camera())
        return false;
    running_.store(true);
    // Start the server thread which will run the asynchronous accept loop.
    serverThread_ = std::thread(&MJPEGServer::run_server, this);
    return true;
}

void MJPEGServer::run_server() {
    try {
        acceptor_ = std::make_unique<tcp::acceptor>(io_, tcp::endpoint(tcp::v4(), port_));
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
    auto socket = std::make_shared<tcp::socket>(io_);
    acceptor_->async_accept(*socket, [this, socket](const boost::system::error_code &ec) {
        if (!ec && running_.load()) {
            std::thread(&MJPEGServer::handle_client, this, std::move(*socket)).detach();
        }
        if (running_.load()) {
            start_accept(); // Continue accepting new connections.
        }
    });
}

void MJPEGServer::stop() {
    running_.store(false);
    if (acceptor_) {
        boost::system::error_code ec;
        acceptor_->close(ec);
    }
    io_.stop();
    if (serverThread_.joinable())
        serverThread_.join();
}
