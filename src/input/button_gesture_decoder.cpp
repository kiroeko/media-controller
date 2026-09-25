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
    press_generated_double_ = false;
    press_generated_long_ = false;
    short_pending_ = false;
    short_deadline_ms_ = 0;
}

// 每轮分四步：识别刚按下、检查长按、处理刚松开、确认过期的短按。
// 最后保存本轮状态，供下一轮判断边沿。
void ButtonGestureDecoder::update(uint32_t now_ms, bool pressed) {
    const bool just_pressed = pressed && !previous_pressed_;
    const bool just_released = !pressed && previous_pressed_;

    // 1. 刚按下：从此刻计时。开启双击时，窗口内的第二次按下直接产生双击。
    if (just_pressed) {
        press_start_ms_ = now_ms;
        // 已有短按候选且本次按下未超过截止时刻，才是双击的第二次按下。
        // 有符号时间差在毫秒计数回绕时仍能比较先后；等于截止时刻也算窗口内。
        const bool completes_double = config_.detect_double && short_pending_ &&
            static_cast<int32_t>(now_ms - short_deadline_ms_) <= 0;
        press_generated_double_ = completes_double;
        if (completes_double) {
            short_pending_ = false;
            double_event_ready_ = true;
        }
    }

    // 2. 按住达到阈值就报长按；刚松开时也检查一次，避免两轮之间跨过阈值而漏报。
    if ((pressed || just_released) && !press_generated_long_ &&
        now_ms - press_start_ms_ >= config_.long_press_ms) {
        press_generated_long_ = true;
        long_event_ready_ = true;
    }

    // 3. 刚松开：先用本次按压的标志判断短按，再结束这次按压。
    if (just_released) {
        if (!press_generated_long_ && !press_generated_double_) {
            if (config_.detect_double) {
                // 建立短按候选，并记录允许第二次稳定按下的截止时刻。
                short_deadline_ms_ = now_ms + config_.double_gap_ms;
                short_pending_ = true;
            } else {
                short_event_ready_ = true;
            }
        }
        // 这两个标志只描述刚结束的按压，松开处理完就清除。
        press_generated_double_ = false;
        press_generated_long_ = false;
    }

    // 4. 候选短按到期：将它变为可由 take_gesture() 取走的 Short 事件。
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
