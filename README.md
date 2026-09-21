# RP2350 Media Dial

一个用于 `Waveshare RP2350-Zero-M` 的 Pico SDK C++ 工程。它通过 USB HID Consumer Control 向 Windows 发送标准媒体按键，不需要为 Windows 安装自定义驱动，也不需要运行串口转发程序。

## 行为

| LED 锁存按钮 | 旋转 EC11 | 按下 EC11 |
| --- | --- | --- |
| 熄灭：音量模式 | 顺时针音量加；逆时针音量减 | 播放 / 暂停 |
| 点亮：切歌模式 | 顺时针下一首；逆时针上一首 | 播放 / 暂停 |

如果实际方向反了，把 [`src/main.cpp`](src/main.cpp) 中的 `kInvertEncoderDirection` 改成 `true` 后重新刷写。若你的按钮模块实际呈现“亮了却是音量模式”，把同一文件中的 `kModeSwitchActiveHigh` 改成 `false`。

## 接线

供电必须使用开发板的 **3V3**，不要把模块的 `VCC` 接到 `VBUS` 或 `5V`。RP2350 的 GPIO 不耐受 5V。

| 模块引脚 | RP2350-Zero-M | 用途 |
| --- | --- | --- |
| LED 锁存按钮 `SIG` | `GP2` | 模式状态：低为音量，高为切歌 |
| LED 锁存按钮 `VCC` | `3V3` | 给模块供电 |
| LED 锁存按钮 `GND` | `GND` | 共地 |
| EC11 `CLK` / `S1A` | `GP3` | 编码器相位 A |
| EC11 `DT` / `S1B` | `GP4` | 编码器相位 B |
| EC11 `SW` | `GP5` | 编码器按键 |
| EC11 `VCC` | `3V3` | 给模块供电 |
| EC11 `GND` | `GND` | 共地 |

把模块之间的 `GND` 都接到开发板任意一个 `GND` 即可。普通 EC11 的 `CLK`、`DT`、`SW` 都是信号脚，代码已为它们打开 MCU 内部上拉；静止时读高，接通地时读低。

按钮模块若是旧式四线 Gravity 接口，`NC` 是不接的脚；使用随模块附带的三线转接线，或只接 `SIG/VCC/GND` 三根线。

## 编译与刷写

1. 在 VS Code 安装 Raspberry Pi 官方的 [Pico VS Code Extension](https://github.com/raspberrypi/pico-vscode)，让它下载 Pico SDK、ARM 工具链和 CMake 工具。
2. 用 VS Code 打开这个 `media-dial` 文件夹，按扩展的 **Import Project** / **Configure** 完成 CMake 配置。选择 RP2350/Pico 2 的 SDK 目标即可；本项目已在 `CMakeLists.txt` 中设定 `PICO_BOARD=pico2`。
3. 运行 CMake Build。生成文件是 `build/media_dial.uf2`。
4. 按住开发板 `BOOT` 键，再用数据 Type-C 线连电脑；出现 `RPI-RP2` U 盘后，把 `media_dial.uf2` 拖进去。开发板会自动重启并在 Windows 中显示为标准 USB 媒体控制设备。

也可以用命令行配置，其中 `PICO_SDK_PATH` 是扩展下载的 Pico SDK 路径：

```powershell
cmake -S . -B build -DPICO_SDK_PATH=C:\path\to\pico-sdk
cmake --build build
```

第一次刷写前，建议先拔掉 `GP2` 到 `GP5` 的外设线，只确认电脑识别 USB HID 设备；随后接入输入模块测试。网易云音乐已打开 SMTC 时，通常能响应这些系统媒体键。

## 项目结构

| 文件 | 职责 |
| --- | --- |
| `src/main.cpp` | 硬件引脚、模式选择和行为映射 |
| `src/encoder.cpp` | EC11 正交解码与输入去抖 |
| `src/media_hid.cpp` | TinyUSB 描述符、媒体 HID 按键队列 |
| `src/tusb_config.h` | TinyUSB 的 RP2350/Pico SDK 配置 |

每个媒体命令都会发送一次“按下”报告，约 8 ms 后发送“松开”报告。这样 Windows 才会把每格旋钮当成一次独立的音量或切歌操作。

## 原型与产品的边界

当前 USB 设备描述符用的是 TinyUSB 示例的原型 `VID/PID`（`0xCAFE/0x4001`）。个人学习和桌面原型可以使用；若将设备对外销售，需要申请或从 USB-IF 成员/芯片厂渠道取得合适的 USB VID/PID，并更新 `src/media_hid.cpp`。
