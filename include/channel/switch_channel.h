#pragma once

#include <cstdint>

#include "pico/stdlib.h"

enum class InputPull : uint8_t { None, Up, Down };

enum class SwitchGesture : uint8_t { None, Short, Long, Double };

class SwitchChannel {
public:
    // 去抖时间由通道按电气特性固定；手势阈值属于产品设计，因此由调用方传入。
    // 只有 detect_double 为 true 时才会使用 double_gap_ms。
    SwitchChannel(uint gpio, bool active_high, InputPull pull, bool detect_double = false,
                  uint32_t long_press_ms = kLongPressMs, uint32_t double_gap_ms = kDoubleGapMs);

    // 配置 GPIO，并将内部状态对齐到初始化时读到的电平，避免启动时误报按下或松开。
    void init(uint32_t now_ms);

    // 根据本次读数推进去抖和手势计时。
    // 调用间隔必须明显小于 kDebounceMs；如果一次按下和松开都发生在两次调用之间，程序会漏掉它们。
    void update(uint32_t now_ms);

    // 返回去抖后的电平，而非 GPIO 原始电平。执行动作时应使用此状态；read_active() 不做滤波。
    [[nodiscard]] bool is_active() const;

    // 取出一个待处理手势，优先级依次为长按、双击、短按。
    // 事件会一直保留到取出为止，因此调用方应循环读取，直到返回 None。
    SwitchGesture take_gesture();

private:
    // 按 active_high_ 将 GPIO 原始电平转换成有效状态；不做去抖或计时。
    [[nodiscard]] bool read_active() const;

    // 原始输入必须持续不变这么久，才会被认定为稳定状态。
    static constexpr uint32_t kDebounceMs = 20;
    // 触发长按所需的默认按住时间。
    static constexpr uint32_t kLongPressMs = 700;
    // 两次按压可构成双击的默认最大间隔。
    static constexpr uint32_t kDoubleGapMs = 300;

    uint gpio_;                         // 本通道读取的 SDK GPIO 编号。
    bool active_high_;                  // GPIO 高电平是否表示有效。
    InputPull pull_;                    // init() 时配置的内部上下拉。
    bool detect_double_;                // 短按是否等待后续按压以判断双击。
    uint32_t long_press_ms_;            // 触发长按所需的按住时间。
    uint32_t double_gap_ms_;            // 两次按压允许的最大间隔。
    bool candidate_active_ = false;     // 最近一次观察到的原始有效状态。
    bool stable_active_ = false;        // 去抖后对调用方公开的有效状态。
    bool was_active_ = false;           // 上一次 update() 的稳定状态。
    uint32_t last_raw_change_ms_ = 0;   // candidate_active_ 最近一次变化的时刻。
    uint32_t press_start_ms_ = 0;       // 当前按压变为稳定状态的时刻。
    uint32_t pending_short_at_ms_ = 0;  // 短按判定截止时刻；0 表示没有待判定短按。
    bool long_fired_ = false;           // 当前按压是否已经触发长按。
    bool suppress_short_ = false;       // 当前松开事件属于双击，不应再报短按。
    bool short_pressed_ = false;        // 尚未取出的短按事件。
    bool long_pressed_ = false;         // 尚未取出的长按事件。
    bool double_pressed_ = false;       // 尚未取出的双击事件。
};
