#include "esp_zenstate.h"

namespace ESPZen {

// =========================================================================
// ZenStateMachine Implementation
// =========================================================================
ZenStateMachine::ZenStateMachine(int initial_state) : 
    _current_state(initial_state), 
    _next_state(initial_state),
    _is_transitioning(true) // Force entry hook on first run
{
    for (int i = 0; i < 32; i++) {
        _states[i].on_enter = nullptr;
        _states[i].on_run = nullptr;
        _states[i].on_exit = nullptr;
    }
}

void ZenStateMachine::define_state(int state, StateCallback on_enter, StateCallback on_run, StateCallback on_exit) {
    if (state >= 0 && state < 32) {
        _states[state].on_enter = on_enter;
        _states[state].on_run = on_run;
        _states[state].on_exit = on_exit;
    }
}

void ZenStateMachine::transition_to(int new_state) {
    if (new_state >= 0 && new_state < 32 && _current_state != new_state) {
        _next_state = new_state;
        _is_transitioning = true;
    }
}

int ZenStateMachine::current_state() const {
    return _current_state;
}

void ZenStateMachine::run() {
    // Check if we need to transition
    if (_is_transitioning) {
        // 1. Run exit hook of old state (skip if very first run)
        if (_current_state != _next_state && _states[_current_state].on_exit != nullptr) {
            _states[_current_state].on_exit();
        }
        
        _current_state = _next_state;
        
        // 2. Run enter hook of new state
        if (_states[_current_state].on_enter != nullptr) {
            _states[_current_state].on_enter();
        }
        
        _is_transitioning = false;
    }

    // 3. Run the continuous logic
    if (_states[_current_state].on_run != nullptr) {
        _states[_current_state].on_run();
    } else {
        ZenOS::yield_ms(10); // Fallback idle
    }
}

// =========================================================================
// ZenOS Implementation
// =========================================================================
void ZenOS::yield_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void ZenOS::spawn_task(TaskFunction_t task_func, const char* name, uint32_t stack_depth, void* params, UBaseType_t priority, BaseType_t core_id) {
    xTaskCreatePinnedToCore(
        task_func,
        name,
        stack_depth,
        params,
        priority,
        NULL,
        core_id
    );
}

// =========================================================================
// ZenUltrasonic Implementation
// =========================================================================
ZenUltrasonic::ZenUltrasonic(uint8_t trigger_pin, uint8_t echo_pin) 
    : _trigger_pin(trigger_pin), _echo_pin(echo_pin), _data_ready(false), _duration_us(0) {}

void ZenUltrasonic::begin() {
#if defined(ARDUINO)
    pinMode(_trigger_pin, OUTPUT);
    pinMode(_echo_pin, INPUT);
    attachInterruptArg(digitalPinToInterrupt(_echo_pin), echo_isr_handler, this, CHANGE);
#else
    gpio_set_direction((gpio_num_t)_trigger_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)_echo_pin, GPIO_MODE_INPUT);
    gpio_set_intr_type((gpio_num_t)_echo_pin, GPIO_INTR_ANYEDGE);
    // Suppress error if already installed by other library
    gpio_install_isr_service(0); 
    gpio_isr_handler_add((gpio_num_t)_echo_pin, echo_isr_handler, this);
#endif
}

void ZenUltrasonic::trigger_async() {
    _data_ready = false;
#if defined(ARDUINO)
    digitalWrite(_trigger_pin, LOW);
    delayMicroseconds(2);
    digitalWrite(_trigger_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trigger_pin, LOW);
#else
    gpio_set_level((gpio_num_t)_trigger_pin, 0);
    esp_rom_delay_us(2);
    gpio_set_level((gpio_num_t)_trigger_pin, 1);
    esp_rom_delay_us(10);
    gpio_set_level((gpio_num_t)_trigger_pin, 0);
#endif
}

bool ZenUltrasonic::is_ready() {
    return _data_ready;
}

float ZenUltrasonic::get_distance_cm() {
    // Float math is done safely in Thread space, not ISR!
    float distance = (_duration_us * 0.0343) / 2.0;
    return distance;
}

void IRAM_ATTR ZenUltrasonic::echo_isr_handler(void* arg) {
    ZenUltrasonic* instance = static_cast<ZenUltrasonic*>(arg);
    
#if defined(ARDUINO)
    int state = digitalRead(instance->_echo_pin);
    int64_t current_time = esp_timer_get_time();
#else
    int state = gpio_get_level((gpio_num_t)instance->_echo_pin);
    int64_t current_time = esp_timer_get_time();
#endif

    if (state == 1) { // RISING edge
        instance->_start_time = current_time;
    } else { // FALLING edge
        instance->_end_time = current_time;
        // Keep to integer calculation inside ISR
        instance->_duration_us = instance->_end_time - instance->_start_time;
        instance->_data_ready = true;
    }
}

} // namespace ESPZen
