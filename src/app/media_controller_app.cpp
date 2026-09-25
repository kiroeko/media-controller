#include "app/media_controller_app.h"

#include "bsp/board_api.h"
#include "pico/stdlib.h"

#include "usb/media_hid.h"

namespace {
// RP2350-Zero-M 排针上的 GP2–GP5；SIA 是编码器 A 相，SIB 是 B 相。
constexpr uint kModeSwitchPin = 2;
constexpr uint kEncoderSiaPin = 3;
constexpr uint kEncoderSibPin = 4;
constexpr uint kEncoderSwPin = 5;

// 交互阈值由应用定义：长按 700 ms；当前关闭双击，250 ms 窗口暂未使用。
constexpr uint32_t kButtonLongPressMs = 700;
constexpr bool kDetectDoublePress = false;
constexpr uint32_t kButtonDoubleGapMs = 250;
constexpr ButtonGestureConfig kButtonGestureConfig{
    kButtonLongPressMs, kDetectDoublePress, kButtonDoubleGapMs,
};

// SDK 返回 32 位毫秒时间；约 49.7 天回绕，使用方应比较时间差。
uint32_t now_ms() {
    return to_ms_since_boot(get_absolute_time());
}
}  // 匿名命名空间

// 将板级接线注入两个物理器件；构造时尚未访问 GPIO。
MediaControllerApp::MediaControllerApp()
    : mode_switch_(kModeSwitchPin)
    , rotation_sensor_(kEncoderSiaPin, kEncoderSibPin, kEncoderSwPin,
                       kButtonGestureConfig) {}

// 初始化后持续轮询输入与 USB；循环不能阻塞，以维持采样和去抖节奏。
[[noreturn]] void MediaControllerApp::run() {
    board_init();

    const uint32_t initial_time_ms = now_ms();
    mode_switch_.init(initial_time_ms);
    rotation_sensor_.init(initial_time_ms);
    media_hid_init();

    while (true) {
        update(now_ms());
        tight_loop_contents();
    }
}

// 取走器件事件，按当前模式选择媒体动作，再推进 USB 状态机。
void MediaControllerApp::update(uint32_t now_ms) {
    mode_switch_.update(now_ms);
    rotation_sensor_.update(now_ms);

    const int detents = rotation_sensor_.take_detents();

    bool short_press = false;
    bool long_press = false;
    for (ButtonGesture gesture = rotation_sensor_.take_button_gesture();
         gesture != ButtonGesture::None;
         gesture = rotation_sensor_.take_button_gesture()) {
        if (gesture == ButtonGesture::Short) {
            short_press = true;
        } else if (gesture == ButtonGesture::Long) {
            long_press = true;
        }
        // 当前关闭双击，不会收到 Double；将来启用时需在此绑定动作。
    }

    // 有输入动作时尝试远程唤醒；仅在 USB 已挂起且主机允许时有效。
    if (detents != 0 || short_press || long_press) {
        media_hid_wake_host();
    }

    // 同一轮内先排入按键动作，避免快速旋转占满剩余队列位置。
    if (short_press) {
        (void)media_hid_enqueue(MediaAction::PlayPause,
                                MediaQueueAdmission::AllowReservedSlot);
    }
    if (long_press) {
        (void)media_hid_enqueue(MediaAction::Mute,
                                MediaQueueAdmission::AllowReservedSlot);
    }

    // 队列满时舍弃新旋转动作，限制停转后仍待发送的步数。
    enqueue_detent_actions(detents, mode_switch_.is_on());

    media_hid_update(now_ms);
}

// 按卡点数逐个排入媒体动作；模式开关闭合时切歌，否则调节音量。
void MediaControllerApp::enqueue_detent_actions(int detents, bool track_mode) {
    while (detents > 0) {
        (void)media_hid_enqueue(track_mode ? MediaAction::NextTrack : MediaAction::VolumeUp);
        --detents;
    }

    while (detents < 0) {
        (void)media_hid_enqueue(track_mode ? MediaAction::PreviousTrack : MediaAction::VolumeDown);
        ++detents;
    }
}
