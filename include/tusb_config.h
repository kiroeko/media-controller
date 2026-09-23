#pragma once

#include "tusb_option.h"  // 引入 TinyUSB 的选项常量，例如 OPT_MCU_RP2040 和 OPT_OS_PICO。

// CFG_TUSB_MCU 选择 TinyUSB 的硬件驱动后端，不是声明芯片型号。
// Pico SDK/TinyUSB 使用 RP2040 USB 驱动支持 RP2350，因此此处选择 OPT_MCU_RP2040。
#define CFG_TUSB_MCU             OPT_MCU_RP2040  // 选择 TinyUSB 使用的 MCU/USB 控制器后端。
#define CFG_TUSB_OS              OPT_OS_PICO  // 使用 Pico SDK 的平台适配；本项目不依赖其他 RTOS。
#define CFG_TUSB_RHPORT0_MODE    (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)  // USB 端口 0 作为全速设备连接电脑，不作为 USB 主机。
#define CFG_TUD_ENDPOINT0_SIZE   64  // 控制端点 EP0 的最大数据包大小；EP0 用于枚举和标准控制请求。

#define CFG_TUD_HID              1  // 启用 1 个设备端 HID 接口实例。
                                    // 电脑实际看到的接口仍由 media_hid.cpp 中的描述符声明。
#define CFG_TUD_HID_EP_BUFSIZE   64  // TinyUSB HID 报告缓冲区大小；本项目的 HID 端点描述符也使用此值作为包大小。
