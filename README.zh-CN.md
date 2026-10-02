# AMDNR — AMD 显卡上的 DLSS 5 神经渲染（OptiScaler 构建版）— v0.3.5

[English](README.md) | **中文** | [Português](README.pt-BR.md) | [Español](README.es.md) | [العربية](README.ar.md) | [Français](README.fr.md) | [Italiano](README.it.md) | [Русский](README.ru.md) | [Polski](README.pl.md)

> **我们需要你的支持。** 加入 Discord 服务器 —— <https://discord.gg/AMDNR> —— 获取帮助、提交
> 问题、领取测试版；每一份带日志的反馈都会让下一个版本更好。

DLSS 5 神经渲染（Neural Rendering）在 AMD 显卡上运行，内置于 OptiScaler，因此 OptiScaler 能挂钩的
任何 Direct3D 12 游戏都可以使用。在神经渲染之上还有：大幅提升帧率的模型交错（model interleave）、
残差合成（residual composition）、解锁至 6X 的 XeSS 帧生成（D3D12 游戏可选开启至 10X），以及面向使用
DLSS 光线重建游戏的 FSR Ray Regeneration。自 0.3.5 起，**AMDNR Anywhere**（preview）通过 AMDNR Launcher 把神经渲染
带到自身没有任何放大器的游戏中，且不会向游戏目录写入任何东西（见"AMDNR Anywhere"）。

**Discord：<https://discord.gg/AMDNR>** —— 支持、问题反馈（`#bug-report`）、测试版。

**支持本项目：<https://ko-fi.com/3zinr>**

> **danielblnc 运行时是 Daniel Blanco 的作品。** `*Runtime.zip` 文件中的 AMD 神经运行时（`dlssnr_amd_pass1..3.dll`）是
> **DLSS-NR on AMD by Daniel Blanco (danielblnc)** —— <https://github.com/danielblnc/DLSS-NR-on-AMD>。
> Copyright (c) 2026 Daniel Blanco, all rights reserved. AMDNR 经他许可，未经修改地分发它；它不是 AMDNR 的作品。
> 请支持他的项目。其他所有人的完整致谢见本页末尾。

> **0.3.5 新内容：** **AMDNR Anywhere**（preview）：为自身没有 DLSS、XeSS 或 FSR 2 的游戏提供神经渲染——AMDNR Launcher
> 中只需一个 **PLAY ANYWHERE** 按钮，不向游戏目录写入任何东西；本次发布支持 RX 9000（RDNA 4）（见"AMDNR Anywhere"）。
> **Ray Regeneration 有了自己的选项卡**，紧跟在 Upscaling 之后，其状态行会按显卡、按 API 说明什么在运行、为什么没有运行；在 RX 7000 上，自 0.3.5.1 起它再次以
> preview 形式提供并默认开启（AMD 的降噪器没有面向 RDNA 3 的提供程序，因此画面在不带降噪的情况下被升采样；Upscaling 选项卡中的
> 勾选框或 `[FSR-RR] FfxDenoiserAllowPreRdna4=false` 可将其关闭，游戏便保留自己的降噪器）。
> **神经通道可以在放大之后运行**（`[DlssNr] AmdPlacement=post`，Neural > Performance > Placement；默认的 `pre` 不变）。
> **Frame Gen 选项卡会说明为什么没有生成任何帧**，并列出五个步骤（见帧生成的常见问题）。**在 RX 7000 和 Z1 Extreme
> 级掌机上更快：**网络时间约少 10%，画面逐位相同（`LmxxfNrRuntime.pak` 中的 0.3.5 模块集）。**RX 9000 上默认启用 Kien 的 lmxxf 0.37（MIT）：**
> RX 9070 XT 上的网络时间约少 20%（在 RX 9060 / 9060 XT 上为实验性；`[DlssNr] AmdLmxxfL37=false` 可将其关闭）。
> 在 RDNA 3 掌机上，**FSR 4 (INT8)** 作为需手动开启的实验性选项提供；自定义的 Style slots 现在会保存完整的外观。
> 动态分辨率游戏不再在每一步都重建网络；在掌机上开启 Model interleave 移动时，被携带的编辑不再消失；Uncharted: Legacy of
> Thieves 在 RX 9000 上不再崩溃；此外还有大量修复。**三个文件都有变化：请同时替换 `OptiScaler.dll`、
> `LmxxfNrRuntime.dll` 和 `LmxxfNrRuntime.pak`；启动器用户：它会自动为你更新。**详情见
> `CHANGELOG.md`。

> **0.3.4.2 新内容（热修复）：** Assetto Corsa 中的菜单：菜单键每按一次只切换一次，短于一帧的点击不再丢失，运行时选择
> 窗口可用 `1` / `2` / `Enter` / `Esc` 操作、标题栏有 X 按钮，关闭菜单即视为"Decide later"。运行时选择窗口不再自行打开
> 菜单（改为一条通知）；Neural 选项卡中的 **Ray Regeneration** 区块不再隐藏——它始终在那里，并用一行浅色文字说明它为什么
> 没有运行；Wine / Proton 的文字改为：Ray Regeneration 在那里是已知问题。AMDNR 还**多接受一种 danielblnc 运行时布局**：
> 更新的 danielblnc 版本无需再等 AMDNR 更新就能在这里运行。**除这一行已接受的布局之外，神经渲染与
> 0.3.4.1 逐字节相同**
> （神经通道、两个运行时和 pak 均未改变）：从 0.3.4.1 或 0.3.4 升级只需替换 `OptiScaler.dll`；启动器用户：它会自动更新。
> 详情见 `CHANGELOG.md`。

> **0.3.4.1 新内容（热修复）：** Windows 上的 Ray Regeneration 不再那么软（游戏没有传入锐化值时加 0.25 的锐化；
> 关闭方法：Image > Sharpness，勾选 Override，滑块拉到 0）；Control Resonant 中不再误弹 "Upscaler failed to run!" 提示；
> 在 Linux / Proton 上菜单可以正常使用（已由一位玩家确认，开启帧生成时也可以），且 Ray Regeneration 在那里不添加默认
> 锐化（见"Linux / Proton"一节）。**AMDNR Launcher 0.3.4.1**，根据你们在 Discord 上的反馈打造：九种语言、搜索、收藏、
> 隐藏、重命名、CHOOSE GAME .EXE、PLAY、完整的 UNINSTALL 等（见"AMDNR Launcher"一节）。神经渲染与 0.3.4 相同（运行时和 pak
> 都没变）：从 0.3.4 升级只需替换 `OptiScaler.dll`；使用启动器的用户：它会自动为你更新。详情见 `CHANGELOG.md`。

> **0.3.4 新内容：** 全新的菜单（重做的 Neural 选项卡、所有选项卡统一的外观，以及把日志打包成 zip 用于反馈的
> **Save report** 按钮）；lmxxf 在 RX 7000 上更快（1440p FSR Quality：RX 7800 XT 上每次网络运行 73.3 -> 52.2 ms，网络时间，在游戏外测得），
> 在 RX 9070 / 9070 XT 上也更快（lmxxf 0.31 内核）；lmxxf 可在掌机 APU 上运行（实验性；一位测试者在游戏中的一次运行：ROG Ally 上的 Shadow of the Tomb Raider 约 29 fps）；lmxxf
> 新增可选的 **Fast mode**；**AMDNR Screen GI**，AMDNR 自己的屏幕空间 GI（preview，默认关闭）；以及大量修复。
> 请同时替换 `OptiScaler.dll`、`LmxxfNrRuntime.dll` 和 `LmxxfNrRuntime.pak`。详情见 `CHANGELOG.md`。

---

## AMDNR - OptiScaler 安装指南

安装非常简单。**在 Windows 上，AMDNR Launcher 会替你完成下面的全部步骤**（见下文"AMDNR Launcher"一节）。
手动安装的步骤如下：

### 1. 下载文件

从 GitHub 上的最新发布页下载这些文件（<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>；
0.3.5 对应的标签是 Alpha0.3.5）：

* `AMDNR-vX.X.X.zip`（0.3.5 为 `AMDNR-v0.3.5.zip`），内含完整的 lmxxf 运行时。
* 如需 danielblnc 运行时，再下载一个运行时 zip：**RX 9000 和 RX 7000** 都用 Alpha0.3.4.2 上的
  `v0.5.0-Runtime.zip`（推荐）；`v0.4.3-Runtime.zip`（Alpha0.3.4.2 和 Alpha0.3.4.1）、`v0.4.1-Runtime.zip` 和
  `v0.4.0-Runtime.zip`（Alpha0.3.4.1）仍然可用。lmxxf 运行时包含在
  `AMDNR-vX.X.X.zip` 中，在 RX 7000 和 RX 9000 上不需要任何运行时 zip；掌机 APU 只使用 lmxxf。AMDNR Launcher
  会为你的显卡选好正确的 zip。见"压缩包里有什么"。

### 2. 解压两个文件

将两个 `.zip` 文件的内容解压出来。

### 3. 把所有文件复制到游戏目录

先把 `AMDNR-vX.X.X` 里的全部文件复制到游戏根目录 —— 也就是游戏 `.exe` 所在的文件夹。

然后把运行时 zip（例如 RX 9000 和 RX 7000 上的 `v0.5.0-Runtime`）里的全部文件同样复制过去。

> **从旧版 AMDNR 升级？** 重新复制全部文件并覆盖。**0.3.5 中有三个文件一起更新了：**`OptiScaler.dll`（用新文件替换你
> 重命名过的那个，例如 `dxgi.dll`，并按同样方式重命名）、`LmxxfNrRuntime.dll` 和 `LmxxfNrRuntime.pak`
> （440 MB）。不要与旧副本混用。你可以保留自己的 `OptiScaler.ini`：新设置会使用默认值。你的 danielblnc 运行时 zip
> 中的文件保持不变。AMDNR Launcher 会替你完成这一步：点 UPDATE ALL，或在显示 "Update available" 的游戏上点
> REPAIR / UPDATE（启动器会先更新自己）。

### 4. 重命名 OptiScaler.dll

在游戏目录里找到：

`OptiScaler.dll`

重命名为：

`dxgi.dll`

推荐使用 `dxgi.dll`。

如果游戏无法启动或模组没有加载，改用下面的名字之一重命名 `OptiScaler.dll`：

* `d3d12.dll`
* `winmm.dll`
* `version.dll`
* `dbghelp.dll`
* `winhttp.dll`
* `wininet.dll`

一次只试一个名字。不要同时保留多份 `OptiScaler.dll` 的副本。模组只会以这些名字加载（另加配合 ASI 加载器使用的
`OptiScaler.asi`）；`d3d11.dll` 不在其中。

> **Resident Evil Requiem（含试玩版）需要 REFramework。** 这是已知的前置要求，不是 AMDNR 的问题：OptiScaler 依赖 REFramework 绕过
> Capcom 的反篡改保护（[OptiScaler wiki](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-9-Requiem)）。没有它，游戏会在启动后 15-60 秒崩溃
> （"An unhandled exception occurred"）。请从最新的 REFramework nightly（<https://github.com/praydog/REFramework-nightly/releases>）下载 `REFramework.zip`，
> 把其中的 `dinput8.dll` 放到 `dxgi.dll` 旁边，并把 REFramework 的菜单键改成别的键（例如 Delete）：它默认也是 Insert。
> 游戏更新后，在 REFramework 跟进更新之前出现崩溃属于预期情况。PRAGMATA、Monster Hunter Wilds、Onimusha 可能也需要它（未确认）。

### 5. 启动游戏

游戏中按 `HOME` 即可开关神经渲染（两个运行时都适用；屏幕上会有一个小提示显示 On / Off）。可在
Neural 选项卡中 Enable 复选框旁边、或 Interface > Keybinds 下重新绑定按键。

就这样。

正常启动游戏，然后按：

`INSERT`

这会打开 OptiScaler / AMDNR 菜单，你可以在里面随意配置模组。

### 如果不起作用

如果上面的名字都试过游戏仍然无法启动，请在 Discord 的 `#bug-report` 频道反馈。

反馈时请一并上传游戏根目录中生成的所有 `.log` 文件。

这些日志非常重要，能帮助我们更快定位问题。

**最简单的方法：Save report。** 如果菜单能打开，点击 **Save report**（Neural > Diagnostics 的最后一行，或 Advanced > Logging 的第一行）。它会在游戏目录中写出一个 zip：
`AMDNR-report-<游戏 exe>-<日期>.zip`（游戏目录只读时写到桌面，否则写到 `%TEMP%`），其中包含 `report.txt`、日志和
ini 文件，菜单会显示保存位置。你的 Windows 用户名和电脑名会被替换为占位符；但位于 `C:\Users\` 之外的游戏路径中的
名字不会被替换。请在 `#bug-report` 中附上这个 zip。AMDNR Launcher 的 **COLLECT LOGS** 可为任何游戏写出同样的 zip，
并且自 0.3.5.1 起，会在崩溃后附上崩溃日志和最新的转储文件。

> 游戏的 `.exe` 通常不在快捷方式指向的位置。Unreal 引擎游戏把它放在
> `<Game>\Binaries\Win64\` 下。

---

### lmxxf 运行时（0.3.0，可选）

第二个神经运行时（MIT 许可，作者 lmxxf）可以代替 danielblnc 的运行时承担这一步。RDNA 4 原生运行；RDNA 3
（RX 7000、Strix Halo）通过 3zwr1 开发的 AMDNR RDNA 3 后端运行——在 RDNA 3 上较慢，见下文"RX 7000"：NR resolution
请从 70% 或更低起步。掌机 APU 也能运行它，属实验性支持（见下文"掌机 APU"）。
它需要游戏目录旁的两个东西：

1. `LmxxfNrRuntime.dll` —— 在本压缩包中，与 `OptiScaler.dll` 并列（会随其余文件一起复制）。
2. `LmxxfNrRuntime.pak`（440 MB，已包含在 AMDNR 压缩包中），放在 `LmxxfNrRuntime.dll` 旁边 —— lmxxf 的
   权重文件、HIP 模块和 HLSL 打包成一个加密并带完整性校验的文件。运行时在内存中打开它，不会向磁盘
   解包任何内容。

首次启动时若检测到已安装运行时且尚未做出选择，菜单会询问使用哪一个（ini 中以
`[DlssNr] NrBackend = daniel | lmxxf` 记录；Neural > Neural runtime 可更改，下次启动游戏时生效）。
lmxxf 的编辑延迟一帧应用，由运动向量携带，因此画面从不等待网络（RX 9070 XT 上 1080p 的网络时间
约 14.1 ms）。它的日志是游戏目录旁的 `lmxxf_backend.log`。

**兼容性（lmxxf）。** 运行时只能看到 DLSS 所看到的东西，因此因游戏而异的只有一份短清单：颜色格式与
HDR、运动向量及其缩放、深度及其方向、反应遮罩（reactive mask）、曝光纹理、Reset 标志，以及该步骤所处的位置
（超分辨率之前，或光线重建之后）。目前已测试：

| 游戏 | API / 位置 | 备注 |
|---|---|---|
| Silent Hill 2 | D3D12，SR 之前 | 参考游戏；已处理 Unreal 带填充的颜色分配 |
| Forza Horizon 6 | D3D12，SR 之前 | |
| Stray | D3D11 经 D3D12 桥接，SR 之前 | |
| GTA V Enhanced | D3D12，SR 之前，HDR，单通道反应遮罩 | 0.3.0 已修复：遮罩曾被读成"处处皆为反应区域"，编辑从未落到画面上 |
| 任何使用光线重建的游戏 | D3D12，RR 之后（写回输出） | 0.3.0 起支持；尚未在游戏中确认 |

如果某个游戏看不到效果：`lmxxf_backend.log` 中有一行 `lmxxf inputs:`（格式、尺寸、运动缩放、深度
方向、遮罩、曝光），以及每 600 帧一行 `lmxxf stats @N:`（曝光、输入亮度、模型的编辑、被携带的编辑、
keep、反应遮罩均值、向量长度与被拒比例）。反馈时附上日志；这两行通常就能说明原因。

两个运行时共用同一个 Neural 选项卡（见下文"菜单"）。当前运行时不具备的控件会变灰并带简短标签，或被隐藏并显示
数量。仅 lmxxf 具有：**Full network**、**Output smoothing**（Quality > More quality options，需要 Network history）、
**Edit detail**、**Edit colour** 和 **Edge guard**（Image look > Model strength：模型编辑细节部分的增益、编辑的色彩
相对其亮度变化的比例、以及在深度边缘处对编辑的衰减），以及自动曝光的高光上限。0.3.4 中 lmxxf 新增：Network
output、Encoding、Residual edge fade、Game exposure、Fast mode、交错节拍读数，以及模型原生的角色遮罩
和 Structure intensity、Character structure（每次更改都会重建网络：约 1 秒的停顿）。

**Full network**（Neural > Performance，`[DlssNr] LmxxfFullNetwork`，仅 lmxxf）运行网络全部 71 个块，
而不是跳过第 42、43、46 块：略微更忠实，1080p 下慢约 0.5 ms（RX 9070 XT 上 16.6 -> 17.1 ms，在 0.3.3 中测得）。默认关闭。

**Fast mode**（Neural > Performance，`[DlssNr] AmdLmxxfFastMode`，仅 lmxxf，可选，默认关闭）让网络低一个尺寸档位运行
（1080 -> 900，900 -> 720）：1080p 下网络时间约少 29%（RX 9070 XT，在游戏外测得），细节略软。自带 Fast mode 的
danielblnc 版本在那里也会显示一行 Fast mode（`[DlssNr] AmdDanielFastMode`）；本次发布的运行时 zip 中的运行时没有
该模式，因此该行隐藏。

### RX 7000（RDNA 3）：借助网络尺寸档位变得更快（0.3.4 新增）

lmxxf 的网络只以少数几个固定尺寸（档位）运行：720（1280x720）、900（1600x900）和 1080（1920x1080），另有 576 和
360（新增，用于掌机）。无论画面填满档位的哪一部分，同一档位的开销都相同。在 RDNA 3（RX 7000、Radeon 8060S /
8050S 以及掌机 APU）上，lmxxf 的 NR 尺寸现在默认会对齐到某个档位：离下一个更小的档位更近时就降到该档位（更省），
否则放大到填满自己的档位（开销不变，细节略多），但绝不超过帧本身的尺寸。

RX 7800 XT 上每次网络运行的时间（由测试者用 lmxxf 的探针测得；仅网络，30 次运行的平均值；900 档的时间在
1600x900 下测得）：

| 游戏设置 | 0.3.3.2 | 0.3.4（RX 7000） |
|---|---|---|
| 1440p，FSR Quality（渲染 1706x960），NR 100% | 1080 档：73.3 ms | 900 档：52.2 ms |
| 1080p 渲染，NR 85% | 1080 档：73.2 ms | 900 档：52.2 ms |
| 1080p 渲染，NR 70% | 900 档：52.2 ms | 720 档：34.4 ms |
| 1080p 渲染，NR 80% | 900 档：52.2 ms | 900 档（填满）：52.2 ms（细节更多） |
| 1080p 渲染，NR 100% | 1080 档：73.2 ms | 不变 |

- 在游戏中，每个显示帧的收益更小：开启 Model interleave 时网络每隔一帧运行一次，而且游戏本身也有开销。尚未在
  游戏中实测。
- 网络看到的画面会略小一些（1440p Quality 下每边约少 6% 的像素），因此细微细节可能稍软。
  `[DlssNr] AmdLmxxfTierSnap=false` 可恢复 0.3.3.2 的尺寸。RX 9000 保持 0.3.3.2 的尺寸，除非你将它设为 `true`。
- **RX 9000：** `[DlssNr] AmdLmxxfTierSnap=true`（那里默认关闭）会在每一种渲染分辨率下把 lmxxf 的 NR 尺寸移到一个网络
  尺寸上：有些尺寸会降一档（1440p FSR Quality，1707x960 -> 900 尺寸：网络耗时 14.08 -> 9.96 ms/次，RX 9070 XT，在游戏外
  实测，画面稍软），有些则在本档内放大（1080p 渲染的 80% -> 1600x900：开销相同，细节略多）。不设置时，RX 9000 保持
  0.3.3.2 的尺寸。
- NR resolution 请从 70% 或更低起步（1080p 渲染下为 720 档；Performance 预设即 70%）。NR resolution 旁显示的开销
  按网络实际运行的档位计算；其悬停提示会写出档位。
- **0.3.5：RX 7000 上的网络时间约少 10%**，画面逐位相同：`LmxxfNrRuntime.pak` 中的 0.3.5 模块集（由一位测试者在
  RX 7800 XT 上测得并做了哈希校验；上表是 0.3.4 的数据）。
- **0.3.5 在 RX 9000 上：默认启用 Kien 的 lmxxf 0.37（MIT）**——在 RX 9070 XT 上 1080 尺寸的网络时间约少 20%
  （14.0 -> 约 11.3 ms，在游戏外测得；画面与 0.3.4.2 并非逐位相同）。在 RX 9060 / 9060 XT 上它也会启用，并与 c32w
  和 FastK 内核一起作为实验（尚未在该显卡上运行过）。关闭开关：`[DlssNr] AmdLmxxfL37=false`、`AmdLmxxfC32w=false`、
  `AmdLmxxfFastK=false`。

### 掌机 APU（实验性，0.3.4 新增）

lmxxf 可在拥有 12 个或更多计算单元的掌机 APU 上运行，通过 3zwr1 开发的 AMDNR RDNA 3 后端：**Z1 Extreme、Z2 和
Radeon 780M**（gfx1103），**Z2 Extreme、Radeon 890M 和 880M**（gfx1150）。这属于实验性支持，速度慢。Neural runtime 一行会在 RDNA 3 署名后显示 "experimental"。
首批测试者结果（ROG Ally，Z1 Extreme）：游戏外的 lmxxf 探针，360p 尺寸下每次网络运行 54.7 ms，576p 下 110.9 ms；
游戏内，一位测试者的一次运行（Shadow of the Tomb Raider，1280x720 配合 XeSS，Handheld 预设），360p 下每次网络运行平均 62 ms，
模型每 4 帧运行一次，开启 NR 时约 29 fps。**在 Z1 Extreme 级别的设备上，0.3.5 让这些数字减少约 10%**（Z1 Extreme、Z2、
Radeon 780M：360p 下约 50 ms，576p 下约 105 ms，画面逐位相同，经一位测试者哈希校验）；Z2 Extreme / 890M / 880M 模块未变。

- **不支持：** Z1 和 Radeon 740M（4 个计算单元）、Radeon 760M（8 个）、Radeon 860M / 840M。danielblnc 的运行时
  不能在掌机 APU 上运行。RX 6000（RDNA 2）计划在 0.3.6 支持；Steam Deck 和其他 RDNA 2 APU 不受支持。
- **它自动做的事**（仅当你的 ini 中没有自己设置的值时）：网络以最小尺寸 360p（640x360）运行，模型每 4 帧运行一次
  （Model interleave；不会保存）。Neural passes 保持为 1。
- **速度，实话实说：**作为参考：RX 7800 XT（60 个计算
  单元）在 720 尺寸下每次网络运行需要 34.4 ms；这些芯片只有 12 到 16 个计算单元，频率也更低。即使在 360p、模型每
  4 帧运行一次的情况下，也要预期帧率大幅下降、长交错带来一些拖影，以及比桌面显卡更软的画面。Neural 选项卡状态行
  末尾（以及 Diagnostics 中）的 NR 开销会显示你设备上的真实数字。
- **设置：**
  - 更清晰但更慢：`[DlssNr] AmdLmxxfTierCap=576`（1024x576 网络尺寸）。
  - 在 720p 或 800p 渲染下，NR resolution 100% 已经输入 360p 尺寸，所以更低的 NR resolution 并不会更省。
  - Model interleave 选 Off 时会保存为 `[DlssNr] AmdInterleave=1`（同样是关闭），这样掌机的默认值不会在下次启动时
    回来。手动关闭时请写 1，而不是 0。
  - Preset > **Handheld** 会设置 NR resolution 100%、关闭 Dynamic NR、模型每 4 帧运行一次、1 个 Neural pass 并关闭 Full network。该按钮只在这些 APU 上显示；在这里 Quality、Balanced 和 Performance 同样保持 360p 网络尺寸（菜单会说明）。
- **FSR 4：** FSR 4 (INT8) 在 RDNA 3 掌机上作为实验性选项提供（Upscaling 选项卡）；未经 AMD 验证。
  勾选 **FSR 4 (INT8) - Experimental on this GPU (restart)** 会写入 `[FSR] Fsr4ForceModel=2`；它从不自动开启（`Dx12Upscaler=auto` 时放大器为 XeSS）。
  在 Z1 Extreme 上每帧约 1.5-3 ms（估计值）；开启 NR 时，FSR 4 与 576 尺寸二选一，不要同时使用。
- **Shadow of the Tomb Raider**（以及两次创建 D3D12 设备的游戏）在放大器启动时不再崩溃（0.3.4 已修复）。
- **驱动：** 请使用 AMD 官方的 Adrenalin 驱动。lmxxf 需要 HIP（`amdhip64_7.dll`），部分掌机厂商的驱动没有附带；
  此时 `amd_bridge.log` 会提示 HIP 不可用。
- **开启 Model interleave 时移动（0.3.5 已修复）：**以前你一移动，被携带的编辑就会消失（"一移动效果就没了"）：在
  Model interleave 下，网络运行在长短不一的帧上，而携带的保护条件假定各帧等长。现在它会按本帧的时长读取上一帧的向量
  （两个运行时都适用）。
- **请一起使用 0.3.5 的文件：**0.3.5 更改了全部三个文件（`OptiScaler.dll`、`LmxxfNrRuntime.dll` 和
  `LmxxfNrRuntime.pak`），请一起替换；当 `OptiScaler.dll` 早于 0.3.4 时，运行时会拒绝在掌机上运行（"this handheld
  needs OptiScaler.dll 0.3.4 or newer"）。
- **有掌机的测试者：** 请在 Discord 上索取掌机测试包（`handheld-test.zip`）。其中的 `run_probe.bat` 会在你的设备上
  测量网络并写出 `handheld_result.txt`（你的 Windows 用户名会被隐藏）。

## AMDNR Anywhere（preview，0.3.5 新增）

**它是什么。** 为自身没有 DLSS、XeSS 或 FSR 2 的游戏提供神经渲染——而且不会向游戏目录写入任何东西。AMDNR 在一个窗口
捕获宿主中运行：宿主捕获游戏窗口，用 FSR 3 把它缩放到你的屏幕，神经渲染在捕获到的画面上运行；我们的菜单绘制在宿主
内部（用你的菜单键，默认为 `INSERT`），并有自己的 **Anywhere** 选项卡。宿主是
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR**：
AMDNR Launcher 从其作者的发布页下载它（467 MB，只需一次）。

**怎么用。** 在 AMDNR Launcher 中，没有放大器的游戏会显示 **PLAY ANYWHERE**，而不是 INSTALL。点它：启动器会（首次）
获取宿主、启动游戏，宿主随即捕获游戏窗口。请以**窗口化或无边框**模式运行游戏，不要用独占全屏，并在宿主内按你的菜单键
打开 AMDNR 菜单。自带放大器的游戏仍走正常的 INSTALL 路线：Anywhere 是为那些没有放大器的游戏准备的。

**宿主设置在菜单里**，位于 Anywhere 选项卡的 Host settings 中，而不在启动器里：游戏窗口尺寸（720p / 900p / 1080p——
这是对游戏内该如何设置的建议；宿主会捕获游戏打开的任何窗口）、阶段列表（V1：一次 FSR 3 通道直接输出到屏幕；V2：先以
1x 运行 FSR 3，再运行一次填充通道）、NR 档位（Auto / 720 / 900 / 1080）、VRR、帧节拍以及**宿主帧率**（Default = 你的
显示器刷新率，最高 60；Auto = 上一次会话中网络所能维持的帧率；30 到 120；Display refresh = 不设上限）。它们在**下一次**
PLAY ANYWHERE 时生效，启动器会在按钮旁显示一份只读摘要。在宿主自己的 `OptiScaler.ini` 中，它们是
`[DlssNr] AnywhereWindow`、`AnywhereEffect`、`AnywhereNrTier`、`AnywhereVrr`、`AnywherePacing` 和 `AnywhereHostFps`。
也请用游戏自己的限帧器把游戏限制在 60 到 90 fps：宿主只能显示游戏画出的帧，而宿主的每一帧都会运行一次网络。

**宿主无法给网络提供什么。** 捕获到的窗口自身没有深度、运动向量、抖动（jitter）或曝光；运动由宿主估算。因此 Anywhere
选项卡会写出哪些是估算的；Neural 选项卡中在那里无法生效的行会被隐藏，或被拒绝并附上原因（Ray Regeneration、Screen GI
和 Model interleave——后者会把被携带的编辑耗费在估算出的运动上）；当网络超出宿主的帧预算时，Anywhere 页面上的一行状态
会说明这一点，并告诉你该调低什么。**1920x1080 或更小的游戏窗口是像素精确的情形：**更大的窗口会先缩小到网络的上限，
再放大回去，页面会说明这一点，并给出网络看到的像素占屏幕像素的比例。NR resolution 一行会显示网络在宿主内的真实尺寸。

**状态：preview。** 本次发布仅支持 RX 9000（RDNA 4）；RX 7000 在那里测试后跟进。快速运动时可能仍有轻微闪烁或抖动
（把 NR 档位降到 720、把游戏限制在 60-90 fps、保持 Model interleave 关闭——宿主会拒绝它）。捕获宿主从其作者的 GitHub
发布页获取，而不是从我们的发布页。反馈：在宿主内的菜单中生成的 **Save report** zip（其标题写的是被缩放的游戏），或启动器的
COLLECT LOGS。

## Linux / Proton（Steam Deck、桌面 Linux）

AMDNR 在 Proton 和 Wine 下作为 OptiScaler 构建版运行。**神经渲染不能在 Linux 上运行（仅限 Windows）：**
两个 NR 运行时都需要 Windows 版 AMD 驱动的 HIP，而 Proton 和 Wine 不提供它。在 Proton 下开启 NR 时，NR 不会运行，
自 0.3.5 起 Neural 选项卡和报告会说明这一点（而不是显示 "Idle"）。这是预期行为，不是崩溃，也不是安装出错。AMDNR Launcher 是一个 Windows 程序，可以在 Proton
下运行（实验性，我们尚未测试，见下文）；不用启动器，手动安装也同样可行。

**可以使用的功能：** FSR 放大器（FSR 3.1，以及在支持它的显卡和驱动上的 FSR 4）、菜单（`INSERT`）和
**Save report**。一位玩家在 Steam Proton 上（RX 9070 XT、vkd3d-proton、Resident Evil Requiem）确认：游戏能启动，
菜单能打开并接管鼠标，Save report 也能用，开启帧生成时同样如此。

**Ray Regeneration 与帧生成：**
- 已知问题：在 Proton 上 Ray Regeneration 可能出现粉色 / 洋红色色块；那里的默认锐化现已关闭，但如果你仍然看到这些色块，请改用普通 FSR（RX 9000 上用 FSR 4），并发送一份 Save report。
  在 Proton 上，当游戏没有传入锐化值时，AMDNR 不会在 Ray Regeneration 之后添加锐化（在 Windows 上会加 0.25）：
  Image > Sharpness 会显示 "RR default 0 (off on Proton)"，Override 仍然可以设置你自己的值。
- **帧生成**现在开启时不会崩溃了，但 fps 计数器会把生成的帧也算进去：在 60 fps 帧率上限或 60 Hz V-Sync 下，
  其实只有 30 个真实帧，看起来就和 30 fps 一样。目前在 Proton 上请先关闭它（`[FrameGen] FGOutput=nofg`），
  或者只在游戏不开帧生成也能达到约 60 fps、且显示器刷新率高于 60 Hz 时使用。
- **Vulkan 游戏**（RTX Remix 游戏、id Tech 8），在 Proton 上和在 Windows 上一样：光线重建和 AMDNR 自己的帧生成按设计
  会得到 "not supported" 的回应（降噪器是 D3D12 的，而光线追踪缓冲区留在 Vulkan 设备上）；自 0.3.5 起，Ray
  Regeneration 和 Frame Gen 选项卡会直接说明这一点，而不再要求你在一个把它们置灰的游戏中开启它们。DLSS 超分辨率可通过
  桥接工作。
- 自 0.3.5 起，Proton 下的 `[Spoofing] Dxgi=true`（FSR 4 升级路径）在启动时不再出错。

**要求：** 较新的 Proton 或 Wine（已测试：Proton 11，即 Wine 11），且游戏运行在 vkd3d-proton（D3D12）或
DXVK（D3D11）上，这也是 Proton 的默认设置。更早的版本未经测试。

**Linux 上的 AMDNR Launcher（实验性，我们尚未测试）。** 启动器就是同一个 Windows 程序 `AMDNR-Launcher.exe`
（独立部署：无需安装 .NET 或其他运行时）。在 Wine / Proton 下，它会检测到 Wine，并显示一条提示，说明该怎么做。
它还会通过 Wine 的 `Z:` 盘查找你的 Linux Steam 游戏库（`~/.steam/steam` 和 `~/.local/share/Steam`，以及
`libraryfolders.vdf` 中列出的游戏库文件夹）。我们自己还没有测试过：如果你试了，请在 Discord 上告诉我们它能不能用。
试用方法：

1. 在 Steam 中把 `AMDNR-Launcher.exe` 添加为非 Steam 游戏（英文界面：**Games > Add a Non-Steam Game to My Library**）。
2. 在它的 **属性 > 兼容性**（英文界面：**Properties > Compatibility**）中强制使用一个 Proton 版本（Proton
   Experimental），然后从 Steam 启动它。
3. 如果游戏不在 **LIBRARY**（游戏库）中，点 **ADD**（添加）并选择游戏的文件夹（你的 Linux 文件夹在 `Z:` 盘上）；
   如果启动器选错了 `.exe`，请使用 **CHOOSE GAME .EXE**（选择游戏 .EXE）。
4. 选中该游戏，点 **INSTALL**（安装）。
5. 在游戏的 **属性 > 通用 > 启动选项**（英文界面：**Properties > General > Launch Options**）中输入
   `WINEDLLOVERRIDES="dxgi=n,b" %command%`（如果启动器为该游戏用了别的 DLL 名称，把 `dxgi` 换成那个名称），
   然后继续完成下文手动安装的第 5 步和第 6 步（请从 Steam 启动游戏；在 Wine / Proton 下，启动器里的 **PLAY**
   按钮已停用）。

**手动安装**（不使用启动器）：

1. 从发布页下载 `AMDNR-vX.X.X.zip`（0.3.5 为 `AMDNR-v0.3.5.zip`）。danielblnc 运行时 zip（`v0.5.0-Runtime.zip`
   及其他几个）只供神经渲染使用，所以在 Linux 上不需要它们（复制过去也没有坏处）。
2. 解压该 zip，把全部内容复制到游戏目录，放在游戏的 `.exe` 旁边。
3. 把 `OptiScaler.dll` 重命名为 `dxgi.dll`。
4. 在 Steam 中打开该游戏的 **属性 > 通用 > 启动选项**（英文界面：**Properties > General > Launch Options**），
   输入：

   ```
   WINEDLLOVERRIDES="dxgi=n,b" %command%
   ```

   这会让 Wine 加载游戏目录中的 `dxgi.dll`，而不是它自带的那个；没有这一步，AMDNR 不会加载。
   如果你用了别的名字（例如 `winmm.dll` 或 `version.dll`），把 `dxgi` 换成那个名字，例如
   `WINEDLLOVERRIDES="winmm=n,b" %command%`。Lutris、Heroic 和 Bottles：在运行器（runner）的 DLL 覆盖
   （DLL overrides）或环境变量设置中添加同样的覆盖（`dxgi` = `native,builtin`）。
5. 在 `OptiScaler.ini` 中设置 `[FrameGen] FGOutput=nofg`（关闭帧生成，见上文）。
6. 启动游戏，按 `INSERT` 打开菜单，在那里设置放大器。

**菜单。** 在 0.3.4 中，开启帧生成时，菜单可能打开了却不接收鼠标或键盘输入，或者根本打不开。0.3.4.1 把菜单
附着到游戏窗口上；一位玩家已在 Proton 上确认菜单能打开并接管鼠标，开启帧生成时也一样。如果在你的环境中仍然
出现这种情况，AMDNR 会显示警告 "Menu window lost"。此时请在 `OptiScaler.ini` 中设置 `[FrameGen] FGOutput=nofg`；
如果菜单仍然没有反应，再设置 `[Menu] OverlayMenu=false`（经典菜单，不依赖叠加层窗口）。

**HDR。** AMDNR 不会在 Proton 下开启 HDR。HDR 取决于你的 Proton 和桌面环境：需要支持 HDR 的 Proton 版本，以及
能显示 HDR 的会话（例如 gamescope，或开启了 HDR 的 Wayland 桌面）。如果游戏在没有 AMDNR 时 HDR 正常，装上 AMDNR
后也会保持正常；如果游戏的 HDR 选项是灰色的，需要在你的 Proton 或桌面设置中解决。

**反馈 Linux 问题：** 请使用菜单中的 **Save report** 按钮（保持 `[Log] LogToFile=true`，即默认值，这样报告里才有
本次会话的日志）；报告会显示游戏是在 Wine/Proton、vkd3d-proton 还是 DXVK 下运行的。请同时写上你的发行版、显卡、
Mesa 版本和 Proton 版本。

## 系统要求

- Windows 10 或 11（64 位），用于神经渲染。AMDNR Launcher 是一个 Windows 程序，可以在 Proton 下运行（实验性，
  我们尚未测试）。在 Linux / Proton 下，AMDNR 作为不带 NR 的 OptiScaler 构建版运行（见"Linux / Proton"一节）。
- 一块使用 AMD Software: Adrenalin Edition 26.9.1 或更新版本驱动的 AMD 显卡。神经运行时通过驱动使用 HIP；不需要 HIP SDK，也不需要开发者模式。
  支持的芯片：
  - RX 9000（RDNA 4）：两个运行时。
  - RX 7000（RDNA 3，桌面与移动）：两个运行时——lmxxf 通过 AMDNR 的 RDNA 3 后端运行，比 RDNA 4 慢（网络尺寸档位
    默认开启，见上文）。
  - Strix Halo（Radeon 8060S / 8050S）：lmxxf。
  - 拥有 12 个以上计算单元的掌机 APU（Z1 Extreme / Z2 / 780M、Z2 Extreme / 890M / 880M）：lmxxf，实验性且较慢。
    Z1（4 CU）、760M / 740M 和 860M / 840M：不支持。
  - RX 6000（RDNA 2）：暂不支持，计划在 0.3.6 支持。Steam Deck 与 RDNA 2 APU：不支持（指神经渲染；在 Proton 下
    使用放大器见"Linux / Proton"一节）。

  Neural 选项卡会显示你的显卡能运行什么（把鼠标悬停在运行时条目上，或查看 Diagnostics 中的 GPU 一行）。
- **AMDNR Anywhere**（preview）：Windows、本次发布中的一块 RX 9000（RDNA 4）显卡，以及负责获取捕获宿主的 AMDNR
  Launcher；游戏以窗口化或无边框模式运行。见"AMDNR Anywhere"。
- 一款 Direct3D 12、Direct3D 11 或 Vulkan 游戏。AMD 神经路径本身是 D3D12；D3D11 与 Vulkan 游戏通过
  OptiScaler 的 D3D12 桥接到达它，这意味着放大器必须是 "w/Dx12" 后端之一（`ffx_12`）。把
  `Dx11Upscaler` / `VulkanUpscaler` 保持为 `auto`，开启神经渲染时本构建版会自动为你选择。开启 Neural Rendering 时，Upscaling 列表会把它们显示为 "... w/Dx12 - Neural"。
- 在 1080p 级别的渲染分辨率下约需 2 GB 空闲显存。

## 压缩包里有什么

**AMDNR-vX.X.X.zip**

| 文件 | 说明 |
|---|---|
| `OptiScaler.dll` | 带 DLSS-NR AMD 后端的 OptiScaler（AMDNR 0.3.5）。按指南重命名。 |
| `OptiScaler.ini` | 设置。神经渲染已启用；日志已开启，以便反馈时有内容可附。 |
| `LmxxfNrRuntime.dll` | lmxxf 神经运行时（0.3.5：使用前会按 pak 自带的摘要列表校验 pak 中的每个 HIP 模块；当没有 HIP 适配器与游戏的 GPU 匹配时写出双方；动态分辨率的变化步骤会被保持而不重建网络；不再读取 lmxxf 那些会改变画面的环境变量；lmxxf 的内核，包括 lmxxf 0.31 的内核、AMDNR 的 c32w 内核、小网络尺寸以及原生角色遮罩）。仅在选中时使用；读取旁边的 `LmxxfNrRuntime.pak`，见"lmxxf 运行时"。 |
| `LmxxfNrRuntime.pak` | lmxxf 运行时的权重、HIP 模块和着色器，打包为一个加密文件（465 MB；0.3.5：面向 RX 7000 和 Z1 Extreme 级掌机——Z1 Extreme、Z2、Radeon 780M——的模块集快了约 10%，画面相同；RX 9000 在其未变的基础模块集之外获得 Kien 的 lmxxf 0.37 / 0.39 模块（MIT）；Z2 Extreme / 890M / 880M 和 Strix Halo 的模块未变）。只有 lmxxf 运行时会读取它；与 danielblnc 运行时并存也无妨。 |
| `OptiScaler\` | OptiScaler 使用的 FSR、XeSS、FidelityFX 去噪器和 D3D12 Agility SDK。 |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | Nukem9 的 dlssg-to-fsr3，未修改、仅重命名：把游戏的 DLSS 帧生成调用交给 FSR 3 帧生成，Vulkan 也可用（`FGNvngxReplacement=Nukems`）。GPLv3，见 `Licenses/`。 |
| `Licenses\`、`LICENSE` | 第三方许可证、AMDNR 声明（`AMDNR_NOTICE.txt`）以及本构建版的 GPL-3.0 许可证。 |
| `SHA256SUMS.txt` | 本 zip 中每个文件的校验和，以及它所列出的 danielblnc 运行时 zip 中各文件的校验和。 |

**danielblnc 运行时 zip**（DLSS-NR on AMD by Daniel Blanco，未经修改，经他许可；选用其中一个）

选哪一个：**RX 9000 和 RX 7000** 都用 Alpha0.3.4.2 上的 `v0.5.0-Runtime.zip`（推荐）；`v0.4.3-Runtime.zip`
（Alpha0.3.4.2 和 Alpha0.3.4.1）、`v0.4.1-Runtime.zip` 和 `v0.4.0-Runtime.zip`（Alpha0.3.4.1）仍然可用。
lmxxf 运行时在 RX 7000 和 RX 9000 上不需要任何运行时 zip；掌机 APU 只使用 lmxxf。AMDNR Launcher 提供 0.5.0（推荐）、0.4.3、0.4.1 和 0.4.0，并会替你选好。

| Zip | 发布页 | danielblnc 运行时 |
|---|---|---|
| `v0.5.0-Runtime.zip` | Alpha0.3.4.2 | 0.5.0，**在 RX 9000 和 RX 7000 上均为推荐版本**；danielblnc 运行时设置可以使用，并且自 0.3.5 起，你在其 `dlssnr_on_amd.ini` 中自己设置的 `Async` 键会传达给它 |
| `v0.4.3-Runtime.zip` | Alpha0.3.4.2（以及 Alpha0.3.4.1） | 0.4.3，仍然可用；danielblnc 运行时设置可以使用 |
| `v0.4.1-Runtime.zip` | Alpha0.3.4.1（以及 Alpha0.3.4） | 0.4.1，仍然可用。使用它时 Network style、Tone curve、Black lift 和 Game exposure 会变灰 |
| `v0.4.0-Runtime.zip` | Alpha0.3.4.1（以及 Alpha0.3.4） | 0.4.0，仍然可用；danielblnc 运行时设置可以使用 |
| `v0.3.3-Runtime.zip` | [Alpha0.3.4](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/tag/Alpha0.3.4) | 0.3.3，已退役：不再推荐。你已经有的话它仍然可用；danielblnc 运行时设置可以使用 |
| `Runtime.zip` | Alpha0.3.4 | 0.3.1；使用它时 danielblnc 运行时设置会变灰 |

**自 0.3.5 起，danielblnc 0.5.0 是推荐的 danielblnc 运行时**（它在 0.3.4.2 上也能运行）。0.3.5 还会把你在
`dlssnr_on_amd.ini` 中自己设置的 `Async`（或较旧的 `Inline`）键传递给它，而不是强制使用同帧模式；两个键都没有时，
默认安装不受影响。比 0.5.0 更新的 danielblnc 版本不会被本次发布驱动。

每个 zip 都包含：

| 文件 | 说明 |
|---|---|
| `dlssnr_amd_pass1..3.dll` | AMD 神经运行时，未经修改。三份副本，多遍（multi-pass）时每遍一份。 |
| `dlssnr_on_amd_weights.bin` | 运行时加载的网络权重。 |
| `danielblnc_ATTRIBUTION.txt` | Daniel Blanco 的署名，以及 AMDNR 分发他的运行时所依据的条款。 |

## 菜单（0.3.4 新增）

按 `INSERT`。所有选项卡外观一致：文字选项卡、带 Discord 和 GitHub（打开本页）的标题行、一行致谢（点击
Daniel Blanco 的名字会打开他的 GitHub 页面）、**Components** 行（OptiScaler 七个组件中有几个处于活动状态；点击查看
列表），以及带 Menu Scale、Save Settings 和 Close 的页脚。
把鼠标悬停在控件名称上即可打开帮助。

**Neural 选项卡，从上到下：**

- **Enable Neural Rendering** 及其按键（按钮，例如 `Home`：点击它，再按另一个键即可重新绑定）。
- **Neural runtime**（danielblnc / lmxxf，并显示你的文件的确切版本，例如 `lmxxf 0.3.4`），带一个状态词：running、restart the game to switch、not installed、
  not for this GPU 或 stopped。下方是当前运行时的署名和一行状态，例如
  `Running - 1920x1080 at 100% - NR 62/s - model 62/s - 15.3 ms`（最后的数字是 NR 开销），以及一个收起的 **Live** 行，显示更多
  细节。需要你注意时会出现一行
  橙色提示，若有解决办法则附带按钮（Retry lmxxf、Switch to danielblnc、Open Upscaling）。默认状态下没有任何提示。
- **Preset**：Quality / Balanced / Performance 把 NR resolution 设为 100 / 85 / 70% 并关闭 Dynamic NR，其他不变。
  掌机 APU 上还有第四个按钮 **Handheld**（见“掌机 APU”一节）。然后是 **NR style** 和 **Style slots**（Store / Apply /
  Clear）。
- **Performance**：**Placement**（放大之前 / 之后，0.3.5 新增；见"值得了解的设置"）、NR resolution（%）及其开销、Neural passes、Full network、Fast mode、Dynamic NR resolution、Model interleave
  （开启时下方显示 Interleave preset 和节拍行）。
- **Quality**：Residual strength、Residual limit、Temporal stability、Sharpening (CAS)，以及 **More quality options**
  （Network history——两个运行时共用一个复选框——、Output smoothing、Stability mode、Residual temporal、Residual edge
  fade、Still-surface steadiness）。
- **Image look**：Colour composition、Detail 与 Colour strength，以及三个折叠区：**Model strength**（Tone 与 Structure
  intensity、Character structure、Edit detail / colour、Edge guard、Native character mask，以及 danielblnc 的 Network
  style、Tone curve 和 Black lift）、**Exposure and highlights**（Auto-exposure、其高光上限、Highlight colour guard、
  Game exposure）和 **Appearance filter**（名称后显示 off / on）。折叠区块名称后淡色的 "default" 或
  "custom" 表示其中是否有改动。
- **Ray Regeneration**：自 0.3.5 起是一行指引。当 Ray Regeneration 在游戏中运行时，它的控件位于其独立的 **Ray
  Regeneration** 选项卡上（紧跟在 Upscaling 之后），这一行会提供一个 **Open Ray Regeneration** 按钮；它没有运行时，
  这一行会说明原因（游戏没有开启光线重建、驱动在这块显卡上拒绝了降噪器、Ray Regeneration 放弃了这款游戏以及原因，
  或它上一次运行的时间）。
- **工具行**，启动时收起：**Diagnostics**（Network output、Debug view、Edit shaper A/B、NR cost、拖影
  与自调读数、GPU 行、**Save report**；自 0.3.5 起 RR 调试视图位于 Ray Regeneration 选项卡上）、**Runtime options**（Encoding、Every-frame NR、NR slots、Highlight proxy）和
  **Experimental**（AMDNR Screen-space GI，preview）。

当前运行时不具备的控件会变灰并带简短标签（例如 "not in lmxxf yet"），或被隐藏并显示数量（"3 danielblnc-only
options hidden"）；切换运行时不会移动其他任何行。

**其他选项卡：** Upscaling 以放大器、一行状态和 Render resolution（原 Upscale Ratio Override 与 Output Scaling）
开头；在非 NVIDIA 显卡上不再列出 "DLSS w/Dx12"。**Ray Regeneration**（0.3.5 新增）在 Ray Regeneration 于游戏中运行时
紧跟在 Upscaling 之后：状态行（按显卡、按 API）、**Denoiser backend** 行（Automatic / Off——Off 会告诉游戏光线重建不受
支持，于是游戏保留自己的降噪器；重启后生效）、各项控件、More Ray Regeneration options，以及它自己的 Diagnostics 区块，
其中有 RR 调试视图和噪点数值（输入与输出的颗粒、静止镜头下的闪烁）；它从不在 AMDNR Anywhere 内绘制。Image 包含
Sharpness、Textures、Init Flags 和 Magnifier。Frame Gen 以 FG Input 和 FG Output 开头，并且自 0.3.5 起有一行文字写出
在生成任何帧之前还缺少哪一步。Interface 包含 FPS 叠加层和 Keybinds（每个按键一个按钮）。Advanced 以 Active Quirks 开头，
然后是 Display（V-Sync）、Compatibility 和 Logging。在 AMDNR Anywhere 内，菜单会显示一个 **Anywhere** 选项卡（捕获与
网络状态行、宿主设置），取代 Frame Gen 和 Advanced 选项卡。设置项、键名以及 Save Settings 写入的内容都没有变化，
`CHANGELOG.md` 中另有说明的除外。

## 值得了解的设置

打开 **Neural** 选项卡。默认值是最近一次测试过的组合，所以最有用的第一步是一次只改一项。

- **NR resolution** —— 主要的画质/开销杠杆。低于 100% 时模型处理较小的画面，只把它的*修正*带回
  全分辨率帧，因此帧保留自己的细节。高于 100% 时开销按平方增长（150% 为 2.25 倍）。滑块以 5% 为
  一档：每个新的 NR 尺寸都可能占住显存直到游戏重启，所以多次调整后请重启游戏。
  旁边的开销在 100% 时为 1.00x；在 lmxxf 下是网络实际运行档位的价格（悬停提示会写出档位）。Preset 按钮会把它设为
  100 / 85 / 70%。
- **Placement**（Neural > Performance，`[DlssNr] AmdPlacement = pre | post`，0.3.5 新增，两个运行时都有）—— 神经通道
  运行的位置。`pre`（默认值，也是以往每个版本的做法）编辑放大器即将读取的渲染分辨率画面。`post` 则改为编辑放大器
  完成后的显示分辨率画面：更清晰，因为放大器不再对编辑结果重新滤波；也更昂贵——网络以显示尺寸运行，最高到它 1920x1080
  的上限，因此 1080p 屏幕在任何 NR resolution 下都要付出最高档位的开销，而 1440p 或 4K 屏幕得到的是一个 1080 行的编辑
  再放大回去——运动中稍欠宽容；如果游戏在放大之前合成 HUD，HUD 也会被包含在内。在 AMDNR Anywhere 内、在最终图像模式
  （final image mode）下，以及 Ray Regeneration 已在游戏中运行过之后，它会被拒绝（回到 `pre`，`amd_bridge.log` 中记
  一行）。`post` 运行时，NR resolution 一行会显示两个尺寸。
- **Residual strength** —— 模型编辑应用的比例；大于 1 会放大。这是改变画面最大的控制项。
- **Residual limit** —— 单个像素可移动的上限。出现斑块：**调低**它。
- **Model interleave** —— 每隔一帧运行模型，大幅提升帧率。跳过的帧由 **Interleave preset** 填补；
  默认是 *Edit accumulation*（预设 10，两个运行时都有）：每一帧都是该帧自身的画面加上模型携带的修正，
  因此不会沿用任何旧画面。*Guided fill v2*（预设 6，danielblnc）和 *Classic carry*（lmxxf）是较早的
  填补方式。两类帧的节拍在 danielblnc 上自动处理，在 lmxxf 上关闭（`[DlssNr] AmdInterleavePacing` 设为 0 到 1
  时两者都会调节节拍，但会损失一些帧率）；预设下方的一行暗色文字会显示测量值。Adaptive interleave 在本版本中已关闭。
- **Neural passes** —— 2 和 3 会叠加模型，收益递减。在 lmxxf 下网络的历史保持为第一遍；额外的遍
  只是空间上的精修。danielblnc 在 Vulkan 游戏中只运行 1 遍（滑块下方会有提示）。
- **Colour composition**（Neural > Image look，两个运行时都有）—— *Classic*（默认）就是你之前的画面。
  *RenoDX (experimental)* 在模型之后运行 RenoDX 的色彩合成，与 NVIDIA 路径相同：Composition detail 与
  Composition colour、以原图为基准约束模型结果的双向 **Highlight guard**（默认 2x），以及可选的皮肤 / 环境
  控制。遇到显示参考（SDR）帧、开启 Network output 或 Encoding 为 sRGB / Gamma 2.2 时，两个运行时都会回退到
  Classic；菜单中的说明会提供一个按钮来关闭阻碍项。NR 风格和预设不会改动这些设置。
- **Native character mask**（Image look > Model strength，`[DlssNr] AutoMask`，默认开启）—— 模型自身对脸部和皮肤
  的处理。取消勾选现在对两个运行时都生效（在 lmxxf 上会重建网络：约 1 秒停顿）；在 lmxxf 上，Structure
  intensity 和 Character structure 现在也会生效。
- **新 ini 中帧生成默认关闭**，开启需要五步：在 Frame Gen 选项卡上选择 FG Input 和 FG Output（例如带 DLSS 帧生成的
  游戏中选 "DLSSG via Streamline"，以及 XeFG）、**Save Settings**、完全重启游戏、在游戏**自己的**设置中开启帧生成，
  然后在 Frame Generation 下勾选 **Active**。自 0.3.5 起，选项卡和日志会写出缺少的是哪一步。完整清单以及需要在游戏中
  关闭的项目，见下文常见问题（"帧生成：fps 没有提升？"）。
- **XeFG 多帧生成** —— 3X 至 6X 已内置并默认开启（`XeFG\UnlockMFG`），对 OptiScaler 的副本和游戏
  自带的副本都有效。如果你还留着 `XeFGUnlock.asi`，**请从 `OptiScaler\plugins` 中删除它**：同一
  补丁的两份副本会导致游戏崩溃。
  **最高 10X 需手动开启**（仅 D3D12 游戏）：在 Frame Gen 选项卡 FG Output 下设置 *XeFG ceiling (restart)*
  （4X、6X 默认、8X 或 10X；`[XeFG] MaxInterpolatedFrames`），重启游戏，再在 MFG 下拉框中选择倍数。
  高于 6X 需要 OptiScaler 自带的 XeFG 提供程序并开启 Extra pacing；游戏自带的 XeSS 3 副本最多 6X。
  10X 需要 360 Hz 及以上的显示器，并把帧率上限设为刷新率 / 10；延迟较高，且提供程序在 4K 下多占用约
  128 MiB 显存。7X-10X 尚未在游戏中确认：测试者请发送 `OptiScaler.log`。
- **FSR Ray Regeneration** —— 在 RX 9000（RDNA 4）上提供；在 RX 7000（RDNA 3）上作为 preview 提供，默认开启，不带降噪器（见下文）；仅在使用 DLSS 光线重建的游戏中（Cyberpunk 2077、Alan Wake 2），且游戏
  运行 DLSS（开启伪装）、光线追踪和光线重建都在游戏自身设置中启用。此时神经渲染在它之后、对它的
  输出运行，开销更大：帧率下降时调低 NR resolution。自 0.3.5 起，它的控件位于其独立的 **Ray Regeneration** 选项卡上，
  紧跟在 Upscaling 之后，在 Ray Regeneration 于游戏中运行时绘制；Neural 选项卡会指向它，并在 Ray Regeneration 没有运行时
  保留那行说明原因的浅色文字（游戏没有开启光线重建、驱动在这块显卡上拒绝了降噪器、Ray Regeneration 放弃了这款游戏以及
  原因，或它上一次运行的时间）。该选项卡的 **Denoiser backend** 行（`[FSR-RR] RrBackend = auto | off`）可以告诉游戏光线
  重建不受支持，于是游戏保留自己的降噪器（在下次启动游戏时生效）。在 **Vulkan** 游戏中，光线重建按设计为 "not
  supported"（降噪器是 D3D12 的），选项卡会说明这一点。**路径追踪配置**（路径追踪下脸部噪点更少）自 0.3.3.1 起需手动开启：
  想在 Resident Evil Requiem 或 PRAGMATA 中试用，请在那里勾选。同一选项卡还有 bias mask 强度和
  **皮肤平滑**（实验性，用于提供 SSS 引导的游戏；默认关闭，但自 0.3.3.2 起在 Resident Evil Requiem 中默认开启）；
  时域调节滑块在 *More Ray Regeneration options* 中，RR 调试视图和噪点数值（输入与输出的颗粒、静止镜头下的闪烁）位于该
  选项卡自己的 Diagnostics 区块中。在 RX 7000（RDNA 3）上，Ray Regeneration 以 preview 形式提供
  （0.3.5.1；0.3.5 未默认提供）：游戏可以开启光线重建，Ray Regeneration 的设置也会显示，但 AMD 的降噪器没有面向 RDNA 3
  的提供程序，因此降噪器会拒绝启动，光线重建画面在不带降噪的情况下被升采样，看起来可能比游戏自己的降噪更嘈杂；
  Ray Regeneration 页面会说明实际运行的是什么。Upscaling 选项卡中的 **Preview: Ray Regeneration on this card (restart)**
  （带有 "preview - on by default" 标签）默认勾选；取消勾选，或设为 `[FSR-RR] FfxDenoiserAllowPreRdna4=false`，
  游戏便保留自己的降噪器。AMDNR 自己的 RX 7000 降噪器已在计划中。
  RX 6000 及更早的显卡只有设为 `[FSR-RR] FfxDenoiserAllowPreRdna4=true` 才会提供（Upscaling
  选项卡：**Offer FSR Ray Regeneration on this GPU (restart)**）。**RR 之后的锐化**（0.3.4.1）：当游戏没有传入锐化值时，
  AMDNR 在 Windows 上会在 RR 之后加 0.25 的锐化（Linux / Proton 上为 0）；关闭方法：Image > Sharpness，勾选
  Override，滑块拉到 0。自 0.3.4.2 起这个数值有了自己的 ini 键 `[Sharpness] RrDefaultSharpness`（默认值仍是 0.25）：
  可以在那里直接写 0.15、0.10 或 0，不必动 Override；而你的 ini 在 Override 关闭时保留的 `[Sharpness] Sharpness`
  数值，菜单里会标出它正在等待 Override。
- **AMDNR Screen GI**（preview，0.3.4 新增，默认关闭；Neural > Experimental，或 `[AmdGi] Enabled=true`）—— AMDNR 自己的屏幕空间反弹光和环境光遮蔽，基于游戏的深度，在 NR 和放大器之前运行；NR 开或关都能用；在 RX 9070 XT 上 1080p 渲染、High 档约 1 ms（在游戏外测得）。它是屏幕空间效果：来自屏幕外的光会缺失。见 `CHANGELOG.md`。
- **Save report**（Neural > Diagnostics 或 Advanced > Logging）—— 一个包含所有日志和 ini 文件的 zip，用于反馈；见上文"如果不起作用"。

## 出了问题怎么办

游戏目录中会出现 `OptiScaler.log`。在 `#bug-report` 中附上它，并说明游戏和显卡型号；Neural > Diagnostics 或 Advanced > Logging 中的
**Save report** 会把它和其他文件一起打包。AMD 后端还会写出 `amd_presr.log` 和 `amd_bridge.log`，当神经渲染这一步
出问题时它们最有用。最近三次会话的日志会保留为 `OptiScaler.previous.<exe>.log`（最新）、
`OptiScaler.previous-1.<exe>.log` 和 `OptiScaler.previous-2.<exe>.log`（`[Log] KeepPreviousLogs`；设为 1 则像以前
一样只保留一份）。崩溃后请一并附上：新日志中会写明 "no clean exit recorded"（自 0.3.4 起，正常退出后不再出现）。

**NR frames 0/s，且 Neural 选项卡或 `amd_presr.log` 说该 pass DLL 是本 AMDNR 不支持的版本？**
你的 `dlssnr_amd_pass1..3.dll` 是本 AMDNR 不认识的 danielblnc 版本（外面流传着一套 0.2.16），或三个文件中缺了一个。
自 0.3.3.2 起，Neural 选项卡会写出文件名和版本，并说明该怎么做。请使用推荐的运行时，三个 pass DLL
须来自同一个压缩包：**RX 9000 和 RX 7000** 都用 Alpha0.3.4.2 上的 `v0.5.0-Runtime.zip`
（149,550,553 字节，SHA256 以 `7a49ab0e` 开头）；`v0.4.3-Runtime.zip`（Alpha0.3.4.2 和 Alpha0.3.4.1；116,484,918 字节，
SHA256 以 `07dd7774` 开头）、`v0.4.1-Runtime.zip` 和 `v0.4.0-Runtime.zip`（Alpha0.3.4.1）
仍然可用（`v0.4.1-Runtime.zip` 中的 `dlssnr_amd_pass1.dll` 为 9,916,928 字节，SHA256 以 `823063eb` 开头；
`v0.4.0-Runtime.zip` 中为 10,027,008 字节，`d62be3d8`）。支持的版本：
0.2.17、0.3.0、0.3.1、0.3.2、0.3.3、0.4.0 以及上面提到的运行时 zip，最高到 0.5.0。不要在 AMDNR 旁边安装 danielblnc 自己的安装程序或它的 `dxgi.dll` / `version.dll` / `winhttp.dll`：
AMDNR 已经在运行他的运行时。**比 0.5.0 更新的 danielblnc 版本不会被本次发布驱动：**Neural 选项卡会写出文件名及其
版本，并说明这一点。使用 0.5.0、0.4.3、0.4.1 和 0.4.0 时，仅 danielblnc 才有的设置（Network style、Tone curve、
Black lift、Game exposure、Fast mode）可以使用，并且自 0.3.5 起，你在 `dlssnr_on_amd.ini` 中自己设置的 `Async` 键会
传达给运行时（见"压缩包里有什么"）。

**在带集成显卡的电脑上，lmxxf 没有任何效果，或一开始就停止？** 0.3.3.2 已修复。在开启了集成显卡的 Ryzen 台式机、
带 AMD APU 和 Radeon 独显的笔记本，或装有两块 AMD 显卡的电脑上，游戏所用的显卡往往不是 HIP 设备 0。lmxxf 因此在
第一帧就失败（`hipErrorInvalidHandle (400)`，随后 `lmxxf_backend.log` 中出现 "session is poisoned"），并在整个会话中
保持关闭。请把 `OptiScaler.dll`（即你重命名后的文件，例如 `dxgi.dll`）和 `LmxxfNrRuntime.dll` 都替换为 0.3.3.2 或更新
版本的文件。尚未在这类电脑上测试：如果 lmxxf 仍然停止，Neural 选项卡现在会说明原因；请发送 `lmxxf_backend.log` 和
`amd_bridge.log`（其中列出了各个 HIP 设备）。

**带集成显卡和 Radeon 独显的电脑（开启了集成显卡的 Ryzen 台式机，或笔记本）：NR 从不启动，而 Neural 选项卡或 GPU 行
显示的是集成显卡？** AMDNR 的神经通道运行在游戏用来绘制的那块 GPU 上。如果 Windows 把游戏启动在集成显卡上，NR 就完全
不会在你的 Radeon 上运行。把游戏指定给独立显卡：Windows 设置 > 系统 > 屏幕 > 显示卡，添加游戏的 `.exe`，选项，高性能；
然后重启游戏，查看 Neural > Diagnostics 中的 GPU 行，它写出 NR 所运行的适配器（当该适配器不是主 GPU 时，
`OptiScaler.log` 中会有一行 `AMD neural: NR runs on ...`）。曾在 Ryzen 台式机上的 Starfield 中出现。自 0.3.5 起，Neural 选项卡中的这一行会说明属于三种情况中的哪一种——还没有
运行时；NR 正运行**在集成显卡上**（某个运行时接受的 APU，例如与 Radeon 显卡并存的 Radeon 780M：NR 在那里运行，比在
显卡上慢得多）；或运行时位于另一个适配器上——并且 `amd_bridge.log` 会把每个适配器列出一次。

**在 RX 9070 / 9070 XT 上，lmxxf 的状态行显示 `c32w=off:nofile`？** 游戏 `.exe` 旁边有一个旧的
`DLSS5-AMD\native-game-tiled-assets` 文件夹（以前安装 lmxxf 时留下的），它会代替 `LmxxfNrRuntime.pak`
被使用。该文件夹里没有 c32w 内核，所以 lmxxf 仍以旧的速度运行。请删除 `DLSS5-AMD` 文件夹或给它改名：pak
已包含 lmxxf 需要的一切。早于 0.3.3.2 的 `LmxxfNrRuntime.pak` 也会显示同样的状态；请换成本次发布中的那个。
同一行中出现 `fk=fff-` 也是同样原因（旧 pak 或散装文件夹）：lmxxf 仍会运行，但速度是旧的。

**danielblnc：NR resolution 离开 100% 时，NR 风格仍会变化？** 0.3.4 到 0.3.5 中仍未解决，默认值不变。100% 时 Residual
strength 0.99 会给出 1.00 的 99%（0.3.3.2 已修复）；离开 100% 时（包括 Dynamic NR 的各档和 Balanced / Performance
预设），strength、limit 和 edge fade 仍作用于整个结果，所以观感可能变化。0.3.4 增加了 A/B 对比来找出正确的修复：
Neural > Diagnostics > **Edit shaper (A/B, not saved)**，可选 Literal、F1 和 F2，另有 Only below 100% 和 Carry cap
（仅 danielblnc；Save Settings 不会保存它；ini 键为 `[DlssNr] AmdEditShaper`、`AmdEditShaperLimit`、
`AmdEditShaperScope` 和 `AmdEditShaperCarryCap`）。如果其中某个选项让 85% 看起来和 100% 一样，请在 Discord 上附截图
告诉我们。lmxxf 不受影响。

**每按一次键菜单就开关两次，或者菜单打开时整个桌面的键盘和鼠标都失灵（Assetto Corsa）？** 0.3.4 已修复：400 ms
内对菜单键或 NR 键的第二次按下会被忽略（`[Hotfix] MenuToggleDebounceMs`，0 = 旧行为）；菜单打开时，会跳过游戏的
低级键盘或鼠标钩子，但按键仍会传递给 Windows（`[Hotfix] MenuLowLevelHookPassThrough=false` = 旧行为）。尚未在
Assetto Corsa 中确认：如果仍然出现，请发送报告 zip。

**菜单在运行时选择窗口上自行打开、点击没有反应，或者菜单键只在按住时才隐藏菜单（Assetto Corsa）？** 0.3.4.2 已修复：
菜单键每一次实际按下只切换一次（迟到的按键消息会被忽略），短于一帧的点击和菜单按键会被重放，运行时选择窗口可用
`1` / `2` / `Enter` / `Esc` 及其标题栏的 X 操作，关闭菜单即视为"Decide later"；该窗口不再自行打开菜单。尚未经
Assetto Corsa 玩家确认：如果仍然出现，请发送报告 zip。要恢复 0.3.4.1 的菜单键和点击行为：
请自行在 `OptiScaler.ini` 的 `[Hotfix]` 下添加 `DiagInputHooksSkip=presslatch,clickreplay`（没有新键；随附的 ini
只在注释中说明这一行）。
0.3.5 为 Assetto Corsa 增加了两项：AMDNR 会避开游戏目录中外来的 `nvngx.dll`，并对 D3D11On12 设备创建加以保护；从不
通过 D3D12 呈现的 DX11 游戏会得到一个供神经通道使用的引导 D3D12 队列（`[DlssNr] AmdBootstrapQueue`，auto）。尚未经
Assetto Corsa 玩家确认。

**Uncharted: Legacy of Thieves Collection 以前在 RX 9000 上开启神经渲染后启动几秒就崩溃？** 0.3.5 已修复：该游戏
在 192 KiB 的小型 fiber 上执行任务，而 HIP 的首次初始化（驱动在游戏进程内编译其辅助内核）曾在第一帧 NR 时撑爆这个栈。
现在神经桥接对 HIP 的首次使用在它自己的大栈上运行（`[DlssNr] BigStackCall`，auto）。如果你为 0.3.4.2 在那里设置了
`[DlssNr] Enabled=false`，请把它重新打开。尚未经 RX 9000 玩家确认：如果仍然出现，请发送报告 zip。

**开启神经渲染后，动态分辨率游戏（The Last of Us Part II）闪烁？** 0.3.5 已修复：该游戏每分钟改变数百次渲染尺寸，
而每一步都会重建网络并重置其历史。现在仍处于已分配范围内的步骤会被保持（不重新稳定、不预热、不重置历史，两个运行时
都适用）；更大的帧或真正的下降仍会重新分配。报告中的 stats 行会统计被保持的步骤数。尚未在该游戏中确认。

**帧生成：fps 没有提升，或者一直提示 "restart the game"？** 在几乎每一份报告中，帧生成只是没有被开启——是关着，而
不是坏了。开启需要五步，自 0.3.5 起 Frame Gen 选项卡和日志会写出缺少的是哪一步：

1. Frame Gen 选项卡：**同时**选择 FG Input 和 FG Output。自带帧生成的 DX12 游戏：以它的 DLSS FG 或 FSR 3.1 FG 作为
   输入（带 FSR 3.1 FG 的游戏：选 "FSR 3.1 FG"，不是 "FSR 3.0 FG"）；游戏完全没有帧生成：FG Input = OptiFG
   (Upscaler)，开启 HUD fix。DX11：只能用 OptiFG。Vulkan：没有 FSR FG / XeFG 输出（请使用游戏自己的）。
2. **Save Settings**。
3. **完全关闭游戏并重新启动**——帧生成无法在游戏过程中开启。
4. 在游戏自己的画面选项中把它的帧生成**打开**：DLSSG 输入对应 DLSS Frame Generation（以 DLSS 作为放大器），
   FSR 3.1 FG 输入对应 FSR 帧生成（配合 FSR）。
5. 再次打开菜单，进入 Frame Gen 选项卡，在 Frame Generation 下勾选 **Active**。勾选之前不会生成任何帧（XeFG 可能
   还需要再重启一次）。

然后在游戏中**关闭**：独占全屏（XeFG 需要无边框）、V-Sync 和帧率上限（或把上限设为基础 fps 的两倍），以及游戏自带的
XeSS 帧生成（如果有的话；每个窗口只能有一个帧生成器；加载了自己 XeSS FG 的游戏会在选项卡中得到一条提示）。fps 没有
提升 = 帧生成是关着的——日志会写 `... Enabled is off ...: no frames are generated. Frame generation off, not broken.`；
fps 减半 = 有帧率上限或 V-Sync 挡住了生成的帧。如果仍然失败，请在失败后点 **Save report**，并把 zip 连同以下信息一起
发出：游戏、你选的输入输出组合、开启的是游戏内哪个帧生成选项、无边框还是全屏、HDR 开还是关，以及你看到了什么。
完整的常见问题置顶在 Discord 的支持频道中。

**AMDNR 旁边还有其他模组（Cyberpunk 2077 的 RED4ext、Cyber Engine Tweaks）或 ReShade？** 自 0.3.5 起，报告和日志
会写出其他模组的代理加载器（`report.txt` 中的 `Mod loaders:`；RED4ext 是 `winmm.dll`，Cyber Engine Tweaks 是通过
ASI 加载器的 `version.dll`），并在 AMDNR 占用了该游戏某个已知加载器的名字却没有链式加载它时发出警告。AMDNR Launcher
从不占用或移动加载器的文件：它会选择另一个代理名（Cyberpunk 2077：`dxgi.dll`），它的 Doctor 会写出之前某次 INSTALL
移到一旁的任何加载器（`AMDNR_backup`）。AMDNR 旁边的 ReShade 通过模块的版本资源或 ReShade 的 add-on 导出来识别，
从不依据文件名，并会在报告和日志中写出（`[Game] ReShade detected: <module>`）；不会加载、挂钩或阻止任何东西，让两者
共享 D3D12 设备的方案已设计好，但尚未实现。

**Neural 选项卡或 `amd_bridge.log` 中出现 `No HIP adapter matches D3D12 LUID`？** 自 0.3.5 起，这一行会写出双方——
游戏的 D3D12 适配器（名称、LUID）以及每个 HIP 设备（序号、名称、gfx、LUID）——并给出可能的原因和该怎么做：游戏运行在
集成显卡或另一块显卡上（Windows 设置 > 系统 > 屏幕 > 显示卡：把游戏指定给独立显卡）、HIP 运行时没有身份信息（游戏旁边
有一个多余的 `amdhip64_7.dll`：删除它）、根本没有 HIP 设备（安装 AMD 官方的 Adrenalin 驱动）、同一块显卡以另一个身份
出现，或者不是 AMD 适配器。两个运行时写出的文字相同。

**Vulkan 游戏（Indiana Jones and the Great Circle）一启动就报 "Could not create the Vulkan device
(VK_ERROR_EXTENSION_NOT_PRESENT)"？** 0.3.2 已修复：继承自 NVIDIA 神经路径的代码向 AMD 驱动请求了两个
仅 NVIDIA 才有的设备扩展。Vulkan 游戏经由 OptiScaler 的 D3D12 桥接进入神经渲染（见"系统要求"）。

**lmxxf 在 Vulkan 游戏的第一帧 NR 时卡死？** 0.3.3 已修复；NR 启动时会有一次约 1 秒的停顿。若某次 Vulkan
会话在 lmxxf 给出第一个结果之前就停止，下次启动会改用 danielblnc 的运行时，Neural 选项卡会说明原因；
在那里点 **Retry lmxxf**（它会删除 `OptiScaler.dll` 旁边的 `lmxxf_vk_launch.pending`）即可再次尝试 lmxxf。

**danielblnc 在 Vulkan 游戏（Indiana Jones）中开 2-3 个 Neural passes 时卡住数秒，然后 NR 停止？** 0.3.3 已修复：
在 Vulkan 游戏中它只运行 1 遍，它在提交后的 80 ms 等待也已去掉。每次会话的第一帧 NR 仍会停顿约 5 秒；
运行时选择下方的说明会解释它的日志行。测试者：`[DlssNr] AmdVkLateCopyWait=true`（实验性，默认关闭，尚未在
游戏中测试）预计可消除这次停顿；请发送 `OptiScaler.log`、`amd_presr.log` 和 `dlssnr_on_amd.log`。

**NR 运行期间 lmxxf 的内存占用一直上涨？** 0.3.3 已修复（此前在 60 NR fps 下每小时约 45 GB）。仍然存在的：
danielblnc 在每个超过约 1 MP 的新 NR 尺寸上仍会占住显存（自 0.3.3.2 起，非 100% 时其尺寸按 64 像素取整，因此只会出现少数几种尺寸）；
使用 danielblnc 多次调整后请重启游戏。自 0.3.3.2 起，lmxxf 每次更改 NR resolution 或 DLSS 模式不再占住约 97 MB：
它为每种网络尺寸只创建一次网络缓冲区并重复使用（每次更改仍会留下约 10-25 MB 的少量显存）。

**Streamline 游戏启动时报 slInit 错误 0x18（在 AMD 上的 NBA 2K27 中出现）？** 0.3.3 堵住了 OptiScaler 的
Streamline 插件钩子可能引发该错误的一条途径，但尚未确认这就是 NBA 2K27 的原因。`OptiScaler.log` 现在会记录
`slInit returned ...` 和 `[SLINIT]` 行：反馈时请附上日志。

**找不到 Ray Regeneration 的设置？** 自 0.3.5 起，它们位于其独立的 **Ray Regeneration** 选项卡上，紧跟在 Upscaling
之后，在 Ray Regeneration 于游戏中运行时绘制（一旦运行过，本次会话内会一直保留）；此时 Neural 选项卡中的 **Ray
Regeneration** 一行会提供一个 **Open Ray Regeneration** 按钮。它没有运行时，该选项卡不会绘制，Neural 选项卡中的那一行
会说明原因：游戏没有开启光线重建、Ray Regeneration 放弃了这款游戏以及原因，或者它上一次运行是多少秒前。在 RX 7000 上，
还有一行浅色文字会补上：那里以 preview 形式提供它，AMD 的降噪器没有面向 RDNA 3 的提供程序，因此一旦游戏开启光线重建，其画面就会
在不带降噪的情况下被升采样，而 `[FSR-RR] FfxDenoiserAllowPreRdna4=false`（或取消勾选 Upscaling 选项卡中的
**Preview: Ray Regeneration on this card (restart)**）会关闭该 preview，让游戏保留自己的降噪器。在 RX 6000 及更早的显卡上则说明：
这块显卡上不提供它，AMD 只为 RDNA 4 发布该降噪器，而 `[FSR-RR] FfxDenoiserAllowPreRdna4=true` 仍可让它提供。
要让它运行，请在游戏自身的画面设置里：把升采样器选为 **DLSS**（不是 FSR，也不是 XeSS），开启**光线追踪**或路径追踪，
并开启**光线重建**（DLSS-RR）；此时 Upscaling 选项卡会显示 "FSR Ray Regeneration"。哪个问题该调哪个设置，见 Ray Regeneration 设置指南
**RR-BEST-SETTINGS.md**（不在 zip 内）。

**Ray Regeneration 看起来有颗粒或噪点？** 请先在**关闭神经渲染**的情况下判断（取消 Neural 选项卡顶部的 **Enable Neural Rendering**、游戏中按 Home，或设置
`[DlssNr] Enabled=false`）：神经通道在 Ray Regeneration 之后、对它的输出运行，所以开着 NR 截的图说明不了降噪器的问题。
然后按颗粒的类型——静止画面里蠕动的颗粒、明亮的亮点、脸上的颗粒、移动角色身后的拖影——可以尝试的设置都在同一份指南 **RR-BEST-SETTINGS.md** 里。**0.3.4.2 没有改动任何降噪器或锐化的默认值**：数值与 0.3.4.1 相同。真正的变化是：AMDNR 在 Ray Regeneration
之后添加的锐化现在有了自己的 ini 键 `[Sharpness] RrDefaultSharpness`（默认值仍是 0.25），所以改成 0.15、0.10 或 0
只需改 ini，不必等新版本。用锐度滑块去追
颗粒之前先看一眼你的 ini：`[Sharpness] Sharpness` 下的数值在 `OverrideSharpness` 关闭时完全无效，而你在菜单里勾上
**Override** 的那一刻它就会立即生效——这也是为什么 Image > Sharpness 现在会标出它（"ini Sharpness 1.00 waits for
Override"）。两件我们不会含糊其辞的事：一部分颗粒来自游戏自身的光线采样——AMD 的降噪器并非为
修复相关性噪声而设计，而提供 DLSS 光线重建的游戏会关掉自己的降噪器、把原始信号交给我们；静止画面里蠕动的颗粒在我们这边
有一个结构性原因，任何滑块都无法把它完全去掉。那是一个已知问题。0.3.5 给它加上了一个数值：Ray Regeneration 选项卡的
Diagnostics 区块会显示输入与输出的颗粒以及静止镜头下的闪烁（在该选项卡打开时测量），并且该选项卡的 **Denoiser backend**
行可以设为 Off，它会告诉游戏光线重建不受支持，于是游戏保留自己的降噪器（重启后生效）。AMDNR 自己的降噪器属于 0.3.6 的工作。

**游戏里 Ray Reconstruction 已开启，但 Neural 选项卡显示 "Ray Regeneration is off in this title"？**
该游戏没有提供 FSR Ray Regeneration 所需的数据：它的 DLSS 插件传入的是空的相机矩阵（Satisfactory），NVIDIA 的光线
重建把它们视为可选，而 FSR Ray Regeneration 必须要有。此时改由 FSR 超分运行，NR 回到它平常的 SR 之前位置；Upscaling
选项卡也会说明。自 0.3.4 起，在具有这种特征的 Unreal 游戏中它会在整个会话内保持关闭。请在游戏中关闭光线重建，并恢复
引擎自身的降噪设置。

**育碧 Anvil 引擎游戏（AC Black Flag Resynced、Shadows、Mirage）弹出 "DX12 Error 0x80070057"？**
这些游戏自带 XeSS 帧生成。自 0.3.5 起，Frame Gen 选项卡会在那里显示一条建议（保持游戏自带的 XeSS FG 关闭，否则两个
生成器会共用一个窗口），而 AMDNR 的 XeFG 输出仍会运行；最简单的做法是使用游戏自己的 XeSS FG 选项，并保持 AMDNR 的
帧生成关闭。若仍然出现，请设置
`[FrameGen] Enabled=false` 和 `[fakenvapi] ForceXeLL=false`，并附日志反馈。

**《最后生还者 第一部》（The Last of Us Part I）启动时崩溃？** 那是游戏自身 Streamline 初始化的问题，是已知的
OptiScaler 问题：把游戏目录中的 `sl.common.dll` 重命名为 `sl.common.dll.bak`，并在游戏设置中选择 **FSR 3.1**
而不是 DLSS。

各版本的完整说明：`CHANGELOG.md`（压缩包和仓库中都有）。

## 路线图

- **0.3.5**（本构建版）—— **AMDNR Anywhere**（preview，RX 9000，通过启动器）；带按显卡、按 API 状态行的 Ray Regeneration
  选项卡、Denoiser backend 的 Off 选项和噪点数值；放大之后的神经通道（`AmdPlacement`）；Frame Gen 选项卡和日志会说明为什么
  没有生成任何帧；RX 9000 上 Kien 的 lmxxf 0.37（MIT）；0.3.5 模块集（在 RX 7000 和 Z1 Extreme 级掌机上快约 10%，
  画面相同）；动态分辨率的变化步骤被保持而
  不重建网络；掌机的携带修复；共享的运行时文件夹（`AmdRuntimePath`）；写出双方信息的 HIP 适配器拒绝提示；报告中写出其他
  模组的加载器和 ReShade；不再干扰反作弊和崩溃报告进程；修复 Uncharted（RX 9000）、Assetto Corsa、F1 25 和 Kingdom Come:
  Deliverance II（Game Pass）、开启帧生成时的 Control Resonant、Tainted Grail: The Fall of Avalon 和 Dead Space、GTA V
  Enhanced、Half-Life 2 RTX 及其他 Vulkan 游戏的问题，以及 Linux / Proton 的文字；AMDNR Launcher 0.3.5.1。
- **0.3.4.2** —— 热修复：Assetto Corsa 的菜单（菜单键每按一次只切换一次、短于一帧的点击会被重放、运行时选择
  窗口可用按键操作、关闭菜单即视为"Decide later"）、运行时选择窗口不再自行打开菜单、Ray Regeneration 区块在 Neural 选项卡中
  始终显示并说明它为什么没有运行、多接受一种 danielblnc 运行时布局、Wine / Proton 文字修正、README 新增
  内容（代理名、Uncharted、混合显卡电脑、RX 9000 上的 `AmdLmxxfTierSnap`、两条 Ray Regeneration 常见问题）；除那一行已接受的
  布局之外，NR 与 0.3.4.1 逐字节相同。
- **0.3.4.1** —— 热修复：游戏没有传入锐化值时 Ray Regeneration 的锐化（Windows；Linux / Proton 上默认
  不加）、Control Resonant 误弹的 "Upscaler failed to run!" 提示、Linux / Proton 上菜单附着到游戏窗口、Save report
  会写明 vkd3d-proton / DXVK；AMDNR Launcher 0.3.4.1（九种语言、搜索、收藏、隐藏、重命名、CHOOSE GAME .EXE、PLAY、
  完整的 UNINSTALL）；NR 不变。
- **0.3.4** —— 全新菜单（重做的 Neural 选项卡、所有选项卡统一的外观、Save report）；lmxxf 在 RX 7000
  上更快（默认启用网络尺寸档位），在 RX 9070 / 9070 XT 上也更快（lmxxf 0.31 内核）；lmxxf
  支持掌机 APU（实验性；新增 360p 和 576p 网络尺寸）；lmxxf 获得 Network output、Encoding、Residual edge fade、原生角色遮罩和可选的 Fast mode；AMDNR Screen GI（preview）；danielblnc 运行时设置（Network style、Tone curve、Black lift、Game exposure）和
  高光色彩保护；Ray Regeneration 调节与诊断；修复 Assetto Corsa 中的菜单输入、Shadow of the Tomb Raider、Marvel's Midnight Suns 和 The Last of Us Part II
  的问题、正常退出记录和日志。
- **0.3.3.x** —— lmxxf 支持 RDNA 3（RX 7000；AMDNR 自己的后端）；两个运行时都可用的 RenoDX
  色彩合成（实验性，需手动开启）；lmxxf：Full network 选项、修复内存泄漏、修复 Vulkan 游戏的问题（在 Vulkan
  桥接内延迟上传权重）、0.29 内核（逐位一致、更快）；danielblnc 在 Vulkan 游戏中：1 个 Neural pass、更清楚的
  提示信息、可选的延迟复制等待；XeFG 最高 10X（需手动开启，D3D12）；Streamline 启动加固与诊断；FSR Ray
  Regeneration 路径追踪配置与皮肤平滑；UE5 健壮性改进。
- **0.3.2** —— 0.3.1 的反馈：Vulkan 游戏可启动并可运行 lmxxf、lmxxf 色彩与 danielblnc 对齐（自动曝光）、
  运行时下拉框、光线重建状态与调节；zip 内附 Nukem9 的 dlssg-to-fsr3，用于 Vulkan 帧生成。
- **0.3.1** —— 修复 0.3.0 首批反馈的问题（仅安装 lmxxf 时从不运行、燕云十六声（Where Winds Meet）NR 无效、切换 DLSS
  画质时崩溃、GTA V Legacy），并新增 NR 风格预设与三个自定义槽位。
- **0.3.0** —— **lmxxf** HIP 神经运行时（RDNA 4）作为可选运行时与 danielblnc 的并列，以
  `LmxxfNrRuntime.dll` + `LmxxfNrRuntime.pak` 发布：网络历史、真实的 Neural passes、编辑整形器、光线
  重建之后的位置、逐游戏诊断与自我修复。特别感谢 TheAutomatic，本次集成建立在他的 DLSS 5 AMD
  project 工作之上。
- **0.3.6** —— RX 6000（RDNA 2）：AMDNR 自己的内核，受硬件门槛把关，以 360 档和 Model interleave 作为 Navi 21 上的
  preview；AMDNR 降噪器（ARD）preview，一个位于光线重建调用背后、AMDNR 自己的降噪器，面向 AMD 降噪器拒绝的显卡；
  AMDNR Anywhere 在 RX 7000 上测试后登陆 RX 7000。
- **0.4.0** —— 1440p 网络档位；通过桥接在 Vulkan 游戏上运行 Ray Regeneration；跨适配器的神经渲染（游戏在一块显卡上，
  网络在 AMD 显卡上）；超越 preview 的 Anywhere（自身没有放大器的游戏，由 OptiScaler 同时提供放大器和神经渲染）。
- **以后** —— 任意窗口（桌面）上的 AMDNR。

---

## 致谢

本构建版是对他人工作的接线整合。如果你觉得它有用，感谢应归于上游。

- **TheAutomatic** —— DLSS 5 AMD project —— https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** —— DLSS-NR on AMD by Daniel Blanco —— https://github.com/danielblnc/DLSS-NR-on-AMD （`*Runtime.zip` 文件，未经修改）
- **lmxxf**（Kien）—— https://github.com/lmxxf/dlss5-on-amd-9070xt-porting （网络移植、内核与 HIP 运行时，MIT）
- **TheAutomatic** —— `LmxxfNrRuntime.cpp`、`LmxxfNrApi.h`、`LmxxfProductionOptions.h`：portions contributed to lmxxf by TheAutomatic (MIT)
- **lmxxf 0.31 内核**，位于 `LmxxfNrRuntime.pak` 中（the ViT projection (lmxxf031-vit-wide-deep), the C512 QKV and mix kernels (lmxxf031-c512-m32-mh, lmxxf031-c512-m32-deep) and one-wave-per-head attention (lmxxf031-c64-wave2)）—— 属于 lmxxf（Kien，MIT），由 AMDNR 按 lmxxf 的源码与构建方法构建；AMDNR 负责加载、SHA-256 固定校验、按显卡启用与回退
- **lmxxf 0.37 和 0.39 内核**，位于 `LmxxfNrRuntime.pak` 中（面向 RX 9000 的 lmxxf037-* 和 lmxxf039-* 模块），其启动代码位于 `LmxxfNrRuntime.dll` 中 —— lmxxf 0.37 / 0.39 by Kien (MIT)，按 lmxxf 构建的原样提供；AMDNR 负责作为一个固定组加载、SHA-256 固定校验、按显卡的默认设置、关闭开关与回退
- **c32w 内核**（0.3.3.2）—— AMDNR 自有的 RDNA 4 单 wave 内核，用于 lmxxf 的网络，Copyright (c) 2026 3zwr1 (AMDNR)；思路参考 AMD 公开的 RDNA 4 WMMA 文档（GPUOpen、ROCm matrix instruction calculator）
- **AMDNR 的 RDNA 3 后端**（0.3.3；0.3.4 中新增掌机构建 gfx1103 / gfx1150）、网络尺寸档位策略和小网络尺寸（0.3.4）—— Copyright (c) 2026 3zwr1 (AMDNR)
- **Matheus / dlss-5-amd** —— https://github.com/MatheusGViana/dlss-5-amd-project
- **Dagherbou / OptiScaler_DLSSNR** —— https://github.com/Dagherbou/OptiScaler_DLSSNR
- **wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass** —— https://github.com/wilsjo2/OptiScaler-DLSSNR-PreSR-Multipass
- **Nukem9** —— dlssg-to-fsr3 —— https://github.com/Nukem9/dlssg-to-fsr3 （GPLv3，未经修改）
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR** —— AMDNR Anywhere 在其中运行的窗口捕获宿主 —— https://github.com/Blinue/Magpie （该分支：<https://github.com/SAOG0721/Magpie>）
- **RenoDX** —— clshortfuse —— https://github.com/clshortfuse/renodx （色彩合成算法，MIT）
- **Coldwood1026** —— XeFGUnlock（GPL-3.0），内置 XeFG 多帧解锁及其节拍的基础
- **Zach Hembree (DarkHelmet)** —— OptiScaler 的 FSR Ray Regeneration，AMDNR 光线再生路径的源头；由 **burak113** 继续开发，AMDNR 从其分支移植（OptiScaler 分支 ffx-denoise-experimental，GPL-3.0）
- **Screen-space GI**（继承的效果；0.3.4 起已从菜单中移除，ini 中为 `[AmdRtgi] Enabled`）—— AMDNR 从 OptiScaler-AMD-PreSR 一脉继承的效果；功劳归于其原作者。它需要 danielblnc 包中的 `experimental_lighting` 文件夹，AMDNR 不附带该文件夹。
- **AMDNR Screen GI**（0.3.4 preview）—— AMDNR 自己的作品，Copyright (c) 2026 3zwr1 (AMDNR)，依据已发表的论文编写（Therrien、Levesque 和 Gilet 2023；Jimenez 等 2016；Schied 等 2017；其余见 `CHANGELOG.md` 和 `Licenses/AMDNR_NOTICE.txt`）
- **OptiScaler** —— Overclockers —— https://github.com/Overclockers/OptiScaler-Releases

## AMDNR Launcher

**AMDNR Launcher**（0.3.4 新增）是一个 Windows 10 / 11 程序，也可以在 Linux 上通过 Proton 运行
（实验性，我们尚未测试：见"Linux / Proton"一节）。它按游戏安装和更新 AMDNR：它会找到你的游戏（Steam、Epic、
Xbox 应用、Ubisoft Connect、EA app、GOG、Rockstar、Battle.net 和 Amazon Games），选好 DLL 名称，下载构建版和你选择的
danielblnc 运行时，用它的 Doctor 检查每一次安装，为自身没有放大器的游戏运行 AMDNR Anywhere（PLAY ANYWHERE），
并能自我更新。请从发布页下载 `AMDNR-Launcher.exe`，即最新发布版中的
AMDNR Launcher：
<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>

**Launcher 0.3.5.1 新内容**（位于 Alpha0.3.5 发布页；它会先更新自己，再更新你的游戏）：

- 在自身没有放大器的游戏上使用 **PLAY ANYWHERE**（见"AMDNR Anywhere"）：启动器从作者的发布页获取捕获宿主，启动游戏，
  并在游戏窗口上运行神经渲染，按钮旁显示宿主设置的只读摘要（设置本身在菜单的 Anywhere 选项卡中）。本次发布支持
  RX 9000。模组无法加载进去的游戏（32 位、没有放大器的 DirectX 9 / OpenGL 游戏）会在 INSTALL 时被拒绝，附上一行清楚的
  说明，并改为提供 Anywhere。
- **UPDATE ALL** 和启动时更新；安装包镜像；下载失败时提供 RETRY / OPEN DOWNLOAD / IMPORT PACKAGE。
- **COLLECT LOGS** 会为一次运行开启崩溃处理器，并附带 `amdnr_crash.log`、最新的转储文件和 danielblnc 的
  `dlssnr_on_amd.ini`。
- **PLAY** 通过应用 ID 启动 Xbox / Microsoft Store 游戏。
- **一个 PLAY 按钮**，带路线选择器（游戏自带的放大器或 AMDNR Anywhere，按游戏记住），每个游戏页面上还会显示图形 API
  （DX9 / 10 / 11 / 12 / Vulkan / OpenGL）。
- **十二种语言**：土耳其语、韩语和匈牙利语加入下面列出的九种语言。
- **Rockstar 游戏通过其商店启动**（Steam、Epic 或 Rockstar Games Launcher），从不直接运行其 exe。
- **AMDNR Anywhere**：游戏以低于正常的 GPU 优先级运行，让宿主和神经渲染优先；捕获宿主的托盘图标被隐藏。
- **Resident Evil 2 / 3 / 4 (2023) / Village**：显示一条设置提示（它们需要 PureDark 的放大器插件配合 REFramework）。
  更多没有 DLSS、XeSS 或 FSR 2+ 的游戏会在任何下载之前被标记为不受支持，并写明原因。
- 在 RDNA 3 掌机的游戏页面上提供 **FSR 4 (INT8)** 实验性选项（需手动开启）。
- **从不占用或移动其他模组的加载器**（RED4ext、Cyber Engine Tweaks 等）：启动器会选择另一个代理名，它的 Doctor 会写出
  之前某次 INSTALL 移到一旁的任何加载器。
- **GTA V Enhanced**：游戏页面带有路线说明（游戏中选择的 FSR 3.1 是输入；神经渲染在它之前运行），以及一条关于
  `settings.xml` 的提示。

**Launcher 0.3.4.1 新内容：你们提出，我们实现**（根据 Discord 上的第一批反馈打造）：

- 九种语言（自 0.3.5.1 起为十二种）：英语、阿拉伯语、中文（简体）、法语、西班牙语、葡萄牙语、意大利语、俄语和
  波兰语，以及土耳其语、韩语和匈牙利语。启动器会跟随你的
  Windows 语言（不在其中时使用英语）；可以在 LANGUAGE（语言）或 SETTINGS（设置）中改选其他语言，首次启动时
  出现的 SETTINGS 页面上也提供语言选项。Doctor 的检查结果、安装消息和 COLLECT LOGS 报告保持为英文，方便支持人员阅读。
- 搜索游戏库；收藏（点亮星标，带星标的游戏排在最前）；隐藏游戏（HIDDEN 可以把它们重新显示出来）；重命名游戏。
- CHOOSE GAME .EXE：启动器选错了 exe 或者没找到时，你可以自己选择游戏的 exe。Cyberpunk 2077 和 The Witcher 3
  （REDengine）现在不用手动选择也能在正确的文件夹中找到。
- 每个游戏的页面上都有 PLAY 和 OPEN FOLDER；STORES 可以整个开启或关闭某个商店；你手动添加的文件夹会一直留在
  列表中，即使所在的驱动器已拔出也是如此，直到你把它们移除。
- UNINSTALL 会先询问，然后删除模组放置的所有内容，包括游戏运行期间它写出的文件（日志、缓存、崩溃转储、未完成的
  报告）；它会保留已完成的 Save report zip，以及使用代理名称但已不再是 OptiScaler 的 DLL（游戏自己的）。
- COLLECT LOGS 可用于任何游戏（无论是否已安装），并附带一份扫描报告。
- 能识别掌机和 APU（ROG Ally Z1 Extreme 及其他 Ryzen APU），并提示 AMDNR 在这些设备上仍处于测试阶段。
- Linux（实验性，我们尚未测试）：同一个 Windows exe 可以在 Proton 下运行，会在那里显示一条提示，说明该怎么做，
  还会查找你 Linux Steam 游戏库中的游戏；步骤见"Linux / Proton"一节。如果能用，请在 Discord 上告诉我们。

其源代码位于本项目 GitHub 仓库的 `Launcher/OpenSource/`，使用单独的许可证 `Launcher/OpenSource/LICENSE.txt`。
它**不**受本仓库 GPL-3.0 `LICENSE` 约束：源代码公开可查看（source-available），保留所有权利，
Copyright (c) 2026 3zwr1 (AMDNR)。启动器的清单文件为 `Launcher/manifest.json`。另见
`Licenses/AMDNR_NOTICE.txt` 第 7 节。

## 版权 / 许可证（Copyright / License）

AMDNR 版权所有 Copyright (c) 2026 3zwr1 (AMDNR)。它是 OptiScaler 的一个分支（fork），按 `LICENSE` 中的
GPL-3.0 许可证分发。

AMDNR 自己的工作附带一条依据 GPL-3.0 第 7(b) 条的附加条款（见 `Licenses/AMDNR_NOTICE.txt`）：任何使用它的
副本、分支或衍生作品都必须保留其声明，并注明 **AMDNR by 3zwr1**（<https://github.com/3zwr1/AMD-NR---OptiScaler>）。

**AMDNR 菜单版权。** AMDNR 菜单——包括其布局、设计、文字以及 AMDNR 为其添加的代码——版权所有 Copyright (c) 2026 3zwr1 (AMDNR)。它是本 GPL-3.0 分支的一部分，并附带以下附加条款（GPL-3.0 section 7）：(b) 任何人复用其中任何部分，都必须保留这行版权声明，并在菜单和 README 中醒目地注明 AMDNR by 3zwr1；(c) 不得把它或其修改后的副本冒充为自己的作品；修改版本必须清楚标明已被修改；(e) 不授予对 AMDNR 名称或标志（logo）的任何权利；其他项目不得使用它们。

上文致谢的上游工作仍归其作者所有，适用其各自的许可证；AMDNR 不对其主张任何版权。

源代码将随 AMDNR 0.5.0 发布。

## 法律声明

本构建版按 `LICENSE` 中的 GPL-3.0 许可证分发；第三方库许可证位于 `Licenses\`。AMD 神经运行时及其
权重按上述原作者署名再分发，仅为方便使用，不主张任何所有权，不提供任何保证。

NVIDIA 的 `nvngx_dlssnr.dll` 不在这些压缩包中。以上内容均未获得 NVIDIA、AMD 或任何游戏发行商的认可、
关联或支持。它直接驱动一项未公开的功能。使用风险自负。
