#include "input.h"

namespace {
// Gray-code transitions per detent = 4 × pulses per revolution ÷ detents per revolution; the Waveshare module is 20 pulses/rev with detents unmarked, so measure before trusting this value.
constexpr int8_t kAccumulatorPerDetent = 4;

// One sample per millisecond: mechanical bounce settles well inside that, and
// missing a turn would need two Gray steps within a window (~25 rev/s by hand).
constexpr int32_t kSampleIntervalMs = 1;
}  // namespace

DebouncedInput::DebouncedInput(uint gpio, bool active_high, InputPull pull)
    : gpio_(gpio), active_high_(active_high), pull_(pull) {}

void DebouncedInput::init(uint32_t now_ms) {
    gpio_init(gpio_);
    gpio_set_dir(gpio_, GPIO_IN);

    if (pull_ == InputPull::Up) {
        gpio_pull_up(gpio_);
    } else if (pull_ == InputPull::Down) {
        gpio_pull_down(gpio_);
    } else {
        gpio_disable_pulls(gpio_);
    }

    candidate_active_ = read_active();
    stable_active_ = candidate_active_;
    last_raw_change_ms_ = now_ms;
}

void DebouncedInput::update(uint32_t now_ms) {
    const bool raw_active = read_active();
    if (raw_active != candidate_active_) {
        candidate_active_ = raw_active;
        last_raw_change_ms_ = now_ms;
    }

    if (candidate_active_ != stable_active_ &&
        static_cast<uint32_t>(now_ms - last_raw_change_ms_) >= kDebounceMs) {
        const bool was_active = stable_active_;
        stable_active_ = candidate_active_;
        if (!was_active && stable_active_) {
            activated_ = true;
        }
    }
}

bool DebouncedInput::is_active() const {
    return stable_active_;
}

bool DebouncedInput::take_activated() {
    const bool result = activated_;
    activated_ = false;
    return result;
}

bool DebouncedInput::read_active() const {
    const bool pin_is_high = gpio_get(gpio_);
    return active_high_ ? pin_is_high : !pin_is_high;
}

QuadratureEncoder::QuadratureEncoder(uint pin_a, uint pin_b, uint pin_switch)
    : pin_a_(pin_a), pin_b_(pin_b), switch_(pin_switch, false, InputPull::Up) {}

void QuadratureEncoder::init(uint32_t now_ms) {
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

void QuadratureEncoder::update(uint32_t now_ms) {
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

int QuadratureEncoder::take_turns() {
    const int result = pending_turns_;
    pending_turns_ = 0;
    return result;
}

bool QuadratureEncoder::take_switch_pressed() {
    return switch_.take_activated();
}

uint8_t QuadratureEncoder::read_state() const {
    return static_cast<uint8_t>((gpio_get(pin_a_) ? 0b10 : 0) |
                                (gpio_get(pin_b_) ? 0b01 : 0));
}

