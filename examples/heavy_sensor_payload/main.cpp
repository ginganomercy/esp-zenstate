/**
 * @file main.cpp
 * @brief Demonstrasi ESP-ZenState untuk membasmi Thermal Throttling
 *        pada beban sensor berat (JSN-SR04T, BTS7960, Relay).
 */

#include <Arduino.h>
#include "esp_zenstate.h"

using namespace ESPZen;

// Definisi Pin Hardware
#define TRIGGER_PIN  5
#define ECHO_PIN     18
#define RELAY_PIN    19
#define MOTOR_R_EN   25
#define MOTOR_L_EN   26

enum ProgramState {
    STATE_INIT = 0,
    STATE_WAIT_WATER_LEVEL,
    STATE_PUMPING_WATER,
    STATE_ERROR
};

ZenStateMachine sm(STATE_INIT);
ZenUltrasonic sonar(TRIGGER_PIN, ECHO_PIN);

// ==========================================================
// C-STYLE HOOKS (No Memory Allocation)
// ==========================================================

void init_on_enter() {
    Serial.println("Memulai Sistem Irigasi Cerdas...");
}

void init_on_run() {
    sm.transition_to(STATE_WAIT_WATER_LEVEL);
}

void wait_water_on_enter() {
    sonar.trigger_async();
}

void wait_water_on_run() {
    ZenOS::yield_ms(60); // Dinginkan CPU sambil nunggu pantulan

    if (sonar.is_ready()) {
        float dist = sonar.get_distance_cm(); // Kalkulasi float Thread-Safe
        Serial.printf("Jarak Air: %.2f cm\n", dist);
        
        if (dist > 50.0) {
            sm.transition_to(STATE_PUMPING_WATER);
        } else {
            sonar.trigger_async(); // Tembak lagi
        }
    }
}

void pump_on_enter() {
    Serial.println("MENGHIDUPKAN POMPA DAN MOTOR!");
    digitalWrite(RELAY_PIN, HIGH);
}

void pump_on_run() {
    // Pompa menyala 5 detik. Alih-alih delay(), kita Yield CPU!
    ZenOS::yield_ms(5000); 
    sm.transition_to(STATE_WAIT_WATER_LEVEL);
}

void pump_on_exit() {
    digitalWrite(RELAY_PIN, LOW); // Pompa PASTI mati saat keluar state
    Serial.println("Pompa Mati.");
}

// ==========================================================
// OS SETUP
// ==========================================================

void network_task(void* pvParameters) {
    while (true) {
        // Simulasi logika MQTT di Core 0
        ZenOS::yield_ms(100); 
    }
}

void setup() {
    Serial.begin(115200);
    
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    
    pinMode(MOTOR_R_EN, OUTPUT);
    pinMode(MOTOR_L_EN, OUTPUT);
    digitalWrite(MOTOR_R_EN, HIGH);
    digitalWrite(MOTOR_L_EN, HIGH); 

    sonar.begin();
    ZenOS::spawn_task(network_task, "NetworkTask", 4096, NULL, 1, 0);

    // Mendaftarkan State Hooks secara statis (Zero Heap Memory)
    sm.define_state(STATE_INIT, init_on_enter, init_on_run, nullptr);
    sm.define_state(STATE_WAIT_WATER_LEVEL, wait_water_on_enter, wait_water_on_run, nullptr);
    sm.define_state(STATE_PUMPING_WATER, pump_on_enter, pump_on_run, pump_on_exit);
}

void loop() {
    sm.run(); // Jalan di Core 1
    ZenOS::yield_ms(10); // Safety Yield
}
