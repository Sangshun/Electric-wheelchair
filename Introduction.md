# Electric Wheel‑Chair: Real‑Time Embedded Sensor & Actuator Dashboard

This is Team 4. The project is an electric wheelchair. It not only can help the elderly move but also can help them stand up. The elderly have some mobility problems. As you can imagine, the elderly cannot easily stand up when they are sitting, so our team came up with this project. Additionally, it also has an ECG detection function. When the ECG connected to the elderly is detected abnormal, the buzzer will sound an alarm, which can draw the attention of people around. These are its main functions. Other additional functions can be added later. Furthermore, this project has a specific structure that helps to stand up. You can control the motor through the button, and then the motor drives the gear to rotate. The gear is fully coupled to the rack, so the gear pushes the rack up, which tilts the cushion to help the elderly stand up. All in all, it is an electric wheelchair that can help the elderly stand up and provide protection for their safety.

---

## 📄 Abstract

This project delivers a "smart" wheelchair controller: ambient light, distance, motion and ECG sensors feed live data into a dual‑motor drive system with TTS prompts and LED indicators. Two web services (sensor dashboard on port 8000; reverse camera on port 8080) present real‑time feedback. Modular, callback‑based architecture ensures sub‑second responsiveness.

---

## 🔬 Introduction

- **Problem statement:** Wheelchairs rarely provide integrated environmental sensing or vital‑sign monitoring, limiting safety in low light or obstacle‑dense scenarios.
- **Solution:** A unified embedded system on Raspberry Pi that measures light, distance, motion and ECG; drives two motors (forward/backward, rise/fall); issues audio prompts; signals state via LEDs; and streams sensor data and camera video to any networked browser.

This final version program is a comprehensive real-time embedded system running on a Raspberry Pi, written in C++ using an object-oriented approach. It controls a motor via GPIO17 and GPIO27, with a button on GPIO5 that triggers different motor states: a short press sets the motor to FORWARD (accompanied by audible feedback from the SYN6288 TTS module on /dev/serial0), while a long press sets it to BACKWARD, launching a reverse camera server (ip:8080). In addition, a second motor is controlled via GPIO22 and GPIO26 using a button on GPIO6; a short press sets this motor to RISE (with audible feedback "RISE"), and a long press sets it to FALL (with audible feedback "FALL"). Additionally, the system monitors ambient light through a sensor on GPIO16, measures distance with an ultrasonic sensor using GPIO23 (trigger) and GPIO24 (echo), and detects motion using a PIR sensor on GPIO25 (via gpiochip0). It also processes ECG data from a module connected through /dev/ttyUSB0 and extracts converted ECG values for display. An integrated HTTP server built with Boost.Asio listens on ip:8000 and provides a dynamic web dashboard that updates sensor data in real time.

Our social media links： https://www.youtube.com/@kabalaqigou

Collaborators: 1.longqishi223: Cunhui Hu 2986514H 2.kabalaqiou: Jiacheng Yu 3028341Y 3.Sangshun: Ziyao Ji 2968717J 4.ccliuup: Chaoyu Liu 2978613L 5.Doltonz: Jiaming Zhang 2970743Z

---

## 🏗️ System Architecture


                                          ┌─────────────────────────────────────────────────────┐
                                          │                   Raspberry Pi                      │
                                          │ ┌─────────┐  ┌──────────┐  ┌──────────┐  ┌───────┐  │
                                          │ │ Buttons │  │  LEDs    │  │  Motors  │  │ TTS   │  │
                                          │ └──┬──────┘  └──┬───────┘  └──┬───────┘  └───┬───┘  │
                                          │    │            │             │              │      │
                                          │ ┌──▼──┐      ┌──▼──┐       ┌──▼──┐        ┌──▼───┐  │
                                          │ │GPIO │      │GPIO │       │GPIO │        │ /dev │  │
                                          │ │5,6  │      │9,11 │       │17,22│        │serial│  │
                                          │ └──┬──┘      └──┬──┘       └──┬──┘        └───┬──┘  │
                                          │    │            │             │               │     │
                                          │ ┌──▼───┐     ┌──▼──┐       ┌──▼──┐         ┌──▼──┐  │
                                          │ │Motor │ ... │LED  │  ...  │Motor│   ...   │SYN  │  │
                                          │ │Ctrl  │     │Ctrl │       │Ctrl │         │6288 │  │
                                          │ └──────┘     └─────┘       └─────┘         └─────┘  │
                                          │                                                     │
                                          │   ┌───────────┐  ┌──────────────┐  ┌───────────┐    │
                                          │   │ Light     │  │ Ultrasonic   │  │    PIR    │    │
                                          │   │Sensor (16)│  │Sensor (23/24)│  │Sensor (25)│    │
                                          │   └───────────┘  └──────────────┘  └───────────┘    │
                                          │                                                     │
                                          │ ┌─────────────────────────────────────────────────┐ │
                                          │ │     HTTP (8000)     │   MJPEG (8080)            │ │
                                          │ │  live dashboard     │  reverse camera stream    │ │
                                          │ └─────────────────────────────────────────────────┘ │
                                          └─────────────────────────────────────────────────────┘

---

## 🛠️ Hardware Setup

### 1. Raspberry Pi (Model 5 with 40‑pin GPIO header)
  <p align="center">
  <img src="https://github.com/user-attachments/assets/6e951966-0cd0-40a9-8a7b-cd38768e5660" width="70%" alt="Raspberry Pi" />
</p>

### 2.Sensors

- **Light sensor** (GPIO 16): can detect ambient light levels in real time, enabling the system to switch LEDs on in darkness and off in bright conditions.                                    
  
#### - **2.2 Ultrasonic module** (Trig GPIO 23, Echo GPIO 24): measures distance by emitting ultrasonic pulses and timing their echo, used for obstacle detection and proximity alerts.  

#### - **2.3 PIR motion sensor** (GPIO 25): senses infrared radiation changes from moving objects, allowing the system to detect human presence or motion events.  

#### - **2.4 ECG serial module** (/dev/ttyUSB0): streams raw electrocardiogram data over a serial port, which is processed to extract heart‑rate values for monitoring.
<p align="center">
  <img src="https://github.com/user-attachments/assets/1d27bde6-b7b0-4c01-84a7-9ed85d17a059" width="50%" alt="Raspberry Pi" />
</p>

### 3. Streams  
- **Reverse‑view camera (MJPEG server on port 8080): provides a live video feed from the mounted camera module, useful for backing up and navigation assistance.**
  <p align="center">
  <img src="https://github.com/user-attachments/assets/0458833e-d222-4392-9483-41c5ab458deb" width="20%" alt="Raspberry Pi" />
</p>

### 4.Actuators

#### - **4.1 Motor 1 via H‑bridge (GPIO 17/27) with control button on GPIO 5：drives forward/backward motion with tactile button input and TTS feedback.  **

#### - **4.2 Motor 2 via H‑bridge (GPIO 22/26) with control button on GPIO 6: handles rise/fall motion similarly, with distinct voice prompts.  **

#### - **4.3 LEDs (GPIO 9 and GPIO 11) for ambient and motion indications: indicate ambient light state by steady glow or blink during any motor activity.  **

#### - **4.4 SYN6288 TTS module (/dev/serial0): delivers audible voice prompts for motor commands and status updates.**
<p align="center">
  <img src="https://github.com/user-attachments/assets/3c078222-af2d-4d17-ad20-dedf3ce6155b" width="50%" alt="Raspberry Pi" />
</p>

---

## ⚙️ Raspberry Pi Configuration
