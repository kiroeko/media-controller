#include "device/waveshare_rotation_sensor.h"

namespace {
// 此模块手动旋转时每毫秒读取一次 A/B 相；时间闸门放在 GPIO 所在的器件层。
constexpr uint32_t kEncoderSampleIntervalMs = 1;

// 当前模块按每个卡点四次格雷码跳变计算；需上板实测确认。
constexpr uint8_t kTransitionsPerDetent = 4;
static_assert(kTransitionsPerDetent > 0);

// 模块的机械按键需持续稳定 20 ms 才认定为按下或松开。
constexpr uint32_t kSwitchDebounceMs = 20;
}  // 匿名命名空间

// 保存模块引脚及应用指定的手势阈值；构造时不访问硬件。
WaveshareRotationSensor::WaveshareRotationSensor(uint pin_a, uint pin_b, uint pin_switch,
                                                 ButtonGestureConfig gesture_config)
    : pin_a_(pin_a)
    , pin_b_(pin_b)
    , pin_switch_(pin_switch)
    , decoder_(kTransitionsPerDetent)
    , switch_(kSwitchDebounceMs)
    , button_gestures_(gesture_config) {}

// 配置模块的三个输入脚，用上电时的读数作为解码起点。
void WaveshareRotationSensor::init(uint32_t now_ms) {
    // 模块自带上拉，不叠加 MCU 内部上下拉。
    gpio_init(pin_a_);
    gpio_set_dir(pin_a_, GPIO_IN);
    gpio_disable_pulls(pin_a_);

    gpio_init(pin_b_);
    gpio_set_dir(pin_b_, GPIO_IN);
    gpio_disable_pulls(pin_b_);

    gpio_init(pin_switch_);
    gpio_set_dir(pin_switch_, GPIO_IN);
    gpio_disable_pulls(pin_switch_);

    decoder_.seed(read_state());
    last_encoder_sample_ms_ = now_ms - kEncoderSampleIntervalMs;
    switch_.seed(now_ms, !gpio_get(pin_switch_));
    button_gestures_.seed(now_ms, switch_.is_active());
}

// 读取按键并按 1 ms 节奏采样 A/B 相，再交给纯输入处理类。
void WaveshareRotationSensor::update(uint32_t now_ms) {
    switch_.update(now_ms, !gpio_get(pin_switch_));
    button_gestures_.update(now_ms, switch_.is_active());

    if (static_cast<uint32_t>(now_ms - last_encoder_sample_ms_) >=
        kEncoderSampleIntervalMs) {
        last_encoder_sample_ms_ = now_ms;
        decoder_.update(read_state());
    }
}

// 取走并清零编码器自上次读取以来累计的整格数。
int WaveshareRotationSensor::take_detents() {
    return decoder_.take_detents();
}

// 取出编码器内置按键的一个手势事件。
ButtonGesture WaveshareRotationSensor::take_button_gesture() {
    return button_gestures_.take_gesture();
}

// 读取 SIA/SIB 并编码成两位相位状态；交换 A/B 接线会反转计数方向。
uint8_t WaveshareRotationSensor::read_state() const {
    return static_cast<uint8_t>((gpio_get(pin_a_) ? 0b10 : 0) |
                                (gpio_get(pin_b_) ? 0b01 : 0));
}
