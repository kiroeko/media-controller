#include "input/button_gesture_decoder.h"

ButtonGestureDecoder::ButtonGestureDecoder(ButtonGestureConfig config)
    : config_(config) {}

// 用稳定的启动状态建立按压基准，并清空待处理事件。
void ButtonGestureDecoder::seed(uint32_t now_ms, bool pressed) {
    short_event_ready_ = false;
    double_event_ready_ = false;
    long_event_ready_ = false;
    previous_pressed_ = pressed;
    press_start_ms_ = now_ms;
    short_deadline_ms_ = 0;
    short_pending_ = false;
    long_reported_for_press_ = false;
    suppress_short_on_release_ = false;
}

// 每轮分四步：识别刚按下、识别刚松开、检查持续按下、确认过期的短按。
// 最后保存本轮状态，供下一轮判断边沿。
void ButtonGestureDecoder::update(uint32_t now_ms, bool pressed) {
    const bool just_pressed = pressed && !previous_pressed_;
    const bool just_released = !pressed && previous_pressed_;

    // 1. 刚按下：从此刻计时。开启双击时，窗口内的第二次按下直接产生双击。
    if (just_pressed) {
        press_start_ms_ = now_ms;
        long_reported_for_press_ = false;
        // 有符号时间差在毫秒计数回绕时仍能比较先后；等于截止时刻也算窗口内。
        const bool completes_double = config_.detect_double && short_pending_ &&
            static_cast<int32_t>(now_ms - short_deadline_ms_) <= 0;
        suppress_short_on_release_ = completes_double;
        if (completes_double) {
            short_pending_ = false;
            double_event_ready_ = true;
        }
    }

    // 2. 刚松开：已报长按或属于双击第二次按压时，不再产生短按。
    if (just_released && !long_reported_for_press_ && !suppress_short_on_release_) {
        if (config_.detect_double) {
            // 暂等第二次按下；到期仍未发生，才确认这次短按。
            short_deadline_ms_ = now_ms + config_.double_gap_ms;
            short_pending_ = true;
        } else {
            short_event_ready_ = true;
        }
    }

    // 3. 持续按下：达到阈值立刻报长按，同一次按压不会重复报告。
    if (pressed && !long_reported_for_press_ &&
        now_ms - press_start_ms_ >= config_.long_press_ms) {
        long_reported_for_press_ = true;
        long_event_ready_ = true;
    }

    // 4. 双击窗口到期：确认第一次短按；新的一次按压可能已经开始。
    if (short_pending_ &&
        static_cast<int32_t>(now_ms - short_deadline_ms_) >= 0) {
        short_pending_ = false;
        short_event_ready_ = true;
    }

    previous_pressed_ = pressed;
}

// 按优先级取走一个待处理手势事件。
ButtonGesture ButtonGestureDecoder::take_gesture() {
    if (long_event_ready_) {
        long_event_ready_ = false;
        return ButtonGesture::Long;
    }
    if (double_event_ready_) {
        double_event_ready_ = false;
        return ButtonGesture::Double;
    }
    if (short_event_ready_) {
        short_event_ready_ = false;
        return ButtonGesture::Short;
    }
    return ButtonGesture::None;
}
