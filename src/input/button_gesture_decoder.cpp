#include "input/button_gesture_decoder.h"

ButtonGestureDecoder::ButtonGestureDecoder(ButtonGestureConfig config)
    : config_(config) {}

// 用稳定的启动状态建立按压基准，并清空待处理事件。
void ButtonGestureDecoder::seed(uint32_t now_ms, bool active) {
    short_pressed_ = false;
    double_pressed_ = false;
    long_pressed_ = false;
    was_active_ = active;
    press_start_ms_ = now_ms;
    pending_short_at_ms_ = 0;
    short_pending_ = false;
    long_fired_ = false;
    suppress_short_ = false;
}

// 依据稳定状态的按下/松开边沿和经过时间生成手势。
void ButtonGestureDecoder::update(uint32_t now_ms, bool active) {
    if (!was_active_ && active) {
        if (config_.detect_double && short_pending_ &&
            static_cast<int32_t>(now_ms - pending_short_at_ms_) <= 0) {
            // 第二次按压落在双击窗口内：取消待确认短按并生成双击事件。
            short_pending_ = false;
            suppress_short_ = true;
            double_pressed_ = true;
        } else {
            suppress_short_ = false;
        }
        press_start_ms_ = now_ms;
        long_fired_ = false;
    } else if (was_active_ && !active) {
        if (!suppress_short_ && !long_fired_) {
            if (config_.detect_double) {
                pending_short_at_ms_ = now_ms + config_.double_gap_ms;
                short_pending_ = true;
            } else {
                short_pressed_ = true;
            }
        }
    }

    if (active && !long_fired_ &&
        now_ms - press_start_ms_ >= config_.long_press_ms) {
        long_fired_ = true;
        long_pressed_ = true;
    }

    if (short_pending_ &&
        static_cast<int32_t>(now_ms - pending_short_at_ms_) >= 0) {
        short_pending_ = false;
        short_pressed_ = true;
    }

    was_active_ = active;
}

// 按优先级取走一个待处理手势事件。
ButtonGesture ButtonGestureDecoder::take_gesture() {
    if (long_pressed_) {
        long_pressed_ = false;
        return ButtonGesture::Long;
    }
    if (double_pressed_) {
        double_pressed_ = false;
        return ButtonGesture::Double;
    }
    if (short_pressed_) {
        short_pressed_ = false;
        return ButtonGesture::Short;
    }
    return ButtonGesture::None;
}
