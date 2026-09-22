#pragma once

#include <cstdint>

// Mechanism layer: raw 2-bit quadrature phase in, signed whole-detent count out.
class QuadratureInput {
public:
    void seed(uint8_t state);
    void feed(uint8_t state);
    int take_turns();

private:
    uint8_t previous_state_ = 0;
    int8_t accumulator_ = 0;
    int8_t pending_turns_ = 0;
};
