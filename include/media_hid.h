#pragma once

#include <cstdint>

// Consumer Control actions. Each one becomes a single usage in one report. The
// report descriptor declares USAGE_MAX 0x03FF, so any Consumer usage at or
// below that is already sendable without touching the descriptor.
enum class MediaAction : uint8_t {
    PlayPause,      // 0xCD
    NextTrack,      // 0xB5
    PreviousTrack,  // 0xB6
    VolumeUp,       // 0xE9
    VolumeDown,     // 0xEA
    Mute,           // 0xE2; a host-side toggle, so one action covers both ways
};

// Start the USB stack. The serial string is filled from the chip's OTP id
// beforehand, because the host caches descriptors per VID/PID/serial triple.
void media_hid_init();

// Service the bus and send at most one queued action. Call it once per main
// loop pass: nothing else drains the queue, and an action is not finished
// until its release report has gone out too.
void media_hid_update(uint32_t now_ms);

// Append an action. False means the queue was full and the action is dropped,
// not deferred; callers discard this rather than retry, because a retry loop
// would starve the rest of the main loop. The queue holds 15 actions: one of
// its 16 slots stays empty so head == tail can only mean "nothing queued".
// Throughput is bounded by the press/release pair, at roughly 100 per second.
bool media_hid_enqueue(MediaAction action);

// Ask the host to resume. False when the host never enabled remote wakeup for
// this device, which is an ordinary setting rather than a fault.
bool media_hid_wake_host();

