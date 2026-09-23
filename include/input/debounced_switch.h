#pragma once

#include <cstdint>

// 将连续的有效/无效采样转换为稳定开关状态；不访问 GPIO，也不识别按键手势。
class DebouncedSwitch {
public:
    explicit DebouncedSwitch(uint32_t debounce_ms);

    // 用启动时的实际状态建立基准，避免产生虚假的状态变化。
    void seed(uint32_t now_ms, bool raw_active);

    // 原始状态持续 debounce_ms 后才会改变公开的稳定状态。
    void update(uint32_t now_ms, bool raw_active);

    [[nodiscard]] bool is_active() const;

private:
    uint32_t debounce_ms_;           // 当前物理开关的去抖窗口。
    bool candidate_active_ = false;  // 最近观察到的原始状态。
    bool stable_active_ = false;     // 对上层公开的稳定状态。
    uint32_t last_change_ms_ = 0;    // 原始状态最近变化的时刻。
};
