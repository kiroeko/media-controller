#pragma once

#include <cstdint>

// 将连续的有效/无效采样转换为稳定开关状态；不访问 GPIO，也不识别按键手势。
class DebouncedSwitch {
public:
    // debounce_ms 是原始状态必须连续保持、才能被接受的毫秒数。
    explicit DebouncedSwitch(uint32_t debounce_ms);

    // 用启动时的采样同时设置候选状态和稳定状态，无需先等待一个去抖窗口。
    void seed(uint32_t now_ms, bool raw_active);

    // 原始状态一变化就重新计时；持续 debounce_ms 后才更新公开的稳定状态。
    void update(uint32_t now_ms, bool raw_active);

    // 返回最后一次确认的稳定状态，不直接反映尚在去抖窗口内的原始变化。
    [[nodiscard]] bool is_active() const;

private:
    bool stable_active_ = false;     // 对上层公开的稳定状态。

    bool candidate_active_ = false;  // 最近观察到的原始状态。
    uint32_t last_change_ms_ = 0;    // 候选状态最近变化的时刻。

    uint32_t debounce_ms_;           // 候选状态被接受前需保持不变的毫秒数。
};
