#include "device/rotation_device.h"

// 保存编码器 A/B 相和按键引脚，并配置按键为低电平有效、无内部上下拉。
RotationDevice::RotationDevice(uint pin_a, uint pin_b, uint pin_switch)
    : pin_a_(pin_a), pin_b_(pin_b), switch_(pin_switch, false, InputPull::None) {}

// 初始化 A/B 相输入，读取当前相位作为解码起点，再初始化内置按键通道。
void RotationDevice::init(uint32_t now_ms) {
    // 模块板上已有上拉；厂商示例不配置 MCU 内部上拉，且波形空闲时为高电平，故这里也不叠加内部上拉。
    gpio_init(pin_a_);
    gpio_set_dir(pin_a_, GPIO_IN);

    gpio_init(pin_b_);
    gpio_set_dir(pin_b_, GPIO_IN);

    decoder_.seed(now_ms, read_state());
    switch_.init(now_ms);
}

// 更新按键和编码器通道；两个通道各自负责去抖或采样节流。
void RotationDevice::update(uint32_t now_ms) {
    // 通道自行控制采样节奏，器件层只负责推进状态。
    switch_.update(now_ms);
    decoder_.update(now_ms, read_state());
}

// 取走并清零编码器自上次读取以来累计的整格数。
int RotationDevice::take_turns() {
    return decoder_.take_turns();
}

// 取出编码器内置按键的一个手势事件。
SwitchGesture RotationDevice::take_switch_gesture() {
    return switch_.take_gesture();
}

// 读取 SIA/SIB 并编码成两位相位状态，供正交解码器处理。
uint8_t RotationDevice::read_state() const {
    return static_cast<uint8_t>((gpio_get(pin_a_) ? 0b10 : 0) |
                                (gpio_get(pin_b_) ? 0b01 : 0));
}
