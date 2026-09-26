#include "device/waveshare_rotation_sensor.h"

namespace {
// 当前 A/B 相的最短采样间隔为 1 ms；主循环若延迟，实际间隔可能更长。
constexpr uint32_t kEncoderSampleIntervalMs = 1;

// 当前模块按每个卡点四次格雷码跳变计算；需上板实测确认。
constexpr uint8_t kTransitionsPerDetent = 4;
static_assert(kTransitionsPerDetent > 0);

// 模块的机械按键需持续稳定 20 ms 才认定为按下或松开。
constexpr uint32_t kSwitchDebounceMs = 20;
}  // 匿名命名空间

// 保存接线、配置 GPIO，再将完整配置和初始读数交给内部输入组件。
void WaveshareRotationSensor::init(const Config& config, uint32_t now_ms) {
    pin_a_ = config.pin_a;
    pin_b_ = config.pin_b;
    pin_switch_ = config.pin_switch;

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

    decoder_.init(kTransitionsPerDetent, read_state());
    last_encoder_sample_ms_ = now_ms - kEncoderSampleIntervalMs;
    switch_.init(kSwitchDebounceMs, now_ms, !gpio_get(pin_switch_));
    button_gestures_.init(config.gesture_config, now_ms, switch_.is_active());
}

// 每轮读取按键；经过至少 1 ms 后读取一次 A/B 相，再交给输入解码器。
void WaveshareRotationSensor::update(uint32_t now_ms) {
    switch_.update(now_ms, !gpio_get(pin_switch_));
    button_gestures_.update(now_ms, switch_.is_active());

    if (now_ms - last_encoder_sample_ms_ >= kEncoderSampleIntervalMs) {
        last_encoder_sample_ms_ = now_ms;
        decoder_.update(read_state());
    }
}

// 取走并清零整格数；顺时针为正，逆时针为负。
int WaveshareRotationSensor::take_detents() {
    return decoder_.take_detents();
}

// 取出编码器内置按键的一个手势事件。
ButtonGesture WaveshareRotationSensor::take_button_gesture() {
    return button_gestures_.take_gesture();
}

// 读取 SIA/SIB 并编码成约定的两位相位状态。
uint8_t WaveshareRotationSensor::read_state() const {
    return static_cast<uint8_t>((gpio_get(pin_a_) ? 0b10 : 0) |
                                (gpio_get(pin_b_) ? 0b01 : 0));
}
