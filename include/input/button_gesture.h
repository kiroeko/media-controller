#pragma once

#include <cstdint>

// ButtonGestureDecoder 根据去抖后的按下/松开状态产生的输入事件。
// 它只描述用户怎样操作按键；播放暂停、静音等动作由应用层另行决定。
enum class ButtonGesture : uint8_t {
    // 当前没有待取出的事件。
    None,

    // 一次按下未达到长按阈值就已松开。关闭双击时立即产生；
    // 开启双击时，窗口结束且没有第二次按下才产生。
    Short,

    // 稳定按下达到长按阈值时产生，每次按下最多产生一次。
    Long,

    // 开启双击后，在第一次松开后的窗口内检测到第二次稳定按下时产生。
    // 如果第二次按压继续达到长按阈值，还可能产生 Long。
    Double,
};

// 应用层选择按键手势的时间规则，再传给 ButtonGestureDecoder。
// 这里的时间从去抖后的状态计算；物理触点的去抖时长由 DebouncedSwitch 单独配置。
struct ButtonGestureConfig {
    uint32_t double_gap_ms;  // 第一次稳定松开到第二次稳定按下允许的最长间隔。
    uint32_t long_press_ms;  // 稳定按下持续多久算长按。
    bool detect_double;      // 开启后识别双击，并延迟确认第一次短按；关闭后短按在松开时立即产生。
};
