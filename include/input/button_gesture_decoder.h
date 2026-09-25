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
    bool short_event_ready_ = false;      // true：Short 已生成，等待 take_gesture() 取走。
    bool double_event_ready_ = false;     // true：Double 已生成，等待 take_gesture() 取走。
    bool long_event_ready_ = false;       // true：Long 已生成，等待 take_gesture() 取走。

    bool previous_pressed_ = false;       // 上一次 seed/update 的 pressed 值，用来识别按下和松开边沿。
    uint32_t press_start_ms_ = 0;         // 最近一次稳定按下的时刻，用来计算本次按压时长。

    bool second_press_of_double_ = false; // true：当前按压已组成双击的第二次，松开时不再报 Short。
    bool short_pending_ = false;          // true：一次松开已形成短按候选，正等第二次按下或截止时间。
    uint32_t short_deadline_ms_ = 0;      // short_pending_ 为 true 时，第二次按下的截止时刻；零也有效。

    bool long_reported_for_press_ = false; // true：本次按压已报 Long，防止重复或松开后再报 Short。

    ButtonGestureConfig config_;           // 手势时间阈值和双击开关。
};
