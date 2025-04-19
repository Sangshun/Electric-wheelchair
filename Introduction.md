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
![FIG1](https://github.com/user-attachments/assets/6e951966-0cd0-40a9-8a7b-cd38768e5660)
<img src="https://github.com/user-attachments/assets/6e951966-0cd0-40a9-8a7b-cd38768e5660" width="50%" />

### - **2.Sensors**

#### - **Light sensor** (GPIO 16)
  ![FIG2](https://github.com/user-attachments/assets/fc7dd699-1beb-4e1b-839c-e6ea1c9aa654)

#### - **Ultrasonic module** (Trig GPIO 23, Echo GPIO 24)
  ![FIG3](https://github.com/user-attachments/assets/2179d177-d182-4b50-b5e9-68de2a1311d8)

#### - **PIR motion sensor** (GPIO 25)
  ![FIG4](https://github.com/user-attachments/assets/5e19a0c3-c4a3-44b7-add4-e7c0c97f32da)

#### - **ECG serial module** (/dev/ttyUSB0)
  ![FIG5](https://github.com/user-attachments/assets/f34e9bde-0a43-43cf-bcf1-d4ef16d2e9dd)

### 3.Actuators

Motor 1 via H‑bridge (GPIO 17/27) with control button on GPIO 5

Motor 2 via H‑bridge (GPIO 22/26) with control button on GPIO 6

LEDs (GPIO 9 and GPIO 11) for ambient and motion indications

SYN6288 TTS module (/dev/serial0)

Network: Ethernet or Wi‑Fi for web UI
