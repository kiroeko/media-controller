#pragma once

#include <cstdint>

#include "pico/stdlib.h"

#include "channel/switch_channel.h"

// YFROBOT LED 自锁按键模块：SIG 电平跟随自锁开关状态，LED 由模块自身驱动。
class ModeDevice {
public:
    // 固件只需读取 SIG；模块自行驱动 LED。
    explicit ModeDevice(uint sig_pin);

    // 配置 SIG，并在启动时把去抖状态对齐到当前电平。重新刷写不会改变自锁开关状态，
    // 因此启动模式由开关当前的位置决定。
    void init(uint32_t now_ms);

    // 推进 SIG 的去抖状态；此模块没有其他固件输入。
    void update(uint32_t now_ms);

    // 返回自锁开关是否处于闭合状态，也就是是否为切歌模式。返回稳定电平而非边沿，
    // 可重复查询且不会丢失状态。
    [[nodiscard]] bool is_on() const;

private:
    SwitchChannel sig_;  // 此模块只使用稳定电平，不取出通道生成的手势事件。
};
