#pragma once

#include <cstdint>

enum class MediaAction : uint8_t {
    PlayPause,
    NextTrack,
    PreviousTrack,
    VolumeUp,
    VolumeDown,
};

void media_hid_init();
void media_hid_update(uint32_t now_ms);
bool media_hid_enqueue(MediaAction action);
bool media_hid_wake_host();

