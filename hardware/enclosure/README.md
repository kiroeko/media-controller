# 音乐控制器外壳：当前版本

**用 SOLIDWORKS 2026 打开 [cad/Music_Controller.SLDASM](cad/Music_Controller.SLDASM)。这是唯一的整机入口。**

本目录是 2026-09-27 整理后的当前设计：20° 斜面、左旋钮右按钮、平整面板、内部固定、后部居中 USB-C。它是待实物试装的样件，尚未打印验证。

**当前底壳 USB-C 改为圆角内凹结构：外槽 13 × 8 mm、深 1.2 mm，内孔 10.5 × 5 mm，槽底厚 1.5 mm。** 按实测 11.26 × 6.43 mm 线头包胶、插到底后包胶前沿距插座口 1.8 mm 设计，包胶距槽底名义余量约 1.06 mm。小孔周围的槽底遮挡内部，保留装配间隙。重新提交打印审核使用 [print/Base_Smooth_20deg.STL](print/Base_Smooth_20deg.STL)，无需另找新版文件夹。[查看原生装配预览](docs/USB_RECESS.png)。

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

10 个组件全部从 `cad` 加载。载板参考为独立实体零件，没有对嘉立创 STEP 的外部文件依赖。最新 USB 修订后，底壳重新打开为 0 错误、0 警告；整机打开为 0 错误，API 返回需要重建提示（32），执行重建成功，保存为 0 错误、0 警告。详见 [检查记录](docs/VERIFICATION.txt)。
