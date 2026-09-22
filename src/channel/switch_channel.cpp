#include "channel/switch_channel.h"

SwitchChannel::SwitchChannel(uint gpio, bool active_high, InputPull pull, bool detect_double,
                             uint32_t long_press_ms, uint32_t double_gap_ms)
    : gpio_(gpio)
    , active_high_(active_high)
    , pull_(pull)
    , detect_double_(detect_double)
    , long_press_ms_(long_press_ms)
    , double_gap_ms_(double_gap_ms) {}

void SwitchChannel::init(uint32_t now_ms) {
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
    was_active_ = stable_active_;
    last_raw_change_ms_ = now_ms;
}

void SwitchChannel::update(uint32_t now_ms) {
    const bool raw_active = read_active();
    if (raw_active != candidate_active_) {
        candidate_active_ = raw_active;
        last_raw_change_ms_ = now_ms;
    }

    if (candidate_active_ != stable_active_ &&
        static_cast<uint32_t>(now_ms - last_raw_change_ms_) >= kDebounceMs) {
        stable_active_ = candidate_active_;
    }

    const bool level = stable_active_;

    if (level && !was_active_) {
        if (detect_double_ && pending_short_at_ms_ != 0 &&
            static_cast<int32_t>(now_ms - pending_short_at_ms_) <= 0) {
            // Second press inside the gap: the waiting short becomes a double.
            pending_short_at_ms_ = 0;
            suppress_short_ = true;
            double_pressed_ = true;
        } else {
            suppress_short_ = false;
        }
        press_start_ms_ = now_ms;
        long_fired_ = false;
    } else if (!level && was_active_) {
        if (!suppress_short_ && !long_fired_) {
            if (detect_double_) {
                pending_short_at_ms_ = now_ms + double_gap_ms_;
            } else {
                short_pressed_ = true;
            }
        }
    }

    if (level && !long_fired_ &&
        static_cast<uint32_t>(now_ms - press_start_ms_) >= long_press_ms_) {
        long_fired_ = true;
        long_pressed_ = true;
    }

    if (pending_short_at_ms_ != 0 &&
        static_cast<int32_t>(now_ms - pending_short_at_ms_) >= 0) {
        pending_short_at_ms_ = 0;
        short_pressed_ = true;
    }

    was_active_ = level;
}

bool SwitchChannel::is_active() const {
    return stable_active_;
}

SwitchGesture SwitchChannel::take_gesture() {
    if (long_pressed_) {
        long_pressed_ = false;
        return SwitchGesture::Long;
    }
    if (double_pressed_) {
        double_pressed_ = false;
        return SwitchGesture::Double;
    }
    if (short_pressed_) {
        short_pressed_ = false;
        return SwitchGesture::Short;
    }
    return SwitchGesture::None;
}

bool SwitchChannel::read_active() const {
    const bool pin_is_high = gpio_get(gpio_);
    return active_high_ ? pin_is_high : !pin_is_high;
}
