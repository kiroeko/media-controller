#include "input/quadrature_decoder.h"

namespace {
// 这是“旧相位 → 新相位”的查表，不是把格雷码顺序直接写成数组。
// 状态编码约定为 0bAB：A 相电平放在 bit 1，B 相电平放在 bit 0。
// 因此 A、B 都低是 0b00，只有 B 高是 0b01，只有 A 高是 0b10，A、B 都高是 0b11。
// 表格的行表示旧状态，列表示新状态，行和列的排列顺序都是 00、01、10、11，
// 所以可以直接写成 kTransitionDelta[旧状态][新状态]。
// 格雷码相邻状态每次只改变一位；沿 00 → 01 → 11 → 10 → 00 这一方向记 -1，
// 反向记 +1。方向正负只是本解码器的约定，实际顺/逆时针还取决于 A/B 接线。
// 相同状态表示没有变化；两位同时变化（如 00 → 11）不是合法相邻步，记 0。
constexpr int8_t kTransitionDelta[4][4] = {
    // 旧状态 00；四列依次表示新状态 00、01、10、11。
    {  0, -1,  1,  0 },
    // 旧状态 01；四列依次表示新状态 00、01、10、11。
    {  1,  0,  0, -1 },
    // 旧状态 10；四列依次表示新状态 00、01、10、11。
    { -1,  0,  0,  1 },
    // 旧状态 11；四列依次表示新状态 00、01、10、11。
    {  0,  1, -1,  0 },
};
}  // 匿名命名空间

// 保存由物理器件给出的每格跳变数；传入 0 时使用最小值 1，避免静止时产生虚假卡点。
// 解码器本身不认识具体 GPIO 或模块。
QuadratureDecoder::QuadratureDecoder(uint8_t transitions_per_detent)
    : transitions_per_detent_(transitions_per_detent == 0 ? 1 : transitions_per_detent) {}

// 初始化相位基准及累计状态。
void QuadratureDecoder::seed(uint8_t state) {
    previous_state_ = static_cast<uint8_t>(state & 0b11U);
    accumulator_ = 0;
    pending_detents_ = 0;
}

// 用上次状态选表格的行、用新采样选列；表格值是本次变化的方向增量（-1、+1 或 0）。
// 把这个增量累计起来，凑够一个卡点所需的变化数后，才产生一个带符号的卡点。
// 例如 00 → 01 → 11 → 10 → 00 每次查表都是 -1，累计 -4 后形成一个负向卡点。
// 若采样跳过一个中间相位而出现两位同时变化，该次增量为 0；随后仍以新状态为基准。
void QuadratureDecoder::update(uint8_t state) {
    state = static_cast<uint8_t>(state & 0b11U);
    accumulator_ += kTransitionDelta[previous_state_][state];
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
