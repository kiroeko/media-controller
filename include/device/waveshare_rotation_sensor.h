#pragma once

#include <cstdint>

#include "pico/stdlib.h"

#include "input/button_gesture_decoder.h"
#include "input/debounced_switch.h"
#include "input/quadrature_decoder.h"

// Waveshare 旋转模块的硬件适配：配置三个 GPIO，采样编码器与内置按键，
// 再把采样值交给不依赖 Pico SDK 的解码器。
// 默认构造后须先调用 init()，再更新或读取结果。
class WaveshareRotationSensor {
public:
    // 应用在 init() 时提供的接线和交互配置；采样、去抖时序由模块自身定义。
    struct Config {
        uint pin_a;                          // SIA 对应的 GPIO 编号。
        uint pin_b;                          // SIB 对应的 GPIO 编号。
        uint pin_switch;                     // SW 对应的 GPIO 编号。
        ButtonGestureConfig gesture_config;  // 内置按键的手势时间阈值与双击开关。
    };

    // 接收完整配置并设置三个输入引脚，再用当前读数和 now_ms 初始化内部输入组件。
    void init(const Config& config, uint32_t now_ms);

    // 每轮读取按键；距上次 A/B 相采样至少 1 ms 才再次读取，主循环延迟时不会补采样。
    void update(uint32_t now_ms);

    // 返回并清除自上次调用以来累计的整格数；顺时针为正，逆时针为负。
    int take_detents();

    // 取出一个按键手势；调用方应持续读取，直到返回 ButtonGesture::None。
    ButtonGesture take_button_gesture();

private:
    // 将 A/B 电平合成为两位状态：SIA 是 bit 1，SIB 是 bit 0。
    [[nodiscard]] uint8_t read_state() const;

    uint pin_a_ = 0;                        // init() 指定的模块 SIA 引脚。
    uint pin_b_ = 0;                        // init() 指定的模块 SIB 引脚。
    uint pin_switch_ = 0;                   // init() 指定的模块 SW 引脚，低电平表示按下。

    uint32_t last_encoder_sample_ms_ = 0;   // 上次采样 A/B 相的时刻。
    QuadratureDecoder decoder_;             // 将相位变化解码为机械卡点数。
    DebouncedSwitch switch_;                // 对 SW 电平去抖。
    ButtonGestureDecoder button_gestures_;  // 从稳定按下状态识别手势。

    // A/B 相的最短采样间隔为 1 ms；主循环若延迟，实际间隔可能更长。
    static constexpr uint32_t encoder_sample_interval_ms = 1;

    // 每个卡点按四次格雷码跳变计算；当前模块已实机验证每格动作符合预期。
    static constexpr uint8_t transitions_per_detent = 4;
    static_assert(transitions_per_detent > 0);

    // 内置机械按键的电平变化后需连续稳定 20 ms 才接受新状态。
    static constexpr uint32_t switch_debounce_ms = 20;
};
