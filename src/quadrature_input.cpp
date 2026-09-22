#include "quadrature_input.h"

namespace {
// Gray-code transitions per detent = 4 × pulses per revolution ÷ detents per revolution; the Waveshare module is 20 pulses/rev with detents unmarked, so measure before trusting this value.
constexpr int8_t kAccumulatorPerDetent = 4;
}  // namespace

void QuadratureInput::seed(uint8_t state) {
    previous_state_ = state;
}

void QuadratureInput::feed(uint8_t state) {
    // Valid quadrature transitions are one Gray-code step apart.
    static constexpr int8_t kTransitionDelta[16] = {
        0, -1, 1, 0,
        1, 0, 0, -1,
        -1, 0, 0, 1,
        0, 1, -1, 0,
    };

    accumulator_ += kTransitionDelta[(previous_state_ << 2U) | state];
    previous_state_ = state;

    if (accumulator_ >= kAccumulatorPerDetent) {
        ++pending_turns_;
        accumulator_ -= kAccumulatorPerDetent;
    } else if (accumulator_ <= -kAccumulatorPerDetent) {
        --pending_turns_;
        accumulator_ += kAccumulatorPerDetent;
    }
}

int QuadratureInput::take_turns() {
    const int result = pending_turns_;
    pending_turns_ = 0;
    return result;
}
