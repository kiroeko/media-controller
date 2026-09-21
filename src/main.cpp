#include "pico/stdlib.h"

#include "bsp/board_api.h"

#include "input.h"
#include "media_hid.h"

namespace {

// RP2350-Zero-M header pin labels are GP2, GP3, GP4 and GP5.
constexpr uint kModeSwitchPin = 2;
constexpr uint kEncoderClkPin = 3;
constexpr uint kEncoderDtPin = 4;
constexpr uint kEncoderSwitchPin = 5;
constexpr bool kModeSwitchActiveHigh = true;

// Set this to true only if clockwise and counter-clockwise feel reversed
// after wiring your particular EC11 module.
constexpr bool kInvertEncoderDirection = false;

uint32_t now_ms() {
    return static_cast<uint32_t>(to_ms_since_boot(get_absolute_time()));
}

void enqueue_turn_actions(int turns, bool track_mode) {
    if (kInvertEncoderDirection) {
        turns = -turns;
    }

    while (turns > 0) {
        (void)media_hid_enqueue(track_mode ? MediaAction::NextTrack : MediaAction::VolumeUp);
        --turns;
    }

    while (turns < 0) {
        (void)media_hid_enqueue(track_mode ? MediaAction::PreviousTrack : MediaAction::VolumeDown);
        ++turns;
    }
}

}  // namespace

int main() {
    board_init();

    const uint32_t initial_time_ms = now_ms();

    // The LED locking button drives its SIG pin high when it is on.
    // Off = volume mode; on = track mode.
    DebouncedInput mode_switch(kModeSwitchPin, kModeSwitchActiveHigh, false);
    mode_switch.init(initial_time_ms);

    // EC11 module: CLK -> GP3, DT -> GP4, SW -> GP5.
    QuadratureEncoder encoder(kEncoderClkPin, kEncoderDtPin, kEncoderSwitchPin);
    encoder.init(initial_time_ms);

    media_hid_init();

    while (true) {
        const uint32_t current_time_ms = now_ms();

        mode_switch.update(current_time_ms);
        encoder.update(current_time_ms);

        enqueue_turn_actions(encoder.take_turns(), mode_switch.is_active());

        if (encoder.take_switch_pressed()) {
            (void)media_hid_enqueue(MediaAction::PlayPause);
        }

        media_hid_update(current_time_ms);
        tight_loop_contents();
    }
}
