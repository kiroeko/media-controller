# media-controller

这是一个用于 **Waveshare RP2350-Zero-M** 的 USB 媒体旋钮固件。它读取 Waveshare Rotation Sensor 和 YFROBOT LED 自锁开关，通过 TinyUSB 向电脑发送标准 HID Consumer Control 媒体按键。电脑不需要安装本项目专用驱动。

## 使用方式

| LED 自锁开关 | 顺时针旋转 | 逆时针旋转 | 短按旋钮 | 长按旋钮 |
| --- | --- | --- | --- | --- |
| 熄灭：音量模式 | 音量加 | 音量减 | 播放/暂停 | 静音切换 |
| 点亮：切歌模式 | 下一首 | 上一首 | 播放/暂停 | 静音切换 |

旋转方向、模式切换、按键动作和切歌冷却机制已在整机上验证。当前冷却时间延长至 500 ms，时长的实际手感还需上板确认。LED 自锁开关单独通电时，测得灯灭时 `SIG` 对 `GND` 稳定约为 0 V、灯亮时约为 3.3 V，与固件设定的高电平有效极性一致。

在目前使用的 Windows 电脑上，一次音量动作会让系统显示值变化 2；电脑自带的独立音量键也是相同步长。这是主机侧的音量刻度，不能据此判断固件发送了两次动作。

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

### 初始化约定

`input` 和 `device` 类型使用默认构造，不显式定义构造函数，也不在构造时访问硬件。调用方通过一次 `init()` 提供所需的外部配置和初始状态。对象默认构造后仍须先调用 `init()`，才能调用 `update()` 或读取结果。

1. `MediaControllerApp::run()` 先调用 `board_init()`，再把接线配置和起始时间交给设备的 `init()`。旋转模块用自身的 `WaveshareRotationSensor::Config` 收拢三个引脚和手势规则；模式开关直接接收 SIG 引脚。
2. 设备的 `init()` 配置 GPIO、读取初始电平，再调用内部输入组件的 `init()`。模块自身的采样间隔、去抖时长和每格跳变数仍由设备实现定义。
3. 输入组件的 `init()` 一次接收算法配置、初始采样以及需要的时间戳，建立状态基准并清空累计结果或待取事件。后续 `update()` 持续处理新采样。

接口参数统一将配置放在前面，时间戳和初始采样放在后面。例如 `DebouncedSwitch::init(debounce_ms, now_ms, raw_active)`，以及 `ButtonGestureDecoder::init(config, now_ms, pressed)`。

### 从旋钮到电脑的一次操作

1. `WaveshareRotationSensor` 在主循环中至少间隔 1 ms 才再次读取 SIA/SIB，组成两位相位状态；SW 在主循环每轮读取。主循环如果延迟，A/B 相的采样也会变慢，不会补读中间状态。
2. `QuadratureDecoder` 查格雷码状态变化，每凑满一格就累计一次正向或反向计数。按本项目的 SIA→GP3、SIB→GP4 接线，查表约定使顺时针卡点为正、逆时针为负。当前主循环每轮最多更新解码器一次，随后立即调用 `take_detents()`，所以此处每轮只会得到 `-1`、`0` 或 `+1`；如果其他调用方连续更新多次后才取走计数，才可能一次得到多格。`DebouncedSwitch` 用时间戳去抖，`ButtonGestureDecoder` 再识别手势。模式自锁开关只使用去抖结果。
3. `MediaControllerApp` 根据模式开关状态，把卡点映射为音量或切歌动作，把短按/长按映射为播放暂停/静音。同一轮先排入按键动作，再排入旋转动作。切歌冷却属于产品交互规则，因此在应用层判断；输入解码器仍处理每个相位变化。
4. `media_hid` 向 TinyUSB 提交 HID Consumer Control 报告；每个动作先发送按下，至少 8 ms 后且 HID 端点就绪时再发送松开。

应用循环不能长时间阻塞，否则会漏掉旋钮相位或按键变化。旋钮的 `take_detents()` 返回的是机械卡点数，不是完整转了几圈。

### 切歌冷却与动作队列

`MediaControllerApp` 不保存待发送动作的队列；它决定输入对应什么媒体动作，再调用 `media_hid_enqueue()`。真正的待发送队列位于 `media_hid`，用来让主循环在 USB 按键发送期间继续读取输入。

1. 音量模式每个有效卡点尝试排入一次音量动作。切歌模式成功排入一次下一首或上一首后，开始固定的 500 ms 冷却期；期间的正反向旋转仍被采样和解码，但不产生切歌动作，也不会留到冷却结束后补发。被忽略的旋转不延长冷却期。只有成功入队才启动冷却；音量与旋钮按键不受它限制。
2. 同一轮同时识别到按键和旋转时，播放/暂停或静音先入队，旋转动作后入队。队列始终按入队顺序发送：新按键不会越过之前排队的旋转动作。
3. 队列使用 16 个数组位置，其中一个保持空闲以区分队列空和满，因此最多有 15 个**待发送**动作。当已有 14 个待发送动作时，旋转动作不能再入队，为按键留出最后一个位置；按键可使用该位置。队列满时新动作直接丢弃，不阻塞、不重试，也不合并或取消旧动作；连续按键同样可能占满队列。
4. USB 层按顺序为每个动作发送“按下”报告，至少 8 ms 后在 HID 端点就绪时发送“松开”报告，然后处理下一个动作。按下报告提交成功时，该动作就从待发送队列移除。切歌冷却从**入队时**计算，不等电脑实际收到动作；若队列积压，停转后仍可能继续发送先前入队的动作。

## 故障排查

| 现象 | 调整位置 |
| --- | --- |
| 顺逆时针相反 | 核对 `SIA`→`GP3`、`SIB`→`GP4` 接线和 [quadrature_decoder.cpp](src/input/quadrature_decoder.cpp) 的方向查表 |
| 灯亮时仍处于音量模式 | 检查 [yfrobot_led_latching_switch.cpp](src/device/yfrobot_led_latching_switch.cpp) 的 `kSigActiveHigh` |
| 一格触发多次，或多格才触发一次 | 实测后调整 [waveshare_rotation_sensor.cpp](src/device/waveshare_rotation_sensor.cpp) 的 `kTransitionsPerDetent` |
| 快速旋转漏格 | 检查主循环是否阻塞、USB 队列是否满，以及 [waveshare_rotation_sensor.cpp](src/device/waveshare_rotation_sensor.cpp) 的 `kEncoderSampleIntervalMs` |

[Waveshare Rotation Sensor 的规格页](https://www.waveshare.com/wiki/Rotation_Sensor)标称每圈 20 个脉冲，但没有给出机械卡点数；当前使用 `kTransitionsPerDetent = 4`，实机操作得到预期的每格动作。应用模块的 `kButtonGestureConfig` 设置长按阈值为 700 ms，250 ms 双击窗口当前未启用。方向查表的符号也已按 SIA→GP3、SIB→GP4 接线调整并完成实机验证。

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
