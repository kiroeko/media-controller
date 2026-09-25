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

    // 每次取走一个手势；若多个事件同时待取，依次返回短按、双击、长按。
    // 可反复调用直到 None；这个顺序不会丢弃其余待取事件。
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

    // 双击开启时，短按松开后先成为候选：short_pending_ 表示候选存在，
    // 窗口内第二次按下生成 Double；到期仍无第二次按下才生成 Short。
    bool short_pending_ = false;
    // 等待第二次按下的截止时刻，零也是合法时刻。
    uint32_t short_deadline_ms_ = 0;

    // 本次按压是否已触发 Double：双击窗口内的第二次按下时置 true。
    // 松开时据此阻止再生成 Short；处理完松开后清零。
    bool press_generated_double_ = false;

    // 本次按压是否已触发 Long：按住或刚松开时达到长按阈值就置 true。
    // 按住期间据此避免重复报 Long，松开时阻止再生成 Short；处理完松开后清零。
    bool press_generated_long_ = false;

    ButtonGestureConfig config_;  // 构造时传入的时间阈值与双击开关。
};
