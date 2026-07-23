#ifndef ESP_ZENSTATE_H
#define ESP_ZENSTATE_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if defined(ARDUINO)
#include "Arduino.h"
#else
#include "driver/gpio.h"
#include "esp_timer.h"
#endif

namespace ESPZen {

// =========================================================================
// 1. ZEN STATE MACHINE (Zero-Heap, Anti-Spaghetti Framework)
// =========================================================================
// Pure C function pointer. 0 Bytes Heap Allocation.
typedef void (*StateCallback)();

struct StateDefinition {
    StateCallback on_enter;
    StateCallback on_run;
    StateCallback on_exit;
};

class ZenStateMachine {
public:
    ZenStateMachine(int initial_state);
    
    // Define behaviors for a specific state
    void define_state(int state, StateCallback on_enter, StateCallback on_run, StateCallback on_exit);
    
    // Switch state safely (triggers exit/enter hooks automatically)
    void transition_to(int new_state);
    
    // Get current state
    int current_state() const;
    
    // Must be called in the task loop
    void run();

private:
    int _current_state;
    int _next_state;
    bool _is_transitioning;
    StateDefinition _states[32]; // Hardcoded max 32 states for zero-heap safety
};

// =========================================================================
// 2. ZEN OS CORE (Thermal & Multi-Core Mitigator)
// =========================================================================
class ZenOS {
public:
    // Yields CPU to IDLE task, physically dropping heat and saving power.
    static void yield_ms(uint32_t ms);

    // Spawns a dedicated task on a specific core (Core 0 for WiFi, Core 1 for App/Sensors)
    static void spawn_task(TaskFunction_t task_func, const char* name, uint32_t stack_depth, void* params, UBaseType_t priority, BaseType_t core_id);
};

// =========================================================================
// 3. ZEN JSN-SR04T (Non-Blocking Ultrasonic Driver)
// =========================================================================
class ZenUltrasonic {
public:
    ZenUltrasonic(uint8_t trigger_pin, uint8_t echo_pin);
    
    void begin();
    
    // Sends the trigger pulse asynchronously.
    void trigger_async();
    
    // Checks if the duration has been calculated in the background
    bool is_ready();
    
    // Calculates float math here (Thread Safe), NOT in ISR!
    float get_distance_cm();

private:
    uint8_t _trigger_pin;
    uint8_t _echo_pin;
    
    volatile bool _data_ready;
    volatile int64_t _duration_us; // Integer only in ISR
    
    volatile int64_t _start_time;
    volatile int64_t _end_time;

    static void IRAM_ATTR echo_isr_handler(void* arg);
};

} // namespace ESPZen

#endif // ESP_ZENSTATE_H
