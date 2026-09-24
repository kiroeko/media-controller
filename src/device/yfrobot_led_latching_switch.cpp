#include "device/yfrobot_led_latching_switch.h"

namespace {
// 当前按 SIG 高电平表示自锁开关闭合、LED 点亮处理；实际极性仍需上板核对。
constexpr bool kSigActiveHigh = true;

// 自锁开关的原始电平需持续 20 ms 才作为稳定状态使用。
constexpr uint32_t kSigDebounceMs = 20;

// 当前不启用内部下拉；若实测释放时 SIG 悬空，应改为 true。
constexpr bool kUseInternalPullDown = false;
}  // 匿名命名空间

// 保存模式模块的 SIG 引脚；构造时不访问硬件。
YfrobotLedLatchingSwitch::YfrobotLedLatchingSwitch(uint sig_pin)
    : sig_pin_(sig_pin), sig_(kSigDebounceMs) {}

// 配置 SIG 输入，并用当前电平建立去抖基准。
void YfrobotLedLatchingSwitch::init(uint32_t now_ms) {
    gpio_init(sig_pin_);
    gpio_set_dir(sig_pin_, GPIO_IN);
    if (kUseInternalPullDown) {
        gpio_pull_down(sig_pin_);
    } else {
        gpio_disable_pulls(sig_pin_);
    }
    sig_.seed(now_ms, read_active());
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
