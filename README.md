# FreeRTOS ESP32 Multitasking System

Real-time multitasking embedded system using FreeRTOS on ESP32.

## Overview

This project demonstrates a multitasking embedded system built using FreeRTOS on the ESP32 microcontroller.

The system integrates:
- DHT11 temperature and humidity sensor
- OLED display
- LEDs
- Push buttons
- Buzzer

Multiple FreeRTOS tasks run concurrently for:
- Sensor data acquisition
- Display handling
- Button input handling
- Serial monitoring
- Output control

Inter-task communication is implemented using queues and semaphores.

---

## Features

- Real-time multitasking
- Queue-based task communication
- Semaphore synchronization
- Automatic and manual operating modes
- OLED live status display
- Temperature and humidity monitoring
- Multi-state alert system

---

## Technologies Used

- ESP32
- FreeRTOS
- Embedded C++
- Arduino IDE

---

## FreeRTOS Concepts Used

- Tasks
- Queues
- Binary Semaphores
- Scheduling
- Inter-task communication

---

## Hardware Components

- ESP32
- DHT11
- SSD1306 OLED Display
- LEDs
- Push Buttons
- Buzzer

---

## Author

- Siddharth Kshatriya
