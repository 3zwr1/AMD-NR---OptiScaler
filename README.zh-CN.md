# AMDNR — AMD 显卡上的 DLSS 5 神经渲染（OptiScaler 构建版）— v0.3.1

[English](README.md) | **中文** | [Português](README.pt-BR.md)

> **我们需要你的支持。** 加入 Discord 服务器 —— <https://discord.gg/QzbzxfKYyh> —— 获取帮助、提交
> 问题、领取测试版；每一份带日志的反馈都会让下一个版本更好。

DLSS 5 神经渲染（Neural Rendering）在 AMD 显卡上运行，内置于 OptiScaler，因此 OptiScaler 能挂钩的
任何 Direct3D 12 游戏都可以使用。在神经渲染之上还有：大幅提升帧率的模型交错（model interleave）、
残差合成（residual composition）、解锁至 6X 的 XeSS 帧生成，以及面向使用 DLSS 光线重建游戏的
FSR Ray Regeneration。

**Discord：<https://discord.gg/QzbzxfKYyh>** —— 支持、问题反馈（`#bug-report`）、测试版。如果你
需要 NV 的 `nv`，可以在那里获取；它不在本压缩包中，AMD 路径也不需要它。

**支持本项目：<https://ko-fi.com/3zinr>**

---

## AMDNR - OptiScaler 安装指南

安装非常简单。

### 1. 下载文件

从 GitHub 下载这两个文件：

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

第二个神经运行时（MIT 许可，作者 lmxxf）可以代替 danielblnc 的运行时承担这一步。仅支持 RDNA 4。
它需要游戏目录旁的两个东西：

1. `LmxxfNrRuntime.dll` —— 在本压缩包中，与 `OptiScaler.dll` 并列（会随其余文件一起复制）。
2. `LmxxfNrRuntime.pak`（382 MB，已包含在 AMDNR 压缩包中），放在 `LmxxfNrRuntime.dll` 旁边 —— lmxxf 的
   权重文件、HIP 模块和 HLSL 打包成一个加密并带完整性校验的文件。运行时在内存中打开它，不会向磁盘
   解包任何内容。旧的文件夹布局同样可用，可替代 pak：`DLSS5-AMD\native-game-tiled-assets\`（约 575 MB），
   来自 lmxxf 自己的发布（<https://github.com/lmxxf/dlss5-on-amd-9070xt-porting>）。该文件夹放在游戏
   exe 旁边，使 `<game>\DLSS5-AMD\native-game-tiled-assets\block0-ffn.f16` 存在。dlss-5-amd-project 1.9.0
   的布局也可以：`native-game-tiled-assets\`（权重）、`lmxxf-modules\` 和 `shaders\` 与游戏 exe 并列，
   即其安装程序放置的位置。

首次启动时若检测到已安装运行时且尚未做出选择，菜单会询问使用哪一个（ini 中以
`[DlssNr] NrBackend = daniel | lmxxf` 记录；Neural > Neural runtime 可更改，下次启动游戏时生效）。
lmxxf 的编辑延迟一帧应用，由运动向量携带，因此画面从不等待网络（RX 9070 XT 上 1080p 约 30 ms，
720p 约 15 ms）。它的日志是游戏目录旁的 `lmxxf_backend.log`。

**兼容性（lmxxf）。** 运行时只能看到 DLSS 所看到的东西，因此因游戏而异的只有一份短清单：颜色格式与
HDR、运动向量及其缩放、深度及其方向、reactive 遮罩、曝光纹理、Reset 标志，以及该步骤所处的位置
（超分辨率之前，或光线重建之后）。目前已测试：

| 游戏 | API / 位置 | 备注 |
|---|---|---|
| Silent Hill 2 | D3D12，SR 之前 | 参考游戏；已处理 Unreal 带填充的颜色分配 |
| Forza Horizon 6 | D3D12，SR 之前 | |
| Stray | D3D11 经 D3D12 桥接，SR 之前 | |
| GTA V Enhanced | D3D12，SR 之前，HDR，单通道 reactive 遮罩 | 本版本已修复：遮罩曾被读成"处处 reactive"，编辑从未落到画面上 |
| 任何使用光线重建的游戏 | D3D12，RR 之后（写回输出） | 本版本起支持；尚未在游戏中确认 |

如果某个游戏看不到效果：`lmxxf_backend.log` 中有一行 `lmxxf inputs:`（格式、尺寸、运动缩放、深度
方向、遮罩、曝光），以及每 600 帧一行 `lmxxf stats @N:`（曝光、输入亮度、模型的编辑、被携带的编辑、
keep、reactive 均值、向量长度与被拒比例）。反馈时附上日志；这两行通常就能说明原因。

在 lmxxf 下，Neural runtime 区块有 **Network history**（模型自身的时序输入），Image look 中有
**lmxxf edit** 组：编辑整形器（Edit detail、Edit colour、Edge guard：模型编辑细节部分的增益、编辑的
色彩相对其亮度变化的比例、以及在深度边缘处对编辑的衰减）和 **Output smoothing**（上游的输出侧
平滑，需要 Network history）。Neural passes、Residual strength/limit、锐化、Debug view 1 和 Appearance
滤镜对两个运行时都适用。ini 键：`[DlssNr]` 下的 `AmdLmxxfHistory`、`AmdLmxxfEditDetail`、
`AmdLmxxfEditSaturation`、`AmdLmxxfEdgeGuard`、`AmdLmxxfOutputSmooth`。

## 系统要求

- 一块使用最新驱动的 AMD 显卡。神经运行时通过驱动使用 HIP；不需要 HIP SDK，也不需要开发者模式。
  支持的芯片：RX 9000（RDNA 4）可运行两个运行时；RX 7000（RDNA 3，桌面与移动）只能运行 danielblnc
  的运行时；掌机 APU（Z1 Extreme / 780M、Z2 Extreme / 890M）与 RDNA 2（RX 6000、Steam Deck）目前两个
  运行时都不支持。Neural 选项卡会显示你的显卡能运行什么。
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
| `LmxxfNrRuntime.dll` | lmxxf 神经运行时（0.3.0）。仅在选中时使用；读取旁边的 `LmxxfNrRuntime.pak`，见"lmxxf 运行时"。 |
| `LmxxfNrRuntime.pak` | lmxxf 运行时的权重、HIP 模块和着色器，打包为一个加密文件（382 MB）。只有 lmxxf 运行时会读取它；与 danielblnc 运行时并存也无妨。 |
| `OptiScaler\` | OptiScaler 使用的 FSR、XeSS、FidelityFX 去噪器和 D3D12 Agility SDK。 |
| `Licenses\`、`LICENSE` | 第三方许可证以及本构建版的 GPL-3.0 许可证。 |
| `SHA256SUMS.txt` | 两个压缩包中每个发布文件的校验和。 |

**Runtime.zip**

| 文件 | 说明 |
|---|---|
| `dlssnr_amd_pass1..3.dll` | AMD 神经运行时，danielblnc 的 v0.3.1，未经修改。三份副本，多遍（multi-pass）时每遍一份。 |
| `dlssnr_on_amd_weights.bin` | 运行时加载的网络权重。 |

## 值得了解的设置

打开 **Neural** 选项卡。默认值是最近一次测试过的组合，所以最有用的第一步是一次只改一项。

- **NR resolution** —— 主要的画质/开销杠杆。低于 100% 时模型处理较小的画面，只把它的*修正*带回
  全分辨率帧，因此帧保留自己的细节。高于 100% 时开销按平方增长（150% 为 2.25 倍）。
- **Residual strength** —— 模型编辑应用的比例；大于 1 会放大。这是改变画面最大的控制项。
- **Residual limit** —— 单个像素可移动的上限。出现斑块：**调低**它。
- **Model interleave** —— 每隔一帧运行模型，大幅提升帧率。跳过的帧由 **Interleave preset** 填补；
  *Guided fill v2* 是默认值，也是正在持续打磨的一项。两类帧的节拍自动处理，**Adaptive interleave**
  （默认开启）在画面运动时每帧都运行模型，因此跳帧 —— 及其瑕疵 —— 只在画面静止时发生。
- **Neural passes** —— 2 和 3 会叠加模型，收益递减。在 lmxxf 下网络的历史保持为第一遍；额外的遍
  只是空间上的精修。
- **新 ini 中帧生成默认关闭。** Frame Gen 选项卡：选择 FG Input（例如带 DLSS 帧生成的游戏中选
  "DLSSG via Streamline"）与 FG Output（XeFG），然后在 Frame Generation (XeFG) 区块勾选 **Active**，
  再按 Save Settings。安装 0.2.0 的 ini 时，0.1.0 ini 中开启的该项不会被沿用。
- **XeFG 多帧生成** —— 3X 至 6X 已内置并默认开启（`XeFG\UnlockMFG`），对 OptiScaler 的副本和游戏
  自带的副本都有效。如果你还留着 `XeFGUnlock.asi`，**请从 `OptiScaler\plugins` 中删除它**：同一
  补丁的两份副本会导致游戏崩溃。
- **FSR Ray Regeneration** —— 仅在使用 DLSS 光线重建的游戏中（Cyberpunk 2077、Alan Wake 2），且游戏
  运行 DLSS（开启伪装）、光线追踪和光线重建都在游戏自身设置中启用。此时神经渲染在它之后、对它的
  输出运行，开销更大：帧率下降时调低 NR resolution。

## 出了问题怎么办

游戏目录中会出现 `OptiScaler.log`。在 `#bug-report` 中附上它，并说明游戏和显卡型号。AMD 后端还会
写出 `amd_presr.log` 和 `amd_bridge.log`，当神经渲染这一步出问题时它们最有用。

**《最后生还者 第一部》启动时崩溃？** 那是游戏自身 Streamline 初始化的问题，是已知的 OptiScaler
问题：把游戏目录中的 `sl.common.dll` 重命名为 `sl.common.dll.bak`，并在游戏设置中选择 **FSR 3.1**
而不是 DLSS。

本版本的完整说明：仓库中的 `RELEASE-NOTES.md`。

## 路线图

- **0.3.1**（本构建版）—— 修复 0.3.0 首批反馈的问题（仅安装 lmxxf 时从不运行、燕云十六声（Where Winds Meet）NR 无效、切换 DLSS
  画质时崩溃、GTA V Legacy 无法启动），并新增 NR 风格预设与三个自定义槽位。
- **0.3.0** —— **lmxxf** HIP 神经运行时（RDNA 4）作为可选运行时与 danielblnc 的并列，以
  `LmxxfNrRuntime.dll` + `LmxxfNrRuntime.pak` 发布：网络历史、真实的 Neural passes、编辑整形器、光线
  重建之后的位置、逐游戏诊断与自我修复。特别感谢 TheAutomatic，本次集成建立在他的 DLSS 5 AMD
  project 工作之上。
- **0.4.0** —— AMDNR Launcher（一键安装运行时与 pak、更新），以及对自身没有放大器的游戏（Stray 一类）
  的支持，由 OptiScaler 同时提供放大器和神经渲染。

---

## 致谢

本构建版是对他人工作的接线整合。如果你觉得它有用，感谢应归于上游。

- **DLSS-NR on AMD** —— *danielblnc* —— <https://github.com/danielblnc/DLSS-NR-on-AMD>
  `Runtime.zip` 中的 AMD 神经运行时与权重是他的 v0.3.1 发布，未经修改地再分发。网络实际计算的
  一切都是他的。
- **DLSS 5 AMD project** —— *TheAutomatic* —— <https://github.com/TheAutomatic/dlss-5-amd-project>
  AMD 硬件上 DLSS 5 神经渲染的基础与参考；0.3.0 中发布的 lmxxf 运行时集成沿着他的工作展开。特别感谢。
- **Matheus / dlss-5-amd** —— <https://github.com/MatheusGViana/dlss-5-amd-project>
  本代码树所派生自的 AMD pre-SR 桥接。
- **lmxxf / dlss5-on-amd-9070xt-porting** —— <https://github.com/lmxxf/dlss5-on-amd-9070xt-porting>
  开源的 HIP 神经渲染运行时（MIT）；这里的差异门控时序模式沿用了他的 `native_output_smooth`。
- **OptiScaler** —— *Overclockers* 及贡献者 —— <https://github.com/Overclockers/OptiScaler-Releases>
  本项目所嵌入的框架：挂钩、FSR/XeSS/帧生成管线、菜单，以及让这一切得以触达的游戏兼容性。

代码谱系：OptiScaler → Dagherbou / OptiScaler_DLSSNR → wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass
→ Matheus / dlss-5-amd → 本构建版。XeFG 解锁与节拍移植自 Coldwood1026 的 XeFGUnlock（GPL-3.0）；
`CubeScale` 色域处理来自 *hhkbble*。

## 法律声明

本构建版按 `LICENSE` 中的 GPL-3.0 许可证分发；第三方库许可证位于 `Licenses\`。AMD 神经运行时及其
权重按上述原作者署名再分发，仅为方便使用，不主张任何所有权，不提供任何保证。

NVIDIA 的 `nvngx_dlssnr.dll` 不在本压缩包中。以上内容均未获得 NVIDIA、AMD 或任何游戏发行商的认可、
关联或支持。它直接驱动一项未公开的功能。使用风险自负。
