cmake_minimum_required(VERSION 3.10)
project(ElectricWheelchair LANGUAGES C CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Threads
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# libgpiod C++ API
pkg_check_modules(GPIO_CXX REQUIRED libgpiodcxx)
# libgpiod C API
pkg_check_modules(GPIO_C   REQUIRED libgpiod)
# libcamera (for MJPEGServer)
pkg_check_modules(LIBC     REQUIRED libcamera)
# libjpeg (for MJPEGServer)
pkg_check_modules(JPEG     REQUIRED libjpeg)

# include dirs
include_directories(
  ${GPIO_CXX_INCLUDE_DIRS}
  ${GPIO_C_INCLUDE_DIRS}
  ${LIBC_INCLUDE_DIRS}
  ${JPEG_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/code/Button
  ${CMAKE_SOURCE_DIR}/code/motor
  ${CMAKE_SOURCE_DIR}/code/LightSensor
  ${CMAKE_SOURCE_DIR}/code/UltrasonicSensor
  ${CMAKE_SOURCE_DIR}/code/pir_sensor
  ${CMAKE_SOURCE_DIR}/code/ecg_processor
  ${CMAKE_SOURCE_DIR}/code/syn6288_controller
  ${CMAKE_SOURCE_DIR}/code/camera
  ${CMAKE_SOURCE_DIR}/code/LED
)

link_directories(
  ${GPIO_CXX_LIBRARY_DIRS}
  ${GPIO_C_LIBRARY_DIRS}
  ${LIBC_LIBRARY_DIRS}
  ${JPEG_LIBRARY_DIRS}
)

set(GPIOD_LIBS
  ${GPIO_CXX_LIBRARIES}
  ${GPIO_C_LIBRARIES}
)
set(CAMERA_LIBS
  ${LIBC_LIBRARIES}
)
set(JPEG_LIBS
  ${JPEG_LIBRARIES}
)

# --- Modules ---

add_library(GPIOButton STATIC
  code/Button/GPIOButton.cpp
  code/Button/GPIOButton.hpp
)
target_link_libraries(GPIOButton
  PRIVATE ${GPIOD_LIBS} Threads::Threads
)

add_library(MotorController STATIC
  code/motor/MotorController.cpp
  code/motor/MotorController.hpp
)
target_link_libraries(MotorController
  PRIVATE ${GPIOD_LIBS} Threads::Threads
)
add_library(LightSensor STATIC
  code/LightSensor/LightSensor.cpp
  code/LightSensor/LightSensor.hpp
)
target_link_libraries(LightSensor
  PRIVATE ${GPIOD_LIBS} Threads::Threads
)

add_library(UltrasonicSensor STATIC
  code/UltrasonicSensor/UltrasonicSensor.cpp
  code/UltrasonicSensor/UltrasonicSensor.hpp
)
target_link_libraries(UltrasonicSensor
  PRIVATE ${GPIOD_LIBS} Threads::Threads
)

add_library(PIRController STATIC
  code/pir_sensor/pir_sensor.cpp
  code/pir_sensor/pir_sensor.hpp
)
target_link_libraries(PIRController
  PRIVATE ${GPIOD_LIBS} Threads::Threads
)

add_library(ECGProcessor STATIC
  code/ecg_processor/ecg_processor.cpp
  code/ecg_processor/ecg_processor.hpp
)
target_link_libraries(ECGProcessor
  PRIVATE Threads::Threads
)

add_library(TTSController STATIC
  code/syn6288_controller/syn6288_controller.cpp
  code/syn6288_controller/syn6288_controller.hpp
)
target_link_libraries(TTSController
  PRIVATE Threads::Threads
)

add_library(MJPEGServer STATIC
  code/camera/mjpeg_server.cpp
  code/camera/mjpeg_server.hpp
)
target_link_libraries(MJPEGServer
  PRIVATE ${CAMERA_LIBS} ${JPEG_LIBS} Threads::Threads
)

add_library(LEDController STATIC
  code/LED/LEDController.cpp
  code/LED/LEDController.hpp
)
target_link_libraries(LEDController
  PRIVATE ${GPIOD_LIBS} Threads::Threads
)

# --- Tests ---

enable_testing()

# MotorController test
add_executable(test_motor_controller
  tests/motor/Motor_test.cpp
)
target_link_libraries(test_motor_controller
  PRIVATE MotorController ${GPIOD_LIBS} Threads::Threads
)
add_test(NAME MotorControllerTest COMMAND test_motor_controller)
set_tests_properties(MotorControllerTest PROPERTIES TIMEOUT 5)

# UltrasonicSensor test
add_executable(test_ultrasonic
  tests/UltrasonicSensor/test_ultrasonic.cpp
)
target_link_libraries(test_ultrasonic
  PRIVATE UltrasonicSensor ${GPIOD_LIBS} Threads::Threads
)
add_test(NAME UltrasonicSensorTest COMMAND test_ultrasonic)
set_tests_properties(UltrasonicSensorTest PROPERTIES TIMEOUT 5)

# ECGProcessor test
add_executable(test_ecg
  tests/ecg_processor/ecg_main.cpp
)
target_link_libraries(test_ecg
  PRIVATE ECGProcessor Threads::Threads
)
add_test(NAME ECGProcessorTest COMMAND test_ecg)
set_tests_properties(ECGProcessorTest PROPERTIES TIMEOUT 5)

# TTSController test
add_executable(test_tts
  tests/syn6288_controller/test_tts.cpp
)
target_link_libraries(test_tts
  PRIVATE TTSController Threads::Threads
)
add_test(NAME TTSControllerTest COMMAND test_tts)
set_tests_properties(TTSControllerTest PROPERTIES TIMEOUT 5)

# LEDController test
add_executable(test_led
  tests/LED/test_led.cpp
)
target_link_libraries(test_led
  PRIVATE LEDController ${GPIOD_LIBS} Threads::Threads
)
add_test(NAME LEDControllerTest COMMAND test_led)
set_tests_properties(LEDControllerTest PROPERTIES TIMEOUT 5)
