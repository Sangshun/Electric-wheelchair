#include <libcamera/libcamera.h>
#include <boost/asio.hpp>
#include <jpeglib.h>
#include <vector>
#include <thread>
#include <sys/mman.h>
#include <mutex>
#include <memory>
#include <iostream>
#include <fstream>
#include <condition_variable>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <cstdint>

using namespace boost::asio;
using namespace boost::asio::ip;


libcamera::CameraManager* cameraManager = nullptr;
std::shared_ptr<libcamera::Camera> camera;
std::unique_ptr<libcamera::CameraConfiguration> config;
std::mutex cameraMutex;
std::vector<std::unique_ptr<libcamera::Request>> requests;
std::condition_variable requestCV;
std::unique_ptr<libcamera::FrameBufferAllocator> allocator;


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

int main() {
    // Creating a Camera Manager
    cameraManager = new libcamera::CameraManager();
    // Launch Camera Manager
    if (cameraManager->start() != 0) {
        std::cerr << "Camera manager initialization failed" << std::endl;
        return 1;
    }
    // Check if a camera is detected
    if (cameraManager->cameras().empty()) {
        std::cerr << "No cameras detected" << std::endl;
        cameraManager->stop();
        delete cameraManager;
        return 1;
    }
    {
        // Lock
        std::unique_lock<std::mutex> lock(cameraMutex);
        // Get the first camera
        camera = cameraManager->cameras()[0];
        std::cout << "Initializing camera: " << camera->id() << std::endl;
        // Get the Camera
        if (camera->acquire() != 0) {
            std::cerr << "Camera acquisition failed" << std::endl;
            return 1;
        }
        // Generate configuration
        config = camera->generateConfiguration({libcamera::StreamRole::Viewfinder});
        if (!config) {
            std::cerr << "Configuration generation failed" << std::endl;
            return 1;
        }
auto& streamConfig = config->at(0);
      
        // Set the pixel format
        streamConfig.pixelFormat = libcamera::formats::SGBRG10;
        // Set the resolution
        streamConfig.size = libcamera::Size(640, 480);
        // Verify the configuration
        if (config->validate() == libcamera::CameraConfiguration::Invalid) {
            std::cerr << "Invalid camera configuration" << std::endl;
            return 1;
        }
        // Configuring the Camera
        if (camera->configure(config.get()) < 0) {
            std::cerr << "Camera configuration failed" << std::endl;
            return 1;
        }
        // Allocating Buffers
        allocator = std::make_unique<libcamera::FrameBufferAllocator>(camera);
        if (allocator->allocate(config->at(0).stream()) < 0) {
            std::cerr << "Buffer allocation failed" << std::endl;
            return 1;
        }
        // Setting control parameters
        libcamera::ControlList controls;
        controls.set(libcamera::controls::FrameDurationLimits,
                     libcamera::Span<const int64_t, 2>({33333, 33333}));
        // Create a request
        for (auto& buffer : allocator->buffers(config->at(0).stream())) {
            auto request = camera->createRequest();
            if (!request)
                continue;
            if (request->addBuffer(config->at(0).stream(), buffer.get()) == 0) {
                requests.push_back(std::move(request));
            }
        }
        // Activate the camera
        if (camera->start() || requests.empty()) {
            std::cerr << "Camera startup failed" << std::endl;
            return 1;
        }
        // Queuing Requests
        for (auto& request : requests) {
            camera->queueRequest(request.get());
        }
        // Connection request completion signal
        camera->requestCompleted.connect(camera.get(), [](libcamera::Request* request) {
            requestCV.notify_all();
        });
    }
    // Creating an IO Context
    io_context io;
    // Creating a TCP Listener
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8080));
    std::cout << "Server running on port 8080" << std::endl;
    // Loop to accept client connections
    while (true) {
        tcp::socket socket(io);
        acceptor.accept(socket);
        // Create a thread to handle client connections
        std::thread(handle_client, std::move(socket)).detach();
    }
    // Stop Camera
    camera->stop();
    // Release the camera
    camera->release();
    // Stop the camera manager
    cameraManager->stop();
    // Remove Camera Manager
    delete cameraManager;
    return 0;
}