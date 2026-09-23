#pragma once

#include <cstdint>

#include "pico/stdlib.h"

#include "input/debounced_switch.h"

// YFROBOT LED 自锁按键模块：SIG 电平跟随自锁开关状态，LED 由模块自身驱动。
class YfrobotLedLatchingSwitch {
public:
    // 固件只需读取 SIG；模块自行驱动 LED。
    explicit YfrobotLedLatchingSwitch(uint sig_pin);

    // 配置 SIG，并将去抖状态对齐到当前电平；自锁开关的机械位置能跨重新刷写保留。
    void init(uint32_t now_ms);

    // 读取 SIG 并推进去抖状态。
    void update(uint32_t now_ms);

    // 返回去抖后的开关闭合状态；具体模式含义由应用层决定。
    [[nodiscard]] bool is_on() const;

private:
    [[nodiscard]] bool read_active() const;

    uint sig_pin_;       // 模块 SIG 引脚。
    DebouncedSwitch sig_;  // 对 SIG 电平去抖。
};
