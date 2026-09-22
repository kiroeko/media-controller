#include "rotation_sensor.h"

namespace {
// Gray-code transitions per detent = 4 × pulses per revolution ÷ detents per revolution; the Waveshare module is 20 pulses/rev with detents unmarked, so measure before trusting this value.
constexpr int8_t kAccumulatorPerDetent = 4;

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

    previous_state_ = read_state();
    last_sample_ms_ = now_ms - 1;
    switch_.init(now_ms);
}

void RotationSensor::update(uint32_t now_ms) {
    if (static_cast<int32_t>(now_ms - last_sample_ms_) < kSampleIntervalMs) {
        return;
    }
    last_sample_ms_ = now_ms;

    // Valid quadrature transitions are one Gray-code step apart.
    static constexpr int8_t kTransitionDelta[16] = {
        0, -1, 1, 0,
        1, 0, 0, -1,
        -1, 0, 0, 1,
        0, 1, -1, 0,
    };

    const uint8_t state = read_state();
    quadrature_accumulator_ += kTransitionDelta[(previous_state_ << 2U) | state];
    previous_state_ = state;

    if (quadrature_accumulator_ >= kAccumulatorPerDetent) {
        ++pending_turns_;
        quadrature_accumulator_ -= kAccumulatorPerDetent;
    } else if (quadrature_accumulator_ <= -kAccumulatorPerDetent) {
        --pending_turns_;
        quadrature_accumulator_ += kAccumulatorPerDetent;
    }

    switch_.update(now_ms);
}

int RotationSensor::take_turns() {
    const int result = pending_turns_;
    pending_turns_ = 0;
    return result;
}

bool RotationSensor::take_switch_pressed() {
    return switch_.take_activated();
}

uint8_t RotationSensor::read_state() const {
    return static_cast<uint8_t>((gpio_get(pin_a_) ? 0b10 : 0) |
                                (gpio_get(pin_b_) ? 0b01 : 0));
}
