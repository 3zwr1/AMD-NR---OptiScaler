# AMDNR — AMD 显卡上的 DLSS 5 神经渲染（OptiScaler 构建版）— v0.3.4

[English](README.md) | **中文** | [Português](README.pt-BR.md) | [Español](README.es.md) | [العربية](README.ar.md) | [Français](README.fr.md) | [Italiano](README.it.md) | [Русский](README.ru.md) | [Polski](README.pl.md)

> **我们需要你的支持。** 加入 Discord 服务器 —— <https://discord.gg/AMDNR> —— 获取帮助、提交
> 问题、领取测试版；每一份带日志的反馈都会让下一个版本更好。

DLSS 5 神经渲染（Neural Rendering）在 AMD 显卡上运行，内置于 OptiScaler，因此 OptiScaler 能挂钩的
任何 Direct3D 12 游戏都可以使用。在神经渲染之上还有：大幅提升帧率的模型交错（model interleave）、
残差合成（residual composition）、解锁至 6X 的 XeSS 帧生成（D3D12 游戏可选开启至 10X），以及面向使用
DLSS 光线重建游戏的 FSR Ray Regeneration。

**Discord：<https://discord.gg/AMDNR>** —— 支持、问题反馈（`#bug-report`）、测试版。

**支持本项目：<https://ko-fi.com/3zinr>**

> **danielblnc 运行时是 Daniel Blanco 的作品。** `Runtime.zip` 中的 AMD 神经运行时（`dlssnr_amd_pass1..3.dll`）是
> **DLSS-NR on AMD by Daniel Blanco (danielblnc)** —— <https://github.com/danielblnc/DLSS-NR-on-AMD>。
> Copyright (c) 2026 Daniel Blanco, all rights reserved. AMDNR 经他许可，未经修改地分发它；它不是 AMDNR 的作品。
> 请支持他的项目。其他所有人的完整致谢见本页末尾。

> **0.3.4 新内容：** 全新的菜单（重做的 Neural 选项卡、所有选项卡统一的外观，以及把日志打包成 zip 用于反馈的
> **Save report** 按钮）；lmxxf 在 RX 7000 上更快（1440p FSR Quality：RX 7800 XT 上每次网络运行 73.3 -> 52.2 ms，网络时间，在游戏外测得），
> 在 RX 9070 / 9070 XT 上也更快（lmxxf 0.31 内核）；lmxxf 可在掌机 APU 上运行（实验性；一位测试者在游戏中的一次运行：ROG Ally 上的 Shadow of the Tomb Raider 约 29 fps）；lmxxf
> 新增可选的 **Fast mode**；**AMDNR Screen GI**，AMDNR 自己的屏幕空间 GI（preview，默认关闭）；以及大量修复。
> 请同时替换 `OptiScaler.dll`、`LmxxfNrRuntime.dll` 和 `LmxxfNrRuntime.pak`。详情见 `CHANGELOG.md`。

---

## AMDNR - OptiScaler 安装指南

安装非常简单。

### 1. 下载文件

从 GitHub 下载这两个文件（<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>）：

* `AMDNR-vX.X.X.zip`
* `Runtime.zip`

### 2. 解压两个文件

将两个 `.zip` 文件的内容解压出来。

### 3. 把所有文件复制到游戏目录

先把 `AMDNR-vX.X.X` 里的全部文件复制到游戏根目录 —— 也就是游戏 `.exe` 所在的文件夹。

然后把 `Runtime` 里的全部文件同样复制过去。

> **从旧版 AMDNR 升级？** 重新复制全部文件并覆盖。0.3.4 中有三个文件一起更新了：`OptiScaler.dll`（用新文件替换你
> 重命名过的那个，例如 `dxgi.dll`，并按同样方式重命名）、`LmxxfNrRuntime.dll` 和 `LmxxfNrRuntime.pak`
> （440 MB，本版本新文件）。不要与旧副本混用。你可以保留自己的 `OptiScaler.ini`：新设置会使用默认值。

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

一次只试一个名字。不要同时保留多份 `OptiScaler.dll` 的副本。

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
名字不会被替换。请在 `#bug-report` 中附上这个 zip。

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
而不是跳过第 42、43、46 块：略微更忠实，1080p 下慢约 0.5 ms（RX 9070 XT 上 16.6 -> 17.1 ms）。默认关闭。

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
- NR resolution 请从 70% 或更低起步（1080p 渲染下为 720 档；Performance 预设即 70%）。NR resolution 旁显示的开销
  按网络实际运行的档位计算；其悬停提示会写出档位。

### 掌机 APU（实验性，0.3.4 新增）

lmxxf 可在拥有 12 个或更多计算单元的掌机 APU 上运行，通过 3zwr1 开发的 AMDNR RDNA 3 后端：**Z1 Extreme、Z2 和
Radeon 780M**（gfx1103），**Z2 Extreme、Radeon 890M 和 880M**（gfx1150）。这属于实验性支持，速度慢。Neural runtime 一行会在 RDNA 3 署名后显示 "experimental"。
首批测试者结果（ROG Ally，Z1 Extreme）：游戏外的 lmxxf 探针，360p 尺寸下每次网络运行 54.7 ms，576p 下 110.9 ms；
游戏内，一位测试者的一次运行（Shadow of the Tomb Raider，1280x720 配合 XeSS，Handheld 预设），360p 下每次网络运行平均 62 ms，
模型每 4 帧运行一次，开启 NR 时约 29 fps。

- **不支持：** Z1 和 Radeon 740M（4 个计算单元）、Radeon 760M（8 个）、Radeon 860M / 840M。danielblnc 的运行时
  不能在掌机 APU 上运行。RX 6000（RDNA 2）计划在 0.4.0 支持；Steam Deck 和其他 RDNA 2 APU 不受支持。
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
- **FSR 4：** 在这些芯片上（以及一般的 Radeon 780M / 760M / 740M 上）FSR 4 INT8 不再自动开启：`Dx12Upscaler=auto`
  时放大器为 XeSS，FSR 3.1 保持为 FSR 3.1。`[FSR] Fsr4ForceModel=2` 仍可强制开启（实验性）。
- **Shadow of the Tomb Raider**（以及两次创建 D3D12 设备的游戏）在放大器启动时不再崩溃（0.3.4 已修复）。
- **驱动：** 请使用 AMD 官方的 Adrenalin 驱动。lmxxf 需要 HIP（`amdhip64_7.dll`），部分掌机厂商的驱动没有附带；
  此时 `amd_bridge.log` 会提示 HIP 不可用。
- **请一起使用 0.3.4 的三个文件：** 只有 0.3.4 的 pak 包含掌机模块，而 0.3.4 的 `LmxxfNrRuntime.dll` 在
  `OptiScaler.dll` 较旧时会拒绝在掌机上运行（"this handheld needs OptiScaler.dll 0.3.4 or newer"）。
- **有掌机的测试者：** 请在 Discord 上索取掌机测试包（`handheld-test.zip`）。其中的 `run_probe.bat` 会在你的设备上
  测量网络并写出 `handheld_result.txt`（你的 Windows 用户名会被隐藏）。

## 系统要求

- 一块使用最新驱动的 AMD 显卡。神经运行时通过驱动使用 HIP；不需要 HIP SDK，也不需要开发者模式。
  支持的芯片：
  - RX 9000（RDNA 4）：两个运行时。
  - RX 7000（RDNA 3，桌面与移动）：两个运行时——lmxxf 通过 AMDNR 的 RDNA 3 后端运行，比 RDNA 4 慢（网络尺寸档位
    默认开启，见上文）。
  - Strix Halo（Radeon 8060S / 8050S）：lmxxf。
  - 拥有 12 个以上计算单元的掌机 APU（Z1 Extreme / Z2 / 780M、Z2 Extreme / 890M / 880M）：lmxxf，实验性且较慢。
    Z1（4 CU）、760M / 740M 和 860M / 840M：不支持。
  - RX 6000（RDNA 2）：暂不支持，计划在 0.4.0 支持。Steam Deck 与 RDNA 2 APU：不支持。

  Neural 选项卡会显示你的显卡能运行什么（把鼠标悬停在运行时条目上，或查看 Diagnostics 中的 GPU 一行）。
- 一款 Direct3D 12、Direct3D 11 或 Vulkan 游戏。AMD 神经路径本身是 D3D12；D3D11 与 Vulkan 游戏通过
  OptiScaler 的 D3D12 桥接到达它，这意味着放大器必须是 "w/Dx12" 后端之一（`ffx_12`）。把
  `Dx11Upscaler` / `VulkanUpscaler` 保持为 `auto`，开启神经渲染时本构建版会自动为你选择。开启 Neural Rendering 时，Upscaling 列表会把它们显示为 "... w/Dx12 - Neural"。
- 在 1080p 级别的渲染分辨率下约需 2 GB 空闲显存。

## 两个压缩包里有什么

**AMDNR-vX.X.X.zip**

| 文件 | 说明 |
|---|---|
| `OptiScaler.dll` | 带 DLSS-NR AMD 后端的 OptiScaler（AMDNR 0.3.4）。按指南重命名。 |
| `OptiScaler.ini` | 设置。神经渲染已启用；日志已开启，以便反馈时有内容可附。 |
| `LmxxfNrRuntime.dll` | lmxxf 神经运行时（0.3.4：lmxxf 的内核，包括 lmxxf 0.31 的内核、AMDNR 的 c32w 内核、小网络尺寸以及原生角色遮罩）。仅在选中时使用；读取旁边的 `LmxxfNrRuntime.pak`，见"lmxxf 运行时"。 |
| `LmxxfNrRuntime.pak` | lmxxf 运行时的权重、HIP 模块和着色器，打包为一个加密文件（440 MB；0.3.4 新增：掌机模块和 lmxxf 0.31 内核）。只有 lmxxf 运行时会读取它；与 danielblnc 运行时并存也无妨。 |
| `OptiScaler\` | OptiScaler 使用的 FSR、XeSS、FidelityFX 去噪器和 D3D12 Agility SDK。 |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | Nukem9 的 dlssg-to-fsr3，未修改、仅重命名：把游戏的 DLSS 帧生成调用交给 FSR 3 帧生成，Vulkan 也可用（`FGNvngxReplacement=Nukems`）。GPLv3，见 `Licenses/`。 |
| `Licenses\`、`LICENSE` | 第三方许可证、AMDNR 声明（`AMDNR_NOTICE.txt`）以及本构建版的 GPL-3.0 许可证。 |
| `SHA256SUMS.txt` | 两个压缩包中每个发布文件的校验和。 |

**Runtime.zip**

| 文件 | 说明 |
|---|---|
| `dlssnr_amd_pass1..3.dll` | AMD 神经运行时，danielblnc 的 v0.3.1，未经修改。三份副本，多遍（multi-pass）时每遍一份。 |
| `dlssnr_on_amd_weights.bin` | 运行时加载的网络权重。 |

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
- **Performance**：NR resolution（%）及其开销、Neural passes、Full network、Fast mode、Dynamic NR resolution、Model interleave
  （开启时下方显示 Interleave preset 和节拍行）。
- **Quality**：Residual strength、Residual limit、Temporal stability、Sharpening (CAS)，以及 **More quality options**
  （Network history——两个运行时共用一个复选框——、Output smoothing、Stability mode、Residual temporal、Residual edge
  fade、Still-surface steadiness）。
- **Image look**：Colour composition、Detail 与 Colour strength，以及三个折叠区：**Model strength**（Tone 与 Structure
  intensity、Character structure、Edit detail / colour、Edge guard、Native character mask，以及 danielblnc 的 Network
  style、Tone curve 和 Black lift）、**Exposure and highlights**（Auto-exposure、其高光上限、Highlight colour guard、
  Game exposure）和 **Appearance filter**（名称后显示 off / on）。折叠区块名称后淡色的 "default" 或
  "custom" 表示其中是否有改动。
- **Ray Regeneration**：独立区块，仅在游戏使用 FSR Ray Regeneration 时显示。
- **工具行**，启动时收起：**Diagnostics**（Network output、Debug view、RR 调试视图、Edit shaper A/B、NR cost、拖影
  与自调读数、GPU 行、**Save report**）、**Runtime options**（Encoding、Every-frame NR、NR slots、Highlight proxy）和
  **Experimental**（AMDNR Screen-space GI，preview）。

当前运行时不具备的控件会变灰并带简短标签（例如 "not in lmxxf yet"），或被隐藏并显示数量（"3 danielblnc-only
options hidden"）；切换运行时不会移动其他任何行。

**其他选项卡：** Upscaling 以放大器、一行状态和 Render resolution（原 Upscale Ratio Override 与 Output Scaling）
开头；在非 NVIDIA 显卡上不再列出 "DLSS w/Dx12"。Image 包含 Sharpness、Textures、Init Flags 和 Magnifier。Frame Gen
以 FG Input 和 FG Output 开头。Interface 包含 FPS 叠加层和 Keybinds（每个按键一个按钮）。Advanced 以 Active Quirks 开头，然后是 Display
（V-Sync）、Compatibility 和 Logging。设置项、键名以及 Save Settings 写入的内容都没有变化，`CHANGELOG.md` 中另有
说明的除外。

## 值得了解的设置

打开 **Neural** 选项卡。默认值是最近一次测试过的组合，所以最有用的第一步是一次只改一项。

- **NR resolution** —— 主要的画质/开销杠杆。低于 100% 时模型处理较小的画面，只把它的*修正*带回
  全分辨率帧，因此帧保留自己的细节。高于 100% 时开销按平方增长（150% 为 2.25 倍）。滑块以 5% 为
  一档：每个新的 NR 尺寸都可能占住显存直到游戏重启，所以多次调整后请重启游戏。
  旁边的开销在 100% 时为 1.00x；在 lmxxf 下是网络实际运行档位的价格（悬停提示会写出档位）。Preset 按钮会把它设为
  100 / 85 / 70%。
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
- **新 ini 中帧生成默认关闭。** Frame Gen 选项卡：选择 FG Input（例如带 DLSS 帧生成的游戏中选
  "DLSSG via Streamline"）与 FG Output（XeFG），然后在 Frame Generation (XeFG) 区块勾选 **Active**，
  再按 Save Settings。安装 0.2.0 的 ini 时，0.1.0 ini 中开启的该项不会被沿用。
- **XeFG 多帧生成** —— 3X 至 6X 已内置并默认开启（`XeFG\UnlockMFG`），对 OptiScaler 的副本和游戏
  自带的副本都有效。如果你还留着 `XeFGUnlock.asi`，**请从 `OptiScaler\plugins` 中删除它**：同一
  补丁的两份副本会导致游戏崩溃。
  **最高 10X 需手动开启**（仅 D3D12 游戏）：在 Frame Gen 选项卡 FG Output 下设置 *XeFG ceiling (restart)*
  （4X、6X 默认、8X 或 10X；`[XeFG] MaxInterpolatedFrames`），重启游戏，再在 MFG 下拉框中选择倍数。
  高于 6X 需要 OptiScaler 自带的 XeFG 提供程序并开启 Extra pacing；游戏自带的 XeSS 3 副本最多 6X。
  10X 需要 360 Hz 及以上的显示器，并把帧率上限设为刷新率 / 10；延迟较高，且提供程序在 4K 下多占用约
  128 MiB 显存。7X-10X 尚未在游戏中确认：测试者请发送 `OptiScaler.log`。
- **FSR Ray Regeneration** —— 默认仅限 RDNA 4（RX 9000）；仅在使用 DLSS 光线重建的游戏中（Cyberpunk 2077、Alan Wake 2），且游戏
  运行 DLSS（开启伪装）、光线追踪和光线重建都在游戏自身设置中启用。此时神经渲染在它之后、对它的
  输出运行，开销更大：帧率下降时调低 NR resolution。它的控件现在有独立区块 **Neural > Ray Regeneration**，
  只在游戏实际运行光线重建时显示。**路径追踪配置**（路径追踪下脸部噪点更少）自 0.3.3.1 起需手动开启：
  想在 Resident Evil Requiem 或 PRAGMATA 中试用，请在那里勾选。同一区块还有 bias mask 强度和
  **皮肤平滑**（实验性，用于提供 SSS 引导的游戏；默认关闭，但自 0.3.3.2 起在 Resident Evil Requiem 中默认开启）；
  时域调节滑块在 *More Ray Regeneration options* 中，RR 调试视图在 Diagnostics 中。在 RX 7000（RDNA 3）上，FSR Ray Regeneration
  重新默认提供（0.3.3.2 只在 RDNA 4 上提供）。AMD 只为 RDNA 4 提供它：若驱动拒绝，游戏会得到不带降噪的 FSR。
  `[FSR-RR] FfxDenoiserAllowPreRdna4=false` 将其限制为 RDNA 4；RX 6000 及更早的显卡只有设为 `true` 才会提供（Upscaling
  选项卡：**Offer FSR Ray Regeneration on this GPU (restart)**）。
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
自 0.3.3.2 起，Neural 选项卡会写出文件名和版本，并说明该怎么做。请使用本发布页的 `v0.4.0-Runtime.zip`（最新）或
`Runtime.zip`（0.3.1），三个 pass DLL 须来自同一个压缩包：`v0.4.0-Runtime.zip` 中的 `dlssnr_amd_pass1.dll` 为
10,027,008 字节，SHA256 以 `d62be3d8` 开头。支持的版本：0.2.17、0.3.0、0.3.1、0.3.2、0.3.3、0.4.0，以及尚未发布的
0.4.x。不要在 AMDNR 旁边安装 danielblnc 自己的安装程序或它的 `dxgi.dll` / `version.dll` / `winhttp.dll`：
AMDNR 已经在运行他的运行时。

**在带集成显卡的电脑上，lmxxf 没有任何效果，或一开始就停止？** 0.3.3.2 已修复。在开启了集成显卡的 Ryzen 台式机、
带 AMD APU 和 Radeon 独显的笔记本，或装有两块 AMD 显卡的电脑上，游戏所用的显卡往往不是 HIP 设备 0。lmxxf 因此在
第一帧就失败（`hipErrorInvalidHandle (400)`，随后 `lmxxf_backend.log` 中出现 "session is poisoned"），并在整个会话中
保持关闭。请把 `OptiScaler.dll`（即你重命名后的文件，例如 `dxgi.dll`）和 `LmxxfNrRuntime.dll` 都替换为 0.3.3.2 的
版本。尚未在这类电脑上测试：如果 lmxxf 仍然停止，Neural 选项卡现在会说明原因；请发送 `lmxxf_backend.log` 和
`amd_bridge.log`（其中列出了各个 HIP 设备）。

**在 RX 9070 / 9070 XT 上，lmxxf 的状态行显示 `c32w=off:nofile`？** 游戏 `.exe` 旁边有一个旧的
`DLSS5-AMD\native-game-tiled-assets` 文件夹（以前安装 lmxxf 时留下的），它会代替 `LmxxfNrRuntime.pak`
被使用。该文件夹里没有 c32w 内核，所以 lmxxf 仍以旧的速度运行。请删除 `DLSS5-AMD` 文件夹或给它改名：pak
已包含 lmxxf 需要的一切。早于 0.3.3.2 的 `LmxxfNrRuntime.pak` 也会显示同样的状态；请换成本次发布中的那个。
同一行中出现 `fk=fff-` 也是同样原因（旧 pak 或散装文件夹）：lmxxf 仍会运行，但速度是旧的。

**danielblnc：NR resolution 离开 100% 时，NR 风格仍会变化？** 0.3.4 中仍未解决，默认值不变。100% 时 Residual
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

**游戏里 Ray Reconstruction 已开启，但 Neural 选项卡显示 "Ray Regeneration is off in this title"？**
该游戏没有提供 FSR Ray Regeneration 所需的数据：它的 DLSS 插件传入的是空的相机矩阵（Satisfactory），NVIDIA 的光线
重建把它们视为可选，而 FSR Ray Regeneration 必须要有。此时改由 FSR 超分运行，NR 回到它平常的 SR 之前位置；Upscaling
选项卡也会说明。自 0.3.4 起，在具有这种特征的 Unreal 游戏中它会在整个会话内保持关闭。请在游戏中关闭光线重建，并恢复
引擎自身的降噪设置。

**育碧 Anvil 引擎游戏（AC Black Flag Resynced、Shadows、Mirage）弹出 "DX12 Error 0x80070057"？**
这些游戏自带 XeSS 帧生成。本构建版会把帧生成交给游戏自己处理（OptiScaler 的 XeFG 输出在此类游戏中自动
停用，Frame Gen 选项卡会说明原因）；请使用游戏自己的 XeSS FG 选项。若仍然出现，请设置
`[FrameGen] Enabled=false` 和 `[fakenvapi] ForceXeLL=false`，并附日志反馈。

**《最后生还者 第一部》（The Last of Us Part I）启动时崩溃？** 那是游戏自身 Streamline 初始化的问题，是已知的
OptiScaler 问题：把游戏目录中的 `sl.common.dll` 重命名为 `sl.common.dll.bak`，并在游戏设置中选择 **FSR 3.1**
而不是 DLSS。

各版本的完整说明：`CHANGELOG.md`（压缩包和仓库中都有）。

## 路线图

- **0.3.4**（本构建版）—— 全新菜单（重做的 Neural 选项卡、所有选项卡统一的外观、Save report）；lmxxf 在 RX 7000
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
- **0.4.0** —— RX 6000（RDNA 2），以及对自身没有放大器的游戏
  （Stray 一类）的支持，由 OptiScaler 同时提供放大器和神经渲染。

---

## 致谢

本构建版是对他人工作的接线整合。如果你觉得它有用，感谢应归于上游。

- **TheAutomatic** —— DLSS 5 AMD project —— https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** —— DLSS-NR on AMD by Daniel Blanco —— https://github.com/danielblnc/DLSS-NR-on-AMD （`Runtime.zip`，未经修改）
- **lmxxf**（Kien）—— https://github.com/lmxxf/dlss5-on-amd-9070xt-porting （网络移植、内核与 HIP 运行时，MIT）
- **TheAutomatic** —— `LmxxfNrRuntime.cpp`、`LmxxfNrApi.h`、`LmxxfProductionOptions.h`：portions contributed to lmxxf by TheAutomatic (MIT)
- **lmxxf 0.31 内核**，位于 `LmxxfNrRuntime.pak` 中（the ViT projection (lmxxf031-vit-wide-deep), the C512 QKV and mix kernels (lmxxf031-c512-m32-mh, lmxxf031-c512-m32-deep) and one-wave-per-head attention (lmxxf031-c64-wave2)）—— 属于 lmxxf（Kien，MIT），由 AMDNR 按 lmxxf 的源码与构建方法构建；AMDNR 负责加载、SHA-256 固定校验、按显卡启用与回退
- **c32w 内核**（0.3.3.2）—— AMDNR 自有的 RDNA 4 单 wave 内核，用于 lmxxf 的网络，Copyright (c) 2026 3zwr1 (AMDNR)；思路参考 AMD 公开的 RDNA 4 WMMA 文档（GPUOpen、ROCm matrix instruction calculator）
- **AMDNR 的 RDNA 3 后端**（0.3.3；0.3.4 中新增掌机构建 gfx1103 / gfx1150）、网络尺寸档位策略和小网络尺寸（0.3.4）—— Copyright (c) 2026 3zwr1 (AMDNR)
- **Matheus / dlss-5-amd** —— https://github.com/MatheusGViana/dlss-5-amd-project
- **Dagherbou / OptiScaler_DLSSNR** —— https://github.com/Dagherbou/OptiScaler_DLSSNR
- **wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass** —— https://github.com/wilsjo2/OptiScaler-DLSSNR-PreSR-Multipass
- **Nukem9** —— dlssg-to-fsr3 —— https://github.com/Nukem9/dlssg-to-fsr3 （GPLv3，未经修改）
- **RenoDX** —— clshortfuse —— https://github.com/clshortfuse/renodx （色彩合成算法，MIT）
- **Coldwood1026** —— XeFGUnlock（GPL-3.0），内置 XeFG 多帧解锁及其节拍的基础
- **burak113** —— FSR Ray Regeneration 预处理器（OptiScaler 分支 ffx-denoise-experimental，GPL-3.0）
- **Screen-space GI**（继承的效果；0.3.4 起已从菜单中移除，ini 中为 `[AmdRtgi] Enabled`）—— AMDNR 从 OptiScaler-AMD-PreSR 一脉继承的效果；功劳归于其原作者。它需要 danielblnc 包中的 `experimental_lighting` 文件夹，AMDNR 不附带该文件夹。
- **AMDNR Screen GI**（0.3.4 preview）—— AMDNR 自己的作品，Copyright (c) 2026 3zwr1 (AMDNR)，依据已发表的论文编写（Therrien、Levesque 和 Gilet 2023；Jimenez 等 2016；Schied 等 2017；其余见 `CHANGELOG.md` 和 `Licenses/AMDNR_NOTICE.txt`）
- **OptiScaler** —— Overclockers —— https://github.com/Overclockers/OptiScaler-Releases

## AMDNR Launcher

**AMDNR Launcher**（0.3.4 新增）按游戏安装和更新 AMDNR。请从 Alpha0.3.4 发布页下载 `AMDNR-Launcher.exe`：
<https://github.com/3zwr1/AMD-NR---OptiScaler/releases/download/Alpha0.3.4/AMDNR-Launcher.exe>

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
