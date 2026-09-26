#include "device/yfrobot_led_latching_switch.h"

// 保存并配置 SIG 引脚，再将去抖时长、时间基准和初始采样交给去抖组件。
void YfrobotLedLatchingSwitch::init(uint sig_pin, uint32_t now_ms) {
    sig_pin_ = sig_pin;
    gpio_init(sig_pin_);
    gpio_set_dir(sig_pin_, GPIO_IN);
    // 使用已验证的接法，不启用 MCU 内部上下拉。
    gpio_disable_pulls(sig_pin_);

    // 单独供电实测灯灭时 SIG 约 0 V、灯亮时约 3.3 V，直接把高电平作为有效状态。
    sig_.init(sig_debounce_ms, now_ms, gpio_get(sig_pin_));
}

// SIG 高电平为开、低电平为关；读取后交给去抖逻辑。
void YfrobotLedLatchingSwitch::update(uint32_t now_ms) {
    sig_.update(now_ms, gpio_get(sig_pin_));
}

// 返回自锁开关的稳定闭合状态。
bool YfrobotLedLatchingSwitch::is_on() const {
    return sig_.is_active();
}
