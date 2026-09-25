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
    // 已确认的手势事件；对应标志保留到 take_gesture() 取走，同类事件只保留一个。
    bool short_event_ready_ = false;
    bool double_event_ready_ = false;
    bool long_event_ready_ = false;

    // 上次稳定按下状态用于识别边沿；本次稳定按下时刻用于计算按压时长。
    bool previous_pressed_ = false;
    uint32_t press_start_ms_ = 0;

    // 当前按压已生成的手势记录：第二次双击按压继续长按时，两者可同时成立。
    // 松开时据此判断是否还需生成 Short，然后一起清除。
    bool press_generated_double_ = false;
    bool press_generated_long_ = false;

    // 双击开启时，短按松开后先成为候选：short_pending_ 表示候选存在，
    // short_deadline_ms_ 是等待第二次按下的截止时刻，零也是合法时刻。
    // 窗口内第二次按下生成 Double；到期仍无第二次按下才生成 Short。
    bool short_pending_ = false;
    uint32_t short_deadline_ms_ = 0;

    ButtonGestureConfig config_;  // 构造时传入的时间阈值与双击开关。
};
