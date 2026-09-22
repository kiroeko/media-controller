#pragma once

#include <cstdint>

#include "pico/stdlib.h"

#include "channel/switch_channel.h"
#include "channel/quadrature_channel.h"

// Models the whole Waveshare Rotation Sensor module, not just its encoder: the
// EC11 quadrature phases and the built-in push button share one connector and
// one ground, so they are one device with two channels.
class RotationDevice {
public:
    // Which board pin each module line lands on is the assembly layer's
    // decision; this type only knows the roles.
    RotationDevice(uint pin_a, uint pin_b, uint pin_switch);

    // Set both phase pins to input, then align both channels to the level
    // present at that instant.
    void init(uint32_t now_ms);

    // Advance both channels once. The order is irrelevant: neither channel's
    // state depends on the other, and each gates its own cadence.
    void update(uint32_t now_ms);

    // Signed whole detents since the last call; drains the encoder only.
    int take_turns();

    // Pop one button gesture; drain to SwitchGesture::None.
    SwitchGesture take_switch_gesture();

private:
    // Sample both phases into one 2-bit Gray code: SIA is bit 1, SIB bit 0.
    // Swapping the two pins is what reverses reported direction.
    [[nodiscard]] uint8_t read_state() const;

    uint pin_a_;                 // Module SIA; read here, since the decoder
                                 // takes phases rather than pins.
    uint pin_b_;                 // Module SIB; same reason.
    QuadratureChannel decoder_;  // Phases -> detents.
    SwitchChannel switch_;       // Owns SW, and so owns its electrical config.
};
