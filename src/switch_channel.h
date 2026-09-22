#pragma once

#include <cstdint>

#include "pico/stdlib.h"

enum class InputPull : uint8_t { None, Up, Down };

class SwitchChannel {
public:
    SwitchChannel(uint gpio, bool active_high, InputPull pull);

    void init(uint32_t now_ms);
    void update(uint32_t now_ms);

    [[nodiscard]] bool is_active() const;
    bool take_activated();

private:
    [[nodiscard]] bool read_active() const;

    static constexpr uint32_t kDebounceMs = 20;

    uint gpio_;
    bool active_high_;
    InputPull pull_;
    bool candidate_active_ = false;
    bool stable_active_ = false;
    bool activated_ = false;
    uint32_t last_raw_change_ms_ = 0;
};
