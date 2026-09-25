#include "usb/media_hid.h"

#include <algorithm>
#include <cstring>

#include "bsp/board_api.h"
#include "pico/unique_id.h"
#include "tusb.h"

namespace {

constexpr uint8_t kReportIdConsumerControl = 1;
constexpr uint32_t kKeyReleaseDelayMs = 8;
constexpr size_t kQueueCapacity = 16;

// TinyUSB 的设备回调是 C 函数，且本板只有一个 USB HID 实例；运行状态集中保存于此。
// 描述符和回调需要这些数据在整个固件运行期间保持有效。
struct MediaHidState {
    char serial_string[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1]{};
    uint16_t string_descriptor[32]{};
    MediaAction action_queue[kQueueCapacity]{};
    size_t queue_head = 0;
    size_t queue_tail = 0;
    bool report_is_pressed = false;
    uint32_t release_at_ms = 0;
    bool suspend_wake_enabled = false;
};

MediaHidState hid_state;

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

// VID 0xCAFE 是 TinyUSB 示例值，并未分配给本项目；PID 是自行设置的，用来避开示例默认值。
// 若要商业销售设备，应换成合法分配的 VID/PID。
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

const char* const kStringDescriptors[] = {
    "",
    "Kiro",
    "Kiro Media Controller",
    hid_state.serial_string,
    "Consumer Control",
};

// 判断媒体动作环形队列是否为空。
bool queue_is_empty() {
    return hid_state.queue_head == hid_state.queue_tail;
}

// 用有符号差值判断截止时间是否到达，同时兼容 32 位毫秒计数回绕。
bool time_reached(uint32_t now_ms, uint32_t target_ms) {
    return static_cast<int32_t>(now_ms - target_ms) >= 0;
}

// 把项目内部的媒体动作映射为 HID Consumer Control usage。
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

}  // 匿名命名空间

// 生成本板唯一序列号，初始化 TinyUSB 设备栈，并完成板级 USB 初始化。
void media_hid_init() {
    pico_get_unique_board_id_string(hid_state.serial_string,
                                    sizeof(hid_state.serial_string));

    tud_init(0);
    board_init_after_tusb();
}

// 共享位置最多占 14 个队列名额，另留一个预留位置；所有动作合计最多 15 个。
bool media_hid_enqueue(MediaAction action, MediaQueueAdmission admission) {
    const size_t next_tail = (hid_state.queue_tail + 1) % kQueueCapacity;
    const bool queue_full = next_tail == hid_state.queue_head;
    const bool reserved_slot_only = (next_tail + 1) % kQueueCapacity == hid_state.queue_head;
    if (queue_full ||
        (admission == MediaQueueAdmission::SharedOnly && reserved_slot_only)) {
        return false;
    }

    hid_state.action_queue[hid_state.queue_tail] = action;
    hid_state.queue_tail = next_tail;
    return true;
}

// 按 USB 远程唤醒协议请求主机恢复；是否允许由主机在挂起时告知设备。
bool media_hid_wake_host() {
    return tud_remote_wakeup();
}

// 推进 TinyUSB 状态机，并按“按下报告、延时、松开报告”的顺序发送队列动作。
void media_hid_update(uint32_t now_ms) {
    tud_task();

    // 主机挂起且未允许远程唤醒时，清空输入动作，避免之后因其他原因恢复时误发旧动作。
    if (tud_suspended() && !hid_state.suspend_wake_enabled) {
        hid_state.queue_head = hid_state.queue_tail = 0;
    }

    if (hid_state.report_is_pressed) {
        if (time_reached(now_ms, hid_state.release_at_ms) && tud_hid_ready()) {
            const uint16_t released = 0;
            if (tud_hid_report(kReportIdConsumerControl, &released, sizeof(released))) {
                hid_state.report_is_pressed = false;
            }
        }
        return;
    }

    if (queue_is_empty() || !tud_hid_ready()) {
        return;
    }

    const MediaAction action = hid_state.action_queue[hid_state.queue_head];
    const uint16_t usage = usage_for(action);
    if (tud_hid_report(kReportIdConsumerControl, &usage, sizeof(usage))) {
        hid_state.queue_head = (hid_state.queue_head + 1) % kQueueCapacity;
        hid_state.report_is_pressed = true;
        hid_state.release_at_ms = now_ms + kKeyReleaseDelayMs;
    }
}

// TinyUSB 向主机提供设备描述符时调用；返回本设备的 VID、PID 和版本信息。
extern "C" uint8_t const* tud_descriptor_device_cb() {
    return reinterpret_cast<uint8_t const*>(&kDeviceDescriptor);
}

// TinyUSB 向主机提供配置描述符时调用；此配置包含一个 Consumer Control HID 接口。
extern "C" uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return kConfigurationDescriptor;
}

// TinyUSB 向主机提供 HID 报告描述符时调用；描述符定义媒体控制报告的格式和用途。
extern "C" uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return kHidReportDescriptor;
}

// TinyUSB 向主机提供字符串描述符时调用；把产品名、序列号等转成 USB 所需的 UTF-16 格式。
extern "C" uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;

    uint16_t* const descriptor = hid_state.string_descriptor;
    uint8_t character_count = 0;

    if (index == kStringLanguage) {
        descriptor[1] = 0x0409;  // 语言 ID：英语（美国）。
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

// 主机通过控制传输发来 GET_REPORT 时返回 0，表示不支持这类主动读取。
// 正常的媒体按键输入报告仍由 tud_hid_report() 通过中断 IN 端点发送。
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

// 本设备不处理主机写入的 HID 输出或特征报告，因此此回调暂不执行操作。
extern "C" void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                                       hid_report_type_t report_type, uint8_t const* buffer,
                                       uint16_t buffer_size) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)buffer_size;
}

// 记录主机是否允许远程唤醒，并清除挂起前排队的动作，避免恢复后误发送。
extern "C" void tud_suspend_cb(bool remote_wakeup_en) {
    hid_state.suspend_wake_enabled = remote_wakeup_en;
    // 保留正在发送动作的状态，让对应松开报告仍能发出；只清除尚未发送的队列。
    hid_state.queue_head = hid_state.queue_tail = 0;
}
