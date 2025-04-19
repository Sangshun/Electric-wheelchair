# Electric Wheel‑Chair: Real‑Time Embedded Sensor & Actuator Dashboard

A complete real‑time embedded system on Raspberry Pi demonstrating multi‑sensor data acquisition, actuator control, voice feedback, LED signaling and two live web interfaces. This document serves as both a user guide and a mini lab report, walking through hardware setup, Raspberry Pi configuration, software build, and system validation.

---

## 📄 Abstract

This project integrates light, distance, motion and ECG sensors with dual‑motor control, TTS voice feedback, LED indicators and web dashboards into a single event‑driven C++ application running on a Raspberry Pi. It showcases real‑time responsiveness (<1 s), modular software architecture and an easy‑to‑reproduce hardware/software platform for prototyping assistive devices.

---

## 🔬 Introduction

- **Problem statement:** Traditional wheel‑chairs lack integrated sensing and feedback for low‑visibility or monitoring vital signs.
- **Solution:** A “smart” wheel‑chair controller that senses ambient light, obstacles, motion, and ECG, then drives motors, LEDs and voice prompts—and streams sensor data and camera video to any browser.

---

## 🏗️ System Architecture

```text
┌─────────────────────────────────────────────────────┐
│                   Raspberry Pi                    │
│ ┌─────────┐  ┌──────────┐  ┌──────────┐  ┌───────┐ │
│ │ Buttons │  │  LEDs    │  │  Motors  │  │ TTS   │ │
│ └──┬──────┘  └──┬───────┘  └──┬───────┘  └───┬───┘ │
│    │            │             │               │   │
│ ┌──▼──┐      ┌──▼──┐       ┌──▼──┐         ┌──▼──┐│
│ │GPIO │      │GPIO │       │GPIO │         │ /dev ││
│ │ 5,6 │      │ 9,11│       │17,22│         │serial││
│ └──┬──┘      └──┬──┘       └──┬──┘         └──┬──┘│
│    │            │             │               │   │
│ ┌──▼──┐      ┌──▼──┐       ┌──▼──┐         ┌──▼──┐│
│ │Motor │ ...  │ LED  │ ...  │Motor │ ...     │ SYN ││
│ │Ctrl  │      │Ctrl │       │Ctrl │         │6288 ││
│ └──────┘      └──────┘      └──────┘         └────┘│
│                                                     │
│ ┌───────────┐  ┌──────────────┐  ┌───────────┐      │
│ │ Light     │  │ Ultrasonic   │  │    PIR    │      │
│ │Sensor (16)│  │Sensor (23/24)│  │Sensor (25)│      │
│ └───────────┘  └──────────────┘  └───────────┘      │
│                                                     │
│ ┌─────────────────────────────────────────────────┐ │
│ │     HTTP (8000)     │   MJPEG (8080)           │ │
│ │  live dashboard     │  reverse camera stream   │ │
│ └─────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────┘
🔧 Hardware & Wiring
Raspberry Pi (models 3/4 or Zero W) with 40‑pin GPIO header

Sensors & Actuators

Light sensor → GPIO 16

Ultrasonic module (Trig→23, Echo→24)

PIR module → GPIO 25 (via gpiochip0)

ECG module → /dev/ttyUSB0

Buttons → GPIO 5 (motor 1), GPIO 6 (motor 2)

Motors → H‑bridge inputs on GPIO 17/27 and 22/26

LEDs → GPIO 9, GPIO 11

TTS (SYN6288) → UART (/dev/serial0, 9600 bps)

Camera → libcamera on Pi CSI interface (for MJPEG)

See electric wheel‑chair/WIRING.md for full diagrams.
![Wiring Diagram](electric wheel‑chair/images/wiring.png)

🖥️ Raspberry Pi Configuration
Update OS & packages

bash
复制
编辑
sudo apt update && sudo apt upgrade -y
Enable interfaces

bash
复制
编辑
sudo raspi-config
#  └> Interface Options
#      ├─ Serial Port: enable (disable login shell)
#      ├─ I2C: enable (if using I2C sensors)
#      ├─ SPI: optional (disable if repurposing GPIO9/11)
#      └─ Camera: enable
User permissions

bash
复制
编辑
sudo usermod -aG gpio,dialout $USER
Reboot

bash
复制
编辑
sudo reboot
🛠️ Software Prerequisites
OS: Raspberry Pi OS (buster, bullseye or later)

Toolchain:

GCC/G++ ≥ 9 (C++17)

Make (optional)

Libraries:

libgpiod (libgpiod-dev)

Boost.Asio (libboost-system-dev)

libcamera & pkg-config (libcamera-dev) (optional)

libjpeg (libjpeg-dev)

📦 Build & Installation
bash
复制
编辑
# 1. Clone repo
git clone https://github.com/Sangshun/Electric-wheelchair.git
cd Electric-wheelchair/electric-wheelchair/code/main

# 2. Compile
g++ -std=c++17 \
  -I../ecg_processor -I../motor -I../Button -I../LightSensor \
  -I../UltrasonicSensor -I../pir_sensor -I../syn6288_controller \
  -I../camera -I../LED \
  $(pkg-config --cflags libcamera) \
  main.cpp \
  ../ecg_processor/ecg_processor.cpp \
  ../motor/MotorController.cpp \
  ../Button/GPIOButton.cpp \
  ../LightSensor/LightSensor.cpp \
  ../UltrasonicSensor/UltrasonicSensor.cpp \
  ../pir_sensor/pir_sensor.cpp \
  ../syn6288_controller/syn6288_controller.cpp \
  ../camera/mjpeg_server.cpp \
  ../LED/LEDController.cpp \
  -o final_system \
  -lpthread -lgpiodcxx -lgpiod -lboost_system \
  $(pkg-config --libs libcamera) -ljpeg
▶️ Usage
Ensure interfaces are enabled (see “Raspberry Pi Configuration”)

Start application

bash
./final_system
Open dashboards in browser

Sensor data: http://<pi-ip>:8000

Reverse camera: http://<pi-ip>:8080

📈 Results & Observations
Ambient light: LED stays ON in dark, OFF in bright; blinks when motors run

Distance readings: ~10 cm resolution, updated every 1 s

Motion detection: near‑instantaneous callback on PIR trigger

ECG values: parsed from serial, displayed every 3 s

Latency: end‑to‑end sensor→web < 500 ms on local network

Include plots/screenshots here
![Sensor Dashboard](docs/images/dashboard.png)

💡 Future Work
Add PID control for motor speed

Integrate mobile app front‑end

Enhance security (HTTPS, auth)

Add data logging and analytics

