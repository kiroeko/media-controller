# 音乐控制器外壳：当前版本

**用 SOLIDWORKS 2026 打开 [cad/Music_Controller.SLDASM](cad/Music_Controller.SLDASM)。这是唯一的整机入口。**

本目录是 2026-09-27 整理后的当前设计：20° 斜面、左旋钮右按钮、平整面板、内部固定、后部居中 USB-C。它是待实物试装的样件，尚未打印验证。

| 要做什么 | 使用哪个文件 |
|---|---|
| 查看、继续设计整机 | `cad/Music_Controller.SLDASM` |
| 修改底壳 | `cad/Base_Smooth_20deg.SLDPRT` |
| 修改上盖 | `cad/Lid_Smooth_20deg.SLDPRT` |
| 修改旋钮模块托架 | `cad/Encoder_Cradle.SLDPRT` |
| 修改按钮模块托架 | `cad/Button_Cradle.SLDPRT` |
| 3D 打印 | `print` 中的四个 STL，各一件，mm、100% 比例 |
| 看装配尺寸、螺丝与打印方向 | [装配与打印](docs/ASSEMBLY.md) |
| 处理嘉立创 STEP 保存问题 | [STEP 导入说明](docs/STEP_IMPORT.md) |

`cad` 中的六个 `REF_` 零件是整机需要的电子器件参考，不是旧版，也不用打印。请把整个 `cad` 文件夹一起保留或复制，不要单独搬走装配体。

目录只保留当前 CAD、打印文件和说明。旧版、重复导出、临时脚本与测试文件已删除，原 Pictures 工作目录也已清理。无需运行宏或导入 STEP。

本次已实际关闭全部文档，再从新目录重新打开：10 个组件全部从 `cad` 加载，打开及再次保存均为 0 错误、0 警告。载板参考为独立实体零件，没有对嘉立创 STEP 的外部文件依赖。详见 [检查记录](docs/VERIFICATION.txt)。
