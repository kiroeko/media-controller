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

    // Configures the pin, then aligns every state field to the level found at
    // that moment so startup cannot register as a press or a release.
    void init(uint32_t now_ms);

    // Advance the debounce window and the gesture timers by one reading.
    // Must be called at an interval well below kDebounceMs: a press and its
    // release that both fall between two calls are invisible, not merged.
    void update(uint32_t now_ms);

    // Debounced level, not the raw pin. This is the only view that is safe to
    // act on; read_active() is the unfiltered one.
    [[nodiscard]] bool is_active() const;

    // Pop one pending gesture, heaviest first (Long, then Double, then Short).
    // Events are held until taken, so the caller must drain to None.
    SwitchGesture take_gesture();

private:
    // Raw pin level mapped through active_high_. No debouncing, no timing.
    [[nodiscard]] bool read_active() const;

    // Raw input must stay unchanged for this long before becoming stable.
    static constexpr uint32_t kDebounceMs = 20;
    // Default hold time required to emit Long.
    static constexpr uint32_t kLongPressMs = 700;
    // Default maximum delay between presses that may form Double.
    static constexpr uint32_t kDoubleGapMs = 300;

    uint gpio_;                         // SDK GPIO number read by this channel.
    bool active_high_;                  // Whether a high pin level is the active level.
    InputPull pull_;                    // Internal pull configured during init().
    bool detect_double_;                // Whether Short waits for a second press.
    uint32_t long_press_ms_;            // Hold time required to emit Long.
    uint32_t double_gap_ms_;            // Maximum delay allowed between two presses.
    bool candidate_active_ = false;     // Most recently observed raw active state.
    bool stable_active_ = false;        // Debounced active state exposed to callers.
    bool was_active_ = false;           // Stable state from the previous update.
    uint32_t last_raw_change_ms_ = 0;   // Time when candidate_active_ last changed.
    uint32_t press_start_ms_ = 0;       // Time when the current press became stable.
    uint32_t pending_short_at_ms_ = 0;  // Short deadline; zero means none is pending.
    bool long_fired_ = false;           // Long was emitted for the current press.
    bool suppress_short_ = false;       // Current release belongs to a Double.
    bool short_pressed_ = false;        // Unconsumed Short event.
    bool long_pressed_ = false;         // Unconsumed Long event.
    bool double_pressed_ = false;       // Unconsumed Double event.
};
