#pragma once

#include <cstdint>

// 将已经采样的 A/B 两相状态解码为带符号的机械卡点数。
// 本类型不访问 GPIO；调用方负责按合适的间隔提供两位状态。
// 相位沿 00 → 01 → 11 → 10 → 00 变化时记正数，反向记负数。
class QuadratureDecoder {
public:
    // 每个机械卡点对应的相位跳变数由实际编码器决定；传入 0 时按最小值 1 处理。
    explicit QuadratureDecoder(uint8_t transitions_per_detent);

    // 用当前相位建立基准，避免把上电时的电平当成一次旋转。
    void seed(uint8_t state);

    // 输入一次两位相位采样，状态编码为 0bAB：A 在 bit 1，B 在 bit 0。
    // 例如 A 低、B 高传入 0b01；A 高、B 低传入 0b10。
    void update(uint8_t state);

    // 取走并清零已累计的整格数；未凑成整格的相位变化继续保留。
    int take_detents();

private:
    int8_t pending_detents_ = 0;    // 等待上层取走的整格数，达到边界时饱和。

    int16_t accumulator_ = 0;       // 尚未凑成一格的相位跳变数。
    uint8_t previous_state_ = 0;    // 上一次采样的两位相位状态。

    uint8_t transitions_per_detent_;  // 一格所需的有效相位跳变数。
};
