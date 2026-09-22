#include "channel/quadrature_channel.h"

namespace {
// Gray-code transitions per detent = 4 × pulses per revolution ÷ detents per revolution; the Waveshare module is 20 pulses/rev with detents unmarked, so measure before trusting this value.
constexpr int8_t kAccumulatorPerDetent = 4;

// One sample per millisecond: mechanical bounce settles well inside that, and
// missing a turn would need two Gray steps within a window (~25 rev/s by hand).
constexpr int32_t kSampleIntervalMs = 1;
}  // namespace

void QuadratureChannel::seed(uint32_t now_ms, uint8_t state) {
    previous_state_ = state;
    last_sample_ms_ = now_ms - 1;
}

void QuadratureChannel::update(uint32_t now_ms, uint8_t state) {
    if (static_cast<int32_t>(now_ms - last_sample_ms_) < kSampleIntervalMs) {
        return;
    }
    last_sample_ms_ = now_ms;
    accumulate(state);
}

void QuadratureChannel::accumulate(uint8_t state) {
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
        // Clamped, not wrapped: a rolled-over count would come back as a burst of
        // steps in the opposite direction. The detent is consumed either way, so
        // accumulator_ stays bounded even while nobody drains.
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

int QuadratureChannel::take_turns() {
    const int result = pending_turns_;
    pending_turns_ = 0;
    return result;
}
