#include "pico/stdlib.h"

#include "bsp/board_api.h"

#include "input.h"
#include "media_hid.h"

namespace {

// RP2350-Zero-M header pin labels are GP2, GP3, GP4 and GP5.
// Encoder names follow the module silkscreen: SIA is phase A, SIB is phase B.
constexpr uint kModeSwitchPin = 2;
constexpr uint kEncoderSiaPin = 3;
constexpr uint kEncoderSibPin = 4;
constexpr uint kEncoderSwPin = 5;
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
    // Off = volume mode; on = track mode. Pull is still undecided pending a
    // bench measurement of the module's own output drive; see README.
    DebouncedInput mode_switch(kModeSwitchPin, kModeSwitchActiveHigh, InputPull::None);
    mode_switch.init(initial_time_ms);

    // Waveshare Rotation Sensor: SIA -> GP3, SIB -> GP4, SW -> GP5.
    QuadratureEncoder encoder(kEncoderSiaPin, kEncoderSibPin, kEncoderSwPin);
    encoder.init(initial_time_ms);

    media_hid_init();

    while (true) {
        const uint32_t current_time_ms = now_ms();

        mode_switch.update(current_time_ms);
        encoder.update(current_time_ms);

        const int turns = encoder.take_turns();
        const bool pressed = encoder.take_switch_pressed();

        // Input while the host sleeps wakes it first; the queued actions follow once the bus resumes.
        if (turns != 0 || pressed) {
            media_hid_wake_host();
        }

        enqueue_turn_actions(turns, mode_switch.is_active());

        if (pressed) {
            (void)media_hid_enqueue(MediaAction::PlayPause);
        }

        media_hid_update(current_time_ms);
        tight_loop_contents();
    }
}
