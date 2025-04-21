cmake_minimum_required(VERSION 3.10)
project(ElectricWheelchair)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)


find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBC_GPIODCXX REQUIRED libgpiodcxx)   # C++ binding for libgpiod
pkg_check_modules(LIBC_GPIOD    REQUIRED libgpiod)      # C API for libgpiod
pkg_check_modules(LIBC_LIBCAMERA REQUIRED libcamera)    # for MJPEGServer
find_package(Boost REQUIRED COMPONENTS system thread)   # for Boost.Asio


include_directories(
  ${LIBC_GPIODCXX_INCLUDE_DIRS}
  ${LIBC_GPIOD_INCLUDE_DIRS}
  ${LIBC_LIBCAMERA_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)


link_directories(
  ${LIBC_GPIODCXX_LIBRARY_DIRS}
  ${LIBC_GPIOD_LIBRARY_DIRS}
  ${LIBC_LIBCAMERA_LIBRARY_DIRS}
)


add_library(GPIOButton
  code/Button/GPIOButton.cpp
)
target_include_directories(GPIOButton PUBLIC code/Button)
target_link_libraries(GPIOButton PUBLIC
  ${LIBC_GPIODCXX_LIBRARIES}
  ${LIBC_GPIOD_LIBRARIES}
  Threads::Threads
)

add_library(MotorController
  code/motor/MotorController.cpp
)
target_include_directories(MotorController PUBLIC code/motor)
target_link_libraries(MotorController PUBLIC
  ${LIBC_GPIODCXX_LIBRARIES}
  ${LIBC_GPIOD_LIBRARIES}
)

add_library(LightSensor
  code/LightSensor/LightSensor.cpp
)
target_include_directories(LightSensor PUBLIC code/LightSensor)
target_link_libraries(LightSensor PUBLIC Threads::Threads)

add_library(UltrasonicSensor
  code/UltrasonicSensor/UltrasonicSensor.cpp
)
target_include_directories(UltrasonicSensor PUBLIC code/UltrasonicSensor)
target_link_libraries(UltrasonicSensor PUBLIC Threads::Threads)

add_library(PIRController
  code/pir_sensor/pir_sensor.cpp
)
target_include_directories(PIRController PUBLIC code/pir_sensor)
target_link_libraries(PIRController PUBLIC Threads::Threads)

add_library(ECGProcessor
  code/ecg_processor/ecg_processor.cpp
)
target_include_directories(ECGProcessor PUBLIC code/ecg_processor)
target_link_libraries(ECGProcessor PUBLIC Threads::Threads)

add_library(TTSController
  code/syn6288_controller/syn6288_controller.cpp
)
target_include_directories(TTSController PUBLIC code/syn6288_controller)
target_link_libraries(TTSController PUBLIC Threads::Threads)

add_library(MJPEGServer
  code/camera/mjpeg_server.cpp
)
target_include_directories(MJPEGServer PUBLIC code/camera)
target_link_libraries(MJPEGServer PUBLIC
  ${LIBC_LIBCAMERA_LIBRARIES}
  Boost::system
  Threads::Threads
)

add_library(LEDController
  code/LED/LEDController.cpp
)
target_include_directories(LEDController PUBLIC code/LED)
target_link_libraries(LEDController PUBLIC Threads::Threads)


add_executable(final_system
  code/main/main.cpp
)
target_link_libraries(final_system PUBLIC
  GPIOButton MotorController LightSensor UltrasonicSensor PIRController
  ECGProcessor TTSController MJPEGServer LEDController
  ${Boost_LIBRARIES}
  ${LIBC_LIBCAMERA_LIBRARIES}
  Threads::Threads
)


enable_testing()


add_executable(test_gpio_button
  tests/Button/test_gpio_button.cpp
)
target_link_libraries(test_gpio_button PRIVATE
  GPIOButton
  ${LIBC_GPIODCXX_LIBRARIES}
  ${LIBC_GPIOD_LIBRARIES}
  Threads::Threads
)
add_test(NAME GPIOButtonTest COMMAND test_gpio_button)


add_executable(test_motor_controller
  tests/motor/Motor_test.cpp
)
target_link_libraries(test_motor_controller PRIVATE
  MotorController
  ${LIBC_GPIODCXX_LIBRARIES}
  ${LIBC_GPIOD_LIBRARIES}
  Threads::Threads
)
add_test(NAME MotorControllerTest COMMAND test_motor_controller)


add_executable(test_light_sensor
  tests/LightSensor/test_light_sensor.cpp
)
target_link_libraries(test_light_sensor PRIVATE
  LightSensor
  Threads::Threads
)
add_test(NAME LightSensorTest COMMAND test_light_sensor)


add_executable(test_ultrasonic
  tests/UltrasonicSensor/test_ultrasonic.cpp
)
target_link_libraries(test_ultrasonic PRIVATE
  UltrasonicSensor
  Threads::Threads
)
add_test(NAME UltrasonicSensorTest COMMAND test_ultrasonic)


add_executable(test_pir_controller
  tests/pir_sensor/test_pir.cpp
)
target_link_libraries(test_pir_controller PRIVATE
  PIRController
  Threads::Threads
)
add_test(NAME PIRControllerTest COMMAND test_pir_controller)


add_executable(test_ecg_processor
  tests/ecg_processor/ecg_main.cpp
)
target_link_libraries(test_ecg_processor PRIVATE
  ECGProcessor
  Threads::Threads
)
add_test(NAME ECGProcessorTest COMMAND test_ecg_processor)


add_executable(test_tts
  tests/syn6288_controller/test_tts.cpp
)
target_link_libraries(test_tts PRIVATE
  TTSController
  Threads::Threads
)
add_test(NAME TTSControllerTest COMMAND test_tts)


add_executable(test_mjpeg_server
  tests/camera/test_mjpeg_server.cpp
)
target_link_libraries(test_mjpeg_server PRIVATE
  MJPEGServer
  Threads::Threads
)
add_test(NAME MJPEGServerTest COMMAND test_mjpeg_server)


add_executable(test_led
  tests/LED/test_led.cpp
)
target_link_libraries(test_led PRIVATE
  LEDController
  Threads::Threads
)
add_test(NAME LEDControllerTest COMMAND test_led)
