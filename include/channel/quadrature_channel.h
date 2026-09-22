#pragma once

#include <cstdint>

// Mechanism layer: raw 2-bit quadrature phase in, signed whole-detent count out.
// Owns its sampling cadence, like every channel does.
class QuadratureChannel {
public:
    // Establish the reference state, so the first real transition is measured
    // against the knob's actual position instead of a reset zero. Also primes
    // the throttle so the next update() is allowed to sample.
    void seed(uint32_t now_ms, uint8_t state);

    // Feed a fresh 2-bit phase reading. Calls arriving faster than the sample
    // interval are dropped, which is what makes the detent constant meaningful:
    // it counts accepted transitions, not electrical ones.
    void update(uint32_t now_ms, uint8_t state);

    // Signed whole detents accumulated so far, reset to zero by this call.
    // Opposite turns net out here, so the caller never sees a doubled step.
    int take_turns();

private:
    // Look up (previous_state_, state) in the Gray-code table and convert
    // accumulated quarter-steps into whole detents.
    void accumulate(uint8_t state);

    uint8_t previous_state_ = 0;    // Phase pair of the last accepted sample.
    uint32_t last_sample_ms_ = 0;   // Time of the last sample that passed the
                                    // throttle, not of the most recent call.
    int8_t accumulator_ = 0;        // Quarter-steps that have not yet made a
                                    // whole detent; smaller in magnitude than
                                    // one detent after every accumulate().
    int8_t pending_turns_ = 0;      // Detents awaiting take_turns(). Signed
                                    // 8-bit, so a caller that stops draining
                                    // would overflow rather than saturate.
};
