#pragma once

#include <cstdint>

#include "pico/stdlib.h"

#include "input/button_gesture_decoder.h"
#include "input/debounced_switch.h"
#include "input/quadrature_decoder.h"

// Waveshare 旋转模块的硬件适配：配置三个 GPIO，采样编码器与内置按键，
// 再把采样值交给不依赖 Pico SDK 的解码器。
class WaveshareRotationSensor {
public:
    // 引脚编号由应用装配；本类型只保存模块各信号的角色。
    WaveshareRotationSensor(uint pin_a, uint pin_b, uint pin_switch,
                            ButtonGestureConfig gesture_config);

    // 配置三个输入引脚，用当前读数建立旋转解码、按键去抖和手势识别的初始状态。
    void init(uint32_t now_ms);

    // 每轮读取按键；距上次 A/B 相采样至少 1 ms 才再次读取，主循环延迟时不会补采样。
    void update(uint32_t now_ms);

    // 返回并清除自上次调用以来累计的带符号整格数。
    int take_detents();

    // 取出一个按键手势；调用方应持续读取，直到返回 ButtonGesture::None。
    ButtonGesture take_button_gesture();

private:
    // 将 A/B 电平合成为两位状态：SIA 是 bit 1，SIB 是 bit 0。
    [[nodiscard]] uint8_t read_state() const;

    uint pin_a_;                            // 模块 SIA 引脚。
    uint pin_b_;                            // 模块 SIB 引脚。
    uint pin_switch_;                       // 模块 SW 引脚，低电平表示按下。
    uint32_t last_encoder_sample_ms_ = 0;   // 上次采样 A/B 相的时刻。
    QuadratureDecoder decoder_;             // 将相位变化解码为机械卡点数。
    DebouncedSwitch switch_;                // 对 SW 电平去抖。
    ButtonGestureDecoder button_gestures_;  // 从稳定按下状态识别手势。
};
