#include "usb/media_hid.h"

#include <algorithm>
#include <cstring>

#include "bsp/board_api.h"
#include "pico/unique_id.h"
#include "tusb.h"

// 本文件内部使用的常量、运行状态和辅助函数；应用通过 media_hid.h 中的函数访问 USB 功能。
namespace {

// Consumer Control 输入报告的编号；报告描述符和 tud_hid_report() 必须使用同一个值。
constexpr uint8_t report_id_consumer_control = 1;

// 成功提交按下报告后，至少等待这些毫秒才尝试松开；端点忙时会继续等待。
constexpr uint32_t key_release_delay_ms = 8;

// 环形队列的存储槽位数；留一个空槽区分队列空/满，实际最多保存 15 个待发送动作。
constexpr size_t queue_capacity = 16;

// TinyUSB 的设备回调是 C 函数，且本板只有一个 USB HID 实例；运行状态集中保存于此。
// 描述符和回调需要这些数据在整个固件运行期间保持有效。
struct MediaHidState {
    // 由 RP2350 唯一 ID 生成的十六进制序列号文本；最后一字节留给字符串结束符。
    char serial_string[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1]{};

    // 字符串描述符的复用缓冲区：第 0 项是长度和类型，其余存语言 ID 或最多 31 个 UTF-16 码元。
    uint16_t string_descriptor[32]{};

    // 等待提交按下报告的媒体动作；按入队顺序发送。
    MediaAction action_queue[queue_capacity]{};

    // 下一项待提交按下报告的位置；提交成功后才向前移动。
    size_t queue_head = 0;

    // 下一次入队要写入的位置；等于 queue_head 时表示没有待发送动作。
    size_t queue_tail = 0;

    // 当前动作已提交按下报告，仍需提交对应的松开报告；松开成功前不发送下一动作。
    bool report_is_pressed = false;

    // 最早可提交松开报告的 32 位毫秒时刻；到时后还需等待 HID 端点就绪。
    uint32_t release_at_ms = 0;

    // 最近一次 USB 挂起时，主机是否授权设备发起远程唤醒。
    bool suspend_wake_enabled = false;
};

// 本板唯一 HID 实例的运行状态；应用调用和 TinyUSB 回调共同使用，整个固件运行期间有效。
MediaHidState hid_state;

// 配置描述符内的 USB 接口编号；编号从 0 开始，当前只有一个 HID 接口。
enum InterfaceNumber : uint8_t {
    interface_hid = 0,  // 媒体控制 HID 接口的编号。
    interface_count,    // 接口总数，供配置描述符填写 bNumInterfaces。
};

// USB 字符串描述符的索引，与下方 string_descriptors 的排列对应。
// 索引 0 专门用于查询支持的语言列表，其余索引用于查询文本。
enum StringIndex : uint8_t {
    string_language = 0,   // 支持的语言 ID 列表。
    string_manufacturer,   // 制造商名称。
    string_product,        // 产品名称。
    string_serial,         // 本板的唯一序列号。
    string_hid_interface,  // HID 接口名称。
};

// 设备描述符：主机据此读取 USB 版本、VID/PID、字符串索引和配置数量。
// VID 0xCAFE 是 TinyUSB 示例值，并未分配给本项目；PID 是自行设置的，用来避开示例默认值。
// 若要商业销售设备，应换成合法分配的 VID/PID。
const tusb_desc_device_t device_descriptor = {
    sizeof(tusb_desc_device_t),  // bLength：设备描述符的字节数。
    TUSB_DESC_DEVICE,            // bDescriptorType：这是设备描述符。
    0x0200,                      // bcdUSB：遵循 USB 2.00 规范，按 BCD 编码。
    0x00,                        // bDeviceClass：设备类别由各接口描述符声明。
    0x00,                        // bDeviceSubClass：设备级子类未指定。
    0x00,                        // bDeviceProtocol：设备级协议未指定。
    CFG_TUD_ENDPOINT0_SIZE,      // bMaxPacketSize0：控制端点 EP0 的最大包长。
    0xCAFE,                      // idVendor：厂商 ID，当前使用示例值。
    0x40A1,                      // idProduct：本项目的产品 ID。
    0x0101,                      // bcdDevice：设备修订号，BCD 编码的 1.01。
    string_manufacturer,         // iManufacturer：制造商字符串索引。
    string_product,              // iProduct：产品字符串索引。
    string_serial,               // iSerialNumber：序列号字符串索引。
    0x01,                        // bNumConfigurations：提供一套 USB 配置。
};

// 字符串索引对应的 ASCII 文本；回调负责转换为 USB 字符串描述符。
// 序列号指向运行状态中的缓冲区，media_hid_init() 会在启动 USB 前填好内容。
const char* const string_descriptors[] = {
    "",                       // 索引 0 的占位；语言列表由回调单独生成。
    "Kiro",                   // 制造商名称。
    "Kiro Media Controller",  // 产品名称。
    hid_state.serial_string,  // 本板唯一序列号。
    "Consumer Control",       // HID 接口名称。
};

// 配置描述符的总字节数，包括配置头、接口描述符、HID 描述符和中断 IN 端点描述符。
enum : uint16_t {
    configuration_length = TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN,
};

// HID 报告描述符：声明 Consumer Control 输入，每份报告携带一个 16 位媒体 usage。
// usage 范围为 0～0x03FF；非零值表示对应媒体键按下，0 表示松开。
// report_id_consumer_control 指定的报告编号由 TinyUSB 添加到实际发送的数据前面。
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(report_id_consumer_control)),
};

// 唯一一套 USB 配置的描述符数据；两个宏按顺序展开为主机枚举时读取的字节。
const uint8_t configuration_descriptor[] = {
    // 生成 9 字节的配置描述符头；此配置的接口、HID 和端点描述符由下一个宏追加。
    TUD_CONFIG_DESCRIPTOR(
        1,                                   // bConfigurationValue：主机用此值选择配置；配置值为 1，查询描述符的配置索引则从 0 开始。
        interface_count,                     // bNumInterfaces：此配置包含的接口总数；当前只有编号为 0 的一个 HID 接口。
        0,                                   // iConfiguration：配置名称的字符串索引；0 表示不提供配置名称。
        configuration_length,                // wTotalLength：配置头及其接口、HID、端点描述符的总字节数；不包含另行读取的报告或字符串描述符。
        TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,  // bmAttributes：声明支持远程唤醒，使用时仍须主机授权；未设置自供电标志，表示由 USB 总线供电。
        100                                  // bMaxPower：声明此配置最大从 USB 取电 100 mA；宏按每单位 2 mA 编码为 50，不会主动限制硬件电流。
    ),
    // 依次生成接口描述符（9 字节）、HID 描述符（9 字节）和中断 IN 端点描述符（7 字节）。
    TUD_HID_DESCRIPTOR(
        interface_hid,                  // bInterfaceNumber：接口编号，当前为 0；宏同时将接口类别声明为 HID。
        string_hid_interface,           // iInterface：接口名称的字符串索引，当前为 4，对应 "Consumer Control"。
        HID_ITF_PROTOCOL_NONE,          // bInterfaceSubClass/bInterfaceProtocol：不声明 Boot 子类或协议，使用报告描述符定义的媒体控制格式。
        sizeof(hid_report_descriptor),  // wDescriptorLength：HID 报告描述符这份格式说明的字节数；每次媒体输入报告本身只有 3 字节。
        0x81,                           // bEndpointAddress：最高位 1 表示 IN（设备到主机），低位 1 表示端点编号 1；宏固定使用中断传输。
        CFG_TUD_HID_EP_BUFSIZE,         // wMaxPacketSize：此端点最大数据包为 64 字节，与 TinyUSB 缓冲区大小一致；报告无需填满 64 字节。
        1                               // bInterval：全速模式下主机轮询此端点的间隔为 1 ms；不要求每次都发送报告，与 8 ms 松开等待分别设置。
    ),
};

// 返回待发送队列是否为空；环形队列以读写位置相等表示空。
// 已提交按下、正在等待松开的动作由 report_is_pressed 单独记录。
bool queue_is_empty() {
    return hid_state.queue_head == hid_state.queue_tail;
}

// 判断 now_ms 是否已到达 target_ms；有符号时间差允许截止时刻跨过 32 位计数回绕点。
// 要求实际时间间隔小于 2^31 ms；本模块用它检查 8 ms 的松开等待时间。
bool time_reached(uint32_t now_ms, uint32_t target_ms) {
    return static_cast<int32_t>(now_ms - target_ms) >= 0;
}

// 将应用传入的媒体动作转换为 HID Consumer Control 的 16 位按键编号。
// 返回值直接写入输入报告；无法识别的动作返回 0，表示没有媒体键按下。
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

// 启动时调用一次；应用已先调用 board_init()，完成板级基础初始化。
// 此处准备设备身份并启动 USB，之后由主循环持续调用 media_hid_update() 处理主机请求。
void media_hid_init() {
    // 把本板唯一 ID 转成十六进制 ASCII 文本，写入持久保存的序列号缓冲区。
    // string_descriptors 的序列号项指向这个缓冲区，因此必须在启动 USB 前填好，
    // 让电脑查询序列号时能取得有效内容。第二个参数包含字符串结束符所需的空间。
    pico_get_unique_board_id_string(hid_state.serial_string,
                                    sizeof(hid_state.serial_string));

    // 初始化 USB 控制器端口 0 的设备协议栈；0 是硬件端口号，接口号和端点号各自独立。
    // 驱动后端、设备模式和全速模式由 tusb_config.h 选择。
    // 初始化返回后，电脑仍需读取描述符、选择配置；这些请求由后续 tud_task() 处理。
    tud_init(0);

    // 调用板级支持包的后初始化钩子，供其完成需要在 TinyUSB 初始化后执行的设置。
    // 按板级 API 的调用顺序，这一步放在 tud_init() 之后。
    board_init_after_tusb();
}

// 将 action 加入待发送队列；admission 决定是否允许使用最后一个预留位置。
// 共享位置最多占 14 个名额，所有待发送动作合计最多 15 个。
// 返回 true 表示已入队；容量不足或预留位置不允许使用时返回 false，不阻塞也不重试。
bool media_hid_enqueue(MediaAction action, MediaQueueAdmission admission) {
    const size_t next_tail = (hid_state.queue_tail + 1) % queue_capacity;
    const bool queue_full = next_tail == hid_state.queue_head;
    const bool reserved_slot_only = (next_tail + 1) % queue_capacity == hid_state.queue_head;
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

// 主循环每轮调用；now_ms 是本轮的启动后毫秒计数，用来判断何时可以提交松开报告。
// 一个媒体动作按顺序经历：提交按下 -> 等待至少 8 ms 且端点就绪 -> 提交松开。
// 每次调用至多提交一份报告；等待期间返回主循环，让应用继续采样旋钮和按键。
// tud_hid_report() 返回 true 表示已提交 USB 传输，电脑接收和处理发生在之后。
void media_hid_update(uint32_t now_ms) {
    // 处理 TinyUSB 收集的 USB 事件，包括枚举控制请求、传输完成和挂起通知。
    // 即使动作队列为空也要调用：电脑仍可能查询描述符或改变 USB 状态。
    // 挂起回调也在这里得到处理，使下方判断使用更新后的 suspend_wake_enabled。
    tud_task();

    // 主机挂起且未允许远程唤醒时，清空输入动作，避免之后因其他原因恢复时误发旧动作。
    // 只清除等待发送按下报告的队列；已经提交按下、尚欠松开的状态仍然保留，
    // 这样总线恢复后可以补交松开报告，结束前一个媒体键的按下状态。
    if (tud_suspended() && !hid_state.suspend_wake_enabled) {
        hid_state.queue_head = hid_state.queue_tail = 0;
    }

    // report_is_pressed 表示前一动作的按下报告已提交，还需要提交对应的松开报告。
    // 它记录 USB 发送状态；音量旋转动作也会经过此状态，与物理按键是否按下无关。
    if (hid_state.report_is_pressed) {
        // 当前动作尚未松开，先等到最早松开时刻，再等端点能接受下一份报告。
        // time_reached() 能处理 32 位毫秒计数回绕；这里检查时间，不用 sleep 等待。
        if (time_reached(now_ms, hid_state.release_at_ms) && tud_hid_ready()) {
            const uint16_t released = 0;  // usage 为 0 表示当前没有媒体键按下。
            // 第一个参数是报告 ID，后两个参数是数据地址和长度：2 字节的 0 usage。
            // TinyUSB 复制数据并在前面加上 ID 1，形成 01 00 00 这份松开报告。
            // 只有提交成功才清除标志；失败时保留状态，下轮继续尝试松开。
            if (tud_hid_report(report_id_consumer_control, &released, sizeof(released))) {
                hid_state.report_is_pressed = false;
            }
        }
        // 未到时间、端点忙或刚提交松开，都结束本轮；下一动作留到之后的调用。
        // 松开提交后，端点仍可能忙，下轮也必须通过 tud_hid_ready() 才能提交下一次按下。
        return;
    }

    // 没有动作就不发送；HID 未就绪时保留队列，等下一轮再试。
    // tud_hid_ready() 检查主机已选择配置、设备未挂起，以及 HID IN 端点存在且空闲。
    if (queue_is_empty() || !tud_hid_ready()) {
        return;
    }

    // 先查看队头，暂不移除；usage_for() 把应用枚举转换为标准的 16 位 Consumer usage。
    // 例如 VolumeUp 对应 0x00E9，告诉电脑“音量加键按下”。
    const MediaAction action = hid_state.action_queue[hid_state.queue_head];
    const uint16_t usage = usage_for(action);
    // sizeof(usage) 为 2，TinyUSB 再添加 1 字节报告 ID；音量加的报告数据为 01 E9 00。
    // 复制到 TinyUSB 缓冲区后局部变量即可结束生存期，不需要一直保存到电脑收到报告。
    // 提交失败时不移除动作、不改变发送状态，下轮仍尝试队头动作。
    if (tud_hid_report(report_id_consumer_control, &usage, sizeof(usage))) {
        // 按下已提交，移除队头；取模使下标在环形队列的 16 个槽位内循环。
        hid_state.queue_head = (hid_state.queue_head + 1) % queue_capacity;
        // 接下来必须先提交松开，不能直接开始下一个动作。
        hid_state.report_is_pressed = true;
        // 用本轮时间计算最早松开时刻；等待从提交按下计算，电脑实际收到按下还在之后。
        // 8 ms 到期后若端点仍忙，会继续等待，因此实际松开可能更晚。
        hid_state.release_at_ms = now_ms + key_release_delay_ms;
    }
}

// TinyUSB 向主机提供设备描述符时调用；返回本设备的 VID、PID 和版本信息。
extern "C" uint8_t const* tud_descriptor_device_cb() {
    return reinterpret_cast<uint8_t const*>(&device_descriptor);
}

// TinyUSB 向主机提供配置描述符时调用；此配置包含一个 Consumer Control HID 接口。
// index 是从 0 开始的配置索引；当前只有一套配置，统一返回这份描述符。
extern "C" uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    return configuration_descriptor;
}

// TinyUSB 向主机提供 HID 报告描述符时调用；描述符定义媒体控制报告的格式和用途。
// instance 是 HID 实例编号；当前只有一个 HID 实例，统一返回这份报告描述符。
extern "C" uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
    return hid_report_descriptor;
}

// index 为 0 时返回语言列表，其余有效索引返回 string_descriptors 中对应的文本。
// 当前只有一套 ASCII 文本，langid 不影响内容；每个字符扩展为 USB 所需的 UTF-16 码元。
// 返回复用缓冲区的地址；索引越界时返回空指针，表示没有对应描述符。
extern "C" uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    uint16_t* const descriptor = hid_state.string_descriptor;
    uint8_t character_count = 0;

    if (index == string_language) {
        descriptor[1] = 0x0409;  // 语言 ID：英语（美国）。
        character_count = 1;
    } else {
        if (index >= sizeof(string_descriptors) / sizeof(string_descriptors[0])) {
            return 0;
        }

        const char* text = string_descriptors[index];
        // 缓冲区第 0 项留给描述符头，文本最多存放 31 个字符。
        character_count = static_cast<uint8_t>(std::min<size_t>(std::strlen(text), 31));
        for (uint8_t character = 0; character < character_count; ++character) {
            descriptor[1 + character] = static_cast<uint8_t>(text[character]);
        }
    }

    // 高字节是描述符类型，低字节是总字节数：2 字节头部加上每个码元的 2 字节。
    descriptor[0] = static_cast<uint16_t>((TUSB_DESC_STRING << 8U) | (2 * character_count + 2));
    return descriptor;
}

// 记录主机是否允许远程唤醒，并清除挂起前排队的动作，避免恢复后误发送。
// remote_wakeup_en 是主机在此次挂起时给予的唤醒授权。
extern "C" void tud_suspend_cb(bool remote_wakeup_en) {
    hid_state.suspend_wake_enabled = remote_wakeup_en;
    // 保留正在发送动作的状态，让对应松开报告仍能发出；只清除尚未发送的队列。
    hid_state.queue_head = hid_state.queue_tail = 0;
}

// 主机通过控制传输发来 GET_REPORT 时返回 0，表示不支持这类主动读取。
// 正常的媒体按键输入报告仍由 tud_hid_report() 通过中断 IN 端点发送。
// instance/report_id/report_type 指定请求对象，buffer/request_length 是可填写的缓冲区及容量。
extern "C" uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                            hid_report_type_t report_type, uint8_t* buffer,
                                            uint16_t request_length) {
    return 0;
}

// 本设备不处理主机写入的 HID 输出或特征报告，因此此回调暂不执行操作。
// instance/report_id/report_type 指定报告对象，buffer/buffer_size 提供主机发送的数据及长度。
extern "C" void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                                       hid_report_type_t report_type, uint8_t const* buffer,
                                       uint16_t buffer_size) {
}
