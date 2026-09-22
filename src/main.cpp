#include "pico/stdlib.h"

#include "bsp/board_api.h"

#include "device/mode_device.h"
#include "device/rotation_device.h"
#include "media_hid.h"

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
    ModeDevice mode_device(kModeSwitchPin);
    mode_device.init(initial_time_ms);

    // Waveshare Rotation Sensor: SIA -> GP3, SIB -> GP4, SW -> GP5.
    RotationDevice rotation_device(kEncoderSiaPin, kEncoderSibPin, kEncoderSwPin);
    rotation_device.init(initial_time_ms);

    media_hid_init();

    while (true) {
        const uint32_t current_time_ms = now_ms();

        mode_device.update(current_time_ms);
        rotation_device.update(current_time_ms);

        const int turns = rotation_device.take_turns();

        bool any_gesture = false;
        bool short_press = false;
        bool long_press = false;
        for (SwitchGesture gesture = rotation_device.take_switch_gesture();
             gesture != SwitchGesture::None;
             gesture = rotation_device.take_switch_gesture()) {
            any_gesture = true;
            if (gesture == SwitchGesture::Short) {
                short_press = true;
            } else if (gesture == SwitchGesture::Long) {
                long_press = true;
            }
            // Double is detected but unbound for now.
        }

        // Input while the host sleeps wakes it first; the queued actions follow once the bus resumes.
        if (turns != 0 || any_gesture) {
            media_hid_wake_host();
        }

        enqueue_turn_actions(turns, mode_device.is_on());

        if (short_press) {
            (void)media_hid_enqueue(MediaAction::PlayPause);
        }

        if (long_press) {
            (void)media_hid_enqueue(MediaAction::Mute);
        }

        media_hid_update(current_time_ms);
        tight_loop_contents();
    }
}
