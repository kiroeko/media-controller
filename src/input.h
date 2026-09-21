#pragma once

#include <cstdint>

#include "pico/stdlib.h"

enum class InputPull : uint8_t { None, Up, Down };

class DebouncedInput {
public:
    DebouncedInput(uint gpio, bool active_high, InputPull pull);

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

class QuadratureEncoder {
public:
    QuadratureEncoder(uint pin_a, uint pin_b, uint pin_switch);

    void init(uint32_t now_ms);
    void update(uint32_t now_ms);

    int take_turns();
    bool take_switch_pressed();

private:
    [[nodiscard]] uint8_t read_state() const;

    uint pin_a_;
    uint pin_b_;
    uint8_t previous_state_ = 0;
    uint32_t last_sample_ms_ = 0;
    int8_t quadrature_accumulator_ = 0;
    int8_t pending_turns_ = 0;
    DebouncedInput switch_;
};

