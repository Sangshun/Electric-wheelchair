# Electric Wheel‑Chair: Real‑Time Embedded Sensor & Actuator Dashboard

A complete, event‑driven C++ application on Raspberry Pi integrating multi‑sensor data acquisition, actuator control, voice feedback, LED signaling and dual live web interfaces. This document combines a user guide with lab‑report–style detail, covering hardware setup, Pi configuration, software build, and system validation.

---

## 📄 Abstract

This project delivers a "smart" wheelchair controller: ambient light, distance, motion and ECG sensors feed live data into a dual‑motor drive system with TTS prompts and LED indicators. Two web services (sensor dashboard on port 8000; reverse camera on port 8080) present real‑time feedback. Modular, callback‑based architecture ensures sub‑second responsiveness.

---

## 🔬 Introduction

- **Problem statement:** Wheelchairs rarely provide integrated environmental sensing or vital‑sign monitoring, limiting safety in low light or obstacle‑dense scenarios.
- **Solution:** A unified embedded system on Raspberry Pi that measures light, distance, motion and ECG; drives two motors (forward/backward, rise/fall); issues audio prompts; signals state via LEDs; and streams sensor data and camera video to any networked browser.

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

#### - **2.1 Light sensor** (GPIO 16)                                  
  
#### - **2.2 Ultrasonic module** (Trig GPIO 23, Echo GPIO 24)

#### - **2.3 PIR motion sensor** (GPIO 25)

#### - **2.4 ECG serial module** (/dev/ttyUSB0)
<p align="center">
  <img src="https://github.com/user-attachments/assets/1d27bde6-b7b0-4c01-84a7-9ed85d17a059" width="50%" alt="Raspberry Pi" />
</p>

### 3. Streams  
- **Reverse‑view camera** (MJPEG server on port 8080)
  <p align="center">
  <img src="https://github.com/user-attachments/assets/0458833e-d222-4392-9483-41c5ab458deb" width="20%" alt="Raspberry Pi" />
</p>

### 4.Actuators

#### - **4.1 Motor 1 via H‑bridge (GPIO 17/27) with control button on GPIO 5**

#### - **4.2 Motor 2 via H‑bridge (GPIO 22/26) with control button on GPIO 6**

#### - **4.3 LEDs (GPIO 9 and GPIO 11) for ambient and motion indications**

#### - **4.4 SYN6288 TTS module (/dev/serial0)**
<p align="center">
  <img src="https://github.com/user-attachments/assets/3c078222-af2d-4d17-ad20-dedf3ce6155b" width="50%" alt="Raspberry Pi" />
</p>

---

## ⚙️ Raspberry Pi Configuration
