#include "rotation_sensor.h"

namespace {
// One sample per millisecond: mechanical bounce settles well inside that, and
// missing a turn would need two Gray steps within a window (~25 rev/s by hand).
constexpr int32_t kSampleIntervalMs = 1;
}  // namespace

RotationSensor::RotationSensor(uint pin_a, uint pin_b, uint pin_switch)
    : pin_a_(pin_a), pin_b_(pin_b), switch_(pin_switch, false, InputPull::Up) {}

void RotationSensor::init(uint32_t now_ms) {
    gpio_init(pin_a_);
    gpio_set_dir(pin_a_, GPIO_IN);
    gpio_pull_up(pin_a_);

    gpio_init(pin_b_);
    gpio_set_dir(pin_b_, GPIO_IN);
    gpio_pull_up(pin_b_);

    last_sample_ms_ = now_ms - 1;
    decoder_.seed(read_state());
    switch_.init(now_ms);
}

void RotationSensor::update(uint32_t now_ms) {
    // The button is polled every loop; only the phase sampling is throttled,
    // so the debounce resolution no longer depends on the encoder sample rate.
    switch_.update(now_ms);

    if (static_cast<int32_t>(now_ms - last_sample_ms_) >= kSampleIntervalMs) {
        last_sample_ms_ = now_ms;
        decoder_.feed(read_state());
    }
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
