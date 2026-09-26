#include "device/yfrobot_led_latching_switch.h"

namespace {
// 单独供电实测灯灭时 SIG 约 0 V、灯亮时约 3.3 V，按高电平有效处理。
constexpr bool kSigActiveHigh = true;

// 自锁开关的原始电平需持续 20 ms 才作为稳定状态使用。
constexpr uint32_t kSigDebounceMs = 20;

// 单独供电实测灯灭时 SIG 稳定约 0 V，当前不启用内部下拉。
constexpr bool kUseInternalPullDown = false;
}  // 匿名命名空间

// 保存并配置 SIG 引脚，再将去抖时长、时间基准和初始采样交给去抖组件。
void YfrobotLedLatchingSwitch::init(uint sig_pin, uint32_t now_ms) {
    sig_pin_ = sig_pin;
    gpio_init(sig_pin_);
    gpio_set_dir(sig_pin_, GPIO_IN);
    if (kUseInternalPullDown) {
        gpio_pull_down(sig_pin_);
    } else {
        gpio_disable_pulls(sig_pin_);
    }
    sig_.init(kSigDebounceMs, now_ms, read_active());
}

// 读取 SIG，再将有效/无效状态交给纯去抖逻辑。
void YfrobotLedLatchingSwitch::update(uint32_t now_ms) {
    sig_.update(now_ms, read_active());
}

// 返回自锁开关的稳定闭合状态。
bool YfrobotLedLatchingSwitch::is_on() const {
    return sig_.is_active();
}

// 将物理高低电平转换成与极性无关的有效状态。
bool YfrobotLedLatchingSwitch::read_active() const {
    const bool pin_is_high = gpio_get(sig_pin_);
    return kSigActiveHigh ? pin_is_high : !pin_is_high;
}
