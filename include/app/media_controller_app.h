#pragma once

#include <cstdint>

#include "device/waveshare_rotation_sensor.h"
#include "device/yfrobot_led_latching_switch.h"

// 产品行为的装配与主循环：把输入器件的状态映射为 USB 媒体动作。
class MediaControllerApp {
public:
    // 初始化板级支持、输入器件和 USB，然后持续处理输入与输出。
    [[noreturn]] void run();

private:
    // 执行一轮应用循环：更新器件、取走输入结果、将媒体动作入队并推进 USB 发送。
    // now_ms 是本轮的毫秒时间戳；由 run() 在初始化完成后持续调用。
    void update(uint32_t now_ms);

    // 将旋转格数映射成媒体动作并尝试入队；detents 正数表示顺时针，负数表示逆时针。
    // track_mode 为 true 时切歌，否则调音量；now_ms 是计算冷却的本轮毫秒时间戳。
    // 音量逐格入队；切歌有旋转且不在冷却期时仅入队一次，成功才开始新冷却。
    // 冷却期内的旋转和队列满时未能入队的动作直接舍弃。
    void enqueue_detent_actions(int detents, bool track_mode, uint32_t now_ms);

    YfrobotLedLatchingSwitch mode_switch_;
    WaveshareRotationSensor rotation_sensor_;

    uint32_t last_track_change_ms_ = 0; // 最近一次成功入队的切歌动作时间。
    bool has_last_track_change_ = false; // 上次切歌时间的有效标志；成功入队切歌后置位。
};
