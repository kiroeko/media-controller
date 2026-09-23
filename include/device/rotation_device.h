#pragma once

#include <cstdint>

#include "pico/stdlib.h"

#include "channel/switch_channel.h"
#include "channel/quadrature_channel.h"

// 表示完整的 Waveshare 旋转传感器模块，而不只是编码器：EC11 正交相位和内置按键
// 共用一个接插件及地线，因此作为一个器件、两个通道处理。
class RotationDevice {
public:
    // 模块信号接到开发板哪些引脚由装配层决定；此类型只关心信号角色。
    RotationDevice(uint pin_a, uint pin_b, uint pin_switch);

    // 将两个相位引脚设为输入，并让两个通道按初始化时读到的电平建立状态。
    void init(uint32_t now_ms);

    // 各推进一次两个通道。它们彼此独立，并分别管理自己的采样节奏。
    void update(uint32_t now_ms);

    // 返回并清除自上次调用以来累计的带符号整格数；只读取编码器通道。
    int take_turns();

    // 取出一个按键手势；调用方应持续读取，直到返回 SwitchGesture::None。
    SwitchGesture take_switch_gesture();

private:
    // 将两个相位采样合成为两位格雷码：SIA 是 bit 1，SIB 是 bit 0。
    // 交换两个引脚即可反转报告的旋转方向。
    [[nodiscard]] uint8_t read_state() const;

    uint pin_a_;                 // 模块 SIA 引脚；在此处读取，因为解码器接收相位值而非 GPIO 编号。
    uint pin_b_;                 // 模块 SIB 引脚；原因同上。
    QuadratureChannel decoder_;  // 将相位变化解码为整格数。
    SwitchChannel switch_;       // 管理 SW 按键及其电气配置。
};
