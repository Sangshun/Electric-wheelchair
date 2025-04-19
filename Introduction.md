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
│ │Motor │ ... │ LED │ ...  │Motor│ ...     │ SYN │   │
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
