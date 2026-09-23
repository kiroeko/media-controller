#pragma once

#include <cstdint>

#include "device/waveshare_rotation_sensor.h"
#include "device/yfrobot_led_latching_switch.h"

// 产品行为的装配与主循环：把输入器件的状态映射为 USB 媒体动作。
class MediaControllerApp {
public:
    MediaControllerApp();

    // 初始化板级支持、输入器件和 USB，然后持续处理输入与输出。
    [[noreturn]] void run();

private:
    void update(uint32_t now_ms);
    void enqueue_detent_actions(int detents, bool track_mode);

    YfrobotLedLatchingSwitch mode_switch_;
    WaveshareRotationSensor rotation_sensor_;
};
