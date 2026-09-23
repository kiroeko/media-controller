#pragma once

#include <cstdint>

// 按键经过去抖与计时后产生的事件，可由器件和应用共同使用。
enum class ButtonGesture : uint8_t { None, Short, Long, Double };

// 交互时间阈值由应用选择，再交给按键手势解码器。
struct ButtonGestureConfig {
    uint32_t long_press_ms;
    uint32_t double_gap_ms;
    bool detect_double;
};
