#include "device/mode_device.h"

namespace {
// 按模块厂商定义，自锁开关闭合时 SIG 为高电平，LED 点亮。
constexpr bool kSigActiveHigh = true;

// 当前按模块自行定义释放电平处理，因此不启用内部上下拉；
// 若实测释放时 SIG 悬空，应改为 InputPull::Down。
constexpr InputPull kSigPull = InputPull::None;
}  // 匿名命名空间

// 将模式模块的 SIG 引脚配置交给开关通道管理。
ModeDevice::ModeDevice(uint sig_pin)
    : sig_(sig_pin, kSigActiveHigh, kSigPull) {}

// 初始化 SIG 输入及其去抖状态。
void ModeDevice::init(uint32_t now_ms) {
    sig_.init(now_ms);
}

// 更新 SIG 的去抖状态。
void ModeDevice::update(uint32_t now_ms) {
    sig_.update(now_ms);
}

// 返回自锁开关是否闭合；闭合时表示切歌模式。
bool ModeDevice::is_on() const {
    return sig_.is_active();
}
