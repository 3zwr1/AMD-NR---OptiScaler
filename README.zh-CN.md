# AMDNR — AMD 显卡上的 DLSS 5 神经渲染（OptiScaler 构建版）— v0.3.3

[English](README.md) | **中文** | [Português](README.pt-BR.md) | [Español](README.es.md)

> **我们需要你的支持。** 加入 Discord 服务器 —— <https://discord.gg/AMDNR> —— 获取帮助、提交
> 问题、领取测试版；每一份带日志的反馈都会让下一个版本更好。

DLSS 5 神经渲染（Neural Rendering）在 AMD 显卡上运行，内置于 OptiScaler，因此 OptiScaler 能挂钩的
任何 Direct3D 12 游戏都可以使用。在神经渲染之上还有：大幅提升帧率的模型交错（model interleave）、
残差合成（residual composition）、解锁至 6X 的 XeSS 帧生成（D3D12 游戏可选开启至 10X），以及面向使用
DLSS 光线重建游戏的 FSR Ray Regeneration。

**Discord：<https://discord.gg/AMDNR>** —— 支持、问题反馈（`#bug-report`）、测试版。

**支持本项目：<https://ko-fi.com/3zinr>**

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

> 游戏的 `.exe` 通常不在快捷方式指向的位置。Unreal 引擎游戏把它放在
> `<Game>\Binaries\Win64\` 下。

---

### lmxxf 运行时（0.3.0，可选）

第二个神经运行时（MIT 许可，作者 lmxxf）可以代替 danielblnc 的运行时承担这一步。支持 RDNA 4，并通过 AMDNR 的 RDNA 3 后端支持 RDNA 3
（RX 7000、Strix Halo）——在 RDNA 3 上较慢：NR resolution 请从 67% 起步。
它需要游戏目录旁的两个东西：

1. `LmxxfNrRuntime.dll` —— 在本压缩包中，与 `OptiScaler.dll` 并列（会随其余文件一起复制）。
2. `LmxxfNrRuntime.pak`（416 MB，已包含在 AMDNR 压缩包中），放在 `LmxxfNrRuntime.dll` 旁边 —— lmxxf 的
   权重文件、HIP 模块和 HLSL 打包成一个加密并带完整性校验的文件。运行时在内存中打开它，不会向磁盘
   解包任何内容。

首次启动时若检测到已安装运行时且尚未做出选择，菜单会询问使用哪一个（ini 中以
`[DlssNr] NrBackend = daniel | lmxxf` 记录；Neural > Neural runtime 可更改，下次启动游戏时生效）。
lmxxf 的编辑延迟一帧应用，由运动向量携带，因此画面从不等待网络（RX 9070 XT 上 1080p
约 17 ms）。它的日志是游戏目录旁的 `lmxxf_backend.log`。

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

在 lmxxf 下，Neural runtime 区块有 **Network history**（模型自身的时序输入），Image look 中有
**lmxxf edit** 组：编辑整形器（Edit detail、Edit colour、Edge guard：模型编辑细节部分的增益、编辑的
色彩相对其亮度变化的比例、以及在深度边缘处对编辑的衰减）和 **Output smoothing**（上游的输出侧
平滑，需要 Network history）。Neural passes、Residual strength/limit、锐化、Debug view 1 和 Appearance
滤镜对两个运行时都适用。

**Full network**（Neural > Performance，`[DlssNr] LmxxfFullNetwork`，仅 lmxxf）运行网络全部 71 个块，
而不是跳过第 42、43、46 块：略微更忠实，1080p 下慢约 0.5 ms（RX 9070 XT 上 16.6 -> 17.1 ms）。默认关闭。

## 系统要求

- 一块使用最新驱动的 AMD 显卡。神经运行时通过驱动使用 HIP；不需要 HIP SDK，也不需要开发者模式。
  支持的芯片：RX 9000（RDNA 4）可运行两个运行时；RX 7000（RDNA 3，桌面与移动）也可运行两个
  运行时——lmxxf 通过 AMDNR 的 RDNA 3 后端运行，比 RDNA 4 慢；Strix Halo（8060S / 8050S）可运行 lmxxf；掌机 APU
  （Z1 Extreme / 780M、Z2 Extreme / 890M）与 RDNA 2（RX 6000、Steam Deck）两个运行时都不支持。
  Neural 选项卡会显示你的显卡能运行什么。
- 一款 Direct3D 12、Direct3D 11 或 Vulkan 游戏。AMD 神经路径本身是 D3D12；D3D11 与 Vulkan 游戏通过
  OptiScaler 的 D3D12 桥接到达它，这意味着放大器必须是 "w/Dx12" 后端之一（`ffx_12`）。把
  `Dx11Upscaler` / `VulkanUpscaler` 保持为 `auto`，开启神经渲染时本构建版会自动为你选择。
- 在 1080p 级别的渲染分辨率下约需 2 GB 空闲显存。

## 两个压缩包里有什么

**AMDNR-vX.X.X.zip**

| 文件 | 说明 |
|---|---|
| `OptiScaler.dll` | 带 DLSS-NR AMD 后端的 OptiScaler。按指南重命名。 |
| `OptiScaler.ini` | 设置。神经渲染已启用；日志已开启，以便反馈时有内容可附。 |
| `LmxxfNrRuntime.dll` | lmxxf 神经运行时（lmxxf 0.29 内核）。仅在选中时使用；读取旁边的 `LmxxfNrRuntime.pak`，见"lmxxf 运行时"。 |
| `LmxxfNrRuntime.pak` | lmxxf 运行时的权重、HIP 模块和着色器，打包为一个加密文件（416 MB）。只有 lmxxf 运行时会读取它；与 danielblnc 运行时并存也无妨。 |
| `OptiScaler\` | OptiScaler 使用的 FSR、XeSS、FidelityFX 去噪器和 D3D12 Agility SDK。 |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | Nukem9 的 dlssg-to-fsr3，未修改、仅重命名：把游戏的 DLSS 帧生成调用交给 FSR 3 帧生成，Vulkan 也可用（`FGNvngxReplacement=Nukems`）。GPLv3，见 `Licenses/`。 |
| `Licenses\`、`LICENSE` | 第三方许可证、AMDNR 声明（`AMDNR_NOTICE.txt`）以及本构建版的 GPL-3.0 许可证。 |
| `SHA256SUMS.txt` | 两个压缩包中每个发布文件的校验和。 |

**Runtime.zip**

| 文件 | 说明 |
|---|---|
| `dlssnr_amd_pass1..3.dll` | AMD 神经运行时，danielblnc 的 v0.3.1，未经修改。三份副本，多遍（multi-pass）时每遍一份。 |
| `dlssnr_on_amd_weights.bin` | 运行时加载的网络权重。 |

## 值得了解的设置

打开 **Neural** 选项卡。默认值是最近一次测试过的组合，所以最有用的第一步是一次只改一项。

- **NR resolution** —— 主要的画质/开销杠杆。低于 100% 时模型处理较小的画面，只把它的*修正*带回
  全分辨率帧，因此帧保留自己的细节。高于 100% 时开销按平方增长（150% 为 2.25 倍）。滑块以 5% 为
  一档：每个新的 NR 尺寸都可能占住显存直到游戏重启，所以多次调整后请重启游戏。
- **Residual strength** —— 模型编辑应用的比例；大于 1 会放大。这是改变画面最大的控制项。
- **Residual limit** —— 单个像素可移动的上限。出现斑块：**调低**它。
- **Model interleave** —— 每隔一帧运行模型，大幅提升帧率。跳过的帧由 **Interleave preset** 填补；
  *Guided fill v2* 是默认值，也是正在持续打磨的一项。两类帧的节拍自动处理，**Adaptive interleave**
  （默认开启）在画面运动时每帧都运行模型，因此跳帧 —— 及其瑕疵 —— 只在画面静止时发生。
- **Neural passes** —— 2 和 3 会叠加模型，收益递减。在 lmxxf 下网络的历史保持为第一遍；额外的遍
  只是空间上的精修。danielblnc 在 Vulkan 游戏中只运行 1 遍（滑块下方会有提示）。
- **Colour composition**（Neural > Image look，两个运行时都有）—— *Classic*（默认）就是你之前的画面。
  *RenoDX (experimental)* 在模型之后运行 RenoDX 的色彩合成，与 NVIDIA 路径相同：Composition detail 与
  Composition colour、以原图为基准约束模型结果的双向 **Highlight guard**（默认 2x），以及可选的皮肤 / 环境
  控制。遇到显示参考（SDR）帧时会回退到 Classic，并在菜单中说明。NR 风格和预设不会改动这些设置。
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
- **FSR Ray Regeneration** —— 仅在使用 DLSS 光线重建的游戏中（Cyberpunk 2077、Alan Wake 2），且游戏
  运行 DLSS（开启伪装）、光线追踪和光线重建都在游戏自身设置中启用。此时神经渲染在它之后、对它的
  输出运行，开销更大：帧率下降时调低 NR resolution。它的控件（Neural > Quality > Ray Regeneration）
  只在游戏实际运行光线重建时显示。Resident Evil Requiem 和 PRAGMATA 会自动启用**路径追踪配置**
  （路径追踪下脸部噪点更少）。同一位置还有 bias mask 强度、RR 调试视图和**皮肤平滑**（实验性，默认
  关闭；用于提供 SSS 引导的游戏，例如 Resident Evil Requiem）。

## 出了问题怎么办

游戏目录中会出现 `OptiScaler.log`。在 `#bug-report` 中附上它，并说明游戏和显卡型号。AMD 后端还会
写出 `amd_presr.log` 和 `amd_bridge.log`，当神经渲染这一步出问题时它们最有用。上一次会话的日志会保留为
`OptiScaler.previous.<exe>.log`；崩溃后请一并附上（新日志中会写明 "no clean exit recorded"）。

**NR frames 0/s、运行时下拉框显示 `pass1?`，且 `amd_presr.log` 说该 pass DLL 是本 OptiScaler 不支持的版本？**
你的 `dlssnr_amd_pass1..3.dll` 不是 danielblnc 的 0.3.1（外面流传着一套 0.2.16）。请使用本发布页的
`Runtime.zip`：`dlssnr_amd_pass1.dll` 为 7,304,192 字节，SHA256 以 `b108d640` 开头。支持的版本：0.2.17、0.3.0、0.3.1。

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
在 lmxxf 下（1080 档）每次更改 NR resolution 或 DLSS 模式会占住约 97 MB 显存和同样多的内存（AMD 驱动的泄漏，修复计划
在 0.3.4），danielblnc 在每个超过约 1 MP 的新 NR 尺寸上也会占住显存。多次调整后请重启游戏。

**Streamline 游戏启动时报 slInit 错误 0x18（在 AMD 上的 NBA 2K27 中出现）？** 0.3.3 堵住了 OptiScaler 的
Streamline 插件钩子可能引发该错误的一条途径，但尚未确认这就是 NBA 2K27 的原因。`OptiScaler.log` 现在会记录
`slInit returned ...` 和 `[SLINIT]` 行：反馈时请附上日志。

**游戏里 Ray Reconstruction 已开启，但 Neural 选项卡显示 "Ray Regeneration is off in this title"？**
该游戏没有提供 FSR Ray Regeneration 所需的数据（Satisfactory：没有相机矩阵）。此时改由 FSR 超分运行，
NR 回到它平常的 SR 之前位置；ini 里没有任何设置能改变这一点。

**育碧 Anvil 引擎游戏（AC Black Flag Resynced、Shadows、Mirage）弹出 "DX12 Error 0x80070057"？**
这些游戏自带 XeSS 帧生成。本构建版会把帧生成交给游戏自己处理（OptiScaler 的 XeFG 输出在此类游戏中自动
停用，Frame Gen 选项卡会说明原因）；请使用游戏自己的 XeSS FG 选项。若仍然出现，请设置
`[FrameGen] Enabled=false` 和 `[fakenvapi] ForceXeLL=false`，并附日志反馈。

**《最后生还者 第一部》（The Last of Us Part I）启动时崩溃？** 那是游戏自身 Streamline 初始化的问题，是已知的
OptiScaler 问题：把游戏目录中的 `sl.common.dll` 重命名为 `sl.common.dll.bak`，并在游戏设置中选择 **FSR 3.1**
而不是 DLSS。

各版本的完整说明：`CHANGELOG.md`（压缩包和仓库中都有）。

## 路线图

- **0.3.3**（本构建版）—— lmxxf 支持 RDNA 3（RX 7000；AMDNR 自己的后端）；两个运行时都可用的 RenoDX
  色彩合成（实验性，需手动开启）；lmxxf：Full network 选项、修复内存泄漏、修复 Vulkan 游戏的问题（在 Vulkan
  桥接内延迟上传权重）、0.29 内核（逐位一致、更快）；danielblnc 在 Vulkan 游戏中：1 个 Neural pass、更清楚的
  提示信息、可选的延迟复制等待；XeFG 最高 10X（需手动开启，D3D12）；Streamline 启动加固与诊断；FSR Ray
  Regeneration 路径追踪配置与皮肤平滑；UE5 健壮性改进。
- **0.3.2** —— 0.3.1 的反馈：Vulkan 游戏可启动并可运行 lmxxf、lmxxf 色彩与 danielblnc 对齐（自动曝光）、
  运行时下拉框、光线重建状态与调节；zip 内附 Nukem9 的 dlssg-to-fsr3，用于 Vulkan 帧生成。
- **0.3.1** —— 
- **0.3.0** —— **lmxxf** HIP 神经运行时（RDNA 4）作为可选运行时与 danielblnc 的并列，以
  `LmxxfNrRuntime.dll` + `LmxxfNrRuntime.pak` 发布：网络历史、真实的 Neural passes、编辑整形器、光线
  重建之后的位置、逐游戏诊断与自我修复。特别感谢 TheAutomatic，本次集成建立在他的 DLSS 5 AMD
  project 工作之上。
- **0.4.0** —— AMDNR Launcher（一键安装运行时与 pak、更新），以及对自身没有放大器的游戏（Stray 一类）
  的支持，由 OptiScaler 同时提供放大器和神经渲染。

---

## 致谢

本构建版是对他人工作的接线整合。如果你觉得它有用，感谢应归于上游。

- **TheAutomatic** —— DLSS 5 AMD project —— https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** —— DLSS-NR on AMD —— https://github.com/danielblnc/DLSS-NR-on-AMD （`Runtime.zip`，未经修改）
- **lmxxf** —— https://github.com/lmxxf/dlss5-on-amd-9070xt-porting （HIP 运行时，MIT）
- **Matheus / dlss-5-amd** —— https://github.com/MatheusGViana/dlss-5-amd-project
- **Nukem9** —— dlssg-to-fsr3 —— https://github.com/Nukem9/dlssg-to-fsr3 （GPLv3，未经修改）
- **RenoDX** —— clshortfuse —— https://github.com/clshortfuse/renodx （色彩合成算法，MIT）
- **Coldwood1026** —— XeFGUnlock（GPL-3.0），内置 XeFG 多帧解锁及其节拍的基础
- **burak113** —— FSR Ray Regeneration 预处理器（OptiScaler 分支 ffx-denoise-experimental，GPL-3.0）
- **OptiScaler** —— Overclockers —— https://github.com/Overclockers/OptiScaler-Releases

## 版权 / 许可证（Copyright / License）

AMDNR 版权所有 Copyright (c) 2026 3zwr1 (AMDNR)。它是 OptiScaler 的一个分支（fork），按 `LICENSE` 中的
GPL-3.0 许可证分发。

AMDNR 自己的工作附带一条依据 GPL-3.0 第 7(b) 条的附加条款（见 `Licenses/AMDNR_NOTICE.txt`）：任何使用它的
副本、分支或衍生作品都必须保留其声明，并注明 **AMDNR by 3zwr1**（<https://github.com/3zwr1/AMD-NR---OptiScaler>）。

上文致谢的上游工作仍归其作者所有，适用其各自的许可证；AMDNR 不对其主张任何版权。

源代码将随 AMDNR 0.5.0 发布。

## 法律声明

本构建版按 `LICENSE` 中的 GPL-3.0 许可证分发；第三方库许可证位于 `Licenses\`。AMD 神经运行时及其
权重按上述原作者署名再分发，仅为方便使用，不主张任何所有权，不提供任何保证。

NVIDIA 的 `nvngx_dlssnr.dll` 不在这些压缩包中。以上内容均未获得 NVIDIA、AMD 或任何游戏发行商的认可、
关联或支持。它直接驱动一项未公开的功能。使用风险自负。
