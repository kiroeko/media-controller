#include "media_hid.h"

#include <algorithm>
#include <cstring>

#include "bsp/board_api.h"
#include "pico/unique_id.h"
#include "tusb.h"

namespace {

constexpr uint8_t kReportIdConsumerControl = 1;
constexpr uint32_t kKeyReleaseDelayMs = 8;
constexpr size_t kQueueCapacity = 16;

enum InterfaceNumber : uint8_t {
    kInterfaceHid = 0,
    kInterfaceCount,
};

enum StringIndex : uint8_t {
    kStringLanguage = 0,
    kStringManufacturer,
    kStringProduct,
    kStringSerial,
    kStringHidInterface,
};

// The VID is TinyUSB's example value and is not assigned to anyone; the PID is
// self-assigned to avoid the example's default. Replace the VID before
// distributing hardware commercially.
const tusb_desc_device_t kDeviceDescriptor = {
    sizeof(tusb_desc_device_t),
    TUSB_DESC_DEVICE,
    0x0200,
    0x00,
    0x00,
    0x00,
    CFG_TUD_ENDPOINT0_SIZE,
    0xCAFE,
    0x40A1,
    0x0101,
    kStringManufacturer,
    kStringProduct,
    kStringSerial,
    0x01,
};

const uint8_t kHidReportDescriptor[] = {
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(kReportIdConsumerControl)),
};

enum : uint16_t {
    kConfigurationLength = TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN,
};

const uint8_t kConfigurationDescriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, kInterfaceCount, 0, kConfigurationLength,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(kInterfaceHid, kStringHidInterface, HID_ITF_PROTOCOL_NONE,
                       sizeof(kHidReportDescriptor), 0x81, CFG_TUD_HID_EP_BUFSIZE, 1),
};

// The USB serial number is per-device, so it is filled from the RP2350's OTP
// unique ID by media_hid_init() before the USB stack is started.
char serial_string[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1] = {0};

const char* const kStringDescriptors[] = {
    "",
    "Kiro",
    "RP2350 Media Dial",
    serial_string,
    "Consumer Control",
};

MediaAction action_queue[kQueueCapacity]{};
size_t queue_head = 0;
size_t queue_tail = 0;
bool report_is_pressed = false;
uint32_t release_at_ms = 0;
bool suspend_wake_enabled = false;

bool queue_is_empty() {
    return queue_head == queue_tail;
}

bool time_reached(uint32_t now_ms, uint32_t target_ms) {
    return static_cast<int32_t>(now_ms - target_ms) >= 0;
}

uint16_t usage_for(MediaAction action) {
    switch (action) {
        case MediaAction::PlayPause:
            return HID_USAGE_CONSUMER_PLAY_PAUSE;
        case MediaAction::NextTrack:
            return HID_USAGE_CONSUMER_SCAN_NEXT;
        case MediaAction::PreviousTrack:
            return HID_USAGE_CONSUMER_SCAN_PREVIOUS;
        case MediaAction::VolumeUp:
            return HID_USAGE_CONSUMER_VOLUME_INCREMENT;
        case MediaAction::VolumeDown:
            return HID_USAGE_CONSUMER_VOLUME_DECREMENT;
        case MediaAction::Mute:
            return HID_USAGE_CONSUMER_MUTE;
    }

    return 0;
}

}  // namespace

void media_hid_init() {
    pico_get_unique_board_id_string(serial_string, sizeof(serial_string));

    tud_init(0);
    board_init_after_tusb();
}

bool media_hid_enqueue(MediaAction action) {
    const size_t next_tail = (queue_tail + 1) % kQueueCapacity;
    if (next_tail == queue_head) {
        return false;
    }

    action_queue[queue_tail] = action;
    queue_tail = next_tail;
    return true;
}

bool media_hid_wake_host() {
    return tud_remote_wakeup();
}

void media_hid_update(uint32_t now_ms) {
    tud_task();

    // Host asleep and unwilling to be woken: input is meaningless, drop it instead of firing it on a later unrelated resume.
    if (tud_suspended() && !suspend_wake_enabled) {
        queue_head = queue_tail = 0;
    }

    if (report_is_pressed) {
        if (time_reached(now_ms, release_at_ms) && tud_hid_ready()) {
            const uint16_t released = 0;
            if (tud_hid_report(kReportIdConsumerControl, &released, sizeof(released))) {
                report_is_pressed = false;
            }
        }
        return;
    }

    if (queue_is_empty() || !tud_hid_ready()) {
        return;
    }

    const MediaAction action = action_queue[queue_head];
    const uint16_t usage = usage_for(action);
    if (tud_hid_report(kReportIdConsumerControl, &usage, sizeof(usage))) {
        queue_head = (queue_head + 1) % kQueueCapacity;
        report_is_pressed = true;
        release_at_ms = now_ms + kKeyReleaseDelayMs;
    }
}

extern "C" uint8_t const* tud_descriptor_device_cb() {
    return reinterpret_cast<uint8_t const*>(&kDeviceDescriptor);
}

extern "C" uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return kConfigurationDescriptor;
}

extern "C" uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return kHidReportDescriptor;
}

extern "C" uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;

    static uint16_t descriptor[32];
    uint8_t character_count = 0;

    if (index == kStringLanguage) {
        descriptor[1] = 0x0409;  // English (United States)
        character_count = 1;
    } else {
        if (index >= sizeof(kStringDescriptors) / sizeof(kStringDescriptors[0])) {
            return 0;
        }

        const char* text = kStringDescriptors[index];
        character_count = static_cast<uint8_t>(std::min<size_t>(std::strlen(text), 31));
        for (uint8_t character = 0; character < character_count; ++character) {
            descriptor[1 + character] = static_cast<uint8_t>(text[character]);
        }
    }

    descriptor[0] = static_cast<uint16_t>((TUSB_DESC_STRING << 8U) | (2 * character_count + 2));
    return descriptor;
}

extern "C" uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                            hid_report_type_t report_type, uint8_t* buffer,
                                            uint16_t request_length) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)request_length;
    return 0;
}

extern "C" void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                                       hid_report_type_t report_type, uint8_t const* buffer,
                                       uint16_t buffer_size) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)buffer_size;
}

extern "C" void tud_suspend_cb(bool remote_wakeup_en) {
    suspend_wake_enabled = remote_wakeup_en;
    // Drop queued actions so they cannot fire on a later unrelated resume, but keep the in-flight press: its release must still reach the host.
    queue_head = queue_tail = 0;
}
