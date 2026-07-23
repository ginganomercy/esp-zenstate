<div align="center">
  <h1>ESP-ZenState 🧊</h1>
  <p><strong>Industrial-Grade Reactive State Machine & Thermal Mitigator for ESP32</strong></p>
  
  ![Platform](https://img.shields.io/badge/PlatformIO-Compatible-orange)
  ![Framework](https://img.shields.io/badge/ESP--IDF%20%7C%20Arduino-Supported-blue)
  ![License](https://img.shields.io/badge/License-MIT-green)
</div>

---

## The Problem: CPU Blocking & Overheating
When building complex IoT devices (e.g., controlling 43A BTS7960 motors, polling JSN-SR04T ultrasonic sensors, and keeping MQTT alive), the standard Arduino `loop()` architecture forces the CPU to run at 100% capacity using blocking functions like `pulseIn()` or `delay()`. This causes:
1. **Thermal Throttling**: The ESP32 overheats (60-70°C).
2. **WiFi/MQTT Drops**: Background OS tasks starve, causing constant reconnect loops.
3. **Spaghetti Code**: Unmaintainable chained `switch-case` logic.

## The Solution: ESP-ZenState
**ESP-ZenState** is a monolithic C++ framework built strictly for ESP32. It solves hardware overheating by fundamentally changing how the processor schedules tasks. 

It fuses a **Zero-Heap State Machine** with **FreeRTOS Hardware Interrupts**, ensuring your CPU stays ice cold even under massive multi-sensor payloads.

---

## Core Features (Production-Ready)

*   **100% Zero-Heap Architecture:** ESP-ZenState completely avoids `std::function` and dynamic `malloc` allocations. It uses raw C-pointers for state callbacks, preventing Heap Fragmentation crashes on long-running devices (5+ years uptime).
*   **Asynchronous Interrupt Drivers:** Built-in drivers (like `ZenUltrasonic`) rely purely on hardware ISR (Interrupt Service Routines). It records the time-of-flight in integer microseconds without ever blocking the CPU. (Floating-point math is done safely in thread-space to protect the RTOS FPU registers).
*   **3-Phase State Hooks:** Complete control with `on_enter`, `on_run`, and `on_exit` hooks. No more calling `digitalWrite()` repeatedly in a loop! Turn on the relay exactly *once* on entry, and turn it off exactly *once* on exit.
*   **Core-Affinity Routing:** Safely route heavy hardware tasks to Core 1, keeping Core 0 completely isolated for WiFi/MQTT stability.
*   **OS-Level CPU Yielding:** Replaces the deadly `delay()` with `ZenOS::yield_ms()`, which explicitly halts the CPU clock via FreeRTOS IDLE task, physically dropping chip temperatures.

---

## Quick Start 

### 1. Define C-Style Hooks
```cpp
#include "esp_zenstate.h"
using namespace ESPZen;

ZenStateMachine sm(0); // Initialize with State 0

void relay_on_enter() {
    digitalWrite(19, HIGH); // Runs exactly ONCE
}

void relay_on_run() {
    ZenOS::yield_ms(5000); // Cools down the CPU for 5 seconds
    sm.transition_to(1);   // Transition state
}

void relay_on_exit() {
    digitalWrite(19, LOW); // Ensures relay is safely turned off
}
```

### 2. Register States
```cpp
void setup() {
    // Define State 0: Entry, Run, Exit
    sm.define_state(0, relay_on_enter, relay_on_run, relay_on_exit);
}
```

### 3. Run the OS Engine
```cpp
void loop() {
    sm.run();
    ZenOS::yield_ms(10); // Safety heartbeat
}
```

---

## Installation (PlatformIO)

Simply copy the `include` and `src` directories into your project, or add the following to your `platformio.ini`:

```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino ; or espidf
```
