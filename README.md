# Electric-wheelchair
This is Team 4. The project is an electric wheelchair. It not only can help the elderly move but also can help them stand up. The elderly have some mobility problems. As you can imagine, the elderly cannot easily stand up when they are sitting, so our team came up with this project. Additionally, it also has an ECG detection function. When the ECG connected to the elderly is detected abnormal, the buzzer will sound an alarm, which can draw the attention of people around. These are its main functions. Other additional functions can be added later. Furthermore, this project has a specific structure that helps to stand up. You can control the motor through the button, and then the motor drives the gear to rotate. The gear is fully coupled to the rack, so the gear pushes the rack up, which tilts the cushion to help the elderly stand up. All in all, it is an electric wheelchair that can help the elderly stand up and provide protection for their safety.

This final version program is a comprehensive real-time embedded system running on a Raspberry Pi, written in C++ using an object-oriented approach. It controls a motor via GPIO17 and GPIO27, with a button on GPIO5 that triggers different motor states: a short press sets the motor to FORWARD (accompanied by audible feedback from the SYN6288 TTS module on /dev/serial0), while a long press sets it to BACKWARD, launching a reverse camera server (ip:8080). In addition, a second motor is controlled via GPIO22 and GPIO26 using a button on GPIO6; a short press sets this motor to RISE (with audible feedback "RISE"), and a long press sets it to FALL (with audible feedback "FALL"). Additionally, the system monitors ambient light through a sensor on GPIO16, measures distance with an ultrasonic sensor using GPIO23 (trigger) and GPIO24 (echo), and detects motion using a PIR sensor on GPIO25 (via gpiochip0). It also processes ECG data from a module connected through /dev/ttyUSB0 and extracts converted ECG values for display. An integrated HTTP server built with Boost.Asio listens on ip:8000 and provides a dynamic web dashboard that updates sensor data in real time.

Our social media links： https://www.youtube.com/@kabalaqigou

Collaborators:
1.longqishi223: Cunhui Hu 2986514H
2.kabalaqiou: Jiacheng Yu 3028341Y
3.Sangshun: Ziyao Ji 2968717J
4.ccliuup: Chaoyu Liu 2978613L
5.Doltonz: Jiaming Zhang 2970743Z
