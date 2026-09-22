#include "pico/stdlib.h"

#include "bsp/board_api.h"

#include "sensor/mode_sensor.h"
#include "media_hid.h"
#include "sensor/rotation_sensor.h"

namespace {

// RP2350-Zero-M header pin labels are GP2, GP3, GP4 and GP5.
// Encoder names follow the module silkscreen: SIA is phase A, SIB is phase B.
constexpr uint kModeSwitchPin = 2;
constexpr uint kEncoderSiaPin = 3;
constexpr uint kEncoderSibPin = 4;
constexpr uint kEncoderSwPin = 5;

uint32_t now_ms() {
    return static_cast<uint32_t>(to_ms_since_boot(get_absolute_time()));
}

void enqueue_turn_actions(int turns, bool track_mode) {
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

    // Off = volume mode; on = track mode.
    ModeSensor mode_sensor(kModeSwitchPin);
    mode_sensor.init(initial_time_ms);

    // Waveshare Rotation Sensor: SIA -> GP3, SIB -> GP4, SW -> GP5.
    RotationSensor rotation_sensor(kEncoderSiaPin, kEncoderSibPin, kEncoderSwPin);
    rotation_sensor.init(initial_time_ms);

    media_hid_init();

    while (true) {
        const uint32_t current_time_ms = now_ms();

        mode_sensor.update(current_time_ms);
        rotation_sensor.update(current_time_ms);

        const int turns = rotation_sensor.take_turns();
        const bool pressed = rotation_sensor.take_switch_pressed();

        // Input while the host sleeps wakes it first; the queued actions follow once the bus resumes.
        if (turns != 0 || pressed) {
            media_hid_wake_host();
        }

        enqueue_turn_actions(turns, mode_sensor.is_on());

        if (pressed) {
            (void)media_hid_enqueue(MediaAction::PlayPause);
        }

        media_hid_update(current_time_ms);
        tight_loop_contents();
    }
}
