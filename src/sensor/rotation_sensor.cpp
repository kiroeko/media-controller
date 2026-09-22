#include "sensor/rotation_sensor.h"

RotationSensor::RotationSensor(uint pin_a, uint pin_b, uint pin_switch)
    : pin_a_(pin_a), pin_b_(pin_b), switch_(pin_switch, false, InputPull::Up) {}

void RotationSensor::init(uint32_t now_ms) {
    gpio_init(pin_a_);
    gpio_set_dir(pin_a_, GPIO_IN);
    gpio_pull_up(pin_a_);

    gpio_init(pin_b_);
    gpio_set_dir(pin_b_, GPIO_IN);
    gpio_pull_up(pin_b_);

    decoder_.seed(now_ms, read_state());
    switch_.init(now_ms);
}

void RotationSensor::update(uint32_t now_ms) {
    // Each channel gates its own cadence; the device only advances them.
    switch_.update(now_ms);
    decoder_.update(now_ms, read_state());
}

int RotationSensor::take_turns() {
    return decoder_.take_turns();
}

bool RotationSensor::take_switch_pressed() {
    return switch_.take_activated();
}

uint8_t RotationSensor::read_state() const {
    return static_cast<uint8_t>((gpio_get(pin_a_) ? 0b10 : 0) |
                                (gpio_get(pin_b_) ? 0b01 : 0));
}
