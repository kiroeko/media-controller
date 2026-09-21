#include "input.h"

DebouncedInput::DebouncedInput(uint gpio, bool active_high, bool enable_pull_up)
    : gpio_(gpio), active_high_(active_high), enable_pull_up_(enable_pull_up) {}

void DebouncedInput::init(uint32_t now_ms) {
    gpio_init(gpio_);
    gpio_set_dir(gpio_, GPIO_IN);

    if (enable_pull_up_) {
        gpio_pull_up(gpio_);
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
    : pin_a_(pin_a), pin_b_(pin_b), switch_(pin_switch, false, true) {}

void QuadratureEncoder::init(uint32_t now_ms) {
    gpio_init(pin_a_);
    gpio_set_dir(pin_a_, GPIO_IN);
    gpio_pull_up(pin_a_);

    gpio_init(pin_b_);
    gpio_set_dir(pin_b_, GPIO_IN);
    gpio_pull_up(pin_b_);

    previous_state_ = read_state();
    switch_.init(now_ms);
}

void QuadratureEncoder::update(uint32_t now_ms) {
    // Valid quadrature transitions are one Gray-code step apart. Four steps
    // form one detent on the common EC11 module.
    static constexpr int8_t kTransitionDelta[16] = {
        0, -1, 1, 0,
        1, 0, 0, -1,
        -1, 0, 0, 1,
        0, 1, -1, 0,
    };

    const uint8_t state = read_state();
    quadrature_accumulator_ += kTransitionDelta[(previous_state_ << 2U) | state];
    previous_state_ = state;

    if (quadrature_accumulator_ >= 4) {
        ++pending_turns_;
        quadrature_accumulator_ -= 4;
    } else if (quadrature_accumulator_ <= -4) {
        --pending_turns_;
        quadrature_accumulator_ += 4;
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

