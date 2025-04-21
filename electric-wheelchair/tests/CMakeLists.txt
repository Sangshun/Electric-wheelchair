cmake_minimum_required(VERSION 3.10)
project(ElectricWheelchair)

set(CMAKE_CXX_STANDARD 17)

# -- add your library modules --
add_library(GPIOButton
    code/Button/GPIOButton.cpp
    code/Button/GPIOButton.hpp
)
target_include_directories(GPIOButton PUBLIC code/Button)

add_library(MotorController
    code/motor/MotorController.cpp
    code/motor/MotorController.hpp
)
target_include_directories(MotorController PUBLIC code/motor)

add_library(LightSensor
    code/LightSensor/LightSensor.cpp
    code/LightSensor/LightSensor.hpp
)
target_include_directories(LightSensor PUBLIC code/LightSensor)

add_library(UltrasonicSensor
    code/UltrasonicSensor/UltrasonicSensor.cpp
    code/UltrasonicSensor/UltrasonicSensor.hpp
)
target_include_directories(UltrasonicSensor PUBLIC code/UltrasonicSensor)

add_library(PIRController
    code/pir_sensor/pir_sensor.cpp
    code/pir_sensor/pir_sensor.hpp
)
target_include_directories(PIRController PUBLIC code/pir_sensor)

add_library(ECGProcessor
    code/ecg_processor/ecg_processor.cpp
    code/ecg_processor/ecg_processor.hpp
)
target_include_directories(ECGProcessor PUBLIC code/ecg_processor)

add_library(TTSController
    code/syn6288_controller/syn6288_controller.cpp
    code/syn6288_controller/syn6288_controller.hpp
)
target_include_directories(TTSController PUBLIC code/syn6288_controller)

add_library(MJPEGServer
    code/camera/mjpeg_server.cpp
    code/camera/mjpeg_server.hpp
)
target_include_directories(MJPEGServer PUBLIC code/camera)

add_library(LEDController
    code/LED/LEDController.cpp
    code/LED/LEDController.hpp
)
target_include_directories(LEDController PUBLIC code/LED)

add_library(HTTPServer
    code/http_server/http_server.cpp
    code/http_server/http_server.hpp
)
target_include_directories(HTTPServer PUBLIC code/http_server)

# -- add tests --
enable_testing()

# GPIOButton test
add_executable(test_gpio_button
    tests/Button/test_gpio_button.cpp
)
target_link_libraries(test_gpio_button PRIVATE GPIOButton pthread)
add_test(NAME GPIOButtonTest COMMAND test_gpio_button)

# MotorController test
add_executable(test_motor_controller
    tests/motor/test_motor_controller.cpp
)
target_link_libraries(test_motor_controller PRIVATE MotorController pthread)
add_test(NAME MotorControllerTest COMMAND test_motor_controller)

# LightSensor test
add_executable(test_light_sensor
    tests/LightSensor/test_light_sensor.cpp
)
target_link_libraries(test_light_sensor PRIVATE LightSensor pthread)
add_test(NAME LightSensorTest COMMAND test_light_sensor)

# UltrasonicSensor test
add_executable(test_ultrasonic_sensor
    tests/UltrasonicSensor/test_ultrasonic_sensor.cpp
)
target_link_libraries(test_ultrasonic_sensor PRIVATE UltrasonicSensor pthread)
add_test(NAME UltrasonicSensorTest COMMAND test_ultrasonic_sensor)

# PIRController test
add_executable(test_pir_controller
    tests/pir_sensor/test_pir_sensor.cpp
)
target_link_libraries(test_pir_controller PRIVATE PIRController pthread)
add_test(NAME PIRControllerTest COMMAND test_pir_controller)

# ECGProcessor test
add_executable(test_ecg_processor
    tests/ecg_processor/test_ecg_processor.cpp
)
target_link_libraries(test_ecg_processor PRIVATE ECGProcessor pthread)
add_test(NAME ECGProcessorTest COMMAND test_ecg_processor)

# TTSController test
add_executable(test_tts_controller
    tests/syn6288_controller/test_syn6288_controller.cpp
)
target_link_libraries(test_tts_controller PRIVATE TTSController pthread)
add_test(NAME TTSControllerTest COMMAND test_tts_controller)

# MJPEGServer test
add_executable(test_mjpeg_server
    tests/camera/test_mjpeg_server.cpp
)
target_link_libraries(test_mjpeg_server PRIVATE MJPEGServer pthread)
add_test(NAME MJPEGServerTest COMMAND test_mjpeg_server)

# LEDController test
add_executable(test_led_controller
    tests/LED/test_led_controller.cpp
)
target_link_libraries(test_led_controller PRIVATE LEDController pthread)
add_test(NAME LEDControllerTest COMMAND test_led_controller)

# HTTPServer test
add_executable(test_http_server
    tests/http_server/test_http_server.cpp
)
target_link_libraries(test_http_server PRIVATE HTTPServer pthread)
add_test(NAME HTTPServerTest COMMAND test_http_server)
