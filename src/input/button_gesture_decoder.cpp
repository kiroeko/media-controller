#include "input/button_gesture_decoder.h"

ButtonGestureDecoder::ButtonGestureDecoder(ButtonGestureConfig config)
    : config_(config) {}

// 用稳定的启动状态建立按压基准，并清空待处理事件。
void ButtonGestureDecoder::seed(uint32_t now_ms, bool active) {
    short_pressed_ = false;
    double_pressed_ = false;
    long_pressed_ = false;
    previous_active_ = active;
    press_start_ms_ = now_ms;
    pending_short_at_ms_ = 0;
    short_pending_ = false;
    long_fired_ = false;
    suppress_short_ = false;
}

// 比较本次和上次的稳定按下状态，识别按下/松开边沿；保持按下时检查长按，
// 等待双击窗口结束时再确认尚未取出的短按。
void ButtonGestureDecoder::update(uint32_t now_ms, bool active) {
    if (!previous_active_ && active) {
        // 从松开变为按下：启用双击且第一次短按仍在确认窗口内，才构成双击。
        // 双击在第二次按下时发出，第二次松开不能再产生短按。
        // 有符号时间差兼容毫秒计数回绕；恰好在截止时刻按下也算双击。
        if (config_.detect_double && short_pending_ &&
            static_cast<int32_t>(now_ms - pending_short_at_ms_) <= 0) {
            short_pending_ = false;
            suppress_short_ = true;
            double_pressed_ = true;
        } else {
            // 普通的新按压可以在松开时生成短按；过期的前一次短按稍后确认。
            suppress_short_ = false;
        }
        // 每次稳定按下都重新计时，长按每次按压最多触发一次。
        press_start_ms_ = now_ms;
        long_fired_ = false;
    } else if (previous_active_ && !active) {
        // 从按下变为松开：双击的第二次松开和已触发长按的松开都不报短按。
        if (!suppress_short_ && !long_fired_) {
            if (config_.detect_double) {
                // 开启双击时先等待窗口结束，以便第二次按下能取消这次短按。
                pending_short_at_ms_ = now_ms + config_.double_gap_ms;
                short_pending_ = true;
            } else {
                // 关闭双击时，稳定松开就立即报短按。
                short_pressed_ = true;
            }
        }
    }

    // 保持按下达到阈值就报长按，不必等到松开；第二次双击按压也可能继续触发长按。
    if (active && !long_fired_ &&
        now_ms - press_start_ms_ >= config_.long_press_ms) {
        long_fired_ = true;
        long_pressed_ = true;
    }

    // 双击窗口到期且尚未发生第二次有效按下时，才确认第一次短按。
    // 此时即使新的一次按压已经开始，前一次短按仍应独立发出。
    if (short_pending_ &&
        static_cast<int32_t>(now_ms - pending_short_at_ms_) >= 0) {
        short_pending_ = false;
        short_pressed_ = true;
    }

    // 留给下一轮做边沿比较。
    previous_active_ = active;
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
