g++ -std=c++17 -I/usr/include/libcamera -o test_mjpeg_server mjpeg_server.cpp test_mjpeg_server.cpp -lboost_system -pthread -ljpeg $(pkg-config --libs libcamera)
