#include "device/rotation_device.h"

RotationDevice::RotationDevice(uint pin_a, uint pin_b, uint pin_switch)
    : pin_a_(pin_a), pin_b_(pin_b), switch_(pin_switch, false, InputPull::None) {}

void RotationDevice::init(uint32_t now_ms) {
    // The module carries on-board pull-ups: the vendor demo configures none and
    // its waveforms idle high, so we stack no internal pulls on top.
    gpio_init(pin_a_);
    gpio_set_dir(pin_a_, GPIO_IN);

    gpio_init(pin_b_);
    gpio_set_dir(pin_b_, GPIO_IN);

    decoder_.seed(now_ms, read_state());
    switch_.init(now_ms);
}

void RotationDevice::update(uint32_t now_ms) {
    // Each channel gates its own cadence; the device only advances them.
    switch_.update(now_ms);
    decoder_.update(now_ms, read_state());
}

int RotationDevice::take_turns() {
    return decoder_.take_turns();
}

SwitchGesture RotationDevice::take_switch_gesture() {
    return switch_.take_gesture();
}

uint8_t RotationDevice::read_state() const {
    return static_cast<uint8_t>((gpio_get(pin_a_) ? 0b10 : 0) |
                                (gpio_get(pin_b_) ? 0b01 : 0));
}
