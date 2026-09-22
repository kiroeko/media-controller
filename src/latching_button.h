#pragma once

#include <cstdint>

#include "pico/stdlib.h"

#include "debounced_input.h"

// The YFROBOT LED latching button module: a self-latching switch whose SIG pin
// follows the latch, with the LED driven by the module itself.
class LatchingButton {
public:
    explicit LatchingButton(uint sig_pin);

    void init(uint32_t now_ms);
    void update(uint32_t now_ms);

    [[nodiscard]] bool is_on() const;

private:
    DebouncedInput sig_;
};
