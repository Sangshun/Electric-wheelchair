# Electric Wheelchair: Real‑Time Embedded Sensor & Actuator Dashboard

This is **Team 4**’s smart electric wheelchair, designed to give older adults both mobility and standing assistance. In addition to driving forward and backward, a motor‑driven gear‑and‑rack mechanism tilts the seat cushion to help the user rise safely. An integrated ECG module continuously monitors heart signals to extract and display real‑time heart rate values. Ambient lighting, ultrasonic obstacle sensing and motion detection further enhance safety, and voice prompts guide each operation. Modular by design, this platform can be extended with new sensors or features as needed.

---

## 📄 Abstract

This project delivers a "smart" wheelchair controller: ambient light, distance, motion and ECG sensors feed live data into a dual‑motor drive system with TTS prompts and LED indicators. Two web services (sensor dashboard on port 8000; reverse camera on port 8080) present real‑time feedback. Modular, callback‑based architecture ensures sub‑second responsiveness.

---

## 🔬 Introduction

- **Problem statement:** Wheelchairs rarely provide integrated environmental sensing or vital‑sign monitoring, limiting safety in low light or obstacle‑dense scenarios.
- **Solution:** A unified embedded system on Raspberry Pi that measures light, distance, motion and ECG; drives two motors (forward/backward, rise/fall); issues audio prompts; signals state via LEDs; and streams sensor data and camera video to any networked browser.

This final version program is a comprehensive real-time embedded system running on a Raspberry Pi, written in C++ using an object-oriented approach. It controls the first pair of motors via GPIO17 and GPIO27, with a button on GPIO5 that triggers different motor states: a short press sets the motor to FORWARD (accompanied by audible feedback from the SYN6288 TTS module on /dev/serial0), while a long press sets it to BACKWARD, launching a reverse camera server (ip:8080). In addition, the second pair of motors is controlled via GPIO22 and GPIO26 using a button on GPIO6; a short press sets this motor to RISE (with audible feedback "RISE"), and a long press sets it to FALL (with audible feedback "FALL"). Additionally, the system monitors ambient light through a sensor on GPIO16, measures distance with an ultrasonic sensor using GPIO23 (trigger) and GPIO24 (echo), and detects motion using a PIR sensor on GPIO25 (via gpiochip0). It also processes ECG data from a module connected through /dev/ttyUSB0 and extracts converted ECG values for display. An integrated HTTP server built with Boost.Asio listens on ip:8000 and provides a dynamic web dashboard that updates sensor data in real time.

Our social media links： **https://www.youtube.com/@kabalaqigou**

The division of the code is as follows:
1. **Sangshun: Ziyao Ji (2968717J)** completed the **camera_preview** and **light code**.
2. **ccliuup: Chaoyu Liu (2978613L)** completed the **ecg_processor** and **button code**.
3. **kabalaqiou: Jiacheng Yu (3028341Y)** completed the **pir_sensor code** and **structure design**.
4. **longqishi223: Cunhui Hu (2986514H)** completed the **syn6288_test** , **motor** and **main code**.
5. **Doltonz: Jiaming Zhang (2970743Z)** completed the **ultrasonic** and **main code**.
   
---

## 🏗️ System Architecture

<div align="center">
<pre style="text-align: left;">
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
</pre>
</div>

---

## 🛠️ Hardware Setup

### 1. Raspberry Pi (Model 5 with 40‑pin GPIO header)
  <p align="center">
  <img src="https://github.com/user-attachments/assets/6e951966-0cd0-40a9-8a7b-cd38768e5660" width="70%" alt="Raspberry Pi" />
</p>

### 2. Sensors

- **Light sensor (GPIO 16):** can detect ambient light levels in real time, enabling the system to switch LEDs on in darkness and off in bright conditions.                                    
  
- **Ultrasonic module (Trig GPIO 23, Echo GPIO 24):**  measures distance by emitting ultrasonic pulses and timing their echo, used for obstacle detection and proximity alerts.  

- **PIR motion sensor (GPIO 25):**  senses infrared radiation changes from moving objects, allowing the system to detect human presence or motion events.  

- **ECG serial module (/dev/ttyUSB0):**  streams raw electrocardiogram data over a serial port, which is processed to extract heart‑rate values for monitoring.
<p align="center">
  <img src="https://github.com/user-attachments/assets/1d27bde6-b7b0-4c01-84a7-9ed85d17a059" width="50%" alt="Raspberry Pi" />
</p>

### 3. Streams  
- **Reverse‑view camera (MJPEG server on port 8080):**  provides a live video feed from the mounted camera module, useful for backing up and navigation assistance.
  <p align="center">
  <img src="https://github.com/user-attachments/assets/0458833e-d222-4392-9483-41c5ab458deb" width="20%" alt="Raspberry Pi" />
</p>

### 4. Actuators

- **Motor 1 via H‑bridge (GPIO 17/27) with control button on GPIO 5:** drives forward/backward motion with tactile button input and TTS feedback.  

- **Motor 2 via H‑bridge (GPIO 22/26) with control button on GPIO 6:**  handles rise/fall motion similarly, with distinct voice prompts.  

- **LEDs (GPIO 9 and GPIO 11) for ambient and motion indications:**  indicate ambient light state by steady glow or blink during any motor activity.  

- **SYN6288 TTS module (/dev/serial0):**  delivers audible voice prompts for motor commands and status updates.
<p align="center">
  <img src="https://github.com/user-attachments/assets/3c078222-af2d-4d17-ad20-dedf3ce6155b" width="50%" alt="Raspberry Pi" />
</p>

### 5. Wheelchair Overview

The wheelchair’s main support frame was **3D‑printed** to maximize structural strength, while the rear wheels, seating surface, and backrest were handcrafted from **stiff cardboard**. To join these diverse materials, we used a combination of **hot‑melt glue**, **electrical tape** and other adhesives, each chosen to suit the specific contact surfaces. The completed assembly is shown below:
  <p align="center">
  <img src="https://github.com/user-attachments/assets/cee450aa-5d63-4733-bd34-65e415bbb03d" width="20%" alt="Raspberry Pi" />
</p>


---

## ⚙️ Raspberry Pi Configuration
**Before building, run:**
```bash
sudo raspi-config
```

**Interface Options → Serial Port:** Enable serial hardware; disable login shell

**Interface Options → SPI:** (Optional) Disable if SPI peripherals conflict with GPIO 9/11

**Localization Options:** Set locale, timezone, keyboard

**System Options → Boot / Auto Login:** Text console with auto login

**Reboot after changes to apply.**

---

## 💻 Software Build & Installation

**Prerequisites:**
```bash
sudo apt update
sudo apt install -y build-essential gpiod libgpiod-dev libboost-system-dev libjpeg-dev libcamera-dev pkg-config git
```
**Clone our codes:**
```bash
git clone https://github.com/Sangshun/Electric-wheelchair.git
```
**Compile:**
```bash
cd Electric-wheelchair/electric-wheelchair/code/main
g++ -std=c++17 \
    -I../ecg_processor -I../motor -I../Button -I../LightSensor \
    -I../UltrasonicSensor -I../pir_sensor -I../syn6288_controller \
    -I../camera -I../LED \
    $(pkg-config --cflags libcamera) main.cpp \
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
```
**Remote Access:**
- **GUI (VNC):** Download and install RealVNC Viewer from
  https://www.realvnc.com/en/connect/download/viewer/
- **SSH (terminal):** Download PuTTY from
  https://www.putty.org/

---

## 📦 CMake Build & Test
Enter electric-wheelchair directory
```bash
cd Electric-wheelchair/electric-wheelchair
```
Create and enter a clean build directory
```bash
mkdir -p build && cd build
```
Configure the project, locating dependencies (libgpiod, libcamera, libjpeg…)
```bash
cmake ..
```
Compile all modules in parallel
```bash
make -j4
```
Run the full test suite, showing failures immediately
```bash
ctest --output-on-failure
```
---

## 🚀 System Validation

**Execution:**
```bash
sudo ./final_system
```
**Sensor dashboard:** Verify web UI at http://<pi-ip>:8000 shows live data.

**Reverse camera:** Access http://<pi-ip>:8080 to see MJPEG stream.

**LED behavior:** In dark ambient, LEDs steady on; in bright ambient, LEDs off until either motor runs—then blink.

**Motor controls:** Buttons on GPIO 5/6 trigger forward/backward and rise/fall motions; verify TTS announcements.

**ECG processing:** Confirm converted ECG values appear on console and web UI.
