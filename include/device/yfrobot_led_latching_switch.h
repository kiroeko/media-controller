#pragma once

#include <cstdint>

#include "pico/stdlib.h"

#include "input/debounced_switch.h"

// YFROBOT LED 自锁按键模块：SIG 电平跟随自锁开关状态，LED 由模块自身驱动。
// 默认构造后须先调用 init()，再更新或读取结果。
class YfrobotLedLatchingSwitch {
public:
    // 接收 SIG 引脚并配置输入，再用当前电平和 now_ms 初始化去抖状态。
    // 上电时已闭合也会立即读为闭合；LED 由模块自身驱动。
    void init(uint sig_pin, uint32_t now_ms);

    // 读取 SIG 并推进去抖状态。
    void update(uint32_t now_ms);

    // 返回去抖后的开关闭合状态；具体模式含义由应用层决定。
    [[nodiscard]] bool is_on() const;

private:
    [[nodiscard]] bool read_active() const;

    uint sig_pin_ = 0;     // init() 指定的模块 SIG 引脚。
    DebouncedSwitch sig_;  // 对 SIG 电平去抖。
};
