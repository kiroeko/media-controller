#pragma once

#include <cstdint>

#include "pico/stdlib.h"

enum class InputPull : uint8_t { None, Up, Down };

enum class SwitchGesture : uint8_t { None, Short, Long, Double };

class SwitchChannel {
public:
    // The debounce window is electrical and stays ours; what counts as a
    // gesture is a product decision, so the caller sets the thresholds.
    // `double_gap_ms` is only consulted when `detect_double` is true.
    SwitchChannel(uint gpio, bool active_high, InputPull pull, bool detect_double = false,
                  uint32_t long_press_ms = kLongPressMs, uint32_t double_gap_ms = kDoubleGapMs);

    void init(uint32_t now_ms);
    void update(uint32_t now_ms);

    [[nodiscard]] bool is_active() const;
    SwitchGesture take_gesture();

private:
    [[nodiscard]] bool read_active() const;

    static constexpr uint32_t kDebounceMs = 20;
    static constexpr uint32_t kLongPressMs = 700;
    static constexpr uint32_t kDoubleGapMs = 300;

    uint gpio_;
    bool active_high_;
    InputPull pull_;
    bool detect_double_;
    uint32_t long_press_ms_;
    uint32_t double_gap_ms_;
    bool candidate_active_ = false;
    bool stable_active_ = false;
    bool was_active_ = false;
    uint32_t last_raw_change_ms_ = 0;
    uint32_t press_start_ms_ = 0;
    uint32_t pending_short_at_ms_ = 0;
    bool long_fired_ = false;
    bool suppress_short_ = false;
    bool short_pressed_ = false;
    bool long_pressed_ = false;
    bool double_pressed_ = false;
};
