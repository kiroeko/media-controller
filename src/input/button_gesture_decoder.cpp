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

// 每轮按顺序处理一次去抖后的状态：建立本次按压、检查长按、处理松开、确认短按。
// 长按检查先于松开处理，刚松开时达到阈值就不会被误判为短按；双击判断先于
// 短按到期判断，恰在截止时刻再次按下仍可算作双击。
void ButtonGestureDecoder::update(uint32_t now_ms, bool pressed) {
    // 与上一轮比较，区分“刚按下/刚松开”和持续按住/持续松开。
    const bool just_pressed = pressed && !previous_pressed_;
    const bool just_released = !pressed && previous_pressed_;

    // 1. 刚按下：记录本次按压的起点，后续用它计算长按时长。
    // 若上次松开留下一个短按候选，且这次按下仍在双击窗口内，就把候选改判为双击。
    if (just_pressed) {
        press_start_ms_ = now_ms;
        // 有符号时间差在毫秒计数回绕时仍能比较先后；等于截止时刻也算窗口内。
        const bool completes_double = config_.detect_double && short_pending_ &&
            static_cast<int32_t>(now_ms - short_deadline_ms_) <= 0;
        press_generated_double_ = completes_double;
        if (completes_double) {
            // 候选短按已被双击消费；记住本次按压生成了双击，松开时不再报短按。
            short_pending_ = false;
            double_event_ready_ = true;
        }
    }

    // 2. 本次按压仍在进行时检查长按；刚松开时补查一次，处理两轮之间跨过阈值的情况。
    // 持续松开时不能再用旧的按下时刻报长按；同一次按压也只能报一次。
    // press_generated_long_ 记录“本次已报过”，即使事件被 take_gesture() 取走也保留到松开。
    if ((pressed || just_released) && !press_generated_long_ &&
        now_ms - press_start_ms_ >= config_.long_press_ms) {
        press_generated_long_ = true;
        long_event_ready_ = true;
    }

    // 3. 刚松开：只有本次按压既没生成双击也没生成长按，才可能生成短按。
    // 因为第 2 步已补查松开瞬间的长按，此处不会把刚达到阈值的按压当成短按。
    if (just_released) {
        if (!press_generated_double_ && !press_generated_long_) {
            if (config_.detect_double) {
                // 开启双击时先暂存短按；第二次按下前不能确定它是不是双击的第一下。
                short_deadline_ms_ = now_ms + config_.double_gap_ms;
                short_pending_ = true;
            } else {
                // 未开启双击时，松开即可确认短按。
                short_event_ready_ = true;
            }
        }
        // 这两个标志只属于刚结束的这次按压；清除后留给下次按压重新记录。
        press_generated_double_ = false;
        press_generated_long_ = false;
    }

    // 4. 双击等待期结束后，若短按候选仍在，说明没有第二次有效按下，确认短按。
    // 这一步放在第 1 步之后，让恰好在截止时刻的第二次按下先消费候选。
    if (short_pending_ &&
        static_cast<int32_t>(now_ms - short_deadline_ms_) >= 0) {
        short_pending_ = false;
        short_event_ready_ = true;
    }

    // 留下本轮状态，供下轮识别刚按下和刚松开的边沿。
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
