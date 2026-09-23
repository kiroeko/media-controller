#include "input/quadrature_decoder.h"

namespace {
// 行是旧状态，列是新状态。只改变一位的相邻状态记为正向或反向一步。
constexpr int8_t kTransitionDelta[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0,
};
}  // 匿名命名空间

// 保存由物理器件给出的每格跳变数；解码器本身不认识具体 GPIO 或模块。
QuadratureDecoder::QuadratureDecoder(uint8_t transitions_per_detent)
    : transitions_per_detent_(transitions_per_detent) {}

// 初始化相位基准及累计状态。
void QuadratureDecoder::seed(uint8_t state) {
    previous_state_ = static_cast<uint8_t>(state & 0b11U);
    accumulator_ = 0;
    pending_detents_ = 0;
}

// 查表确定方向，累计相位跳变；跳过中间相位的非法变化记为零。
void QuadratureDecoder::update(uint8_t state) {
    state = static_cast<uint8_t>(state & 0b11U);
    accumulator_ += kTransitionDelta[(previous_state_ << 2U) | state];
    previous_state_ = state;

    if (accumulator_ >= transitions_per_detent_) {
        // 待取计数饱和后仍消费本次整格，避免累计器持续增长。
        if (pending_detents_ < INT8_MAX) {
            ++pending_detents_;
        }
        accumulator_ -= transitions_per_detent_;
    } else if (accumulator_ <= -static_cast<int16_t>(transitions_per_detent_)) {
        if (pending_detents_ > INT8_MIN) {
            --pending_detents_;
        }
        accumulator_ += transitions_per_detent_;
    }
}

// 返回并清零整格数；正反方向的格数已在累计过程中抵消。
int QuadratureDecoder::take_detents() {
    const int result = pending_detents_;
    pending_detents_ = 0;
    return result;
}
