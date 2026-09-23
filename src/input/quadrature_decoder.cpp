#include "input/quadrature_decoder.h"

namespace {
// 这是“旧相位 → 新相位”的查表，不是把格雷码顺序直接写成数组。
// A/B 两位状态分别是 00、01、10、11；数组按 4 行 × 4 列展开：
//   行 = 旧状态，列 = 新状态，行和列的顺序都是 00、01、10、11。
// 索引由 (旧状态 << 2) | 新状态 得到，也就是旧状态占高两位、新状态占低两位。
// 格雷码相邻状态每次只改变一位；沿 00 → 01 → 11 → 10 → 00 这一方向记 -1，
// 反向记 +1。方向正负只是本解码器的约定，实际顺/逆时针还取决于 A/B 接线。
// 相同状态表示没有变化；两位同时变化（如 00 → 11）不是合法相邻步，记 0。
constexpr int8_t kTransitionDelta[16] = {
    // 旧状态 00 → 新状态 00、01、10、11。
     0, -1,  1,  0,
    // 旧状态 01 → 新状态 00、01、10、11。
     1,  0,  0, -1,
    // 旧状态 10 → 新状态 00、01、10、11。
    -1,  0,  0,  1,
    // 旧状态 11 → 新状态 00、01、10、11。
     0,  1, -1,  0,
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

// 把新采样与上次状态组成查表索引：旧状态是高两位，新状态是低两位。
// 查表结果表示这一次变化带来的方向增量（-1、+1 或 0），再累计到未完成的卡点中。
// 例如 00 → 01 → 11 → 10 → 00 每次查表都是 -1，累计 -4 后形成一个负向卡点。
// 若采样跳过一个中间相位而出现两位同时变化，该次增量为 0；随后仍以新状态为基准。
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
