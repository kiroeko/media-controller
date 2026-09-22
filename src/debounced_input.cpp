#include "debounced_input.h"

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
