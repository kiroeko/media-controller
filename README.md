# media-controller

这是一个用于 **Waveshare RP2350-Zero-M** 的 USB 媒体旋钮固件。它读取 Waveshare Rotation Sensor 和 YFROBOT LED 自锁开关，通过 TinyUSB 向电脑发送标准 HID Consumer Control 媒体按键。电脑不需要安装本项目专用驱动。

## 预期使用方式

| LED 自锁开关 | 顺时针旋转 | 逆时针旋转 | 短按旋钮 | 长按旋钮 |
| --- | --- | --- | --- | --- |
| 熄灭：音量模式 | 音量加 | 音量减 | 播放/暂停 | 静音切换 |
| 点亮：切歌模式 | 下一首 | 上一首 | 播放/暂停 | 静音切换 |

这是固件当前的目标映射。LED 自锁开关单独通电时，已测得灯灭时 `SIG` 对 `GND` 稳定约为 0 V、灯亮时约为 3.3 V，与固件设定的高电平有效极性一致。装好整机后，还需转动旋钮核对旋转方向，以及灯灭调音量、灯亮切歌的实际效果。

短按在去抖后的松开时识别；长按在去抖后的按下状态持续 700 ms 后识别。手势解码器支持双击，但当前关闭该功能，250 ms 双击窗口不参与判断，也没有绑定媒体动作。

当主机挂起 USB 且允许远程唤醒时，新输入会尝试唤醒主机；成功入队的动作在总线恢复后继续发送。能否唤醒整机睡眠取决于主机配置，不能保证所有睡眠状态都可唤醒。

## 接线

两个模块都接开发板的 **3V3**，并与开发板共地；本项目不从 `5V` 或 `VBUS` 给模块供电。[RP2350 数据手册](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)指出，使用的 GP2～GP5 属于容忍较高输入电压的 FT GPIO，且仅在 IOVDD 已供 3.3V 时才能容忍最高 5.5V 输入。统一使用 3.3V 供电可避免依赖这一条件及不同电源的上电顺序。

| 模块 | 信号 | RP2350-Zero-M |
| --- | --- | --- |
| LED 自锁开关 | `SIG` | `GP2` |
| EC11 Rotation Sensor | `SIA`（A 相） | `GP3` |
| EC11 Rotation Sensor | `SIB`（B 相） | `GP4` |
| EC11 Rotation Sensor | `SW`（内置按键） | `GP5` |
| 两个模块 | `VCC`、`GND` | `3V3`、`GND` |

LED 自锁开关的 `NC` 不接。EC11 模块的 A/B/SW 信号由模块板上拉高，固件不启用 MCU 内部上下拉。模式开关的 `SIG` 也不启用内部上下拉；单独供电实测灯灭时 `SIG` 对 `GND` 稳定约为 0 V，灯亮时约为 3.3 V。

开发板的排针丝印是裸数字，例如丝印 `3` 对应代码中的 `GP3`。接线时请以板上丝印和 [Waveshare RP2350-Zero 官方资料](https://www.waveshare.com/wiki/RP2350-Zero)中的引脚图为准，尤其要区分 `5V`、`GND` 与 `3V3`。

## 代码怎样运行

```text
main.cpp
  └─ MediaControllerApp::run()：初始化并持续轮询
       ├─ YfrobotLedLatchingSwitch / WaveshareRotationSensor：配置 GPIO、读取模块、控制采样节奏
       │    └─ DebouncedSwitch / ButtonGestureDecoder / QuadratureDecoder：只处理采样值
       ├─ 把模式、旋转格数和按键手势映射为媒体动作
       └─ media_hid：把动作排队，经 TinyUSB 发送 HID 报告
```

`main.cpp` 只创建应用对象并启动它。板上引脚编号和“旋转代表音量还是切歌”的规则集中在应用模块。器件类持有引脚，知道模块的高低电平、上下拉和采样节奏，向应用返回稳定状态、手势或机械卡点数。纯输入处理类接收采样值，按需要使用时间戳，不依赖 Pico SDK。`media_hid` 管理 USB 描述符、动作队列以及一次媒体按键的“按下 → 松开”报告。

| 位置 | 职责 |
| --- | --- |
| [src/main.cpp](src/main.cpp) | 固件入口 |
| [src/app/media_controller_app.cpp](src/app/media_controller_app.cpp) | 初始化、主循环、板级接线与产品行为映射 |
| [src/device/waveshare_rotation_sensor.cpp](src/device/waveshare_rotation_sensor.cpp) | 配置并读取 Waveshare 旋钮模块的 A/B/SW，A/B 两次采样至少间隔 1 ms |
| [src/device/yfrobot_led_latching_switch.cpp](src/device/yfrobot_led_latching_switch.cpp) | 配置并读取 YFROBOT LED 自锁开关的 SIG |
| [src/input/quadrature_decoder.cpp](src/input/quadrature_decoder.cpp) | 把两位 A/B 相位变化转换为带符号的机械卡点数 |
| [src/input/debounced_switch.cpp](src/input/debounced_switch.cpp) | 将原始有效/无效采样去抖，供模式开关和旋钮按键复用 |
| [src/input/button_gesture_decoder.cpp](src/input/button_gesture_decoder.cpp) | 从去抖后的按下/松开状态识别短按、长按、双击 |
| [include/input/button_gesture.h](include/input/button_gesture.h) | 器件、解码器和应用共用的手势事件与时间配置 |
| [src/usb/media_hid.cpp](src/usb/media_hid.cpp) | TinyUSB 描述符、HID 媒体报告与发送队列 |
| [include/usb/tusb_config.h](include/usb/tusb_config.h) | TinyUSB 编译配置；CMake 将此目录加入头文件搜索路径 |

这里没有额外包装 GPIO、SPI、USB 的通用“物理层”；底层访问直接使用 Pico SDK 和 TinyUSB。以后接屏幕时，屏幕驱动负责面板命令和总线传输；只有像素转换或渲染逻辑变复杂时，才需要单独提取不依赖硬件的编码组件。

各 `.cpp` 文件里的 `constexpr` 引脚、时序和转换表只供该文件使用，属于只读配置。[media_hid.cpp](src/usb/media_hid.cpp) 将 USB 描述符保存为文件内常量；`MediaHidState` 则保存序列号、字符串描述符缓冲区、动作队列和按键发送状态，供 USB 回调在固件运行期间使用。这些状态不暴露给应用层。

### 从旋钮到电脑的一次操作

1. `WaveshareRotationSensor` 在主循环中至少间隔 1 ms 才再次读取 SIA/SIB，组成两位相位状态；SW 在主循环每轮读取。主循环如果延迟，A/B 相的采样也会变慢，不会补读中间状态。
2. `QuadratureDecoder` 查格雷码状态变化，每凑满一格就累计一次正向或反向计数。按本项目的 SIA→GP3、SIB→GP4 接线，查表约定使顺时针卡点为正、逆时针为负。当前主循环每轮最多更新解码器一次，随后立即调用 `take_detents()`，所以此处每轮只会得到 `-1`、`0` 或 `+1`；如果其他调用方连续更新多次后才取走计数，才可能一次得到多格。`DebouncedSwitch` 用时间戳去抖，`ButtonGestureDecoder` 再识别手势。模式自锁开关只使用去抖结果。
3. `MediaControllerApp` 根据模式开关状态，把卡点映射为音量或切歌动作，把短按/长按映射为播放暂停/静音。同一轮先排入按键动作，再排入旋转动作。
4. `media_hid` 向 TinyUSB 提交 HID Consumer Control 报告；每个动作先发送按下，至少 8 ms 后且 HID 端点就绪时再发送松开。

应用循环不能长时间阻塞，否则会漏掉旋钮相位或按键变化。旋钮的 `take_detents()` 返回的是机械卡点数，不是完整转了几圈。

USB 动作队列最多容纳 15 个待发送动作，其中一个名额为按键动作预留：旋转动作最多占 14 个名额，按键可使用第 15 个名额。按键在同一轮也先于旋转动作入队。预留名额只影响能否入队，已入队的动作仍按先后顺序发送。若输入速度持续超过 HID 发送速度，新动作仍会在容量耗尽时被舍弃；应用不会阻塞或为被舍弃的动作另行补发。因此停转后可能还有短暂的队列延迟。连续按键也可能占满全部名额。

## 上板后要核对

| 现象 | 调整位置 |
| --- | --- |
| 刷入新固件后顺逆时针仍相反 | 核对 `SIA`→`GP3`、`SIB`→`GP4` 接线和 [quadrature_decoder.cpp](src/input/quadrature_decoder.cpp) 的方向查表 |
| 灯亮时仍处于音量模式 | 检查 [yfrobot_led_latching_switch.cpp](src/device/yfrobot_led_latching_switch.cpp) 的 `kSigActiveHigh` |
| 一格触发多次，或多格才触发一次 | 实测后调整 [waveshare_rotation_sensor.cpp](src/device/waveshare_rotation_sensor.cpp) 的 `kTransitionsPerDetent` |
| 快速旋转漏格 | 检查主循环是否阻塞、USB 队列是否满，以及 [waveshare_rotation_sensor.cpp](src/device/waveshare_rotation_sensor.cpp) 的 `kEncoderSampleIntervalMs` |

[Waveshare Rotation Sensor 的规格页](https://www.waveshare.com/wiki/Rotation_Sensor)标称每圈 20 个脉冲，但没有给出机械卡点数；`kTransitionsPerDetent = 4` 是当前的换算值，应以实物操作结果确认。应用模块的 `kButtonGestureConfig` 设置长按阈值为 700 ms，250 ms 双击窗口当前未启用。原固件在本项目接线下的旋转方向与预期相反，现已互换方向查表的正负号，刷入后需确认效果。

## 构建与刷写

项目使用 Pico SDK v2.3.1、ARM 工具链和 Ninja。`CMakeLists.txt` 选择 pico-sdk 自带的 `waveshare_rp2350_zero` 板级定义；VS Code 用户可以通过 Raspberry Pi Pico 官方扩展导入本目录。CMake 文件顶部的 VS Code 扩展钩子由扩展管理。

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

编译产物是 `build/media-controller.uf2`。按住开发板 `BOOT` 键，通过数据线连接电脑；出现 `RPI-RP2` 盘后，将 UF2 文件复制进去，开发板会重启并枚举为 USB HID 媒体控制设备。

第一次刷写时可以先拔下 GP2～GP5 的外设线，只确认电脑能识别 HID 设备，再接回模块核对方向、模式和按键手势。

USB 产品名当前是 `Kiro Media Controller`，定义在 [media_hid.cpp](src/usb/media_hid.cpp) 的字符串描述符中。每块板子的序列号由 RP2350 唯一 ID 生成。首次刷写前修改名称不会遇到旧设备缓存；以后若在相同 VID/PID/序列号下修改描述符，建议同步更新 `kDeviceDescriptor` 中的 `bcdDevice`，并让主机重新枚举设备。

当前 VID `0xCAFE` 是 TinyUSB 示例值，并非分配给本项目；对外销售时需要更换为合法取得的 VID/PID。远程唤醒也需要电脑允许，Windows 设备管理器不一定会为此类设备提供或启用唤醒选项。
