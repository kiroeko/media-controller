#pragma once

#include <cstdint>

// Consumer Control 媒体控制动作：先发送对应 usage 的按下报告，再发送值为 0 的松开报告。
// 报告描述符声明的 USAGE_MAX 为 0x03FF，因此范围内的 Consumer usage 无需修改描述符即可发送。
enum class MediaAction : uint8_t {
    PlayPause,      // 播放/暂停，usage 0xCD。
    NextTrack,      // 下一首，usage 0xB5。
    PreviousTrack,  // 上一首，usage 0xB6。
    VolumeUp,       // 音量增加，usage 0xE9。
    VolumeDown,     // 音量降低，usage 0xEA。
    Mute,           // 请求主机切换静音状态，usage 0xE2。
};

// 高优先级动作可使用为其预留的一个队列名额；发送顺序仍按入队顺序。
enum class MediaQueuePriority : uint8_t { Normal, High };

// 启动 USB 协议栈。启动前用 RP2350 OTP 中的唯一 ID 填充序列号，
// 让主机区分同型号的不同设备，并稳定识别重新连接的同一块板。
void media_hid_init();

// 处理 USB 总线，每次至多提交一份 HID 报告：队列动作的按下，或前一动作的松开。
// 按下报告提交成功后动作便从队列移除；松开报告提交后才开始下一个动作。
void media_hid_update(uint32_t now_ms);

// 把动作加入队列。普通动作最多占 14 个位置，为高优先级动作预留一个；
// 所有动作合计最多 15 个。返回 false 表示容量耗尽，调用方不应阻塞重试。
// 16 个槽位中始终留一个空位，使 head == tail 唯一表示队列为空。
bool media_hid_enqueue(MediaAction action,
                       MediaQueuePriority priority = MediaQueuePriority::Normal);

// 请求主机从 USB 挂起中恢复；仅在设备已挂起、配置支持且主机允许远程唤醒时返回 true。
bool media_hid_wake_host();
