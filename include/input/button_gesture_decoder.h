#pragma once

#include <cstdint>

#include "input/button_gesture.h"

// 根据已去抖的按下/松开状态识别手势；不访问 GPIO，也不负责电平去抖。
class ButtonGestureDecoder {
public:
    explicit ButtonGestureDecoder(ButtonGestureConfig config);

    void seed(uint32_t now_ms, bool active);
    void update(uint32_t now_ms, bool active);

    // 取走一个手势，优先级依次为长按、双击、短按；可循环取到 None。
    ButtonGesture take_gesture();

private:
    ButtonGestureConfig config_;         // 手势时间阈值和双击开关。
    bool was_active_ = false;            // 上一次输入的稳定状态。
    uint32_t press_start_ms_ = 0;        // 当前稳定按压开始的时刻。
    uint32_t pending_short_at_ms_ = 0;   // 待确认短按的截止时刻，零表示没有。
    bool long_fired_ = false;            // 当前按压是否已触发长按。
    bool suppress_short_ = false;        // 双击的第二次松开不应再报短按。
    bool short_pressed_ = false;         // 尚未取出的短按事件。
    bool long_pressed_ = false;          // 尚未取出的长按事件。
    bool double_pressed_ = false;        // 尚未取出的双击事件。
};
