# media-controller

一个用于 **Waveshare RP2350-Zero-M** 的 Pico SDK C++ 工程。它通过 USB HID Consumer Control 向 Windows 发送标准媒体按键，不需要为 Windows 安装自定义驱动，也不需要运行串口转发程序。

板级定义使用 pico-sdk 自带的 `waveshare_rp2350_zero`（已在 `CMakeLists.txt` 中设定），它比通用的 `pico2` 更准确：flash 时钟分频为 `CLKDIV 3`（`pico2` 是 `2`），并声明了板载 RGB 灯在 `GP16`。

Windows 设备管理器里显示的产品名来自 USB 字符串描述符，目前是 `RP2350 Media Dial`，与文件夹名无关；要改就改 `src/media_hid.cpp` 的 `kStringDescriptors`。

序列号不是写死的：`media_hid_init()` 调 `pico_get_unique_board_id_string()`，从 RP2350 的 OTP 取 8 字节片内唯一 ID，转成 16 位大写十六进制填进描述符，所以每台板子的设备实例路径天然不同。这个 ID 由 `pico_unique_id` 组件在 `main()` 之前的构造函数里读好，直接调用即可；它走的是 ROM 的 `get_sys_info`，不碰 XIP flash，没有 `hardware_flash` 那套"调用期间不能有 flash 驻留中断"的约束。

三个身份字段的分工，改的时候别混：VID/PID 说明"这是什么设备"（主机据此选驱动、缓存 HID 集合），序列号说明"这是哪一台"，`bcdDevice`（`media_hid.cpp` 的 `kDeviceDescriptor`）说明"描述符是第几版"——**改任何描述符都要递增它**，否则 Windows 会继续用注册表里缓存的旧描述符，你会去追一个不存在的 bug。

## 行为

| LED 自锁按键 | 旋转 EC11 | 按下 EC11 |
| --- | --- | --- |
| 熄灭：音量模式 | 顺时针音量加；逆时针音量减 | 播放 / 暂停 |
| 点亮：切歌模式 | 顺时针下一首；逆时针上一首 | 播放 / 暂停 |

按键是**机械自锁**（锁存）型：按一下保持，再一下释放，LED 与 `SIG` 同步跟随，所以灯亮即代表当前处于切歌模式。

电脑睡眠时**转动或按下** EC11 都会先发起 USB remote wakeup 唤醒主机，动作本身在总线恢复后照常发出：转动变成音量/切歌步进，按下变成播放/暂停。代价是睡眠中误碰旋钮也会唤醒电脑并带上那几下步进，这是有意接受的取舍。挂起瞬间队列里已有的旧动作会被清空，避免电脑因别的原因醒来时打出幽灵按键；若主机根本没使能唤醒（电源管理没勾），睡眠期间的输入会被持续丢弃，不会欠到下次醒来再补发。

## 上电后需要实测的三个参数

都在 [`src/main.cpp`](src/main.cpp) 和 [`src/encoder.cpp`](src/encoder.cpp) 里，改完重新刷写：

| 现象 | 调整 |
| --- | --- |
| 顺/逆时针反了 | `kInvertEncoderDirection` 改为 `true` |
| 灯亮了却是音量模式 | `kModeSwitchActiveHigh` 改为 `false` |
| 转一格出两下（或拧一格没反应） | `src/encoder.cpp` 的 `kAccumulatorPerDetent`（每格跳变数 = 4 × 每圈脉冲 ÷ 每圈格数；微雪标 20 脉冲/圈，格数未标）。**测之前确认固件已含采样节流**（本仓库版本已内置 `kSampleIntervalMs`），否则多出来的跳变是机械抖动，你会把常量调去补偿噪声 |

A/B 相以 1 kHz 采样（`src/encoder.cpp` 的 `kSampleIntervalMs`），不会漏手拧：漏计的门槛是一个采样窗口内走满两个格雷码跳变，按每圈 80 跳变算约 25 rev/s，带格感的旋钮人手达不到；真漏了也只是少计一格，查表对非法跳转记 0，不会多出幽灵格。

## 接线

供电必须用开发板的 **3V3**，不要把模块 `VCC` 接到 `VBUS` 或 `5V`——RP2350 的 GPIO 不耐受 5V。

| 模块引脚 | RP2350-Zero-M | 用途 |
| --- | --- | --- |
| LED 自锁按键 `SIG` | `GP2` | 模式状态：低为音量，高为切歌 |
| LED 自锁按键 `NC` | 不接 | 空脚 |
| LED 自锁按键 `VCC` | `3V3` | 给模块供电 |
| LED 自锁按键 `GND` | `GND` | 共地 |
| EC11 `SIA` | `GP3` | 编码器相位 A |
| EC11 `SIB` | `GP4` | 编码器相位 B |
| EC11 `SW` | `GP5` | 编码器按键 |
| EC11 `VCC` | `3V3` | 给模块供电 |
| EC11 `GND` | `GND` | 共地 |

各模块的 `GND` 接到开发板任意一个 `GND` 共地即可。EC11 模块是微雪 **Rotation Sensor**，5 针为 `SIA` / `SIB` / `SW` / `GND` / `VCC`，编码器公共脚在板内已并到 `GND`，所以没有单独的 `C` 脚。`SIA`、`SIB`、`SW` 都是信号脚，代码已为它们打开 MCU 内部上拉：静止读高，接通地读低。

按键模块是四线 Gravity 兼容接口（`SIG` / `NC` / `VCC` / `GND`），只接三根线，`NC` 悬空。

> **已知隐患**：`GP2` 当前**没有启用任何上下拉**（`src/main.cpp` 里 `DebouncedInput` 第三参是 `InputPull::None`）。自锁开关释放时若 `SIG` 悬空，模式会随机漂移；若台架实测发现模块自带板载下拉，则保持 `None` 即可。真机若出现"没碰按键却自己换模式"，把该参改成 `InputPull::Down` 重新刷写。

**避开 `GP16`**——那是板载 WS2812 彩灯。

## 环境准备（VS Code 官方扩展）

1. 在 VS Code 扩展市场安装 Raspberry Pi 官方的 [Pico VS Code Extension](https://github.com/raspberrypi/pico-vscode)。
   注意它**只在用 VS Code 打开含 `pico_sdk_import.cmake` 的文件夹时才会激活**，所以在别的编辑器里装了不会生效。
2. `File → Open Folder` 打开本目录，`Ctrl+Shift+P` → **`Raspberry Pi Pico: Import Pico Project`**。
3. 向导里选 **SDK v2.3.1**、**ARM 工具链 15.2.Rel1**、Ninja / CMake 用默认值。几个坑：
   - **RISC-V 问"是否使用"→ 选 No**。RP2350 虽是双架构，但本项目走 ARM（`rp2350-arm-s`），TinyUSB 与调试工具链在 RISC-V 侧成熟度低。
   - **`Console over USB` 保持不勾**。它会占用 USB 接口，和本项目的自定义 HID 描述符直接冲突。
   - Board type 下拉只有 4 个第一方板子。选 `Other` 会在**窗口顶部**弹一个原生列表（不是 webview 里的输入框）；没弹出来就先随便选一个建完，再用 **`Raspberry Pi Pico: Switch Board`** 改成 `waveshare_rp2350_zero`。
4. 组件会装到 `%USERPROFILE%\.pico-sdk\`（`sdk\`、`toolchain\`、`picotool\`、`cmake\`、`ninja\`）。`CMakeLists.txt` 顶部那段 `DO NOT EDIT` 注释就是给扩展注入这些路径用的钩子，别删。

## 构建与刷写

因为上面那个钩子存在，**命令行构建不需要设任何环境变量**：

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

产物是 `build/media-controller.uf2`。

刷写：按住开发板 `BOOT` 键，用**数据线** Type-C 连电脑；出现 `RPI-RP2` U 盘后把 uf2 拖进去，开发板自动重启并在 Windows 中枚举为标准 USB 媒体控制设备。

第一次刷写前，建议先拔掉 `GP2`–`GP5` 的外设线，只确认电脑识别出 HID 设备；随后再接入输入模块测试。网易云音乐等已启用 SMTC 的播放器通常能直接响应这些系统媒体键。

## 项目结构

| 文件 | 职责 |
| --- | --- |
| `src/main.cpp` | 硬件引脚、模式选择和行为映射 |
| `src/input.h` | 去抖输入与正交编码器的类声明 |
| `src/encoder.cpp` | EC11 正交解码与输入去抖实现 |
| `src/media_hid.cpp` | TinyUSB 描述符、媒体 HID 按键队列 |
| `src/tusb_config.h` | TinyUSB 的 RP2350 / Pico SDK 配置 |

每个媒体命令发送一次"按下"报告，约 8 ms 后发送"松开"报告，Windows 才会把每格旋钮当成一次独立操作。端点轮询间隔为 1 ms（全速设备的下限，`src/media_hid.cpp` 的 `TUD_HID_DESCRIPTOR` 末参），所以吞吐的瓶颈是那个 8 ms 释放延迟，不是总线。

## 当前已知限制

以下是已识别但尚未处理的问题，不影响基本功能：

- **唤醒是否生效取决于主机**：固件已实现 remote wakeup（睡眠中转动或按下 EC11 都会发起），但 Windows 不一定为 consumer-control 类设备显示或默认勾选"允许此设备唤醒计算机"，Modern Standby 的机器可能整体忽略 USB 唤醒。上板后在设备管理器 → 该设备 → 电源管理 里确认；没勾上的话睡眠中操作旋钮不会有任何反应。
- **VID 仍是 TinyUSB 示例值 `0xCAFE`**，不是分配给你的；PID 已自定义为 `0x40A1`，避开 HID 例程默认的 `0x4001`（同一台电脑上插另一块跑该例程的板子才会撞实例路径）。个人学习和桌面原型可以使用（序列号已唯一，两台设备不会再互相顶掉同一个实例）；若对外销售，需申请或从 USB-IF 成员/芯片厂渠道取得合适的 VID，并更新 `src/media_hid.cpp` 的 `kDeviceDescriptor`。
