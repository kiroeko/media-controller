#pragma once

#include <cstdint>

// 机制层：输入两位正交相位，输出带符号的整格计数。
// 本通道自行管理采样节奏。
class QuadratureChannel {
public:
    // 用旋钮当前相位建立基准，避免把启动时的实际位置误认为从零开始的转动；
    // 同时预置采样时间，使下一次 update() 可以立即采样。
    void seed(uint32_t now_ms, uint8_t state);

    // 输入新的两位相位读数。比采样间隔更频繁的调用会被忽略，
    // 因此每格计数对应的是已接收的相位变化，而非所有电气抖动。
    void update(uint32_t now_ms, uint8_t state);

    // 返回当前累计的带符号整格数，并清零累计值。相反方向的转动会在这里抵消。
    int take_turns();

private:
    // 在格雷码转换表中查找 (previous_state_, state)，并把累计的四分之一格换算成整格。
    void accumulate(uint8_t state);

    uint8_t previous_state_ = 0;    // 最近一次被接受采样的相位组合。
    uint32_t last_sample_ms_ = 0;   // 最近一次通过采样节流的时刻，而非最近一次函数调用时刻。
    int8_t accumulator_ = 0;        // 尚未凑成整格的四分之一格计数；每次 accumulate() 后绝对值小于一格。
    int8_t pending_turns_ = 0;      // 等待 take_turns() 取走的整格数；达到 int8_t 上限时饱和，避免溢出反向跳变。
};
