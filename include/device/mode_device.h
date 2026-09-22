#pragma once

#include <cstdint>

#include "pico/stdlib.h"

#include "channel/switch_channel.h"

// The YFROBOT LED latching button module: a self-latching switch whose SIG pin
// follows the latch, with the LED driven by the module itself.
class ModeDevice {
public:
    // Only SIG is a firmware concern; the module drives its own LED.
    explicit ModeDevice(uint sig_pin);

    // Configure SIG and align its debounce state to the level at boot. A
    // re-flash does not unlatch the switch, so the starting mode belongs to the
    // user, not to us.
    void init(uint32_t now_ms);

    // Advance the debounce on SIG; there is nothing else on this module.
    void update(uint32_t now_ms);

    // Whether the latch is engaged, i.e. track mode. A level, not an edge, so
    // it can be polled as often as the caller likes and never loses state.
    [[nodiscard]] bool is_on() const;

private:
    SwitchChannel sig_;  // Its gesture events are deliberately left undrained.
};
