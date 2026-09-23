#include "channel/quadrature_channel.h"

namespace {
// 每格格雷码跳变数 = 每圈脉冲数 × 4 ÷ 每圈格数。Waveshare 模块标称每圈 20 个脉冲，
// 但未标出格数，因此此值应以实测为准。
constexpr int8_t kAccumulatorPerDetent = 4;

// 每毫秒采样一次：机械抖动会在采样间隔内稳定；手动旋转要快到约 25 圈/秒，
// 才可能在一个采样窗口内漏掉两个格雷码跳变。
constexpr int32_t kSampleIntervalMs = 1;
}  // 匿名命名空间

// 以当前两相状态建立解码基准，并让下一次 update() 可以立即采样。
void QuadratureChannel::seed(uint32_t now_ms, uint8_t state) {
    previous_state_ = state;
    last_sample_ms_ = now_ms - 1;
}

// 按采样间隔节流相位输入，再交给 Gray 码累积器解码。
void QuadratureChannel::update(uint32_t now_ms, uint8_t state) {
    if (static_cast<int32_t>(now_ms - last_sample_ms_) < kSampleIntervalMs) {
        return;
    }
    last_sample_ms_ = now_ms;
    accumulate(state);
}

// 用相邻采样状态查表得到旋转方向，累计四分之一格并在达到整格时记数。
void QuadratureChannel::accumulate(uint8_t state) {
    // 合法的正交相位变化每次只跨越一个格雷码状态。
    static constexpr int8_t kTransitionDelta[16] = {
        0, -1, 1, 0,
        1, 0, 0, -1,
        -1, 0, 0, 1,
        0, 1, -1, 0,
    };

    accumulator_ += kTransitionDelta[(previous_state_ << 2U) | state];
    previous_state_ = state;

    if (accumulator_ >= kAccumulatorPerDetent) {
        // 计数饱和而不回绕，否则溢出后会反向产生一串虚假格数。
        // 无论待取计数是否已饱和，都消费本次整格，保持 accumulator_ 有界。
        if (pending_turns_ < INT8_MAX) {
            ++pending_turns_;
        }
        accumulator_ -= kAccumulatorPerDetent;
    } else if (accumulator_ <= -kAccumulatorPerDetent) {
        if (pending_turns_ > INT8_MIN) {
            --pending_turns_;
        }
        accumulator_ += kAccumulatorPerDetent;
    }
}

// 返回并清零待处理格数；相反方向的格数此前已在累积过程中抵消。
int QuadratureChannel::take_turns() {
    const int result = pending_turns_;
    pending_turns_ = 0;
    return result;
}
