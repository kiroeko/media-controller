#pragma once

#include <cstdint>

#include "input/button_gesture.h"

// 根据已去抖的按下/松开状态识别手势；不访问 GPIO，也不负责电平去抖。
class ButtonGestureDecoder {
public:
    explicit ButtonGestureDecoder(ButtonGestureConfig config);

    // 用当前稳定的按下状态建立基准，并清空尚未取出的手势事件。
    void seed(uint32_t now_ms, bool pressed);

    // 每轮输入去抖后的按下状态；pressed 为 true 表示按键稳定按下。
    // 松开→按下时记录起点；保持按下或刚松开时检查长按，再判断短按。
    void update(uint32_t now_ms, bool pressed);

    // 取走一个手势，优先级依次为长按、双击、短按；可循环取到 None。
    // 每种手势只保存一个待取标志，同类事件在取走前重复出现会合并。
    ButtonGesture take_gesture();

private:
    bool short_event_ready_ = false;     // 已确认、尚未取出的短按事件。
    bool double_event_ready_ = false;    // 已确认、尚未取出的双击事件。
    bool long_event_ready_ = false;      // 已确认、尚未取出的长按事件。

    bool previous_pressed_ = false;      // 上一次输入的稳定按下状态，用于识别边沿。
    uint32_t press_start_ms_ = 0;        // 当前稳定按压开始的时刻。

    bool second_press_of_double_ = false; // 当前按压是双击的第二次，松开后清除。
    bool short_pending_ = false;         // 第一次短按仍待双击窗口结束，尚不可取。
    uint32_t short_deadline_ms_ = 0;     // 待确认短按的截止时刻；可为零。

    bool long_reported_for_press_ = false; // 当前按压是否已报长按，松开处理后清除。

    ButtonGestureConfig config_;         // 手势时间阈值和双击开关。
};
