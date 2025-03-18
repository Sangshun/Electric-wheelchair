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


static inline uint16_t get_pixel(const uint16_t* bayer, int r, int c, int width, int height, int rawStride, int shift) {
    if (r < 0)
        r = 0;
    if (r >= height)
        r = height - 1;
    if (c < 0)
        c = 0;
    if (c >= width)
        c = width - 1;
    return bayer[r * rawStride + c] >> shift;
}


std::vector<uint8_t> demosaic_malvar(const uint16_t* bayer, int width, int height, int rawStride, int shift) {

    const double R_gain = 5.0;
    const double G_gain = 4.5;
    const double B_gain = 4.8;


    std::vector<uint8_t> rgb(width * height * 3, 0);


    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            double R = 0.0, G = 0.0, B = 0.0;

            if ((r % 2 == 1) && (c % 2 == 0)) {

                R = get_pixel(bayer, r, c, width, height, rawStride, shift);
                double sum1 = get_pixel(bayer, r, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 1, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c, width, height, rawStride, shift);
                double sum2 = get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 2, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 2, c, width, height, rawStride, shift);
                G = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sum1 - sum2);
                double diag = get_pixel(bayer, r - 1, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 1, c + 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c + 1, width, height, rawStride, shift);
                double sumB2 = get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r + 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c + 2, width, height, rawStride, shift);
                B = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * diag - sumB2);
            } else if ((r % 2 == 0) && (c % 2 == 1)) {

                B = get_pixel(bayer, r, c, width, height, rawStride, shift);
                double sum1 = get_pixel(bayer, r, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 1, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c, width, height, rawStride, shift);
                double sum2 = get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 2, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 2, c, width, height, rawStride, shift);
                G = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sum1 - sum2);
                double diag = get_pixel(bayer, r - 1, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r - 1, c + 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c + 1, width, height, rawStride, shift);
                double sumR2 = get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r + 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c + 2, width, height, rawStride, shift);
                R = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * diag - sumR2);
            } else if ((r % 2 == 0) && (c % 2 == 0)) {
            
                G = get_pixel(bayer, r, c, width, height, rawStride, shift);
                double sumR = get_pixel(bayer, r - 1, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c, width, height, rawStride, shift);
                double sumB = get_pixel(bayer, r, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 1, width, height, rawStride, shift);
                double sumR2 = get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r + 2, c, width, height, rawStride, shift);
                double sumB2 = get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c + 2, width, height, rawStride, shift);
                R = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sumR - sumR2);
                B = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sumB - sumB2);
            } else { 
                G = get_pixel(bayer, r, c, width, height, rawStride, shift);
double sumR = get_pixel(bayer, r, c - 1, width, height, rawStride, shift) +
                              get_pixel(bayer, r, c + 1, width, height, rawStride, shift);
                double sumB = get_pixel(bayer, r - 1, c, width, height, rawStride, shift) +
                              get_pixel(bayer, r + 1, c, width, height, rawStride, shift);
                double sumR2 = get_pixel(bayer, r, c - 2, width, height, rawStride, shift) +
                               get_pixel(bayer, r, c + 2, width, height, rawStride, shift);
                double sumB2 = get_pixel(bayer, r - 2, c, width, height, rawStride, shift) +
                               get_pixel(bayer, r + 2, c, width, height, rawStride, shift);
                R = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sumR - sumR2);
                B = get_pixel(bayer, r, c, width, height, rawStride, shift) + 0.125 * (2 * sumB - sumB2);
            }

            R *= R_gain;
            G *= G_gain;
            B *= B_gain;
            

            auto clamp_scale = [](double val) -> uint8_t {
                if (val < 0) val = 0;
                if (val > 1023) val = 1023;
                return static_cast<uint8_t>((val * 255 + 511) / 1023);
            };
            int index = (r * width + c) * 3;
            rgb[index]     = clamp_scale(R);
            rgb[index + 1] = clamp_scale(G);
            rgb[index + 2] = clamp_scale(B);
        }
    }
    return rgb;
}


std::vector<unsigned char> encode_jpeg(const libcamera::FrameBuffer* buffer) {
    const auto& planes = buffer->planes();
    const auto& streamConfig = config->at(0);
    const unsigned int width = streamConfig.size.width;
    const unsigned int height = streamConfig.size.height;

    std::vector<uint8_t> rgbBuffer;

    if (streamConfig.pixelFormat == libcamera::formats::SGBRG10 ||
        streamConfig.pixelFormat == libcamera::formats::SGBRG16) {
        if (planes.size() != 1) {
            throw std::runtime_error("Unexpected plane count for raw Bayer format");
        }
        void* data = mmap(nullptr, planes[0].length, PROT_READ, MAP_SHARED, planes[0].fd.get(), 0);
        if (data == MAP_FAILED) {
            throw std::runtime_error("mmap failed: " + std::string(strerror(errno)));
        }

        const uint16_t* bayer = reinterpret_cast<const uint16_t*>(data);
        int rawStride = streamConfig.stride / 2; // Convert byte stride to pixel count

        int shift = (streamConfig.pixelFormat == libcamera::formats::SGBRG16) ? 6 : 0;
        rgbBuffer = demosaic_malvar(bayer, width, height, rawStride, shift);
        munmap(data, planes[0].length);
 } else {
        throw std::runtime_error("This example only supports SGBRG10/SGBRG16 raw Bayer format");
    }


    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    unsigned char* jpegData = nullptr;
    unsigned long jpegSize = 0;
    jpeg_mem_dest(&cinfo, &jpegData, &jpegSize);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 80, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

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
        const std::string header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";

        boost::system::error_code ec;
        write(socket, buffer(header), ec);
        if (ec) {
            std::cerr << "Initial write error: " << ec.message() << std::endl;
            return;
        }

        auto last_frame = std::chrono::steady_clock::now();
        while (socket.is_open()) {
            std::unique_lock<std::mutex> lock(cameraMutex);
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame);
            if (elapsed < std::chrono::milliseconds(33))
                std::this_thread::sleep_for(std::chrono::milliseconds(33) - elapsed);
            last_frame = now;

            bool frame_ready = requestCV.wait_for(lock, std::chrono::seconds(1), [&] {
                return std::any_of(requests.begin(), requests.end(), [](const auto& req) {
                    return req->status() == libcamera::Request::RequestComplete;
                });
            });
            if (!frame_ready)
                continue;
 for (auto& request : requests) {
                if (request->status() != libcamera::Request::RequestComplete)
                    continue;
                if (!socket.is_open())
                    break;
                try {
                    auto jpeg_buffer = encode_jpeg(request->buffers().begin()->second);
                    std::string part_header =
                        "--frame\r\n"
                        "Content-Type: image/jpeg\r\n"
                        "Content-Length: " + std::to_string(jpeg_buffer.size()) + "\r\n\r\n";

                    std::vector<const_buffer> buffers;
                    buffers.push_back(buffer(part_header));
                    buffers.push_back(buffer(jpeg_buffer));
                    buffers.push_back(buffer("\r\n"));

                    write(socket, buffers, ec);
                    if (ec) {
                        std::cerr << "Write error: " << ec.message() << std::endl;
                        socket.close();
                        break;
                    }
                    request->reuse(libcamera::Request::ReuseFlag::ReuseBuffers);
                    if (camera->queueRequest(request.get()) < 0)
                        throw std::runtime_error("Requeue failed");
                } catch (const std::exception& e) {
                    std::cerr << "Processing error: " << e.what() << std::endl;
                    try {
                        request->reuse(libcamera::Request::ReuseFlag::ReuseBuffers);
                        camera->queueRequest(request.get());
                    } catch (...) {}
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Client handler error: " << e.what() << std::endl;
    }

    if (socket.is_open()) {
        boost::system::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);
        socket.close(ec);
    }
}

int main() {
    cameraManager = new libcamera::CameraManager();
    if (cameraManager->start() != 0) {
        std::cerr << "Camera manager initialization failed" << std::endl;
        return 1;
    }
    if (cameraManager->cameras().empty()) {
        std::cerr << "No cameras detected" << std::endl;
        cameraManager->stop();
        delete cameraManager;
        return 1;
    }
    {
        std::unique_lock<std::mutex> lock(cameraMutex);
        camera = cameraManager->cameras()[0];
        std::cout << "Initializing camera: " << camera->id() << std::endl;
        if (camera->acquire() != 0) {
            std::cerr << "Camera acquisition failed" << std::endl;
            return 1;
        }
        config = camera->generateConfiguration({libcamera::StreamRole::Viewfinder});
        if (!config) {
            std::cerr << "Configuration generation failed" << std::endl;
            return 1;
        }
auto& streamConfig = config->at(0);
      
        streamConfig.pixelFormat = libcamera::formats::SGBRG10;
        streamConfig.size = libcamera::Size(640, 480);
        if (config->validate() == libcamera::CameraConfiguration::Invalid) {
            std::cerr << "Invalid camera configuration" << std::endl;
            return 1;
        }
        if (camera->configure(config.get()) < 0) {
            std::cerr << "Camera configuration failed" << std::endl;
            return 1;
        }
        allocator = std::make_unique<libcamera::FrameBufferAllocator>(camera);
        if (allocator->allocate(config->at(0).stream()) < 0) {
            std::cerr << "Buffer allocation failed" << std::endl;
            return 1;
        }
        libcamera::ControlList controls;
        controls.set(libcamera::controls::FrameDurationLimits,
                     libcamera::Span<const int64_t, 2>({33333, 33333}));
        for (auto& buffer : allocator->buffers(config->at(0).stream())) {
            auto request = camera->createRequest();
            if (!request)
                continue;
            if (request->addBuffer(config->at(0).stream(), buffer.get()) == 0) {
                requests.push_back(std::move(request));
            }
        }
        if (camera->start() || requests.empty()) {
            std::cerr << "Camera startup failed" << std::endl;
            return 1;
        }
        for (auto& request : requests) {
            camera->queueRequest(request.get());
        }
        camera->requestCompleted.connect(camera.get(), [](libcamera::Request* request) {
            requestCV.notify_all();
        });
    }
    io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8080));
    std::cout << "Server running on port 8080" << std::endl;
    while (true) {
        tcp::socket socket(io);
        acceptor.accept(socket);
        std::thread(handle_client, std::move(socket)).detach();
    }
    camera->stop();
    camera->release();
    cameraManager->stop();
    delete cameraManager;
    return 0;
}
