#pragma once

#include <cstdint>

#include "pico/stdlib.h"

#include "switch_channel.h"
#include "quadrature_channel.h"

// Models the whole Waveshare Rotation Sensor module, not just its encoder: the
// EC11 quadrature phases and the built-in push button share one connector and
// one ground, so they are one device with two channels.
class RotationSensor {
public:
    RotationSensor(uint pin_a, uint pin_b, uint pin_switch);

    void init(uint32_t now_ms);
    void update(uint32_t now_ms);

    int take_turns();
    bool take_switch_pressed();

private:
    [[nodiscard]] uint8_t read_state() const;

    uint pin_a_;
    uint pin_b_;
    QuadratureChannel decoder_;
    SwitchChannel switch_;
};
