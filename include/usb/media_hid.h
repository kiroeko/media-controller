#pragma once

#include <cstdint>

// Consumer Control 媒体控制动作；每个动作会作为一个 usage 放进一份报告。
// 报告描述符声明的 USAGE_MAX 为 0x03FF，因此范围内的 Consumer usage 无需修改描述符即可发送。
enum class MediaAction : uint8_t {
    PlayPause,      // 播放/暂停，usage 0xCD。
    NextTrack,      // 下一首，usage 0xB5。
    PreviousTrack,  // 上一首，usage 0xB6。
    VolumeUp,       // 音量增加，usage 0xE9。
    VolumeDown,     // 音量降低，usage 0xEA。
    Mute,           // 静音切换，usage 0xE2；主机每次收到后切换静音状态。
};

// 高优先级动作可使用预留的最后一个队列位置；发送顺序仍按入队顺序。
enum class MediaQueuePriority : uint8_t { Normal, High };

// 启动 USB 协议栈。启动前先用芯片 OTP 唯一 ID 填充序列号，
// 因为主机会按 VID/PID/序列号组合缓存设备描述符。
void media_hid_init();

// 处理 USB 总线并至多发送一个排队动作。主循环每轮调用一次；
// 队列只在这里消费，而且动作要等松开报告发出后才算完成。
void media_hid_update(uint32_t now_ms);

// 把动作加入队列。普通动作最多占 14 个位置，为高优先级动作预留一个；
// 所有动作合计最多 15 个。返回 false 表示容量耗尽，调用方不应阻塞重试。
// 16 个槽位中始终留一个空位，使 head == tail 唯一表示队列为空。
bool media_hid_enqueue(MediaAction action,
                       MediaQueuePriority priority = MediaQueuePriority::Normal);

// 请求唤醒主机。如果主机没有为此设备启用远程唤醒，则返回 false；这是主机设置，不代表设备故障。
bool media_hid_wake_host();
