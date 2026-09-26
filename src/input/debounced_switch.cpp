#include "input/debounced_switch.h"

// 设置去抖时长并直接接受启动采样，无需让初始状态先等待一个去抖窗口。
void DebouncedSwitch::init(uint32_t debounce_ms, uint32_t now_ms, bool raw_active) {
    debounce_ms_ = debounce_ms;
    stable_active_ = raw_active;
    candidate_active_ = raw_active;
    last_change_ms_ = now_ms;
}

// 只有原始采样连续稳定达到指定时间才更新去抖状态。
void DebouncedSwitch::update(uint32_t now_ms, bool raw_active) {
    if (raw_active != candidate_active_) {
        candidate_active_ = raw_active;
        last_change_ms_ = now_ms;
    }

    if (candidate_active_ != stable_active_ &&
        (now_ms - last_change_ms_) >= debounce_ms_) {
        stable_active_ = candidate_active_;
    }
}

bool DebouncedSwitch::is_active() const {
    return stable_active_;
}
