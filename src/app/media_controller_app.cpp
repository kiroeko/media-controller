#include "app/media_controller_app.h"

#include "bsp/board_api.h"
#include "pico/stdlib.h"

#include "usb/media_hid.h"

// 将 SDK 的启动时间转换为 32 位毫秒计数，供应用循环使用。
uint32_t MediaControllerApp::now_ms() {
    return to_ms_since_boot(get_absolute_time());
}

// 初始化后持续轮询输入与 USB；循环不能阻塞，以维持采样和去抖节奏。
[[noreturn]] void MediaControllerApp::run() {
    board_init();

    const uint32_t initial_time_ms = now_ms();
    mode_switch_.init(mode_switch_pin, initial_time_ms);
    rotation_sensor_.init(rotation_sensor_config, initial_time_ms);

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
        media_hid_enqueue(MediaAction::PlayPause,
                          MediaQueueAdmission::AllowReservedSlot);
    }
    if (long_press) {
        media_hid_enqueue(MediaAction::Mute,
                          MediaQueueAdmission::AllowReservedSlot);
    }

    // 队列满时舍弃新旋转动作，限制停转后仍待发送的步数。
    enqueue_detent_actions(detents, mode_switch_.is_on(), now_ms);

    media_hid_update(now_ms);
}

// 切歌模式每个冷却窗口只接收一个卡点；音量模式仍逐格处理。
// 冷却只限制应用动作，编码器继续采样，避免恢复时补发窗口内的旋转。
void MediaControllerApp::enqueue_detent_actions(int detents, bool track_mode, uint32_t now_ms) {
    if (track_mode) {
        if (detents == 0 ||
            (has_last_track_change_ &&
             now_ms - last_track_change_ms_ < track_change_cooldown_ms)) {
            return;
        }

        const MediaAction action = detents > 0 ? MediaAction::NextTrack
                                               : MediaAction::PreviousTrack;
        // 队列满时没有切歌动作，因此也不启动新的冷却窗口。
        if (media_hid_enqueue(action)) {
            last_track_change_ms_ = now_ms;
            has_last_track_change_ = true;
        }
        return;
    }

    while (detents > 0) {
        media_hid_enqueue(MediaAction::VolumeUp);
        --detents;
    }

    while (detents < 0) {
        media_hid_enqueue(MediaAction::VolumeDown);
        ++detents;
    }
}
