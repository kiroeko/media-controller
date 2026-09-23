#include "pico/stdlib.h"

#include "bsp/board_api.h"

#include "device/mode_device.h"
#include "device/rotation_device.h"
#include "media_hid.h"

namespace {

// RP2350-Zero-M 排针上的 GP2、GP3、GP4、GP5 用于本项目的输入模块。
// 编码器名称按模块丝印命名：SIA 是 A 相，SIB 是 B 相。
constexpr uint kModeSwitchPin = 2;
constexpr uint kEncoderSiaPin = 3;
constexpr uint kEncoderSibPin = 4;
constexpr uint kEncoderSwPin = 5;

// 返回开机后的毫秒数并截断为 32 位，因此约 49.7 天后会回绕。
// 调用方必须通过时间差判断，不能直接比较绝对时刻。
uint32_t now_ms() {
    return static_cast<uint32_t>(to_ms_since_boot(get_absolute_time()));
}

// 按旋转格数逐个生成媒体动作，并按顺序加入队列。
// 通道已将相反方向的转动抵消，因此左右各转一格会以 0 到达，不会发送动作。
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

// 固件入口：初始化板级支持、输入器件和 USB HID，然后持续处理输入并发送媒体报告。
int main() {
    board_init();

    const uint32_t initial_time_ms = now_ms();

    // 开关未按下为音量模式；按下并点亮时为切歌模式。
    ModeDevice mode_device(kModeSwitchPin);
    mode_device.init(initial_time_ms);

    // Waveshare 旋转传感器接线：SIA -> GP3、SIB -> GP4、SW -> GP5。
    RotationDevice rotation_device(kEncoderSiaPin, kEncoderSibPin, kEncoderSwPin);
    rotation_device.init(initial_time_ms);

    media_hid_init();

    // 每轮循环推进所有输入、取走通道事件，再处理 USB 总线。
    // 此处不能阻塞：按键去抖、1 ms 相位采样和 8 位格数计数都依赖循环持续运行。
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
            // 当前会检测双击，但尚未为它绑定媒体动作。
        }

        // 主机睡眠时收到输入会先请求唤醒；USB 总线恢复后再发送已排队的媒体动作。
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
