#include "input/debounced_switch.h"

DebouncedSwitch::DebouncedSwitch(uint32_t debounce_ms)
    : debounce_ms_(debounce_ms) {}

// 启动时直接接受当前状态，不把它当作新一次按下或松开。
void DebouncedSwitch::seed(uint32_t now_ms, bool raw_active) {
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
