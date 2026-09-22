#pragma once

#include <cstdint>

// Mechanism layer: raw 2-bit quadrature phase in, signed whole-detent count out.
// Owns its sampling cadence, like every channel does.
class QuadratureChannel {
public:
    void seed(uint32_t now_ms, uint8_t state);
    void update(uint32_t now_ms, uint8_t state);
    int take_turns();

private:
    void accumulate(uint8_t state);

    uint8_t previous_state_ = 0;
    uint32_t last_sample_ms_ = 0;
    int8_t accumulator_ = 0;
    int8_t pending_turns_ = 0;
};
