cmake_minimum_required(VERSION 3.10)
project(ElectricWheelchair)

set(CMAKE_CXX_STANDARD 17)


add_executable(final_system
    code/main/main.cpp              
    code/ecg_processor/ecg_processor.cpp
    code/motor/MotorController.cpp
    code/Button/GPIOButton.cpp
    code/LightSensor/LightSensor.cpp
    code/UltrasonicSensor/UltrasonicSensor.cpp
    code/pir_sensor/pir_sensor.cpp
    code/syn6288_controller/syn6288_controller.cpp
    code/camera/mjpeg_server.cpp
    code/LED/LEDController.cpp
)
target_link_libraries(final_system
    pthread gpiodcxx gpiod boost_system jpeg
    ${LIBCAMERA_LIBRARIES}           
)
target_include_directories(final_system PRIVATE
    code/main
    code/ecg_processor
    code/motor
    code/Button
    code/LightSensor
    code/UltrasonicSensor
    code/pir_sensor
    code/syn6288_controller
    code/camera
    code/LED
)


enable_testing()

# GPIOButton
add_executable(test_gpio_button
    tests/Button/test_gpio_button.cpp
)
target_link_libraries(test_gpio_button PRIVATE GPIOButton pthread)
add_test(NAME GPIOButtonTest COMMAND test_gpio_button)

# MotorController
add_executable(test_motor_controller
    tests/motor/test_motor_controller.cpp
)
target_link_libraries(test_motor_controller PRIVATE MotorController pthread)
add_test(NAME MotorControllerTest COMMAND test_motor_controller)

# LightSensor
add_executable(test_light_sensor
    tests/LightSensor/test_light_sensor.cpp   
)
target_link_libraries(test_light_sensor PRIVATE LightSensor pthread)
add_test(NAME LightSensorTest COMMAND test_light_sensor)

# UltrasonicSensor
add_executable(test_ultrasonic_sensor
    tests/UltrasonicSensor/test_ultrasonic_sensor.cpp
)
target_link_libraries(test_ultrasonic_sensor PRIVATE UltrasonicSensor pthread)
add_test(NAME UltrasonicSensorTest COMMAND test_ultrasonic_sensor)

# PIRController
add_executable(test_pir_sensor
    tests/pir_sensor/test_pir_sensor.cpp      
)
target_link_libraries(test_pir_sensor PRIVATE PIRController pthread)
add_test(NAME PIRControllerTest COMMAND test_pir_sensor)

# ECGProcessor
add_executable(test_ecg_processor
    tests/ecg_processor/test_ecg_processor.cpp
)
target_link_libraries(test_ecg_processor PRIVATE ECGProcessor pthread)
add_test(NAME ECGProcessorTest COMMAND test_ecg_processor)

# TTSController
add_executable(test_tts_controller
    tests/syn6288_controller/test_syn6288_controller.cpp
)
target_link_libraries(test_tts_controller PRIVATE TTSController pthread)
add_test(NAME TTSControllerTest COMMAND test_tts_controller)

# MJPEGServer
add_executable(test_mjpeg_server
    tests/camera/test_mjpeg_server.cpp
)
target_link_libraries(test_mjpeg_server PRIVATE MJPEGServer pthread)
add_test(NAME MJPEGServerTest COMMAND test_mjpeg_server)

# LEDController
add_executable(test_led_controller
    tests/LED/test_led_controller.cpp
)
target_link_libraries(test_led_controller PRIVATE LEDController pthread)
add_test(NAME LEDControllerTest COMMAND test_led_controller)


