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

```text
┌─────────────────────────────────────────────────────┐
│                   Raspberry Pi                      │
│ ┌─────────┐  ┌──────────┐  ┌──────────┐  ┌───────┐  │
│ │ Buttons │  │  LEDs    │  │  Motors  │  │ TTS   │  │
│ └──┬──────┘  └──┬───────┘  └──┬───────┘  └───┬───┘  │
│    │            │             │              │      │
│ ┌──▼──┐      ┌──▼──┐       ┌──▼──┐        ┌──▼───┐  │
│ │GPIO │      │GPIO │       │GPIO │        │ /dev │  │
│ │ 5,6 │      │ 9,11│       │17,22│        │serial│  │
│ └──┬──┘      └──┬──┘       └─┬───┘        └──┬───┘  │
│    │            │            │               │      │
│ ┌──▼───┐     ┌──▼──┐      ┌──▼──┐         ┌──▼──┐   │
│ │Motor │ ... │ LED │ ...  │Motor│   ...   │ SYN │   │
│ │Ctrl  │     │Ctrl │      │Ctrl │         │6288 │   │
│ └──────┘     └─────┘      └─────┘         └─────┘   │
│                                                     │
│ ┌───────────┐  ┌──────────────┐  ┌───────────┐      │
│ │ Light     │  │ Ultrasonic   │  │    PIR    │      │
│ │Sensor (16)│  │Sensor (23/24)│  │Sensor (25)│      │
│ └───────────┘  └──────────────┘  └───────────┘      │
│                                                     │
│ ┌─────────────────────────────────────────────────┐ │
│ │     HTTP (8000)     │   MJPEG (8080)            │ │
│ │  live dashboard     │  reverse camera stream    │ │
│ └─────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────┘
