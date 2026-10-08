# REPORT-README - AMDNR 0.3.5.3 READMEs (rounds 1 to 4)

Prepared 2026-10-08 for the owner to copy in. This lane wrote nothing under `release\`; no commit, push, upload, build
or GPU.

## 0. Read first

- **Finished copies (9 files, rounds 1 to 4):** `E:\amdnr-scratch\rel0353-final\github\README*.md`.
- **The release draft folder holds the round-1 copies.** At about 12:50 the 9 files in
  `C:\Users\Administrator\Desktop\dlss-amd\OptiScaler-DLSSNR-PreSR-Multipass-main\release\draft-035\github\` became
  byte-identical to the round-1 outputs (sha256 column "Round 1" below). This lane did not write there; someone copied
  them in. **Copy the 9 files again** to get rounds 2 to 4 (still true when round 4 was written).
- The original drafts are no longer on disk in that folder; this report's "draft" column was rebuilt from the round-1
  files and the two old lines of each, and its sha256 matches the drafts read before round 1.

## 1. What changed

**Round 1 - AMDNR Screen-space GI by 3zwr1 is official (2 lines per file).** The feature bullet and the credits line,
from `E:\amdnr-scratch\ssgi0353-root\CHANGELOG-ENTRY.md` section 4, with the cost figure as
`E:\amdnr-scratch\ssgicost0353-root\CHANGELOG-ENTRY.md` 1c and `E:\amdnr-scratch\ssgitemp0353-root\CHANGELOG-ENTRY.md`
section 2 leave it: "about 0.6 ms at High for a 1080p render on an RX 9070 XT (measured)" (the release-build
measurement, 0.618 ms, agrees). `E:\amdnr-scratch\ssgimove0353-root\CHANGELOG-ENTRY.md` section 2 says "README: no
change"; confirmed, the rest of that entry is CHANGELOG, Discord and orchestrator text.

**Round 2a - Magpie wording (4 spots per file).** "fetched from the author, not redistributed by AMDNR" is false.
Every README now says what section 5 of `E:\amdnr-scratch\rel0353-final\github\Licenses\AMDNR_NOTICE.txt` says: the host
is Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), in AMDNR's own build; `Magpie.exe` rebuilt with four
fixes; the patches, the base commit and the build options in `Anywhere-host-source/` beside it; no NVIDIA files; the
AMDNR Launcher downloads it from AMDNR's GitHub release. The brief named two spots (the "What it is" paragraph and the
credits line). Two more sentences made the same claim and were fixed the same way: the Anywhere "Status: preview"
paragraph ("fetched from its author's GitHub release, not from ours") and the PLAY ANYWHERE line of "New in Launcher
0.3.5.1" ("fetches the capture host from its author's release"). The manifest, not the launcher version, decides where
the host comes from, so that line was wrong for every launcher too.
The size "467 MB" (the author's package) became "about 100 MB": the NVIDIA-free host package 3e is 101,875,356 B
(`E:\amdnr-scratch\reports\r1006\MAGPIE-CLEAN-0353.md`). That assumes the owner publishes 3e as that report's steps
say, which is also the package the new NOTICE describes.

**Round 2b - GI menu layout (2 spots per file).** Read from the r035 code:
- `E:\amdnr-scratch\r035\OptiScaler\dlssnr\menu\NeuralGi.cpp` and `dlssnr\gi\GiRules.h`: the section header
  "Screen-space GI" (`kSectionLabel`), the checkbox "AMDNR Screen-space GI" with the dim tag "by 3zwr1", off by default.
  While ticked: GI quality (Low / Medium / High / Ultra, Auto per GPU until you press one, a tag with the measured ms or
  an estimate), Bounce light, Ambient occlusion, Radius, Object thickness, Camera FOV, More GI options (Bounce colour,
  Sky light, Multi-bounce, FOV axis, Colour encoding, Debug view) and Reset GI. Drawn on every AMD GPU, with or
  without an NR runtime; inside the Anywhere host its controls are disabled with the reason.
- `dlssnr\DlssNr_Menu.cpp` RenderMenu: the tab draws Image look, then Screen-space GI, then the tools row.
- `dlssnr\menu\NeuralTools.cpp` DrawTools / DrawExperimental: the Experimental button is drawn only while
  `[AmdRtgi] Enabled` is on, and then holds only the retired inherited checkbox, tagged "retired". No pointer line
  (owner 2026-10-06; `GiRules.h` last line). `E:\amdnr-scratch\ssgi0353-root\PROGRESS.md` has the move out of the
  drawer; the pointer line it mentions was removed after it (`E:\amdnr-scratch\reports\r1006\VERIFY-0353.md` O5).
So the tools-row line no longer names GI in Experimental, and a new "Screen-space GI" line sits before the tools row,
in the list's style (bold section name, menu rows in English, folds in bold).

**Round 3 - the Neural-tab list's Ray Regeneration line (1 spot per file).** It described a pointer row with an "Open
Ray Regeneration" button; the owner took Ray Regeneration out of the Neural tab on 2026-10-06. Read from the r035 code:
- `E:\amdnr-scratch\r035\OptiScaler\dlssnr\DlssNr_Menu.cpp` RenderMenu draws no Ray Regeneration piece on the Neural
  tab any more (only the comment "take the RR out of the Neural tab" where the call was).
- `dlssnr\menu\RayRegenTab.cpp`: `DrawRayRegenerationPointer` (the pointer row and its status block) is defined but
  called nowhere; `RenderRayRegenerationTab` is the only caller of `DrawRayRegeneration`, with ownTab = true.
- `dlssnr\menu\NeuralLook.cpp` lines 210-212: `DrawRrBackendRow` (Denoiser backend) is called only `if (ownTab)`, so it
  is drawn on the Ray Regeneration tab only; the `if (!ownTab)` debug-view hook in the same function never runs.
- What the Neural tab still shows about RR: the attention line under the runtime (`dlssnr\menu\NeuralTop.cpp` offers 9
  and 9b), "Ray Regeneration is off in this title" or "Ray Regeneration could not start on this GPU", no button, the
  reason in its hover; not inside the Anywhere host.
So the bullet became one line in the list's style: Ray Regeneration is not on this tab since 0.3.5.3, its controls and
status are on its own tab, and the orange line can say those two things.

## 2. Byte-level checks (python, before and after each round)

| File | Draft sha256 / lines | Round 1 | Round 2 | Round 3 | Final (round 4) sha256 | Final bytes / lines | Release folder now |
|---|---|---|---|---|---|---|---|
| README.md | `87131cb1b0fae415` / 1014 | `3a829e7b4f1961ba` | `5d8e83bcc725cd33` | `28c45377a13f2651` | `19a3cb785e745a4b` | 85,847 / 993 | round-1 copy |
| README.ar.md | `ecda041e89c05e26` / 1029 | `e9b8a790fb710387` | `90f003b04407bd42` | `982fc8aa37fa4ff2` | `052cc09960ce4e66` | 129,719 / 1009 | round-1 copy |
| README.es.md | `66e5f3fb0b77b60e` / 1087 | `59e913b79b439a5d` | `4071f86cc14c7525` | `92b5c0717141295d` | `adad02e8d45c2d99` | 96,298 / 1061 | round-1 copy |
| README.fr.md | `4927d8f5ebc14184` / 1104 | `e8dd03fb69fc0bb7` | `6266d01da16c211e` | `705aac8a34e7a14f` | `6691bac19abc9ce6` | 100,806 / 1079 | round-1 copy |
| README.it.md | `780b1cc2cab7eac1` / 1071 | `e190b261f295a1d2` | `b8bf53d766bb57dd` | `eb6950361e29a217` | `84be31367e228697` | 95,650 / 1045 | round-1 copy |
| README.pl.md | `d48abb6b10c6f79e` / 1054 | `06a5bc02f4c0d5ee` | `0f57d00fa2a3a43a` | `a4ca82a2d2b7c589` | `d30878005d47626b` | 94,705 / 1032 | round-1 copy |
| README.pt-BR.md | `1667b36c972b5279` / 1066 | `06f36b26ce8b19d0` | `6fcf2cd2f7b6bdc3` | `34853f7fbae9d269` | `2bc08e4d3704a8de` | 93,782 / 1040 | round-1 copy |
| README.ru.md | `0e01807372ea114c` / 1061 | `3b2085d17d447bb0` | `2538f671d154ac19` | `ff3e35e7da50cbe0` | `9beaf39a89a5ab74` | 138,485 / 1037 | round-1 copy |
| README.zh-CN.md | `533161e2471c2074` / 892 | `00ad84a728b91166` | `a1ac2b33a2340fbb` | `839035dd79703b66` | `b4d7155d6e7d1a75` | 91,367 / 874 | round-1 copy |

Full sha256 of the final files:

- `README.md`: `19a3cb785e745a4b1f8ca1f1aca3426d6ccb43b3c5e8b6d93fbfa38985604493`
- `README.ar.md`: `052cc09960ce4e66057be585490dc2f6efdc5de0093ed3e46e8df3cc9927919a`
- `README.es.md`: `adad02e8d45c2d99e206cbb6738592cec3be34d1a437f554a2aa00880effa932`
- `README.fr.md`: `6691bac19abc9ce6148514356db28341598a9f3e0df8499cde2aa6e2ee405ae6`
- `README.it.md`: `84be31367e2286973e6ac8a2138d7e4847a453a5ad7747b623fbd9c760a52c4e`
- `README.pl.md`: `d30878005d47626b1020053df786b375b7a4515a3690344a343a7dc35b0476d9`
- `README.pt-BR.md`: `2bc08e4d3704a8dec479350df74c66bb322aff2162cb82c88bb25cf9b9d145f5`
- `README.ru.md`: `9beaf39a89a5ab7428470e8d9f8b018378ecb068c2380c5c19f2de0aa1280420`
- `README.zh-CN.md`: `b4d7155d6e7d1a7577fd02690819d9fe73bf4876c6bbba95d2ace63619d118d4`

All 9 outputs: UTF-8, no BOM, CRLF only (no lone LF or CR), no byte under 32 other than tab, CR and LF, last line ends
with CRLF; CRLF count = line count. `diff` against the round-1 copies showed exactly the 6 round-2 changes per file
(for README.md: `316,317c316 344c343 529a529 532,533c532 918c917 939c938`), and against the round-2 copies exactly the
one round-3 change (README.md `525,528c525`). The new text has no NBSP, no curly apostrophe, no BOM, no competitor name.

## 3. Formatting choices

- **One line per changed spot.** Round 1 replaced one line by one line. In round 2, where an old spot ran over two
  wrapped lines, the new text is one line, and the new section is one inserted line. Round 3 made the 4-5 line
  Ray Regeneration bullet one line. Line numbers below are draft -> final (after round 3). The Ray Regeneration line
  (round 3) sits right above the new Screen-space GI line (round 2), so the two are shown as one block.
- **Round 1 dash.** README.md uses the lane text's ASCII " - " after the title exactly, as instructed (its neighbours
  use " — "; README.md line 257, the lmxxf 0.37 bullet, is the same case). Each translation does what it did with that
  same bullet: ar, es, fr, it and pt-BR keep " - "; pl and ru use " — "; zh-CN uses "——" in the file's spacing. One
  character per line if the owner wants " — " everywhere. Round 2 keeps each line's own dashes.
- **Kept in English in every language:** the names AMDNR Screen-space GI by 3zwr1 and Magpie by Blinue, experimental
  fork by SAOG0721 (GPL-3.0) (bold, as the files had them), menu words (Neural > Screen-space GI, Screen-space GI,
  GI quality, Low / Medium / High / Ultra, Auto, Bounce light ... Reset GI, More GI options, Experimental, Runtime
  options, "retired"), `[AmdGi] Enabled=true`, `[AmdRtgi] Enabled`, `Magpie.exe`, `Anywhere-host-source/`, file names,
  numbers (decimal point, as every file writes ms values; ru "мс", fr "Mo", ru "МБ" as the files write them).
- **French:** ordinary spaces before ":" and ";" and inside « », as the edited lines and that list already use.

## 4. Per file: old and new text, rounds 1-3 (draft line numbers -> line numbers after round 3)

Round 4 (section 4b) changed lines after these; the line numbers here are as they stood after round 3.

### README.md (English)

**Round 2 - Magpie: AMDNR Anywhere, "What it is"** - draft lines 316-317 -> final line 316. Old:

```text
SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR**: the AMDNR Launcher downloads it from its
author's release (467 MB, once).
```

New:

```text
SAOG0721 (GPL-3.0)**, in AMDNR's own build: its `Magpie.exe` is rebuilt with four fixes (the patches, the base commit and the build options are in `Anywhere-host-source/` beside it), and the package carries no NVIDIA files. The AMDNR Launcher downloads it from AMDNR's GitHub release (about 100 MB, once).
```

**Round 2 - Magpie: AMDNR Anywhere, "Status: preview"** - draft line 344 -> final line 343. Old:

```text
the host refuses it). The capture host is fetched from its author's GitHub release, not from ours. Reports: the
```

New:

```text
the host refuses it). The capture host is AMDNR's own build, downloaded from AMDNR's GitHub release. Reports: the
```

**Round 3 + Round 2 - Neural-tab list: the Ray Regeneration line (round 3) and the new Screen-space GI line under it (round 2)** - draft lines 526-529 -> final lines 525-526. Old:

```text
- **Ray Regeneration**: since 0.3.5 a pointer. While Ray Regeneration runs in the title, its controls are on their
  own **Ray Regeneration** tab (right after Upscaling) and this line offers an **Open Ray Regeneration** button;
  while it is not running, the line says why (the game has not turned Ray Reconstruction on, the driver refused the
  denoiser on this card, Ray Regeneration gave this title up and why, or when it last ran).
```

New:

```text
- **Ray Regeneration**: not on this tab since 0.3.5.3; its controls and status are on its own **Ray Regeneration** tab (right after Upscaling). Here only the orange line under the runtime can say "Ray Regeneration is off in this title" or "Ray Regeneration could not start on this GPU" (hover it for the reason).
- **Screen-space GI** (new in 0.3.5.3): **AMDNR Screen-space GI**, off by default, with a dim "by 3zwr1" after it; under it, while it is on: GI quality (Low / Medium / High / Ultra; Auto picks one for your GPU until you press one, and its tag shows the measured GPU time, or an estimate until GI has run), Bounce light, Ambient occlusion, Radius, Object thickness, Camera FOV, **More GI options** (Bounce colour, Sky light, Multi-bounce, FOV axis, Colour encoding, Debug view) and Reset GI. On every AMD GPU, with or without an NR runtime; greyed with its reason inside AMDNR Anywhere.
```

**Round 2 - GI layout: the tools row (Experimental)** - draft lines 532-533 -> final line 529. Old:

```text
  since 0.3.5), **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy) and **Experimental**
  (AMDNR Screen-space GI, a preview).
```

New:

```text
  since 0.3.5) and **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy); **Experimental** is there only while your ini has the retired inherited Screen-space GI on (`[AmdRtgi] Enabled`), with just its checkbox, tagged "retired".
```

**Round 1 - feature bullet, AMDNR Screen-space GI by 3zwr1** - draft line 634 -> final line 630. Old:

```text
- **AMDNR Screen GI** (preview, new in 0.3.4, off by default; Neural > Experimental, or `[AmdGi] Enabled=true`) — AMDNR's own screen-space bounce light and ambient occlusion, from the game's depth, before NR and the upscaler; works with NR on or off; about 1 ms at High for a 1080p render on an RX 9070 XT (measured outside a game). Screen-space: light from off-screen is missing. See `CHANGELOG.md`.
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** (off by default; Neural > Screen-space GI, or `[AmdGi] Enabled=true`) - AMDNR's own screen-space bounce light and ambient occlusion from the game's depth, before NR, the upscaler and the UI; works with NR on or off; Low / Medium / High / Ultra (Auto per GPU), about 0.6 ms at High for a 1080p render on an RX 9070 XT (measured). Screen-space: light from off screen is missing. See `CHANGELOG.md`.
```

**Round 2 - Magpie: credits line** - draft line 918 -> final line 914. Old:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR** — the window-capture host AMDNR Anywhere runs inside — https://github.com/Blinue/Magpie (the fork: https://github.com/SAOG0721/Magpie)
```

New:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** — the window-capture host AMDNR Anywhere runs inside, in AMDNR's own build, published on AMDNR's GitHub releases (`Magpie.exe` rebuilt with four fixes; the patches, the base commit and the build options in `Anywhere-host-source/` beside it; no NVIDIA files) — https://github.com/Blinue/Magpie (the fork: https://github.com/SAOG0721/Magpie)
```

**Round 1 - credits line, AMDNR Screen-space GI by 3zwr1** - draft line 923 -> final line 919. Old:

```text
- **AMDNR Screen GI** (0.3.4 preview) — AMDNR's own, Copyright (c) 2026 3zwr1 (AMDNR), written from published papers (Therrien, Levesque and Gilet 2023; Jimenez et al. 2016; Schied et al. 2017; and the others listed in `CHANGELOG.md` and `Licenses/AMDNR_NOTICE.txt`)
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** - AMDNR's own, Copyright (c) 2026 3zwr1 (AMDNR), written from published research (listed in `CHANGELOG.md` and `Licenses/AMDNR_NOTICE.txt`).
```

**Round 2 - Magpie: AMDNR Launcher, PLAY ANYWHERE line** - draft line 939 -> final line 935. Old:

```text
  host from its author's release, starts the game and runs Neural Rendering on its window, with a read-only summary
```

New:

```text
  host from AMDNR's GitHub release, starts the game and runs Neural Rendering on its window, with a read-only summary
```

**Terminology, round 1:** The lane text exactly: ssgi0353 section 4 with the cost figure as ssgicost0353 1c and ssgitemp0353 section 2 leave it ("about 0.6 ms at High for a 1080p render on an RX 9070 XT (measured)"); the credits line word for word, full stop included. Only the hard wraps were joined.

**Terminology, round 2:** Round 2 text written from the NOTICE's Magpie bullet, the coordinator's five points, and the r035 menu code (row names exactly as the menu draws them).

**Terminology, round 3:** the menu's own strings stay English and quoted; the tab's place and the orange line use this file's existing words for them (its old Ray Regeneration bullet and its Neural runtime bullet).

### README.ar.md (Arabic)

**Round 2 - Magpie: AMDNR Anywhere, "What it is"** - draft lines 323-324 -> final line 323. Old:

```text
افتراضيًا) مع تبويب **Anywhere** خاص بها. والمضيف هو **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR**:
يحمّله AMDNR Launcher من إصدار مؤلفه (467 MB، مرة واحدة).
```

New:

```text
افتراضيًا) مع تبويب **Anywhere** خاص بها. والمضيف هو **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** في نسخة AMDNR الخاصة: أُعيد بناء ملفه `Magpie.exe` مع أربعة إصلاحات، وملفات الباتش والـ commit الأساسي وخيارات البناء موجودة بجانبه في `Anywhere-host-source/`، ولا تحتوي الحزمة على أي ملف من NVIDIA. ويحمّله AMDNR Launcher من إصدار AMDNR على GitHub (نحو 100 MB، مرة واحدة).
```

**Round 2 - Magpie: AMDNR Anywhere, "Status: preview"** - draft line 352 -> final line 351. Old:

```text
interleave مطفأً - فالمضيف يرفضه). يُجلب مضيف الالتقاط من إصدار مؤلفه على GitHub، لا من إصدارنا. البلاغات: ملف zip
```

New:

```text
interleave مطفأً - فالمضيف يرفضه). مضيف الالتقاط نسخة AMDNR الخاصة، ويُجلب من إصدار AMDNR على GitHub. البلاغات: ملف zip
```

**Round 3 + Round 2 - Neural-tab list: the Ray Regeneration line (round 3) and the new Screen-space GI line under it (round 2)** - draft lines 535-538 -> final lines 534-535. Old:

```text
- **Ray Regeneration**: منذ 0.3.5 صار سطر إشارة. فما دامت Ray Regeneration تعمل في اللعبة، تكون عناصر التحكم الخاصة
  بها في تبويب **Ray Regeneration** المستقل (بعد Upscaling مباشرة)، ويعرض هذا السطر زر **Open Ray Regeneration**؛
  وحين لا تعمل، يوضّح السطر السبب (اللعبة لم تفعّل Ray Reconstruction، أو أن التعريف رفض مزيل التشويش على هذه
  البطاقة، أو أن Ray Regeneration تخلّت عن هذه اللعبة والسبب، أو متى عملت آخر مرة).
```

New:

```text
- **Ray Regeneration**: لم تعد في هذا التبويب منذ 0.3.5.3؛ فعناصر التحكم والحالة الخاصة بها في تبويب **Ray Regeneration** المستقل (بعد Upscaling مباشرة). ولا يبقى هنا إلا السطر البرتقالي تحت بيئة التشغيل، الذي قد يقول "Ray Regeneration is off in this title" أو "Ray Regeneration could not start on this GPU" (مرّر المؤشر فوقه لمعرفة السبب).
- **Screen-space GI** (جديد في 0.3.5.3): **AMDNR Screen-space GI**، معطّل افتراضيًا، وبعده كلمة "by 3zwr1" الباهتة؛ وتحته، ما دام مفعّلًا: GI quality (Low / Medium / High / Ultra؛ يختار Auto واحدًا منها لبطاقتك إلى أن تضغط أحدها، ويعرض وسمه زمن GPU المقيس، أو تقديرًا إلى أن يعمل GI)، وBounce light، وAmbient occlusion، وRadius، وObject thickness، وCamera FOV، و**More GI options** (Bounce colour وSky light وMulti-bounce وFOV axis وColour encoding وDebug view)، وReset GI. ويظهر على كل بطاقة رسوميات من AMD، مع بيئة تشغيل NR أو بدونها؛ وداخل AMDNR Anywhere يظهر رماديًا مع ذكر السبب.
```

**Round 2 - GI layout: the tools row (Experimental)** - draft lines 541-542 -> final line 538. Old:

```text
  و**Runtime options** (Encoding وEvery-frame NR وNR slots وHighlight proxy)، و**Experimental** (AMDNR Screen-space GI،
  بنسخة preview).
```

New:

```text
  و**Runtime options** (Encoding وEvery-frame NR وNR slots وHighlight proxy)؛ ولا يظهر **Experimental** إلا ما دام ملف ini لديك يفعّل تأثير Screen-space GI الموروث الذي أُزيل من القائمة (`[AmdRtgi] Enabled`)، ولا يحوي إلا مربعه، مع وسم "retired".
```

**Round 1 - feature bullet, AMDNR Screen-space GI by 3zwr1** - draft line 646 -> final line 642. Old:

```text
- **AMDNR Screen GI** (preview، جديد في 0.3.4، معطّل افتراضيًا؛ Neural > Experimental، أو `[AmdGi] Enabled=true`) — الضوء المرتد وحجب الإضاءة المحيطة (ambient occlusion) في مساحة الشاشة من صنع AMDNR، من عمق اللعبة، قبل NR والـ upscaler؛ يعمل مع NR مفعّلًا أو معطّلًا؛ نحو 1 ms على High مع رسم بدقة 1080p على RX 9070 XT (مقيس خارج لعبة). وهو يعمل في مساحة الشاشة: الضوء القادم من خارج الشاشة غير موجود. راجع `CHANGELOG.md`.
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** (معطّل افتراضيًا؛ Neural > Screen-space GI، أو `[AmdGi] Enabled=true`) - الضوء المرتد وحجب الإضاءة المحيطة (ambient occlusion) في مساحة الشاشة من صنع AMDNR، من عمق اللعبة، قبل NR والـ upscaler وواجهة المستخدم (UI)؛ يعمل مع NR مفعّلًا أو معطّلًا؛ Low / Medium / High / Ultra (Auto حسب بطاقة الرسوميات)، نحو 0.6 ms على High مع رسم بدقة 1080p على RX 9070 XT (مقيس). وهو يعمل في مساحة الشاشة: الضوء القادم من خارج الشاشة غير موجود. راجع `CHANGELOG.md`.
```

**Round 2 - Magpie: credits line** - draft line 933 -> final line 929. Old:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR** — مضيف التقاط النوافذ الذي يعمل داخله AMDNR Anywhere — https://github.com/Blinue/Magpie (التفرّع: https://github.com/SAOG0721/Magpie)
```

New:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** — مضيف التقاط النوافذ الذي يعمل داخله AMDNR Anywhere، في نسخة AMDNR الخاصة المنشورة في إصدارات AMDNR على GitHub (`Magpie.exe` أُعيد بناؤه مع أربعة إصلاحات؛ وملفات الباتش والـ commit الأساسي وخيارات البناء بجانبه في `Anywhere-host-source/`؛ ولا ملفات من NVIDIA) — https://github.com/Blinue/Magpie (التفرّع: https://github.com/SAOG0721/Magpie)
```

**Round 1 - credits line, AMDNR Screen-space GI by 3zwr1** - draft line 938 -> final line 934. Old:

```text
- **AMDNR Screen GI** (preview في 0.3.4) — عمل AMDNR الخاص، Copyright (c) 2026 3zwr1 (AMDNR)، مكتوب انطلاقًا من أوراق بحثية منشورة (Therrien وLevesque وGilet 2023؛ Jimenez وآخرون 2016؛ Schied وآخرون 2017؛ والبقية مذكورة في `CHANGELOG.md` و`Licenses/AMDNR_NOTICE.txt`)
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** - عمل AMDNR الخاص، Copyright (c) 2026 3zwr1 (AMDNR)، مكتوب انطلاقًا من أبحاث منشورة (مذكورة في `CHANGELOG.md` و`Licenses/AMDNR_NOTICE.txt`).
```

**Round 2 - Magpie: AMDNR Launcher, PLAY ANYWHERE line** - draft line 954 -> final line 950. Old:

```text
  من إصدار مؤلفه، ويشغّل اللعبة، ويشغّل Neural Rendering على نافذتها، مع ملخص للقراءة فقط لإعدادات المضيف بجانب الزر
```

New:

```text
  من إصدار AMDNR على GitHub، ويشغّل اللعبة، ويشغّل Neural Rendering على نافذتها، مع ملخص للقراءة فقط لإعدادات المضيف بجانب الزر
```

**Terminology, round 1:** Menu path, preset names, Auto and the ini key stay in English, as the old spot had "Neural > Experimental". "off by default" = "معطّل افتراضيًا" and "measured" = "مقيس" (the old spot's words; "(مقيس خارج لعبة)" became "(مقيس)"). "the UI" = "واجهة المستخدم (UI)": this file also uses "واجهة" for API and backend, so the English term follows in brackets, the way the file writes "(ambient occlusion)" and "(HUD)". "per GPU" = "حسب بطاقة الرسوميات", the file's words for per-GPU defaults in the credits. "published research" = "أبحاث منشورة" (was "أوراق بحثية منشورة", published papers); "listed in" = "مذكورة في", the old line's word. "ms", Arabic comma and semicolon as in the file.

**Terminology, round 2:** "نسخة AMDNR الخاصة" for AMDNR's own build (the file says "نسخة" for a build, e.g. "نسخة OptiScaler"); "أُعيد بناء" rebuilt; "إصلاحات" fixes and "الباتش" patch, the file's words; "خيارات البناء" build options; "إصدار AMDNR على GitHub" (the file says "إصدار" for a release). GI line: the list's own phrasing ("ما دام مفعّلًا"، "الباهتة"، "وسم"، "يظهر رماديًا مع ذكر السبب"), "لبطاقتك" for your GPU, "مربعه" for its checkbox (the file says "مربع"), "ملف ini لديك".

**Terminology, round 3:** the menu's own strings stay English and quoted; the tab's place and the orange line use this file's existing words for them (its old Ray Regeneration bullet and its Neural runtime bullet).

### README.es.md (Spanish)

**Round 2 - Magpie: AMDNR Anywhere, "What it is"** - draft lines 336-337 -> final line 336. Old:

```text
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR**:
el AMDNR Launcher lo descarga de la release de su autor (467 MB, una sola vez).
```

New:

```text
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)**, en la build propia de AMDNR: su `Magpie.exe` está recompilado con cuatro correcciones (los parches, el commit base y las opciones de compilación están a su lado, en `Anywhere-host-source/`) y el paquete no lleva ningún archivo de NVIDIA. El AMDNR Launcher lo descarga de la release de AMDNR en GitHub (unos 100 MB, una sola vez).
```

**Round 2 - Magpie: AMDNR Anywhere, "Status: preview"** - draft lines 367-368 -> final line 366. Old:

```text
interleave desactivado — el host lo rechaza). El host de captura se descarga de la release de GitHub de su autor, no
de la nuestra. Reportes: el zip de **Save report** desde el menú dentro del host (su título nombra el juego
```

New:

```text
interleave desactivado — el host lo rechaza). El host de captura es la build propia de AMDNR y se descarga de la release de AMDNR en GitHub. Reportes: el zip de **Save report** desde el menú dentro del host (su título nombra el juego
```

**Round 3 + Round 2 - Neural-tab list: the Ray Regeneration line (round 3) and the new Screen-space GI line under it (round 2)** - draft lines 560-564 -> final lines 558-559. Old:

```text
- **Ray Regeneration**: desde 0.3.5, un puntero. Mientras Ray Regeneration se ejecuta en el título, sus controles
  están en su propia pestaña **Ray Regeneration** (justo después de Upscaling) y esta línea ofrece un botón **Open
  Ray Regeneration**; mientras no se ejecuta, la línea dice por qué (el juego no ha activado Ray Reconstruction, el
  controlador rechazó el eliminador de ruido en esta tarjeta, Ray Regeneration abandonó este título y por qué, o
  cuándo se ejecutó por última vez).
```

New:

```text
- **Ray Regeneration**: ya no está en esta pestaña desde 0.3.5.3; sus controles y su estado están en su propia pestaña **Ray Regeneration** (justo después de Upscaling). Aquí solo la línea naranja bajo el runtime puede decir "Ray Regeneration is off in this title" o "Ray Regeneration could not start on this GPU" (pasa el ratón por encima para ver el motivo).
- **Screen-space GI** (sección nueva en 0.3.5.3): **AMDNR Screen-space GI**, desactivado por defecto, con un "by 3zwr1" tenue detrás; debajo, mientras está activado: GI quality (Low / Medium / High / Ultra; Auto elige uno para tu GPU hasta que pulses uno, y su etiqueta muestra el tiempo de GPU medido, o una estimación mientras GI no se ha ejecutado), Bounce light, Ambient occlusion, Radius, Object thickness, Camera FOV, **More GI options** (Bounce colour, Sky light, Multi-bounce, FOV axis, Colour encoding, Debug view) y Reset GI. En toda GPU AMD, con o sin runtime de NR; dentro de AMDNR Anywhere aparece en gris con su motivo.
```

**Round 2 - GI layout: the tools row (Experimental)** - draft lines 567-568 -> final line 562. Old:

```text
  la pestaña Ray Regeneration desde 0.3.5), **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy)
  y **Experimental** (AMDNR Screen-space GI, en preview).
```

New:

```text
  la pestaña Ray Regeneration desde 0.3.5) y **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy); **Experimental** solo aparece mientras tu ini tiene activado el Screen-space GI heredado y retirado (`[AmdRtgi] Enabled`), y solo con su casilla, con la etiqueta "retired".
```

**Round 1 - feature bullet, AMDNR Screen-space GI by 3zwr1** - draft line 680 -> final line 674. Old:

```text
- **AMDNR Screen GI** (preview, nuevo en 0.3.4, desactivado por defecto; Neural > Experimental, o `[AmdGi] Enabled=true`) — la luz rebotada y la oclusión ambiental en espacio de pantalla propias de AMDNR, a partir de la profundidad del juego, antes de NR y del upscaler; funciona con NR activado o desactivado; alrededor de 1 ms en High con un render de 1080p en una RX 9070 XT (medido fuera de un juego). Es espacio de pantalla: falta la luz que viene de fuera de la pantalla. Ver `CHANGELOG.md`.
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** (desactivado por defecto; Neural > Screen-space GI, o `[AmdGi] Enabled=true`) - la luz rebotada y la oclusión ambiental en espacio de pantalla propias de AMDNR, a partir de la profundidad del juego, antes de NR, del upscaler y de la interfaz; funciona con NR activado o desactivado; Low / Medium / High / Ultra (Auto por GPU), alrededor de 0.6 ms en High con un render de 1080p en una RX 9070 XT (medido). Es espacio de pantalla: falta la luz que viene de fuera de la pantalla. Ver `CHANGELOG.md`.
```

**Round 2 - Magpie: credits line** - draft line 986 -> final line 980. Old:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR** — el host de captura de ventana dentro del que se ejecuta AMDNR Anywhere — https://github.com/Blinue/Magpie (el fork: https://github.com/SAOG0721/Magpie)
```

New:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** — el host de captura de ventana dentro del que se ejecuta AMDNR Anywhere, en la build propia de AMDNR publicada en las releases de AMDNR en GitHub (`Magpie.exe` recompilado con cuatro correcciones; los parches, el commit base y las opciones de compilación a su lado, en `Anywhere-host-source/`; ningún archivo de NVIDIA) — https://github.com/Blinue/Magpie (el fork: https://github.com/SAOG0721/Magpie)
```

**Round 1 - credits line, AMDNR Screen-space GI by 3zwr1** - draft line 991 -> final line 985. Old:

```text
- **AMDNR Screen GI** (preview de 0.3.4) — obra propia de AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), escrita a partir de artículos publicados (Therrien, Levesque y Gilet 2023; Jimenez et al. 2016; Schied et al. 2017; y los demás que se citan en `CHANGELOG.md` y `Licenses/AMDNR_NOTICE.txt`)
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** - obra propia de AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), escrita a partir de investigaciones publicadas (que se citan en `CHANGELOG.md` y `Licenses/AMDNR_NOTICE.txt`).
```

**Round 2 - Magpie: AMDNR Launcher, PLAY ANYWHERE line** - draft line 1007 -> final line 1001. Old:

```text
  de la release de su autor, inicia el juego y ejecuta Neural Rendering sobre su ventana, con un resumen de solo
```

New:

```text
  de la release de AMDNR en GitHub, inicia el juego y ejecuta Neural Rendering sobre su ventana, con un resumen de solo
```

**Terminology, round 1:** "desactivado por defecto", "upscaler" and "High" as in the old spot. "per GPU" = "por GPU" (the file: "los valores por defecto por GPU"). "the UI" = "la interfaz" (the file has no UI word; "HUD" is only used for the HUD). "measured" = "(medido)" (was "medido fuera de un juego"). Credits: "investigaciones publicadas (que se citan en ...)", reusing the old line's "que se citan en".

**Terminology, round 2:** "la build propia de AMDNR" (build is feminine in this file), "recompilado", "correcciones", "parches" (the file says "parche"), "opciones de compilación", "la release de AMDNR en GitHub". GI line: the list's phrasing ("mientras está activado", "tenue", "etiqueta", "aparece en gris"), "sección nueva en 0.3.5.3", "tu ini", "casilla".

**Terminology, round 3:** the menu's own strings stay English and quoted; the tab's place and the orange line use this file's existing words for them (its old Ray Regeneration bullet and its Neural runtime bullet).

### README.fr.md (French)

**Round 2 - Magpie: AMDNR Anywhere, "What it is"** - draft lines 339-340 -> final line 339. Old:

```text
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR** :
l'AMDNR Launcher le télécharge depuis la release de son auteur (467 Mo, une seule fois).
```

New:

```text
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)**, dans le build propre à AMDNR : son `Magpie.exe` est recompilé avec quatre correctifs (les patchs, le commit de base et les options de compilation sont à côté, dans `Anywhere-host-source/`) et le paquet ne contient aucun fichier NVIDIA. L'AMDNR Launcher le télécharge depuis la release GitHub d'AMDNR (environ 100 Mo, une seule fois).
```

**Round 2 - Magpie: AMDNR Anywhere, "Status: preview"** - draft lines 371-372 -> final line 370. Old:

```text
jeu à 60-90 fps, laissez Model interleave désactivé - l'hôte le refuse). L'hôte de capture est récupéré depuis la
release GitHub de son auteur, pas depuis la nôtre. Rapports : le zip **Save report** du menu dans l'hôte (son titre
```

New:

```text
jeu à 60-90 fps, laissez Model interleave désactivé - l'hôte le refuse). L'hôte de capture est le build propre à AMDNR, récupéré depuis la release GitHub d'AMDNR. Rapports : le zip **Save report** du menu dans l'hôte (son titre
```

**Round 3 + Round 2 - Neural-tab list: the Ray Regeneration line (round 3) and the new Screen-space GI line under it (round 2)** - draft lines 570-574 -> final lines 568-569. Old:

```text
- **Ray Regeneration** : depuis la 0.3.5, un renvoi. Tant que Ray Regeneration tourne dans le titre, ses réglages se
  trouvent dans son propre onglet **Ray Regeneration** (juste après Upscaling) et cette ligne propose un bouton
  **Open Ray Regeneration** ; tant qu'il ne tourne pas, la ligne dit pourquoi (le jeu n'a pas activé Ray
  Reconstruction, le pilote a refusé le débruiteur sur cette carte, Ray Regeneration a renoncé à ce titre et
  pourquoi, ou quand il a tourné pour la dernière fois).
```

New:

```text
- **Ray Regeneration** : n'est plus dans cet onglet depuis la 0.3.5.3 ; ses réglages et son état sont dans son propre onglet **Ray Regeneration** (juste après Upscaling). Ici, seule la ligne orange sous le runtime peut dire « Ray Regeneration is off in this title » ou « Ray Regeneration could not start on this GPU » (survolez-la pour la raison).
- **Screen-space GI** (nouvelle section en 0.3.5.3) : **AMDNR Screen-space GI**, désactivé par défaut, suivi d'un « by 3zwr1 » discret ; dessous, tant qu'il est activé : GI quality (Low / Medium / High / Ultra ; Auto en choisit un pour votre GPU tant que vous n'en avez choisi aucun, et son étiquette affiche le temps GPU mesuré, ou une estimation tant que la GI n'a pas tourné), Bounce light, Ambient occlusion, Radius, Object thickness, Camera FOV, **More GI options** (Bounce colour, Sky light, Multi-bounce, FOV axis, Colour encoding, Debug view) et Reset GI. Sur tout GPU AMD, avec ou sans runtime NR ; dans AMDNR Anywhere, il est grisé avec sa raison.
```

**Round 2 - GI layout: the tools row (Experimental)** - draft lines 577-578 -> final line 572. Old:

```text
  l'onglet Ray Regeneration depuis la 0.3.5), **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight
  proxy) et **Experimental** (AMDNR Screen-space GI, en preview).
```

New:

```text
  l'onglet Ray Regeneration depuis la 0.3.5) et **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy) ; **Experimental** n'apparaît que tant que votre ini active le Screen-space GI hérité et retiré (`[AmdRtgi] Enabled`), avec seulement sa case, marquée « retired ».
```

**Round 1 - feature bullet, AMDNR Screen-space GI by 3zwr1** - draft line 691 -> final line 685. Old:

```text
- **AMDNR Screen GI** (preview, nouveau en 0.3.4, désactivé par défaut ; Neural > Experimental, ou `[AmdGi] Enabled=true`) — la lumière rebondie et l'occlusion ambiante en espace écran propres à AMDNR, à partir de la profondeur du jeu, avant NR et l'upscaler ; fonctionne avec NR activé ou non ; environ 1 ms en High pour un rendu 1080p sur une RX 9070 XT (mesuré hors jeu). C'est de l'espace écran : la lumière venant de hors de l'écran manque. Voir `CHANGELOG.md`.
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** (désactivé par défaut ; Neural > Screen-space GI, ou `[AmdGi] Enabled=true`) - la lumière rebondie et l'occlusion ambiante en espace écran propres à AMDNR, à partir de la profondeur du jeu, avant NR, l'upscaler et l'interface ; fonctionne avec NR activé ou non ; Low / Medium / High / Ultra (Auto par GPU), environ 0.6 ms en High pour un rendu 1080p sur une RX 9070 XT (mesuré). C'est de l'espace écran : la lumière venant de hors de l'écran manque. Voir `CHANGELOG.md`.
```

**Round 2 - Magpie: credits line** - draft line 1001 -> final line 995. Old:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR** — l'hôte de capture de fenêtre dans lequel tourne AMDNR Anywhere — https://github.com/Blinue/Magpie (le fork : https://github.com/SAOG0721/Magpie)
```

New:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** — l'hôte de capture de fenêtre dans lequel tourne AMDNR Anywhere, dans le build propre à AMDNR publié sur les releases GitHub d'AMDNR (`Magpie.exe` recompilé avec quatre correctifs ; les patchs, le commit de base et les options de compilation à côté, dans `Anywhere-host-source/` ; aucun fichier NVIDIA) — https://github.com/Blinue/Magpie (le fork : https://github.com/SAOG0721/Magpie)
```

**Round 1 - credits line, AMDNR Screen-space GI by 3zwr1** - draft line 1006 -> final line 1000. Old:

```text
- **AMDNR Screen GI** (preview 0.3.4) — le travail propre d'AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), écrit à partir d'articles publiés (Therrien, Levesque et Gilet 2023 ; Jimenez et al. 2016 ; Schied et al. 2017 ; et les autres cités dans `CHANGELOG.md` et `Licenses/AMDNR_NOTICE.txt`)
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** - le travail propre d'AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), écrit à partir de recherches publiées (citées dans `CHANGELOG.md` et `Licenses/AMDNR_NOTICE.txt`).
```

**Round 2 - Magpie: AMDNR Launcher, PLAY ANYWHERE line** - draft line 1025 -> final line 1019. Old:

```text
  capture depuis la release de son auteur, lance le jeu et fait tourner Neural Rendering sur sa fenêtre, avec un
```

New:

```text
  capture depuis la release GitHub d'AMDNR, lance le jeu et fait tourner Neural Rendering sur sa fenêtre, avec un
```

**Terminology, round 1:** "désactivé par défaut", "upscaler", "en High" as in the old spot; ordinary spaces before ";" and ":" exactly as the old spot (the file mostly uses ordinary spaces). "per GPU" = "par GPU" (the file: "les réglages par défaut par GPU"; GPU is masculine in this file). "the UI" = "l'interface". "(mesuré)" (was "mesuré hors jeu"). Credits: "recherches publiées (citées dans ...)", from the old "les autres cités dans".

**Terminology, round 2:** "le build propre à AMDNR" (masculine, as the file mostly writes it), "recompilé", "correctifs" (the file's word for fixes), "patchs", "options de compilation", "la release GitHub d'AMDNR", "100 Mo". GI line: "tant qu'il est activé", "discret", "étiquette", "grisé", "nouvelle section en 0.3.5.3", "votre ini", "case"; ordinary spaces before ":" ";" and inside « ».

**Terminology, round 3:** the menu's own strings stay English and quoted; the tab's place and the orange line use this file's existing words for them (its old Ray Regeneration bullet and its Neural runtime bullet).

### README.it.md (Italian)

**Round 2 - Magpie: AMDNR Anywhere, "What it is"** - draft lines 331-332 -> final line 331. Old:

```text
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR**:
l'AMDNR Launcher lo scarica dalla release del suo autore (467 MB, una sola volta).
```

New:

```text
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)**, nella build propria di AMDNR: il suo `Magpie.exe` è ricompilato con quattro correzioni (le patch, il commit di base e le opzioni di build sono accanto, in `Anywhere-host-source/`) e il pacchetto non contiene file NVIDIA. L'AMDNR Launcher lo scarica dalla release GitHub di AMDNR (circa 100 MB, una sola volta).
```

**Round 2 - Magpie: AMDNR Anywhere, "Status: preview"** - draft lines 361-362 -> final line 360. Old:

```text
lascia Model interleave spento - l'host lo rifiuta). L'host di cattura viene scaricato dalla release GitHub del suo
autore, non dalla nostra. Segnalazioni: lo zip di **Save report** dal menu dentro l'host (il suo titolo indica il gioco
```

New:

```text
lascia Model interleave spento - l'host lo rifiuta). L'host di cattura è la build propria di AMDNR, scaricata dalla release GitHub di AMDNR. Segnalazioni: lo zip di **Save report** dal menu dentro l'host (il suo titolo indica il gioco
```

**Round 3 + Round 2 - Neural-tab list: the Ray Regeneration line (round 3) and the new Screen-space GI line under it (round 2)** - draft lines 552-556 -> final lines 550-551. Old:

```text
- **Ray Regeneration**: dalla 0.3.5 un rimando. Mentre Ray Regeneration è in funzione nel titolo, i suoi controlli sono
  su una scheda **Ray Regeneration** tutta loro (subito dopo Upscaling) e questa riga offre un pulsante **Open Ray
  Regeneration**; mentre non è in funzione, la riga dice perché (il gioco non ha attivato la Ray Reconstruction, il
  driver ha rifiutato il denoiser su questa scheda video, Ray Regeneration ha rinunciato a questo titolo e perché, o
  quando è stata eseguita l'ultima volta).
```

New:

```text
- **Ray Regeneration**: non è più in questa scheda dalla 0.3.5.3; i suoi controlli e il suo stato sono su una scheda **Ray Regeneration** tutta sua (subito dopo Upscaling). Qui solo la riga arancione sotto il runtime può dire "Ray Regeneration is off in this title" o "Ray Regeneration could not start on this GPU" (passa il mouse sopra per il motivo).
- **Screen-space GI** (novità della 0.3.5.3): **AMDNR Screen-space GI**, disattivato di default, con un "by 3zwr1" tenue dopo il nome; sotto, mentre è attivo: GI quality (Low / Medium / High / Ultra; Auto ne sceglie uno per la tua GPU finché non ne premi uno, e la sua etichetta mostra il tempo GPU misurato, o una stima finché la GI non è partita), Bounce light, Ambient occlusion, Radius, Object thickness, Camera FOV, **More GI options** (Bounce colour, Sky light, Multi-bounce, FOV axis, Colour encoding, Debug view) e Reset GI. Su ogni GPU AMD, con o senza runtime NR; dentro AMDNR Anywhere è in grigio con il motivo.
```

**Round 2 - GI layout: the tools row (Experimental)** - draft lines 559-560 -> final line 554. Old:

```text
  Regeneration dalla 0.3.5), **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy) ed
  **Experimental** (AMDNR Screen-space GI, in preview).
```

New:

```text
  Regeneration dalla 0.3.5) e **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy); **Experimental** compare solo finché il tuo ini tiene attivo lo Screen-space GI ereditato e ritirato (`[AmdRtgi] Enabled`), con la sola casella, etichettata "retired".
```

**Round 1 - feature bullet, AMDNR Screen-space GI by 3zwr1** - draft line 670 -> final line 664. Old:

```text
- **AMDNR Screen GI** (preview, nuovo in 0.3.4, disattivato di default; Neural > Experimental, o `[AmdGi] Enabled=true`) — la luce rimbalzata e l'occlusione ambientale in screen space di AMDNR, dalla profondità del gioco, prima di NR e dell'upscaler; funziona con NR attivo o spento; circa 1 ms in High con un render 1080p su una RX 9070 XT (misurato fuori da un gioco). È screen space: manca la luce che arriva da fuori schermo. Vedi `CHANGELOG.md`.
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** (disattivato di default; Neural > Screen-space GI, o `[AmdGi] Enabled=true`) - la luce rimbalzata e l'occlusione ambientale in screen space di AMDNR, dalla profondità del gioco, prima di NR, dell'upscaler e dell'interfaccia; funziona con NR attivo o spento; Low / Medium / High / Ultra (Auto per GPU), circa 0.6 ms in High con un render 1080p su una RX 9070 XT (misurato). È screen space: manca la luce che arriva da fuori schermo. Vedi `CHANGELOG.md`.
```

**Round 2 - Magpie: credits line** - draft line 972 -> final line 966. Old:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR** — l'host di cattura della finestra dentro cui gira AMDNR Anywhere — https://github.com/Blinue/Magpie (il fork: https://github.com/SAOG0721/Magpie)
```

New:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** — l'host di cattura della finestra dentro cui gira AMDNR Anywhere, nella build propria di AMDNR pubblicata sulle release GitHub di AMDNR (`Magpie.exe` ricompilato con quattro correzioni; le patch, il commit di base e le opzioni di build accanto, in `Anywhere-host-source/`; nessun file NVIDIA) — https://github.com/Blinue/Magpie (il fork: https://github.com/SAOG0721/Magpie)
```

**Round 1 - credits line, AMDNR Screen-space GI by 3zwr1** - draft line 977 -> final line 971. Old:

```text
- **AMDNR Screen GI** (preview 0.3.4) — lavoro proprio di AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), scritto a partire da articoli pubblicati (Therrien, Levesque e Gilet 2023; Jimenez et al. 2016; Schied et al. 2017; e gli altri elencati in `CHANGELOG.md` e `Licenses/AMDNR_NOTICE.txt`)
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** - lavoro proprio di AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), scritto a partire da ricerche pubblicate (elencate in `CHANGELOG.md` e `Licenses/AMDNR_NOTICE.txt`).
```

**Round 2 - Magpie: AMDNR Launcher, PLAY ANYWHERE line** - draft line 993 -> final line 987. Old:

```text
  cattura dalla release del suo autore, avvia il gioco ed esegue il Neural Rendering sulla sua finestra, con un
```

New:

```text
  cattura dalla release GitHub di AMDNR, avvia il gioco ed esegue il Neural Rendering sulla sua finestra, con un
```

**Terminology, round 1:** "disattivato di default", "screen space", "upscaler", "in High" as in the old spot. "per GPU" = "per GPU" (the file: "i default per GPU"). "the UI" = "l'interfaccia" ("prima di NR, dell'upscaler e dell'interfaccia"). "(misurato)" (was "misurato fuori da un gioco"). Credits: "ricerche pubblicate (elencate in ...)", from the old "gli altri elencati in".

**Terminology, round 2:** "la build propria di AMDNR" (feminine, as the file), "ricompilato", "correzioni", "patch" (invariable), "opzioni di build", "la release GitHub di AMDNR". GI line: "mentre è attivo", "tenue", "etichetta", "in grigio", "novità della 0.3.5.3" (the list's own words for new), "il tuo ini", "casella"; "ed" before Experimental became "e" before Runtime options.

**Terminology, round 3:** the menu's own strings stay English and quoted; the tab's place and the orange line use this file's existing words for them (its old Ray Regeneration bullet and its Neural runtime bullet).

### README.pl.md (Polish)

**Round 2 - Magpie: AMDNR Anywhere, "What it is"** - draft lines 328-329 -> final line 328. Old:

```text
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR**:
AMDNR Launcher pobiera go z wydania jego autora (467 MB, jednorazowo).
```

New:

```text
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** we własnym buildzie AMDNR: jego `Magpie.exe` jest przebudowany z czterema poprawkami (łatki, bazowy commit i opcje kompilacji są obok, w `Anywhere-host-source/`), a pakiet nie zawiera żadnych plików NVIDIA. AMDNR Launcher pobiera go z wydania AMDNR na GitHubie (około 100 MB, jednorazowo).
```

**Round 2 - Magpie: AMDNR Anywhere, "Status: preview"** - draft lines 357-358 -> final line 356. Old:

```text
fps, zostaw Model interleave wyłączone - host go odrzuca). Host przechwytujący jest pobierany z wydania jego autora na
GitHubie, nie z naszego. Zgłoszenia: zip z **Save report** z menu wewnątrz hosta (jego tytuł podaje skalowaną grę)
```

New:

```text
fps, zostaw Model interleave wyłączone - host go odrzuca). Host przechwytujący to własny build AMDNR, pobierany z wydania AMDNR na GitHubie. Zgłoszenia: zip z **Save report** z menu wewnątrz hosta (jego tytuł podaje skalowaną grę)
```

**Round 3 + Round 2 - Neural-tab list: the Ray Regeneration line (round 3) and the new Screen-space GI line under it (round 2)** - draft lines 546-549 -> final lines 544-545. Old:

```text
- **Ray Regeneration**: od 0.3.5 odsyłacz. Gdy Ray Regeneration działa w danym tytule, jego ustawienia są na
  własnej zakładce **Ray Regeneration** (zaraz po Upscaling), a ta linia oferuje przycisk **Open Ray Regeneration**;
  gdy nie działa, linia mówi dlaczego (gra nie włączyła Ray Reconstruction, sterownik odrzucił denoiser na tej karcie,
  Ray Regeneration zrezygnowało z tej gry i dlaczego, albo kiedy działało ostatni raz).
```

New:

```text
- **Ray Regeneration**: od 0.3.5.3 nie ma go na tej zakładce; jego ustawienia i stan są na własnej zakładce **Ray Regeneration** (zaraz po Upscaling). Tutaj tylko pomarańczowa linia pod runtime'em może napisać "Ray Regeneration is off in this title" albo "Ray Regeneration could not start on this GPU" (najedź na nią, żeby zobaczyć powód).
- **Screen-space GI** (nowość w 0.3.5.3): **AMDNR Screen-space GI**, domyślnie wyłączone, z przygaszonym "by 3zwr1" po nazwie; pod nim, gdy jest włączone: GI quality (Low / Medium / High / Ultra; Auto wybiera jeden dla Twojego GPU, dopóki sam nie naciśniesz któregoś, a jego etykieta pokazuje zmierzony czas GPU albo szacunek, zanim GI zadziała), Bounce light, Ambient occlusion, Radius, Object thickness, Camera FOV, **More GI options** (Bounce colour, Sky light, Multi-bounce, FOV axis, Colour encoding, Debug view) i Reset GI. Na każdym GPU AMD, z runtime'em NR lub bez; wewnątrz AMDNR Anywhere jest wyszarzone z podaniem powodu.
```

**Round 2 - GI layout: the tools row (Experimental)** - draft lines 552-553 -> final line 548. Old:

```text
  Regeneration), **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy) i **Experimental** (AMDNR
  Screen-space GI, w wersji preview).
```

New:

```text
  Regeneration) i **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy); **Experimental** pojawia się tylko wtedy, gdy Twój ini ma włączony odziedziczony efekt Screen-space GI, wycofany z menu (`[AmdRtgi] Enabled`), i zawiera tylko jego pole z etykietą "retired".
```

**Round 1 - feature bullet, AMDNR Screen-space GI by 3zwr1** - draft line 659 -> final line 654. Old:

```text
- **AMDNR Screen GI** (preview, nowość w 0.3.4, domyślnie wyłączone; Neural > Experimental albo `[AmdGi] Enabled=true`) — własne światło odbite i okluzja otoczenia w przestrzeni ekranu od AMDNR, z głębi gry, przed NR i upscalerem; działa z włączonym i wyłączonym NR; około 1 ms na High przy renderze 1080p na RX 9070 XT (zmierzone poza grą). To przestrzeń ekranu: brakuje światła spoza ekranu. Patrz `CHANGELOG.md`.
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** (domyślnie wyłączone; Neural > Screen-space GI albo `[AmdGi] Enabled=true`) — własne światło odbite i okluzja otoczenia w przestrzeni ekranu od AMDNR, z głębi gry, przed NR, upscalerem i interfejsem; działa z włączonym i wyłączonym NR; Low / Medium / High / Ultra (Auto według GPU), około 0.6 ms na High przy renderze 1080p na RX 9070 XT (zmierzone). To przestrzeń ekranu: brakuje światła spoza ekranu. Patrz `CHANGELOG.md`.
```

**Round 2 - Magpie: credits line** - draft line 955 -> final line 950. Old:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR** — host przechwytujący okno, wewnątrz którego działa AMDNR Anywhere — https://github.com/Blinue/Magpie (fork: https://github.com/SAOG0721/Magpie)
```

New:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** — host przechwytujący okno, wewnątrz którego działa AMDNR Anywhere, we własnym buildzie AMDNR publikowanym w wydaniach AMDNR na GitHubie (`Magpie.exe` przebudowany z czterema poprawkami; łatki, bazowy commit i opcje kompilacji obok, w `Anywhere-host-source/`; bez plików NVIDIA) — https://github.com/Blinue/Magpie (fork: https://github.com/SAOG0721/Magpie)
```

**Round 1 - credits line, AMDNR Screen-space GI by 3zwr1** - draft line 960 -> final line 955. Old:

```text
- **AMDNR Screen GI** (preview 0.3.4) — własna praca AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), napisana na podstawie opublikowanych prac (Therrien, Levesque i Gilet 2023; Jimenez i in. 2016; Schied i in. 2017; oraz pozostałe wymienione w `CHANGELOG.md` i `Licenses/AMDNR_NOTICE.txt`)
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** — własna praca AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), napisana na podstawie opublikowanych badań (wymienionych w `CHANGELOG.md` i `Licenses/AMDNR_NOTICE.txt`).
```

**Round 2 - Magpie: AMDNR Launcher, PLAY ANYWHERE line** - draft line 977 -> final line 972. Old:

```text
  z wydania jego autora, uruchamia grę i uruchamia Neural Rendering na jej oknie, z podsumowaniem ustawień hosta
```

New:

```text
  z wydania AMDNR na GitHubie, uruchamia grę i uruchamia Neural Rendering na jej oknie, z podsumowaniem ustawień hosta
```

**Terminology, round 1:** "domyślnie wyłączone", "upscaler", "na High" as in the old spot; no comma before "albo", as the old spot. "the UI" = "interfejs", in the instrumental after "przed" ("przed NR, upscalerem i interfejsem"). "per GPU" = "według GPU" (the file: "ustawienia domyślne według GPU"). "(zmierzone)" (was "zmierzone poza grą"). Credits: "opublikowanych badań (wymienionych w ...)", from the old "pozostałe wymienione w". Separator " — ", as this file did with the line-268 lmxxf 0.37 bullet.

**Terminology, round 2:** "własny build AMDNR" (masculine, as the file; locative "buildzie"), "przebudowany", "poprawki", "łatki" (the file's word for patches), "opcje kompilacji", "wydanie AMDNR na GitHubie" (the file says "wydanie" for a release). GI line: "gdy jest włączone", "przygaszone", "etykieta", "wyszarzone z podaniem powodu", "nowość w 0.3.5.3", "Twój ini", "pole".

**Terminology, round 3:** the menu's own strings stay English and quoted; the tab's place and the orange line use this file's existing words for them (its old Ray Regeneration bullet and its Neural runtime bullet).

### README.pt-BR.md (Portuguese (Brazil))

**Round 2 - Magpie: AMDNR Anywhere, "What it is"** - draft lines 325-326 -> final line 325. Old:

```text
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR**: o AMDNR
Launcher o baixa da release do autor dele (467 MB, uma única vez).
```

New:

```text
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)**, numa build própria do AMDNR: o `Magpie.exe` dela é recompilado com quatro correções (os patches, o commit base e as opções de build ficam ao lado, em `Anywhere-host-source/`) e o pacote não traz nenhum arquivo da NVIDIA. O AMDNR Launcher o baixa da release do AMDNR no GitHub (cerca de 100 MB, uma única vez).
```

**Round 2 - Magpie: AMDNR Anywhere, "Status: preview"** - draft lines 355-356 -> final line 354. Old:

```text
deixe o Model interleave desligado - o host o recusa). O host de captura é baixado da release do autor dele no
GitHub, não da nossa. Relatos: o zip do **Save report** do menu dentro do host (o título dele cita o jogo escalado),
```

New:

```text
deixe o Model interleave desligado - o host o recusa). O host de captura é uma build própria do AMDNR, baixada da release do AMDNR no GitHub. Relatos: o zip do **Save report** do menu dentro do host (o título dele cita o jogo escalado),
```

**Round 3 + Round 2 - Neural-tab list: the Ray Regeneration line (round 3) and the new Screen-space GI line under it (round 2)** - draft lines 548-552 -> final lines 546-547. Old:

```text
- **Ray Regeneration**: desde o 0.3.5, um ponteiro. Enquanto o Ray Regeneration roda no título, os controles dele
  ficam na aba **Ray Regeneration** própria (logo depois de Upscaling) e esta linha oferece um botão **Open Ray
  Regeneration**; enquanto ele não está rodando, a linha diz por que (o jogo não ligou o Ray Reconstruction, o driver
  recusou o denoiser nesta placa, o Ray Regeneration desistiu deste título e por que, ou quando ele rodou pela última
  vez).
```

New:

```text
- **Ray Regeneration**: não fica mais nesta aba desde o 0.3.5.3; os controles e o status dele ficam na aba **Ray Regeneration** própria (logo depois de Upscaling). Aqui só a linha laranja abaixo do runtime pode dizer "Ray Regeneration is off in this title" ou "Ray Regeneration could not start on this GPU" (passe o mouse nela para ver o motivo).
- **Screen-space GI** (seção nova no 0.3.5.3): **AMDNR Screen-space GI**, desligado por padrão, com um "by 3zwr1" discreto depois do nome; abaixo dele, enquanto está ligado: GI quality (Low / Medium / High / Ultra; o Auto escolhe um para a sua GPU até você apertar um, e a etiqueta dele mostra o tempo de GPU medido, ou uma estimativa até o GI rodar), Bounce light, Ambient occlusion, Radius, Object thickness, Camera FOV, **More GI options** (Bounce colour, Sky light, Multi-bounce, FOV axis, Colour encoding, Debug view) e Reset GI. Em toda GPU AMD, com ou sem runtime de NR; dentro do AMDNR Anywhere aparece em cinza com o motivo.
```

**Round 2 - GI layout: the tools row (Experimental)** - draft lines 555-556 -> final line 550. Old:

```text
  na aba Ray Regeneration desde o 0.3.5), **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy) e
  **Experimental** (AMDNR Screen-space GI, em preview).
```

New:

```text
  na aba Ray Regeneration desde o 0.3.5) e **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy); **Experimental** só aparece enquanto o seu ini tiver ligado o Screen-space GI herdado e retirado (`[AmdRtgi] Enabled`), e só com a caixa dele, marcada "retired".
```

**Round 1 - feature bullet, AMDNR Screen-space GI by 3zwr1** - draft line 664 -> final line 658. Old:

```text
- **AMDNR Screen GI** (preview, novo no 0.3.4, desligado por padrão; Neural > Experimental, ou `[AmdGi] Enabled=true`) — a luz rebatida e a oclusão ambiente em espaço de tela do próprio AMDNR, a partir da profundidade do jogo, antes do NR e do upscaler; funciona com o NR ligado ou desligado; cerca de 1 ms em High com render 1080p numa RX 9070 XT (medido fora de um jogo). É espaço de tela: falta a luz que vem de fora da tela. Veja `CHANGELOG.md`.
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** (desligado por padrão; Neural > Screen-space GI, ou `[AmdGi] Enabled=true`) - a luz rebatida e a oclusão ambiente em espaço de tela do próprio AMDNR, a partir da profundidade do jogo, antes do NR, do upscaler e da interface; funciona com o NR ligado ou desligado; Low / Medium / High / Ultra (Auto por GPU), cerca de 0.6 ms em High com render 1080p numa RX 9070 XT (medido). É espaço de tela: falta a luz que vem de fora da tela. Veja `CHANGELOG.md`.
```

**Round 2 - Magpie: credits line** - draft line 968 -> final line 962. Old:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR** — o host de captura de janela dentro do qual o AMDNR Anywhere roda — https://github.com/Blinue/Magpie (o fork: https://github.com/SAOG0721/Magpie)
```

New:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** — o host de captura de janela dentro do qual o AMDNR Anywhere roda, numa build própria do AMDNR publicada nas releases do AMDNR no GitHub (`Magpie.exe` recompilado com quatro correções; os patches, o commit base e as opções de build ao lado, em `Anywhere-host-source/`; nenhum arquivo da NVIDIA) — https://github.com/Blinue/Magpie (o fork: https://github.com/SAOG0721/Magpie)
```

**Round 1 - credits line, AMDNR Screen-space GI by 3zwr1** - draft line 973 -> final line 967. Old:

```text
- **AMDNR Screen GI** (preview do 0.3.4) — trabalho próprio do AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), escrito a partir de artigos publicados (Therrien, Levesque e Gilet 2023; Jimenez et al. 2016; Schied et al. 2017; e os outros listados em `CHANGELOG.md` e `Licenses/AMDNR_NOTICE.txt`)
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** - trabalho próprio do AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), escrito a partir de pesquisas publicadas (listadas em `CHANGELOG.md` e `Licenses/AMDNR_NOTICE.txt`).
```

**Round 2 - Magpie: AMDNR Launcher, PLAY ANYWHERE line** - draft line 989 -> final line 983. Old:

```text
  release do autor dele, inicia o jogo e roda o Neural Rendering na janela dele, com um resumo somente leitura das
```

New:

```text
  release do AMDNR no GitHub, inicia o jogo e roda o Neural Rendering na janela dele, com um resumo somente leitura das
```

**Terminology, round 1:** "desligado por padrão", "o NR", "upscaler", "em High" as in the old spot. "per GPU" = "por GPU" (the file: "os padrões por GPU"). "the UI" = "a interface" ("antes do NR, do upscaler e da interface"). "(medido)" (was "medido fora de um jogo"). Credits: "pesquisas publicadas (listadas em ...)", from the old "os outros listados em".

**Terminology, round 2:** "uma build própria do AMDNR" (feminine, as the file), "recompilado", "correções", "patches" (the file says "patch"), "opções de build", "a release do AMDNR no GitHub". GI line: "enquanto está ligado", "discreto", "etiqueta", "aparece em cinza", "seção nova no 0.3.5.3", "o seu ini", "caixa".

**Terminology, round 3:** the menu's own strings stay English and quoted; the tab's place and the orange line use this file's existing words for them (its old Ray Regeneration bullet and its Neural runtime bullet).

### README.ru.md (Russian)

**Round 2 - Magpie: AMDNR Anywhere, "What it is"** - draft lines 323-324 -> final line 323. Old:

```text
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR**:
AMDNR Launcher скачивает его из релиза автора (467 МБ, один раз).
```

New:

```text
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** в собственной сборке AMDNR: его `Magpie.exe` пересобран с четырьмя исправлениями (патчи, базовый коммит и параметры сборки лежат рядом, в `Anywhere-host-source/`), а в пакете нет файлов NVIDIA. AMDNR Launcher скачивает его из релиза AMDNR на GitHub (около 100 МБ, один раз).
```

**Round 2 - Magpie: AMDNR Anywhere, "Status: preview"** - draft lines 353-354 -> final line 352. Old:

```text
Model interleave выключенным — хост его отклоняет). Хост захвата скачивается из релиза его автора на GitHub, а не из
нашего. Отчёты: zip **Save report** из меню внутри хоста (его заголовок называет масштабируемую игру) или COLLECT LOGS
```

New:

```text
Model interleave выключенным — хост его отклоняет). Хост захвата — собственная сборка AMDNR, он скачивается из релиза AMDNR на GitHub. Отчёты: zip **Save report** из меню внутри хоста (его заголовок называет масштабируемую игру) или COLLECT LOGS
```

**Round 3 + Round 2 - Neural-tab list: the Ray Regeneration line (round 3) and the new Screen-space GI line under it (round 2)** - draft lines 544-548 -> final lines 542-543. Old:

```text
- **Ray Regeneration**: с 0.3.5 — указатель. Пока в игре работает Ray Regeneration, его настройки находятся на
  собственной вкладке **Ray Regeneration** (сразу после Upscaling), а эта строка предлагает кнопку **Open Ray
  Regeneration**; пока он не работает, строка говорит почему (игра не включила Ray Reconstruction, драйвер отказал в
  денойзере на этой видеокарте, Ray Regeneration отказался от этой игры и почему, или когда он работал в последний
  раз).
```

New:

```text
- **Ray Regeneration**: с 0.3.5.3 его нет на этой вкладке; его настройки и статус — на собственной вкладке **Ray Regeneration** (сразу после Upscaling). Здесь только оранжевая строка под рантаймом может сказать "Ray Regeneration is off in this title" или "Ray Regeneration could not start on this GPU" (наведите на неё курсор, чтобы увидеть причину).
- **Screen-space GI** (новое в 0.3.5.3): **AMDNR Screen-space GI**, по умолчанию выключено, с бледным "by 3zwr1" после названия; под ним, пока оно включено: GI quality (Low / Medium / High / Ultra; Auto выбирает один из них для вашей видеокарты, пока вы не нажмёте кнопку сами, а пометка рядом показывает измеренное время GPU, до первого запуска GI — оценку), Bounce light, Ambient occlusion, Radius, Object thickness, Camera FOV, **More GI options** (Bounce colour, Sky light, Multi-bounce, FOV axis, Colour encoding, Debug view) и Reset GI. На любой видеокарте AMD, с рантаймом NR и без него; внутри AMDNR Anywhere показано серым с причиной.
```

**Round 2 - GI layout: the tools row (Experimental)** - draft lines 551-552 -> final line 546. Old:

```text
  вкладке Ray Regeneration), **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy) и
  **Experimental** (AMDNR Screen-space GI, preview).
```

New:

```text
  вкладке Ray Regeneration) и **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy); **Experimental** появляется, только пока в вашем ini включён унаследованный эффект Screen-space GI, убранный из меню (`[AmdRtgi] Enabled`), и содержит лишь его галочку с пометкой "retired".
```

**Round 1 - feature bullet, AMDNR Screen-space GI by 3zwr1** - draft line 659 -> final line 653. Old:

```text
- **AMDNR Screen GI** (preview, новое в 0.3.4, по умолчанию выключено; Neural > Experimental или `[AmdGi] Enabled=true`) — собственный отражённый свет и затенение окружения (ambient occlusion) в экранном пространстве от AMDNR, по глубине игры, до NR и апскейлера; работает с включённым и выключенным NR; около 1 мс на High при рендере 1080p на RX 9070 XT (измерено вне игры). Это экранное пространство: света из-за пределов экрана нет. См. `CHANGELOG.md`.
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** (по умолчанию выключено; Neural > Screen-space GI или `[AmdGi] Enabled=true`) — собственный отражённый свет и затенение окружения (ambient occlusion) в экранном пространстве от AMDNR, по глубине игры, до NR, апскейлера и интерфейса; работает с включённым и выключенным NR; Low / Medium / High / Ultra (Auto по видеокарте), около 0.6 мс на High при рендере 1080p на RX 9070 XT (измерено). Это экранное пространство: света из-за пределов экрана нет. См. `CHANGELOG.md`.
```

**Round 2 - Magpie: credits line** - draft line 962 -> final line 956. Old:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR** — хост захвата окна, внутри которого работает AMDNR Anywhere — https://github.com/Blinue/Magpie (форк: https://github.com/SAOG0721/Magpie)
```

New:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** — хост захвата окна, внутри которого работает AMDNR Anywhere, в собственной сборке AMDNR из релизов AMDNR на GitHub (`Magpie.exe` пересобран с четырьмя исправлениями; патчи, базовый коммит и параметры сборки рядом, в `Anywhere-host-source/`; без файлов NVIDIA) — https://github.com/Blinue/Magpie (форк: https://github.com/SAOG0721/Magpie)
```

**Round 1 - credits line, AMDNR Screen-space GI by 3zwr1** - draft line 967 -> final line 961. Old:

```text
- **AMDNR Screen GI** (preview 0.3.4) — собственная работа AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), написана по опубликованным статьям (Therrien, Levesque и Gilet 2023; Jimenez и др. 2016; Schied и др. 2017; и другие, перечисленные в `CHANGELOG.md` и `Licenses/AMDNR_NOTICE.txt`)
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** — собственная работа AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), написана по опубликованным исследованиям (перечислены в `CHANGELOG.md` и `Licenses/AMDNR_NOTICE.txt`).
```

**Round 2 - Magpie: AMDNR Launcher, PLAY ANYWHERE line** - draft line 983 -> final line 977. Old:

```text
  релиза его автора, запускает игру и выполняет Neural Rendering на её окне; рядом с кнопкой — сводка настроек хоста
```

New:

```text
  релиза AMDNR на GitHub, запускает игру и выполняет Neural Rendering на её окне; рядом с кнопкой — сводка настроек хоста
```

**Terminology, round 1:** "по умолчанию выключено", "апскейлер", "на High", "мс" as in the old spot; no comma before "или", as the old spot. "the UI" = "интерфейс" ("до NR, апскейлера и интерфейса"). "per GPU" = "по видеокарте": in running text this file says "видеокарта" for the GPU (e.g. "выбор по видеокарте" for per-GPU gating). "(измерено)" (was "измерено вне игры"). Credits: "по опубликованным исследованиям (перечислены в ...)", from the old "перечисленные в". Separator " — ", as this file did with the line-265 lmxxf 0.37 bullet.

**Terminology, round 2:** "собственная сборка AMDNR" (the file says "сборка" for a build), "пересобран", "исправления", "патчи", "параметры сборки", "релиз AMDNR на GitHub", "100 МБ". GI line: "пока оно включено", "бледное", "пометка", "показано серым с причиной", "новое в 0.3.5.3", "ваш ini", "галочка", "видеокарта" for the GPU in running text.

**Terminology, round 3:** the menu's own strings stay English and quoted; the tab's place and the orange line use this file's existing words for them (its old Ray Regeneration bullet and its Neural runtime bullet).

### README.zh-CN.md (Chinese (Simplified))

**Round 2 - Magpie: AMDNR Anywhere, "What it is"** - draft lines 283-284 -> final line 283. Old:

```text
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR**：
AMDNR Launcher 从其作者的发布页下载它（467 MB，只需一次）。
```

New:

```text
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)**，使用 AMDNR 自己的构建版：其 `Magpie.exe` 经重新构建，带四项修复（补丁、基础提交和构建选项都在旁边的 `Anywhere-host-source/` 中），且该包不含任何 NVIDIA 文件。AMDNR Launcher 从 AMDNR 的 GitHub 发布页下载它（约 100 MB，只需一次）。
```

**Round 2 - Magpie: AMDNR Anywhere, "Status: preview"** - draft lines 305-306 -> final line 304. Old:

```text
（把 NR 档位降到 720、把游戏限制在 60-90 fps、保持 Model interleave 关闭——宿主会拒绝它）。捕获宿主从其作者的 GitHub
发布页获取，而不是从我们的发布页。反馈：在宿主内的菜单中生成的 **Save report** zip（其标题写的是被缩放的游戏），或启动器的
```

New:

```text
（把 NR 档位降到 720、把游戏限制在 60-90 fps、保持 Model interleave 关闭——宿主会拒绝它）。捕获宿主是 AMDNR 自己的构建版，从 AMDNR 的 GitHub 发布页获取。反馈：在宿主内的菜单中生成的 **Save report** zip（其标题写的是被缩放的游戏），或启动器的
```

**Round 3 + Round 2 - Neural-tab list: the Ray Regeneration line (round 3) and the new Screen-space GI line under it (round 2)** - draft lines 479-482 -> final lines 477-478. Old:

```text
- **Ray Regeneration**：自 0.3.5 起是一行指引。当 Ray Regeneration 在游戏中运行时，它的控件位于其独立的 **Ray
  Regeneration** 选项卡上（紧跟在 Upscaling 之后），这一行会提供一个 **Open Ray Regeneration** 按钮；它没有运行时，
  这一行会说明原因（游戏没有开启光线重建、驱动在这块显卡上拒绝了降噪器、Ray Regeneration 放弃了这款游戏以及原因，
  或它上一次运行的时间）。
```

New:

```text
- **Ray Regeneration**：自 0.3.5.3 起不在本选项卡上；它的控件和状态位于其独立的 **Ray Regeneration** 选项卡上（紧跟在 Upscaling 之后）。这里只有运行时下方的橙色提示行可能显示 "Ray Regeneration is off in this title" 或 "Ray Regeneration could not start on this GPU"（把鼠标悬停在上面可查看原因）。
- **Screen-space GI**（0.3.5.3 新增）：**AMDNR Screen-space GI**，默认关闭，后面有淡色的 "by 3zwr1"；开启时其下方显示：GI quality（Low / Medium / High / Ultra；在你按下其中一个之前，Auto 会为你的显卡选一个；其标签显示实测的 GPU 时间，GI 运行之前显示估算值）、Bounce light、Ambient occlusion、Radius、Object thickness、Camera FOV、**More GI options**（Bounce colour、Sky light、Multi-bounce、FOV axis、Colour encoding、Debug view）和 Reset GI。所有 AMD 显卡上都有，无论是否安装了 NR 运行时；在 AMDNR Anywhere 内会变灰并注明原因。
```

**Round 2 - GI layout: the tools row (Experimental)** - draft lines 484-485 -> final line 480. Old:

```text
  与自调读数、GPU 行、**Save report**；自 0.3.5 起 RR 调试视图位于 Ray Regeneration 选项卡上）、**Runtime options**（Encoding、Every-frame NR、NR slots、Highlight proxy）和
  **Experimental**（AMDNR Screen-space GI，preview）。
```

New:

```text
  与自调读数、GPU 行、**Save report**；自 0.3.5 起 RR 调试视图位于 Ray Regeneration 选项卡上）和 **Runtime options**（Encoding、Every-frame NR、NR slots、Highlight proxy）；只有当你的 ini 开启了继承的、已从菜单中移除的 Screen-space GI（`[AmdRtgi] Enabled`）时才会出现 **Experimental**，其中只有它的复选框，带 "retired" 标签。
```

**Round 1 - feature bullet, AMDNR Screen-space GI by 3zwr1** - draft line 569 -> final line 564. Old:

```text
- **AMDNR Screen GI**（preview，0.3.4 新增，默认关闭；Neural > Experimental，或 `[AmdGi] Enabled=true`）—— AMDNR 自己的屏幕空间反弹光和环境光遮蔽，基于游戏的深度，在 NR 和放大器之前运行；NR 开或关都能用；在 RX 9070 XT 上 1080p 渲染、High 档约 1 ms（在游戏外测得）。它是屏幕空间效果：来自屏幕外的光会缺失。见 `CHANGELOG.md`。
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1**（默认关闭；Neural > Screen-space GI，或 `[AmdGi] Enabled=true`）—— AMDNR 自己的屏幕空间反弹光和环境光遮蔽，基于游戏的深度，在 NR、放大器和 UI 之前运行；NR 开或关都能用；Low / Medium / High / Ultra（Auto 按显卡选择），在 RX 9070 XT 上 1080p 渲染、High 档约 0.6 ms（实测）。它是屏幕空间效果：来自屏幕外的光会缺失。见 `CHANGELOG.md`。
```

**Round 2 - Magpie: credits line** - draft line 808 -> final line 803. Old:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR** —— AMDNR Anywhere 在其中运行的窗口捕获宿主 —— https://github.com/Blinue/Magpie （该分支：<https://github.com/SAOG0721/Magpie>）
```

New:

```text
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** —— AMDNR Anywhere 在其中运行的窗口捕获宿主，使用 AMDNR 自己的构建版，发布在 AMDNR 的 GitHub 发布页上（`Magpie.exe` 经重新构建，带四项修复；补丁、基础提交和构建选项在旁边的 `Anywhere-host-source/` 中；不含 NVIDIA 文件）—— https://github.com/Blinue/Magpie （该分支：<https://github.com/SAOG0721/Magpie>）
```

**Round 1 - credits line, AMDNR Screen-space GI by 3zwr1** - draft line 813 -> final line 808. Old:

```text
- **AMDNR Screen GI**（0.3.4 preview）—— AMDNR 自己的作品，Copyright (c) 2026 3zwr1 (AMDNR)，依据已发表的论文编写（Therrien、Levesque 和 Gilet 2023；Jimenez 等 2016；Schied 等 2017；其余见 `CHANGELOG.md` 和 `Licenses/AMDNR_NOTICE.txt`）
```

New:

```text
- **AMDNR Screen-space GI by 3zwr1** —— AMDNR 自己的作品，Copyright (c) 2026 3zwr1 (AMDNR)，依据已发表的研究编写（列于 `CHANGELOG.md` 和 `Licenses/AMDNR_NOTICE.txt`）。
```

**Round 2 - Magpie: AMDNR Launcher, PLAY ANYWHERE line** - draft line 828 -> final line 823. Old:

```text
- 在自身没有放大器的游戏上使用 **PLAY ANYWHERE**（见"AMDNR Anywhere"）：启动器从作者的发布页获取捕获宿主，启动游戏，
```

New:

```text
- 在自身没有放大器的游戏上使用 **PLAY ANYWHERE**（见"AMDNR Anywhere"）：启动器从 AMDNR 的 GitHub 发布页获取捕获宿主，启动游戏，
```

**Terminology, round 1:** Full-width punctuation as in the file. "默认关闭", "放大器" for the upscaler and "High 档" as in the old spot. "the UI" = "UI" (the usual word in Chinese game text). "per GPU" = "按显卡选择" (the file: "按显卡的默认设置"; it says "显卡" for the GPU in running text). "(measured)" = "（实测）" (the file uses "实测" and "测得"; the old spot had "（在游戏外测得）"). Credits: "依据已发表的研究编写（列于 ...）。" ("论文", papers, became "研究", research). Dash spacing as the file: "）—— " in the feature list, " —— " in the credits list.

**Terminology, round 2:** "AMDNR 自己的构建版" (the file's word for a build), "重新构建", "修复", "补丁", "构建选项", "基础提交" for the base commit, "AMDNR 的 GitHub 发布页" (the file says "发布页" for a release page). GI line: "开启时", "淡色", "标签", "变灰并注明原因", "0.3.5.3 新增", "你的 ini", "复选框"; full-width punctuation as the file.

**Terminology, round 3:** the menu's own strings stay English and quoted; the tab's place and the orange line use this file's existing words for them (its old Ray Regeneration bullet and its Neural runtime bullet).


## 4b. Round 4: Ray Regeneration on RX 7000 / RX 6000 and the Denoiser backend row (round-3 lines -> final lines)

What is true in 0.3.5.3, read from the r035 code and the 0.3.5.3 CHANGELOG (`E:\amdnr-scratch\rel0353-final\github\CHANGELOG.md`,
"The AMDNR Ray Denoiser (preview)"):
- `Config.h` around 1705 and `shaders\fsrd_preprocess\FSRDArdRules.h` Serving: `[FSR-RR] RrBackend` = auto / amd /
  amdnr / off; Automatic is AMD's FSR Ray Regeneration on RDNA 4 and the AMDNR Ray Denoiser on every other card;
  switching among the first three applies while the game runs, only Off (and back) waits for a restart.
- `dlssnr\menu\RrOffRules.h`: the row is "Denoiser backend" with Automatic / AMD FSR Ray Regeneration / AMDNR Ray
  Denoiser (preview) / Off (the game's own).
- `dlssnr\menu\RrArdMenuRules.h`: RX 7000 and the RDNA 3 / 3.5 APUs get Ray Regeneration as a preview answered by the
  AMDNR Ray Denoiser, `[FSR-RR] FfxDenoiserAllowPreRdna4=false` turns it off; RX 6000 and older: "not offered on this
  GPU by default", the Upscaling tab's box **Offer Ray Regeneration on this GPU (restart)** (tag "preview",
  `FfxDenoiserAllowPreRdna4=true`) offers it, answered by the AMDNR Ray Denoiser. The old label "Offer FSR Ray
  Regeneration on this GPU (restart)" is no longer drawn on those cards; the RX 7000 box keeps its label.
- The Neural tab has no Ray Regeneration line (round 3); only the orange line under the runtime.

8 spots per file, each now one line: the "New in 0.3.5" note's "upscaled without a denoiser", the other tabs'
Denoiser backend row, the feature bullet's first line ("without a denoiser"), its Neural-tab pointer and
`RrBackend = auto | off`, its RX 7000 "no provider" sentence, its "AMDNR's own denoiser for RX 7000 is planned" and
RX 6000 box, the FAQ "You cannot find the Ray Regeneration settings?" (the how-to after it unchanged), and the grain
FAQ's "AMDNR's own denoiser is 0.3.6 work". The brief named lines 30, 603, 619-620 and 791-800; the other four said
the same false thing (the Neural-tab line, the two-choice row, "planned", "0.3.6 work").

### README.md (round 4)

**Round 4 - "New in 0.3.5" note: RX 7000 without a denoiser** - round-3 lines 29-30 -> final line 29. Old:

```text
> its status lines say per card and per API what runs and why not; on RX 7000 it is, since 0.3.5.1, offered again as a preview, on by default (AMD's
> denoiser has no RDNA 3 provider, so the picture is upscaled without a denoiser; the Upscaling tab's box or
```

New:

```text
> its status lines say per card and per API what runs and why not; on RX 7000 it is, since 0.3.5.1, offered again as a preview, on by default (since 0.3.5.3 the AMDNR Ray Denoiser (preview) denoises it there; the Upscaling tab's box or
```

**Round 4 - The other tabs: the Denoiser backend row** - round-3 lines 537-538 -> final line 536. Old:

```text
card and per API), the **Denoiser backend** row (Automatic / Off - Off tells the game Ray Reconstruction is
unsupported, so it keeps its own denoiser; after a restart), the controls, More Ray Regeneration options, and its
```

New:

```text
card and per API), the **Denoiser backend** row (Automatic / AMD FSR Ray Regeneration / AMDNR Ray Denoiser (preview) / Off (the game's own): Automatic is AMD's on RX 9000 and the AMDNR Ray Denoiser on every other card, the first three switch while the game runs, and Off tells the game Ray Reconstruction is unsupported, so it keeps its own denoiser, after a restart), the controls, More Ray Regeneration options, and its
```

**Round 4 - Feature bullet, first line: RX 7000 without a denoiser** - round-3 line 603 -> final line 601. Old:

```text
- **FSR Ray Regeneration** — RX 9000 (RDNA 4); on RX 7000 (RDNA 3) as a preview, on by default, without a denoiser (see below); only in games that use DLSS Ray Reconstruction (Cyberpunk 2077,
```

New:

```text
- **FSR Ray Regeneration** — RX 9000 (RDNA 4); on RX 7000 (RDNA 3) as a preview, on by default, denoised by the AMDNR Ray Denoiser (see below); only in games that use DLSS Ray Reconstruction (Cyberpunk 2077,
```

**Round 4 - Feature bullet: Neural-tab pointer and RrBackend** - round-3 lines 607-611 -> final line 605. Old:

```text
  Regeneration** tab, right after Upscaling, drawn while Ray Regeneration runs in the title; the Neural tab points
  at it and, while Ray Regeneration is not running, keeps the dim line that says why (the game has not turned Ray
  Reconstruction on, the driver refused the denoiser on this card, Ray Regeneration gave this title up and why, or
  when it last ran). The tab's **Denoiser backend** row (`[FSR-RR] RrBackend = auto | off`) can tell the game Ray
  Reconstruction is unsupported, so it keeps its own denoiser (at the next start of the game). On a **Vulkan**
```

New:

```text
  Regeneration** tab, right after Upscaling, drawn while Ray Regeneration runs in the title; since 0.3.5.3 the Neural tab has no Ray Regeneration line, only the orange line under the runtime when Ray Regeneration is off in a title or could not start on this GPU. The tab's **Denoiser backend** row (`[FSR-RR] RrBackend` = auto / amd / amdnr / off) offers Automatic (AMD FSR Ray Regeneration on RX 9000, the AMDNR Ray Denoiser (preview) on every other card), AMD FSR Ray Regeneration, AMDNR Ray Denoiser (preview) and Off (the game's own); the first three switch while the game runs, and Off tells the game Ray Reconstruction is unsupported, so it keeps its own denoiser (at the next start of the game). On a **Vulkan**
```

**Round 4 - Feature bullet: RX 7000 "no provider for RDNA 3"** - round-3 lines 619-621 -> final line 613. Old:

```text
  Ray Reconstruction on and the Ray Regeneration settings show, but AMD's denoiser has no provider for RDNA 3, so
  the denoiser refuses to start and the Ray Reconstruction picture is upscaled without a denoiser, which can look
  noisier than the game's own; the Ray Regeneration page says what runs. The Upscaling tab's box
```

New:

```text
  Ray Reconstruction on and the Ray Regeneration settings show, and the AMDNR Ray Denoiser (preview) denoises it, on the RDNA 3 / 3.5 APUs too (AMD's FSR Ray Regeneration is built for RDNA 4); the Ray Regeneration page says what runs. The Upscaling tab's box
```

**Round 4 - Feature bullet: "planned" and the RX 6000 box** - round-3 lines 623-625 -> final line 615. Old:

```text
  untick it, or set `[FSR-RR] FfxDenoiserAllowPreRdna4=false`, and the game keeps its own denoiser. AMDNR's own
  denoiser for RX 7000 is planned. RX 6000 and older get it only with
  `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (Upscaling tab: **Offer FSR Ray Regeneration on this GPU (restart)**). **Sharpening after RR** (0.3.4.1): when the game
```

New:

```text
  untick it, or set `[FSR-RR] FfxDenoiserAllowPreRdna4=false`, and the game keeps its own denoiser. RX 6000 and older get it only as an opt-in, `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (Upscaling tab: **Offer Ray Regeneration on this GPU (restart)**, tagged "preview"), also denoised by the AMDNR Ray Denoiser. **Sharpening after RR** (0.3.4.1): when the game
```

**Round 4 - FAQ "You cannot find the Ray Regeneration settings?"** - round-3 lines 793-800 -> final line 783. Old:

```text
the Neural tab's **Ray Regeneration** line then offers an **Open Ray Regeneration** button. While it is not running,
the tab is not drawn and that line in the Neural tab says why: the game has not turned Ray Reconstruction on, Ray
Regeneration gave this title up and why, or it last ran N seconds ago. On RX 7000 a further dim line adds that Ray
Regeneration is offered there as a preview: AMD's denoiser has no provider for RDNA 3, so once the game turns Ray
Reconstruction on its picture is upscaled without a denoiser, and `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (or the
Upscaling tab's box **Preview: Ray Regeneration on this card (restart)**, unticked) turns the preview off so the game
keeps its own denoiser. On RX 6000 and older it says that it is not offered on this GPU, that AMD ships it for RDNA 4, and that
`[FSR-RR] FfxDenoiserAllowPreRdna4=true` offers it anyway. To get it running, in the game's own graphics settings: pick **DLSS** as
```

New:

```text
since 0.3.5.3 the Neural tab has no Ray Regeneration line. While it is not running, the tab is not drawn; if Ray Regeneration is off in this title or could not start on this GPU, the orange line under the runtime on the Neural tab says so. On RX 7000 and the RDNA 3 / 3.5 APUs Ray Regeneration is offered as a preview, on by default, and the AMDNR Ray Denoiser (preview) denoises it (AMD's FSR Ray Regeneration is built for RDNA 4); `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (or the Upscaling tab's box **Preview: Ray Regeneration on this card (restart)**, unticked) turns the preview off so the game keeps its own denoiser. On RX 6000 and older it is not offered by default: the Upscaling tab's box **Offer Ray Regeneration on this GPU (restart)** (`[FSR-RR] FfxDenoiserAllowPreRdna4=true`) offers it, denoised by the AMDNR Ray Denoiser (preview). To get it running, in the game's own graphics settings: pick **DLSS** as
```

**Round 4 - Grain FAQ: "AMDNR's own denoiser is 0.3.6 work"** - round-3 line 820 -> final line 803. Old:

```text
Reconstruction is unsupported so it keeps its own denoiser (after a restart). AMDNR's own denoiser is 0.3.6 work.
```

New:

```text
Reconstruction is unsupported so it keeps its own denoiser (after a restart). Since 0.3.5.3 the same row also offers AMDNR's own denoiser, the AMDNR Ray Denoiser (preview), which Automatic uses on every card but RX 9000.
```

### README.ar.md (round 4)

**Round 4 - "New in 0.3.5" note: RX 7000 without a denoiser** - round-3 lines 33-34 -> final line 33. Old:

```text
> تُعرض من جديد منذ 0.3.5.1 كمعاينة (preview) مفعّلة افتراضيًا (مزيل التشويش من AMD لا يملك مزوّدًا لـ RDNA 3، فتُرفع دقة
> الصورة دون إزالة التشويش؛ وتوقفها خانة تبويب Upscaling أو `[FSR-RR] FfxDenoiserAllowPreRdna4=false` فتبقى اللعبة على
```

New:

```text
> تُعرض من جديد منذ 0.3.5.1 كمعاينة (preview) مفعّلة افتراضيًا (ومنذ 0.3.5.3 يزيل AMDNR Ray Denoiser (preview) التشويش هناك؛ وتوقفها خانة تبويب Upscaling أو `[FSR-RR] FfxDenoiserAllowPreRdna4=false` فتبقى اللعبة على
```

**Round 4 - The other tabs: the Denoiser backend row** - round-3 lines 546-547 -> final line 545. Old:

```text
واجهة برمجية)، وصف **Denoiser backend** (Automatic / Off - الخيار Off يخبر اللعبة أن Ray Reconstruction غير مدعوم،
فتبقى على مزيل التشويش الخاص بها؛ بعد إعادة التشغيل)، وعناصر التحكم، وMore Ray Regeneration options، وقسم Diagnostics
```

New:

```text
واجهة برمجية)، وصف **Denoiser backend** (Automatic / AMD FSR Ray Regeneration / AMDNR Ray Denoiser (preview) / Off (the game's own): الخيار Automatic هو مزيل AMD على RX 9000 وAMDNR Ray Denoiser على كل بطاقة أخرى، والخيارات الثلاثة الأولى تتبدّل أثناء اللعب، والخيار Off يخبر اللعبة أن Ray Reconstruction غير مدعوم، فتبقى على مزيل التشويش الخاص بها، بعد إعادة التشغيل)، وعناصر التحكم، وMore Ray Regeneration options، وقسم Diagnostics
```

**Round 4 - Feature bullet, first line: RX 7000 without a denoiser** - round-3 line 615 -> final line 613. Old:

```text
- **تقنية FSR Ray Regeneration** — متاحة على RX 9000 (RDNA 4)؛ أما على RX 7000 (RDNA 3) فهي معاينة (preview) مفعّلة افتراضيًا ودون مزيل تشويش (انظر أدناه)؛ وتعمل فقط في الألعاب التي تستخدم
```

New:

```text
- **تقنية FSR Ray Regeneration** — متاحة على RX 9000 (RDNA 4)؛ أما على RX 7000 (RDNA 3) فهي معاينة (preview) مفعّلة افتراضيًا ويزيل تشويشها AMDNR Ray Denoiser (انظر أدناه)؛ وتعمل فقط في الألعاب التي تستخدم
```

**Round 4 - Feature bullet: Neural-tab pointer and RrBackend** - round-3 lines 619-623 -> final line 617. Old:

```text
  Regeneration** المستقل، بعد Upscaling مباشرة، ويُرسم ما دامت Ray Regeneration تعمل في اللعبة؛ ويشير إليه تبويب
  Neural، وحين لا تعمل Ray Regeneration يُبقي السطر الباهت الذي يوضّح السبب (اللعبة لم تفعّل Ray Reconstruction، أو أن
  التعريف رفض مزيل التشويش على هذه البطاقة، أو أن Ray Regeneration تخلّت عن هذه اللعبة والسبب، أو متى عملت آخر مرة).
  ويستطيع صف **Denoiser backend** في التبويب (`[FSR-RR] RrBackend = auto | off`) أن يخبر اللعبة أن Ray Reconstruction
  غير مدعوم، فتبقى على مزيل التشويش الخاص بها (عند التشغيل التالي للعبة). وفي لعبة تعمل بـ **Vulkan** يكون Ray
```

New:

```text
  Regeneration** المستقل، بعد Upscaling مباشرة، ويُرسم ما دامت Ray Regeneration تعمل في اللعبة؛ ومنذ 0.3.5.3 لم يعد في تبويب Neural سطر لـ Ray Regeneration، بل السطر البرتقالي تحت بيئة التشغيل فقط حين تكون Ray Regeneration معطّلة في لعبة ما أو تعذّر بدؤها على هذه البطاقة. ويعرض صف **Denoiser backend** في التبويب (`[FSR-RR] RrBackend` = auto / amd / amdnr / off) الخيارات Automatic (AMD FSR Ray Regeneration على RX 9000، وAMDNR Ray Denoiser (preview) على كل بطاقة أخرى) وAMD FSR Ray Regeneration وAMDNR Ray Denoiser (preview) وOff (the game's own)؛ والثلاثة الأولى تتبدّل أثناء اللعب، أما Off فيخبر اللعبة أن Ray Reconstruction غير مدعوم، فتبقى على مزيل التشويش الخاص بها (عند التشغيل التالي للعبة). وفي لعبة تعمل بـ **Vulkan** يكون Ray
```

**Round 4 - Feature bullet: RX 7000 "no provider for RDNA 3"** - round-3 lines 631-632 -> final line 625. Old:

```text
  Ray Regeneration، لكن مزيل التشويش من AMD لا يملك مزوّدًا لـ RDNA 3، فيرفض مزيل التشويش أن يبدأ وتُرفع دقة صورة
  Ray Reconstruction دون إزالة التشويش، وقد تبدو أكثر تشويشًا مما يعطيه مزيل التشويش الخاص باللعبة؛ وتوضّح صفحة
```

New:

```text
  Ray Regeneration، ويزيل AMDNR Ray Denoiser (preview) تشويشها، وعلى معالجات APU من RDNA 3 / 3.5 أيضًا (فـ AMD FSR Ray Regeneration مصمّم لـ RDNA 4)؛ وتوضّح صفحة
```

**Round 4 - Feature bullet: "planned" and the RX 6000 box** - round-3 lines 635-636 -> final line 628. Old:

```text
  فتبقى اللعبة على مزيل التشويش الخاص بها. ومزيل تشويش خاص بـ AMDNR لبطاقات RX 7000 مخطط له. ولا تحصل عليه RX 6000 والأقدم إلا مع `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (تبويب
  Upscaling: **Offer FSR Ray Regeneration on this GPU (restart)**). **الشحذ بعد RR (Sharpening after RR)** (0.3.4.1):
```

New:

```text
  فتبقى اللعبة على مزيل التشويش الخاص بها. ولا تحصل عليه RX 6000 والأقدم إلا كخيار اختياري، مع `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (تبويب Upscaling: **Offer Ray Regeneration on this GPU (restart)**، وعليها وسم "preview")، ويزيل تشويشها هناك أيضًا AMDNR Ray Denoiser. **الشحذ بعد RR (Sharpening after RR)** (0.3.4.1):
```

**Round 4 - FAQ "You cannot find the Ray Regeneration settings?"** - round-3 lines 807-814 -> final line 799. Old:

```text
ويُرسم ما دامت Ray Regeneration تعمل في اللعبة (ويبقى طوال الجلسة بمجرد أن تكون قد عملت)؛ وعندها يعرض سطر **Ray
Regeneration** في تبويب Neural زر **Open Ray Regeneration**. وحين لا تعمل، لا يُرسم التبويب، ويوضّح ذلك السطر في
تبويب Neural السبب: اللعبة لم تفعّل Ray Reconstruction، أو أن Ray Regeneration تخلّت عن هذه اللعبة والسبب، أو كم ثانية
مضت على آخر تشغيل لها. وعلى RX 7000 يضيف سطر باهت آخر أنها معروضة هناك كمعاينة (preview): مزيل التشويش من AMD لا يملك مزوّدًا لـ RDNA 3،
فبمجرد أن تفعّل اللعبة Ray Reconstruction تُرفع دقة صورتها دون إزالة التشويش، ويوقف `[FSR-RR] FfxDenoiserAllowPreRdna4=false`
(أو إلغاء تفعيل خانة تبويب Upscaling **Preview: Ray Regeneration on this card (restart)**) هذه المعاينة فتبقى اللعبة على
مزيل التشويش الخاص بها. وعلى RX 6000 وما قبلها يقول إنها غير معروضة على هذه البطاقة، وأن AMD تصدر مزيل
التشويش لـ RDNA 4، وأن `[FSR-RR] FfxDenoiserAllowPreRdna4=true` يعرضها مع ذلك. ولتشغيلها، من إعدادات الرسوميات في اللعبة نفسها: اختر
```

New:

```text
ويُرسم ما دامت Ray Regeneration تعمل في اللعبة (ويبقى طوال الجلسة بمجرد أن تكون قد عملت)؛ ومنذ 0.3.5.3 لم يعد في تبويب Neural سطر لـ Ray Regeneration. وحين لا تعمل، لا يُرسم التبويب؛ وإن كانت Ray Regeneration معطّلة في هذه اللعبة أو تعذّر بدؤها على هذه البطاقة، يقول ذلك السطر البرتقالي تحت بيئة التشغيل في تبويب Neural. وعلى RX 7000 ومعالجات APU من RDNA 3 / 3.5 تُعرض Ray Regeneration كمعاينة (preview) مفعّلة افتراضيًا، ويزيل تشويشها AMDNR Ray Denoiser (preview) (فـ AMD FSR Ray Regeneration مصمّم لـ RDNA 4)؛ ويوقف `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (أو إلغاء تفعيل خانة تبويب Upscaling **Preview: Ray Regeneration on this card (restart)**) هذه المعاينة فتبقى اللعبة على مزيل التشويش الخاص بها. وعلى RX 6000 وما قبلها لا تُعرض افتراضيًا: تعرضها خانة تبويب Upscaling **Offer Ray Regeneration on this GPU (restart)** (`[FSR-RR] FfxDenoiserAllowPreRdna4=true`)، ويزيل تشويشها AMDNR Ray Denoiser (preview). ولتشغيلها، من إعدادات الرسوميات في اللعبة نفسها: اختر
```

**Round 4 - Grain FAQ: "AMDNR's own denoiser is 0.3.6 work"** - round-3 lines 832-833 -> final line 817. Old:

```text
Reconstruction غير مدعوم فتبقى على مزيل التشويش الخاص بها (بعد إعادة التشغيل). ومزيل التشويش الخاص بـ AMDNR من عمل
0.3.6.
```

New:

```text
Reconstruction غير مدعوم فتبقى على مزيل التشويش الخاص بها (بعد إعادة التشغيل). ومنذ 0.3.5.3 يعرض الصف نفسه أيضًا مزيل التشويش الخاص بـ AMDNR، وهو AMDNR Ray Denoiser (preview)، الذي يستخدمه Automatic على كل البطاقات عدا RX 9000.
```

### README.es.md (round 4)

**Round 4 - "New in 0.3.5" note: RX 7000 without a denoiser** - round-3 lines 30-31 -> final line 30. Old:

```text
> de nuevo como preview, activa por defecto (el eliminador de ruido de AMD no tiene proveedor para RDNA 3, así que la
> imagen se escala sin eliminador de ruido; la casilla de la pestaña Upscaling o `[FSR-RR] FfxDenoiserAllowPreRdna4=false`
```

New:

```text
> de nuevo como preview, activa por defecto (desde 0.3.5.3 el AMDNR Ray Denoiser (preview) elimina ahí el ruido; la casilla de la pestaña Upscaling o `[FSR-RR] FfxDenoiserAllowPreRdna4=false`
```

**Round 4 - The other tabs: the Denoiser backend row** - round-3 lines 570-571 -> final line 569. Old:

```text
estado (por tarjeta y por API), la fila **Denoiser backend** (Automatic / Off — Off le dice al juego que Ray
Reconstruction no está soportado, así que conserva su propio denoiser; tras un reinicio), los controles, More Ray
```

New:

```text
estado (por tarjeta y por API), la fila **Denoiser backend** (Automatic / AMD FSR Ray Regeneration / AMDNR Ray Denoiser (preview) / Off (the game's own): Automatic es el de AMD en RX 9000 y el AMDNR Ray Denoiser en las demás tarjetas, los tres primeros cambian con el juego en marcha, y Off le dice al juego que Ray Reconstruction no está soportado, así que conserva su propio denoiser, tras un reinicio), los controles, More Ray
```

**Round 4 - Feature bullet, first line: RX 7000 without a denoiser** - round-3 line 643 -> final line 641. Old:

```text
- **FSR Ray Regeneration** — RX 9000 (RDNA 4); en RX 7000 (RDNA 3) como preview, activa por defecto, sin eliminador de ruido (ver abajo); solo en juegos que usan DLSS Ray Reconstruction (Cyberpunk 2077,
```

New:

```text
- **FSR Ray Regeneration** — RX 9000 (RDNA 4); en RX 7000 (RDNA 3) como preview, activa por defecto, con el AMDNR Ray Denoiser como eliminador de ruido (ver abajo); solo en juegos que usan DLSS Ray Reconstruction (Cyberpunk 2077,
```

**Round 4 - Feature bullet: Neural-tab pointer and RrBackend** - round-3 lines 648-652 -> final line 646. Old:

```text
  Regeneration se ejecuta en el título; la pestaña Neural apunta a ella y, mientras Ray Regeneration no se ejecuta,
  conserva la línea atenuada que dice por qué (el juego no ha activado Ray Reconstruction, el controlador rechazó el
  eliminador de ruido en esta tarjeta, Ray Regeneration abandonó este título y por qué, o cuándo se ejecutó por
  última vez). La fila **Denoiser backend** de la pestaña (`[FSR-RR] RrBackend = auto | off`) puede decirle al juego
  que Ray Reconstruction no está soportado, para que conserve su propio denoiser (en el siguiente inicio del juego).
```

New:

```text
  Regeneration se ejecuta en el título; desde 0.3.5.3 la pestaña Neural no tiene ninguna línea de Ray Regeneration, solo la línea naranja bajo el runtime cuando Ray Regeneration está apagado en un título o no pudo arrancar en esta GPU. La fila **Denoiser backend** de la pestaña (`[FSR-RR] RrBackend` = auto / amd / amdnr / off) ofrece Automatic (AMD FSR Ray Regeneration en RX 9000, el AMDNR Ray Denoiser (preview) en las demás tarjetas), AMD FSR Ray Regeneration, AMDNR Ray Denoiser (preview) y Off (the game's own); los tres primeros cambian con el juego en marcha, y Off le dice al juego que Ray Reconstruction no está soportado, para que conserve su propio denoiser (en el siguiente inicio del juego).
```

**Round 4 - Feature bullet: RX 7000 "no provider for RDNA 3"** - round-3 lines 661-663 -> final line 655. Old:

```text
  Reconstruction y los ajustes de Ray Regeneration se muestran, pero el eliminador de ruido de AMD no tiene proveedor
  para RDNA 3, así que el eliminador de ruido se niega a arrancar y la imagen de Ray Reconstruction se escala sin
  eliminador de ruido, que puede verse con más ruido que el del propio juego; la página de Ray Regeneration dice qué
```

New:

```text
  Reconstruction y los ajustes de Ray Regeneration se muestran, y el AMDNR Ray Denoiser (preview) le quita el ruido, también en las APU RDNA 3 / 3.5 (AMD FSR Ray Regeneration está hecho para RDNA 4); la página de Ray Regeneration dice qué
```

**Round 4 - Feature bullet: "planned" and the RX 6000 box** - round-3 lines 666-668 -> final line 658. Old:

```text
  `[FSR-RR] FfxDenoiserAllowPreRdna4=false`, y el juego conserva su propio eliminador de ruido. Está previsto un
  eliminador de ruido propio de AMDNR para RX 7000. RX 6000 y anteriores lo reciben solo con `[FSR-RR] FfxDenoiserAllowPreRdna4=true`
  (pestaña Upscaling: **Offer FSR Ray Regeneration on this GPU (restart)**). **Enfoque después de RR** (0.3.4.1):
```

New:

```text
  `[FSR-RR] FfxDenoiserAllowPreRdna4=false`, y el juego conserva su propio eliminador de ruido. RX 6000 y anteriores lo reciben solo como opción, con `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (pestaña Upscaling: **Offer Ray Regeneration on this GPU (restart)**, con la etiqueta "preview"), también con el AMDNR Ray Denoiser. **Enfoque después de RR** (0.3.4.1):
```

**Round 4 - FAQ "You cannot find the Ray Regeneration settings?"** - round-3 lines 845-854 -> final line 835. Old:

```text
de la partida una vez que se ha ejecutado); la línea **Ray Regeneration** de la pestaña Neural ofrece entonces un
botón **Open Ray Regeneration**. Mientras no se ejecuta, la pestaña no se dibuja y esa línea de la pestaña Neural
dice por qué: el juego no ha activado Ray Reconstruction, Ray Regeneration abandonó este título y por qué, o cuántos
segundos hace que se ejecutó por última vez. En una RX 7000 otra línea atenuada añade que ahí se ofrece como preview: el
eliminador de ruido de AMD no tiene proveedor para RDNA 3, así que en cuanto el juego activa Ray Reconstruction su
imagen se escala sin eliminador de ruido, y `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (o la casilla de la pestaña
Upscaling **Preview: Ray Regeneration on this card (restart)**, desmarcada) apaga la preview para que el juego conserve
su propio eliminador de ruido. En RX 6000 y anteriores dice
que no se ofrece en esa GPU, que AMD publica el eliminador de ruido para RDNA 4, y que
`[FSR-RR] FfxDenoiserAllowPreRdna4=true` lo ofrece de todos modos. Para ponerlo en marcha, en los ajustes gráficos
```

New:

```text
de la partida una vez que se ha ejecutado); desde 0.3.5.3 la pestaña Neural no tiene ninguna línea de Ray Regeneration. Mientras no se ejecuta, la pestaña no se dibuja; si Ray Regeneration está apagado en este título o no pudo arrancar en esta GPU, lo dice la línea naranja bajo el runtime de la pestaña Neural. En RX 7000 y en las APU RDNA 3 / 3.5 Ray Regeneration se ofrece como preview, activa por defecto, y el AMDNR Ray Denoiser (preview) le quita el ruido (AMD FSR Ray Regeneration está hecho para RDNA 4); `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (o la casilla de la pestaña Upscaling **Preview: Ray Regeneration on this card (restart)**, desmarcada) apaga la preview para que el juego conserve su propio eliminador de ruido. En RX 6000 y anteriores no se ofrece por defecto: la casilla de la pestaña Upscaling **Offer Ray Regeneration on this GPU (restart)** (`[FSR-RR] FfxDenoiserAllowPreRdna4=true`) lo ofrece, con el AMDNR Ray Denoiser (preview). Para ponerlo en marcha, en los ajustes gráficos
```

**Round 4 - Grain FAQ: "AMDNR's own denoiser is 0.3.6 work"** - round-3 lines 878-879 -> final line 859. Old:

```text
le dice al juego que Ray Reconstruction no está soportado para que conserve su propio denoiser (tras un reinicio). El
eliminador de ruido propio de AMDNR es trabajo de 0.3.6.
```

New:

```text
le dice al juego que Ray Reconstruction no está soportado para que conserve su propio denoiser (tras un reinicio). Desde 0.3.5.3 la misma fila ofrece también el eliminador de ruido propio de AMDNR, el AMDNR Ray Denoiser (preview), que Automatic usa en todas las tarjetas menos en RX 9000.
```

### README.fr.md (round 4)

**Round 4 - "New in 0.3.5" note: RX 7000 without a denoiser** - round-3 lines 30-31 -> final line 30. Old:

```text
> depuis la 0.3.5.1, de nouveau proposée en preview, activée par défaut (le débruiteur d'AMD n'a pas de fournisseur pour
> RDNA 3, donc l'image est mise à l'échelle sans débruiteur ; la case de l'onglet Upscaling ou
```

New:

```text
> depuis la 0.3.5.1, de nouveau proposée en preview, activée par défaut (depuis la 0.3.5.3, l'AMDNR Ray Denoiser (preview) la débruite là-bas ; la case de l'onglet Upscaling ou
```

**Round 4 - The other tabs: the Denoiser backend row** - round-3 lines 580-581 -> final line 579. Old:

```text
d'état (par carte et par API), la ligne **Denoiser backend** (Automatic / Off - Off indique au jeu que Ray
Reconstruction n'est pas pris en charge, il garde donc son propre débruiteur ; après un redémarrage), les réglages,
```

New:

```text
d'état (par carte et par API), la ligne **Denoiser backend** (Automatic / AMD FSR Ray Regeneration / AMDNR Ray Denoiser (preview) / Off (the game's own) : Automatic est celui d'AMD sur RX 9000 et l'AMDNR Ray Denoiser sur toutes les autres cartes, les trois premiers basculent pendant que le jeu tourne, et Off indique au jeu que Ray Reconstruction n'est pas pris en charge, il garde donc son propre débruiteur, après un redémarrage), les réglages,
```

**Round 4 - Feature bullet, first line: RX 7000 without a denoiser** - round-3 line 653 -> final line 651. Old:

```text
- **FSR Ray Regeneration** — RX 9000 (RDNA 4) ; sur RX 7000 (RDNA 3), en preview, activée par défaut, sans débruiteur (voir plus bas) ; et seulement dans les jeux qui utilisent DLSS Ray Reconstruction (Cyberpunk 2077,
```

New:

```text
- **FSR Ray Regeneration** — RX 9000 (RDNA 4) ; sur RX 7000 (RDNA 3), en preview, activée par défaut, débruitée par l'AMDNR Ray Denoiser (voir plus bas) ; et seulement dans les jeux qui utilisent DLSS Ray Reconstruction (Cyberpunk 2077,
```

**Round 4 - Feature bullet: Neural-tab pointer and RrBackend** - round-3 lines 658-662 -> final line 656. Old:

```text
  titre ; l'onglet Neural y renvoie et, tant que Ray Regeneration ne tourne pas, garde la ligne grisée qui dit
  pourquoi (le jeu n'a pas activé Ray Reconstruction, le pilote a refusé le débruiteur sur cette carte, Ray
  Regeneration a renoncé à ce titre et pourquoi, ou depuis combien de temps il a tourné pour la dernière fois). La
  ligne **Denoiser backend** de l'onglet (`[FSR-RR] RrBackend = auto | off`) peut indiquer au jeu que Ray
  Reconstruction n'est pas pris en charge, pour qu'il garde son propre débruiteur (au prochain démarrage du jeu).
```

New:

```text
  titre ; depuis la 0.3.5.3, l'onglet Neural n'a plus de ligne Ray Regeneration, seulement la ligne orange sous le runtime quand Ray Regeneration est coupée dans un titre ou n'a pas pu démarrer sur ce GPU. La ligne **Denoiser backend** de l'onglet (`[FSR-RR] RrBackend` = auto / amd / amdnr / off) propose Automatic (AMD FSR Ray Regeneration sur RX 9000, l'AMDNR Ray Denoiser (preview) sur toutes les autres cartes), AMD FSR Ray Regeneration, AMDNR Ray Denoiser (preview) et Off (the game's own) ; les trois premiers basculent pendant que le jeu tourne, et Off indique au jeu que Ray Reconstruction n'est pas pris en charge, pour qu'il garde son propre débruiteur (au prochain démarrage du jeu).
```

**Round 4 - Feature bullet: RX 7000 "no provider for RDNA 3"** - round-3 lines 672-674 -> final line 666. Old:

```text
  et les réglages de Ray Regeneration s'affichent, mais le débruiteur d'AMD n'a pas de fournisseur pour RDNA 3, donc le
  débruiteur refuse de démarrer et l'image Ray Reconstruction est mise à l'échelle sans débruiteur, ce qui peut
  paraître plus bruité que le débruiteur du jeu ; la page Ray Regeneration dit ce qui tourne. La case de l'onglet
```

New:

```text
  et les réglages de Ray Regeneration s'affichent, et l'AMDNR Ray Denoiser (preview) la débruite, sur les APU RDNA 3 / 3.5 aussi (AMD FSR Ray Regeneration est conçu pour RDNA 4) ; la page Ray Regeneration dit ce qui tourne. La case de l'onglet
```

**Round 4 - Feature bullet: "planned" and the RX 6000 box** - round-3 lines 677-679 -> final line 669. Old:

```text
  Un débruiteur propre à AMDNR pour les RX 7000 est prévu. Les RX 6000
  et plus anciennes ne l'ont qu'avec `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (onglet Upscaling :
  **Offer FSR Ray Regeneration on this GPU (restart)**). **Sharpening après RR**
```

New:

```text
  Les RX 6000 et plus anciennes ne l'ont qu'en option, avec `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (onglet Upscaling : **Offer Ray Regeneration on this GPU (restart)**, marquée « preview »), débruitée elle aussi par l'AMDNR Ray Denoiser. **Sharpening après RR**
```

**Round 4 - FAQ "You cannot find the Ray Regeneration settings?"** - round-3 lines 860-869 -> final line 850. Old:

```text
pour la session une fois qu'il a tourné) ; la ligne **Ray Regeneration** de l'onglet Neural propose alors un bouton
**Open Ray Regeneration**. Tant qu'il ne tourne pas, l'onglet n'est pas affiché et cette ligne de l'onglet Neural
dit pourquoi : le jeu n'a pas activé Ray Reconstruction, Ray Regeneration a renoncé à ce titre et pourquoi, ou il y
a combien de secondes il a tourné pour la dernière fois. Sur une RX 7000, une ligne grisée supplémentaire ajoute
qu'il y est proposé en preview : le débruiteur d'AMD n'a pas de fournisseur pour RDNA 3, donc dès que le jeu active
Ray Reconstruction, son image est mise à l'échelle sans débruiteur, et `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (ou
la case de l'onglet Upscaling **Preview: Ray Regeneration on this card (restart)**, décochée) coupe la preview pour
que le jeu garde son propre débruiteur. Sur
RX 6000 et plus ancien, elle dit qu'il n'est pas proposé sur ce GPU, qu'AMD publie le débruiteur pour RDNA 4, et que
`[FSR-RR] FfxDenoiserAllowPreRdna4=true` le propose quand même. Pour le faire tourner, dans
```

New:

```text
pour la session une fois qu'il a tourné) ; depuis la 0.3.5.3, l'onglet Neural n'a plus de ligne Ray Regeneration. Tant qu'il ne tourne pas, l'onglet n'est pas affiché ; si Ray Regeneration est coupée dans ce titre ou n'a pas pu démarrer sur ce GPU, la ligne orange sous le runtime de l'onglet Neural le dit. Sur RX 7000 et les APU RDNA 3 / 3.5, Ray Regeneration est proposée en preview, activée par défaut, et l'AMDNR Ray Denoiser (preview) la débruite (AMD FSR Ray Regeneration est conçu pour RDNA 4) ; `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (ou la case de l'onglet Upscaling **Preview: Ray Regeneration on this card (restart)**, décochée) coupe la preview pour que le jeu garde son propre débruiteur. Sur RX 6000 et plus ancien, elle n'est pas proposée par défaut : la case de l'onglet Upscaling **Offer Ray Regeneration on this GPU (restart)** (`[FSR-RR] FfxDenoiserAllowPreRdna4=true`) la propose, débruitée par l'AMDNR Ray Denoiser (preview). Pour le faire tourner, dans
```

**Round 4 - Grain FAQ: "AMDNR's own denoiser is 0.3.6 work"** - round-3 line 894 -> final line 875. Old:

```text
(après un redémarrage). Le débruiteur propre à AMDNR est un travail de la 0.3.6.
```

New:

```text
(après un redémarrage). Depuis la 0.3.5.3, la même ligne propose aussi le débruiteur propre à AMDNR, l'AMDNR Ray Denoiser (preview), qu'Automatic utilise sur toutes les cartes sauf les RX 9000.
```

### README.it.md (round 4)

**Round 4 - "New in 0.3.5" note: RX 7000 without a denoiser** - round-3 lines 30-31 -> final line 30. Old:

```text
> nuovo offerta come preview, attiva di default (il denoiser di AMD non ha un provider per RDNA 3, quindi l'immagine viene
> scalata senza denoiser; la casella della scheda Upscaling o `[FSR-RR] FfxDenoiserAllowPreRdna4=false` la spegne e il
```

New:

```text
> nuovo offerta come preview, attiva di default (dalla 0.3.5.3 lì il denoiser è l'AMDNR Ray Denoiser (preview); la casella della scheda Upscaling o `[FSR-RR] FfxDenoiserAllowPreRdna4=false` la spegne e il
```

**Round 4 - The other tabs: the Denoiser backend row** - round-3 lines 562-563 -> final line 561. Old:

```text
API), la riga **Denoiser backend** (Automatic / Off - Off dice al gioco che la Ray Reconstruction non è supportata, così
tiene il proprio denoiser; dopo un riavvio), i controlli, More Ray Regeneration options, e un blocco Diagnostics tutto
```

New:

```text
API), la riga **Denoiser backend** (Automatic / AMD FSR Ray Regeneration / AMDNR Ray Denoiser (preview) / Off (the game's own): Automatic è quello di AMD su RX 9000 e l'AMDNR Ray Denoiser su tutte le altre schede, i primi tre si cambiano a gioco in corso, e Off dice al gioco che la Ray Reconstruction non è supportata, così tiene il proprio denoiser, dopo un riavvio), i controlli, More Ray Regeneration options, e un blocco Diagnostics tutto
```

**Round 4 - Feature bullet, first line: RX 7000 without a denoiser** - round-3 line 634 -> final line 632. Old:

```text
- **FSR Ray Regeneration** — su RX 9000 (RDNA 4); su RX 7000 (RDNA 3) come preview, attiva di default, senza denoiser (vedi sotto); solo nei giochi che usano DLSS Ray Reconstruction (Cyberpunk 2077,
```

New:

```text
- **FSR Ray Regeneration** — su RX 9000 (RDNA 4); su RX 7000 (RDNA 3) come preview, attiva di default, con l'AMDNR Ray Denoiser come denoiser (vedi sotto); solo nei giochi che usano DLSS Ray Reconstruction (Cyberpunk 2077,
```

**Round 4 - Feature bullet: Neural-tab pointer and RrBackend** - round-3 lines 639-643 -> final line 637. Old:

```text
  nel titolo; la scheda Neural vi rimanda e, mentre Ray Regeneration non è in funzione, mantiene la riga in grigio che
  dice perché (il gioco non ha attivato la Ray Reconstruction, il driver ha rifiutato il denoiser su questa scheda
  video, Ray Regeneration ha rinunciato a questo titolo e perché, o quando è stata eseguita l'ultima volta). La riga
  **Denoiser backend** della scheda (`[FSR-RR] RrBackend = auto | off`) può dire al gioco che la Ray Reconstruction
  non è supportata, così tiene il proprio denoiser (al prossimo avvio del gioco). In un titolo **Vulkan** la Ray
```

New:

```text
  nel titolo; dalla 0.3.5.3 la scheda Neural non ha più una riga di Ray Regeneration, solo la riga arancione sotto il runtime quando Ray Regeneration è spenta in un titolo o non è potuta partire su questa GPU. La riga **Denoiser backend** della scheda (`[FSR-RR] RrBackend` = auto / amd / amdnr / off) offre Automatic (AMD FSR Ray Regeneration su RX 9000, l'AMDNR Ray Denoiser (preview) su tutte le altre schede), AMD FSR Ray Regeneration, AMDNR Ray Denoiser (preview) e Off (the game's own); i primi tre si cambiano a gioco in corso, e Off dice al gioco che la Ray Reconstruction non è supportata, così tiene il proprio denoiser (al prossimo avvio del gioco). In un titolo **Vulkan** la Ray
```

**Round 4 - Feature bullet: RX 7000 "no provider for RDNA 3"** - round-3 lines 652-654 -> final line 646. Old:

```text
  Regeneration compaiono, ma il denoiser di AMD non ha un provider per RDNA 3, quindi il denoiser si rifiuta di partire
  e l'immagine della Ray Reconstruction viene scalata senza denoiser, che può risultare più rumorosa del denoiser del
  gioco; la pagina Ray Regeneration dice cosa gira. La casella della scheda Upscaling
```

New:

```text
  Regeneration compaiono, e l'AMDNR Ray Denoiser (preview) le toglie il rumore, anche sulle APU RDNA 3 / 3.5 (AMD FSR Ray Regeneration è fatta per RDNA 4); la pagina Ray Regeneration dice cosa gira. La casella della scheda Upscaling
```

**Round 4 - Feature bullet: "planned" and the RX 6000 box** - round-3 lines 656-658 -> final line 648. Old:

```text
  togli la spunta, o imposta `[FSR-RR] FfxDenoiserAllowPreRdna4=false`, e il gioco tiene il proprio denoiser. È
  previsto un denoiser tutto di AMDNR per le RX 7000. Le RX 6000 e precedenti la ricevono solo con `[FSR-RR] FfxDenoiserAllowPreRdna4=true`
  (scheda Upscaling: **Offer FSR Ray Regeneration on this GPU (restart)**). **Sharpening dopo RR** (0.3.4.1): quando il
```

New:

```text
  togli la spunta, o imposta `[FSR-RR] FfxDenoiserAllowPreRdna4=false`, e il gioco tiene il proprio denoiser. Le RX 6000 e precedenti la ricevono solo come opzione, con `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (scheda Upscaling: **Offer Ray Regeneration on this GPU (restart)**, con il tag "preview"), anche lì con l'AMDNR Ray Denoiser. **Sharpening dopo RR** (0.3.4.1): quando il
```

**Round 4 - FAQ "You cannot find the Ray Regeneration settings?"** - round-3 lines 835-844 -> final line 825. Old:

```text
gioco una volta che è entrata in funzione); la riga **Ray Regeneration** della scheda Neural offre allora un pulsante
**Open Ray Regeneration**. Mentre non è in funzione, la scheda non viene disegnata e quella riga della scheda Neural
dice perché: il gioco non ha attivato la Ray Reconstruction, Ray Regeneration ha rinunciato a questo titolo e perché, o
da quanti secondi è stata eseguita l'ultima volta. Su una RX 7000 un'ulteriore riga in grigio aggiunge che lì viene
offerta come preview: il denoiser di AMD non ha un provider per RDNA 3, quindi appena il gioco attiva la Ray
Reconstruction la sua immagine viene scalata senza denoiser, e `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (o la casella
della scheda Upscaling **Preview: Ray Regeneration on this card (restart)**, senza spunta) spegne la preview, così il
gioco tiene il proprio denoiser. Su RX 6000
e precedenti dice che non viene offerta su quella GPU, che AMD pubblica il denoiser per RDNA 4, e che
`[FSR-RR] FfxDenoiserAllowPreRdna4=true` la offre comunque. Per farla partire,
```

New:

```text
gioco una volta che è entrata in funzione); dalla 0.3.5.3 la scheda Neural non ha più una riga di Ray Regeneration. Mentre non è in funzione, la scheda non viene disegnata; se Ray Regeneration è spenta in questo titolo o non è potuta partire su questa GPU, lo dice la riga arancione sotto il runtime della scheda Neural. Su RX 7000 e sulle APU RDNA 3 / 3.5 Ray Regeneration è offerta come preview, attiva di default, e l'AMDNR Ray Denoiser (preview) le toglie il rumore (AMD FSR Ray Regeneration è fatta per RDNA 4); `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (o la casella della scheda Upscaling **Preview: Ray Regeneration on this card (restart)**, senza spunta) spegne la preview, così il gioco tiene il proprio denoiser. Su RX 6000 e precedenti non viene offerta di default: la casella della scheda Upscaling **Offer Ray Regeneration on this GPU (restart)** (`[FSR-RR] FfxDenoiserAllowPreRdna4=true`) la offre, con l'AMDNR Ray Denoiser (preview). Per farla partire,
```

**Round 4 - Grain FAQ: "AMDNR's own denoiser is 0.3.6 work"** - round-3 lines 868-869 -> final line 849. Old:

```text
Reconstruction non è supportata, così tiene il proprio denoiser (dopo un riavvio). Il denoiser proprio di AMDNR è
lavoro per la 0.3.6.
```

New:

```text
Reconstruction non è supportata, così tiene il proprio denoiser (dopo un riavvio). Dalla 0.3.5.3 la stessa riga offre anche il denoiser proprio di AMDNR, l'AMDNR Ray Denoiser (preview), che Automatic usa su tutte le schede tranne le RX 9000.
```

### README.pl.md (round 4)

**Round 4 - "New in 0.3.5" note: RX 7000 without a denoiser** - round-3 lines 30-31 -> final line 30. Old:

```text
> 0.3.5.1 jest znów oferowane jako preview, domyślnie włączone (denoiser AMD nie ma providera dla RDNA 3, więc obraz jest
> skalowany bez odszumiania; pole w zakładce Upscaling albo `[FSR-RR] FfxDenoiserAllowPreRdna4=false` je wyłącza, a gra
```

New:

```text
> 0.3.5.1 jest znów oferowane jako preview, domyślnie włączone (od 0.3.5.3 odszumia je tam AMDNR Ray Denoiser (preview); pole w zakładce Upscaling albo `[FSR-RR] FfxDenoiserAllowPreRdna4=false` je wyłącza, a gra
```

**Round 4 - The other tabs: the Denoiser backend row** - round-3 lines 556-557 -> final line 555. Old:

```text
każdej karty i każdego API), wiersz **Denoiser backend** (Automatic / Off - Off mówi grze, że Ray Reconstruction nie
jest obsługiwane, więc gra zachowuje własny denoiser; po restarcie), ustawienia, More Ray Regeneration options oraz
```

New:

```text
każdej karty i każdego API), wiersz **Denoiser backend** (Automatic / AMD FSR Ray Regeneration / AMDNR Ray Denoiser (preview) / Off (the game's own): Automatic to denoiser AMD na RX 9000 i AMDNR Ray Denoiser na każdej innej karcie, trzy pierwsze przełączają się w trakcie gry, a Off mówi grze, że Ray Reconstruction nie jest obsługiwane, więc gra zachowuje własny denoiser, po restarcie), ustawienia, More Ray Regeneration options oraz
```

**Round 4 - Feature bullet, first line: RX 7000 without a denoiser** - round-3 line 625 -> final line 623. Old:

```text
- **FSR Ray Regeneration** — na RX 9000 (RDNA 4); na RX 7000 (RDNA 3) jako preview, domyślnie włączone, bez denoisera (patrz niżej); tylko w grach korzystających z DLSS Ray Reconstruction (Cyberpunk 2077,
```

New:

```text
- **FSR Ray Regeneration** — na RX 9000 (RDNA 4); na RX 7000 (RDNA 3) jako preview, domyślnie włączone, z AMDNR Ray Denoiser jako denoiserem (patrz niżej); tylko w grach korzystających z DLSS Ray Reconstruction (Cyberpunk 2077,
```

**Round 4 - Feature bullet: Neural-tab pointer and RrBackend** - round-3 lines 629-634 -> final line 627. Old:

```text
  Regeneration**, zaraz po Upscaling, rysowanej, gdy Ray Regeneration działa w danym tytule; zakładka Neural na nią
  wskazuje i, gdy Ray Regeneration nie działa, zachowuje przygaszoną linijkę, która mówi dlaczego (gra nie włączyła
  Ray Reconstruction, sterownik odrzucił denoiser na tej karcie, Ray Regeneration zrezygnowało z tej gry i dlaczego,
  albo kiedy działało ostatni raz). Wiersz **Denoiser backend** tej zakładki (`[FSR-RR] RrBackend = auto | off`) może
  powiedzieć grze, że Ray Reconstruction nie jest obsługiwane, więc gra zachowuje własny denoiser (od następnego
  uruchomienia gry). W tytule **Vulkan** Ray Reconstruction jest z założenia "not supported" (denoiser działa na
```

New:

```text
  Regeneration**, zaraz po Upscaling, rysowanej, gdy Ray Regeneration działa w danym tytule; od 0.3.5.3 zakładka Neural nie ma żadnej linii Ray Regeneration, tylko pomarańczową linię pod runtime'em, gdy Ray Regeneration jest wyłączone w danym tytule albo nie mogło wystartować na tym GPU. Wiersz **Denoiser backend** tej zakładki (`[FSR-RR] RrBackend` = auto / amd / amdnr / off) oferuje Automatic (AMD FSR Ray Regeneration na RX 9000, AMDNR Ray Denoiser (preview) na każdej innej karcie), AMD FSR Ray Regeneration, AMDNR Ray Denoiser (preview) i Off (the game's own); trzy pierwsze przełączają się w trakcie gry, a Off mówi grze, że Ray Reconstruction nie jest obsługiwane, więc gra zachowuje własny denoiser (od następnego uruchomienia gry). W tytule **Vulkan** Ray Reconstruction jest z założenia "not supported" (denoiser działa na
```

**Round 4 - Feature bullet: RX 7000 "no provider for RDNA 3"** - round-3 lines 642-643 -> final line 635. Old:

```text
  Ray Regeneration się pokazują, ale denoiser AMD nie ma providera dla RDNA 3, więc denoiser odmawia startu, a obraz
  Ray Reconstruction jest skalowany bez odszumiania, co może wyglądać na bardziej zaszumione niż własny denoiser gry;
```

New:

```text
  Ray Regeneration się pokazują, a odszumia je AMDNR Ray Denoiser (preview), także na APU RDNA 3 / 3.5 (AMD FSR Ray Regeneration powstało dla RDNA 4);
```

**Round 4 - Feature bullet: "planned" and the RX 6000 box** - round-3 lines 646-648 -> final line 638. Old:

```text
  `[FSR-RR] FfxDenoiserAllowPreRdna4=false`, a gra zostanie przy swoim denoiserze. Własny denoiser AMDNR dla RX 7000 jest
  planowany. RX 6000 i starsze dostają je tylko z `[FSR-RR] FfxDenoiserAllowPreRdna4=true`
  (zakładka Upscaling: **Offer FSR Ray Regeneration on this GPU (restart)**). **Wyostrzanie po RR** (0.3.4.1): gdy
```

New:

```text
  `[FSR-RR] FfxDenoiserAllowPreRdna4=false`, a gra zostanie przy swoim denoiserze. RX 6000 i starsze dostają je tylko jako opcję, z `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (zakładka Upscaling: **Offer Ray Regeneration on this GPU (restart)**, z dopiskiem "preview"), również z AMDNR Ray Denoiser. **Wyostrzanie po RR** (0.3.4.1): gdy
```

**Round 4 - FAQ "You cannot find the Ray Regeneration settings?"** - round-3 lines 823-830 -> final line 813. Old:

```text
linia **Ray Regeneration** w zakładce Neural oferuje wtedy przycisk **Open Ray Regeneration**. Gdy nie działa,
zakładka nie jest rysowana, a ta linia w zakładce Neural mówi dlaczego: gra nie włączyła Ray Reconstruction, Ray
Regeneration zrezygnowało z tej gry i dlaczego, albo ile sekund temu działało ostatni raz. Na RX 7000 kolejna
przygaszona linijka dodaje, że jest tam oferowane jako preview: denoiser AMD nie ma providera dla RDNA 3, więc gdy tylko
gra włączy Ray Reconstruction, jej obraz jest skalowany bez odszumiania, a `[FSR-RR] FfxDenoiserAllowPreRdna4=false`
(albo odznaczone pole w zakładce Upscaling **Preview: Ray Regeneration on this card (restart)**) wyłącza preview, więc
gra zostaje przy swoim denoiserze. Na RX 6000 i starszych mówi, że na tej karcie nie jest oferowane, że AMD udostępnia denoiser dla RDNA 4
i że `[FSR-RR] FfxDenoiserAllowPreRdna4=true` udostępnia je mimo to. Aby je uruchomić, w ustawieniach graficznych gry: wybierz **DLSS** jako upscaler (nie FSR, nie XeSS),
```

New:

```text
od 0.3.5.3 zakładka Neural nie ma żadnej linii Ray Regeneration. Gdy nie działa, zakładka nie jest rysowana; jeśli Ray Regeneration jest wyłączone w tym tytule albo nie mogło wystartować na tym GPU, mówi o tym pomarańczowa linia pod runtime'em w zakładce Neural. Na RX 7000 i APU RDNA 3 / 3.5 Ray Regeneration jest oferowane jako preview, domyślnie włączone, a odszumia je AMDNR Ray Denoiser (preview) (AMD FSR Ray Regeneration powstało dla RDNA 4); `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (albo odznaczone pole w zakładce Upscaling **Preview: Ray Regeneration on this card (restart)**) wyłącza preview, więc gra zostaje przy swoim denoiserze. Na RX 6000 i starszych nie jest domyślnie oferowane: pole w zakładce Upscaling **Offer Ray Regeneration on this GPU (restart)** (`[FSR-RR] FfxDenoiserAllowPreRdna4=true`) je udostępnia, z AMDNR Ray Denoiser (preview). Aby je uruchomić, w ustawieniach graficznych gry: wybierz **DLSS** jako upscaler (nie FSR, nie XeSS),
```

**Round 4 - Grain FAQ: "AMDNR's own denoiser is 0.3.6 work"** - round-3 line 852 -> final line 835. Old:

```text
(po restarcie). Własny denoiser AMDNR to praca na 0.3.6.
```

New:

```text
(po restarcie). Od 0.3.5.3 ten sam wiersz oferuje też własny denoiser AMDNR, AMDNR Ray Denoiser (preview), którego Automatic używa na każdej karcie poza RX 9000.
```

### README.pt-BR.md (round 4)

**Round 4 - "New in 0.3.5" note: RX 7000 without a denoiser** - round-3 lines 30-31 -> final line 30. Old:

```text
> oferecido como preview, ligado por padrão (o denoiser da AMD não tem provedor para RDNA 3, então a imagem é escalada
> sem denoiser; a caixa da aba Upscaling ou `[FSR-RR] FfxDenoiserAllowPreRdna4=false` o desliga e o jogo fica com o
```

New:

```text
> oferecido como preview, ligado por padrão (desde o 0.3.5.3 o denoiser ali é o AMDNR Ray Denoiser (preview); a caixa da aba Upscaling ou `[FSR-RR] FfxDenoiserAllowPreRdna4=false` o desliga e o jogo fica com o
```

**Round 4 - The other tabs: the Denoiser backend row** - round-3 lines 558-559 -> final line 557. Old:

```text
por API), a linha **Denoiser backend** (Automatic / Off - Off diz ao jogo que o Ray Reconstruction não é suportado,
então ele fica com o denoiser dele; depois de reiniciar), os controles, More Ray Regeneration options e um bloco
```

New:

```text
por API), a linha **Denoiser backend** (Automatic / AMD FSR Ray Regeneration / AMDNR Ray Denoiser (preview) / Off (the game's own): Automatic é o da AMD na RX 9000 e o AMDNR Ray Denoiser em todas as outras placas, os três primeiros trocam com o jogo rodando, e Off diz ao jogo que o Ray Reconstruction não é suportado, então ele fica com o denoiser dele, depois de reiniciar), os controles, More Ray Regeneration options e um bloco
```

**Round 4 - Feature bullet, first line: RX 7000 without a denoiser** - round-3 line 629 -> final line 627. Old:

```text
- **FSR Ray Regeneration** — RX 9000 (RDNA 4); na RX 7000 (RDNA 3) como preview, ligado por padrão, sem denoiser (veja abaixo); só em jogos que usam DLSS Ray Reconstruction (Cyberpunk 2077, Alan
```

New:

```text
- **FSR Ray Regeneration** — RX 9000 (RDNA 4); na RX 7000 (RDNA 3) como preview, ligado por padrão, com o AMDNR Ray Denoiser como denoiser (veja abaixo); só em jogos que usam DLSS Ray Reconstruction (Cyberpunk 2077, Alan
```

**Round 4 - Feature bullet: Neural-tab pointer and RrBackend** - round-3 lines 633-638 -> final line 631. Old:

```text
  Regeneration** própria, logo depois de Upscaling, desenhada enquanto o Ray Regeneration roda no título; a aba Neural
  aponta para ela e, enquanto o Ray Regeneration não está rodando, mantém a linha esmaecida que diz por que (o jogo
  não ligou o Ray Reconstruction, o driver recusou o denoiser nesta placa, o Ray Regeneration desistiu deste título e
  por que, ou quando ele rodou pela última vez). A linha **Denoiser backend** da aba
  (`[FSR-RR] RrBackend = auto | off`) pode dizer ao jogo que o Ray Reconstruction não é suportado, para que ele fique com o denoiser dele (no
  próximo início do jogo). Num título **Vulkan** o Ray Reconstruction é "not supported" por design (o denoiser é
```

New:

```text
  Regeneration** própria, logo depois de Upscaling, desenhada enquanto o Ray Regeneration roda no título; desde o 0.3.5.3 a aba Neural não tem nenhuma linha de Ray Regeneration, só a linha laranja abaixo do runtime quando o Ray Regeneration está desligado num título ou não conseguiu iniciar nesta GPU. A linha **Denoiser backend** da aba (`[FSR-RR] RrBackend` = auto / amd / amdnr / off) oferece Automatic (AMD FSR Ray Regeneration na RX 9000, o AMDNR Ray Denoiser (preview) em todas as outras placas), AMD FSR Ray Regeneration, AMDNR Ray Denoiser (preview) e Off (the game's own); os três primeiros trocam com o jogo rodando, e Off diz ao jogo que o Ray Reconstruction não é suportado, para que ele fique com o denoiser dele (no próximo início do jogo). Num título **Vulkan** o Ray Reconstruction é "not supported" por design (o denoiser é
```

**Round 4 - Feature bullet: RX 7000 "no provider for RDNA 3"** - round-3 lines 646-648 -> final line 639. Old:

```text
  ajustes do Ray Regeneration aparecem, mas o denoiser da AMD não tem provedor para RDNA 3, então o denoiser se recusa a
  iniciar e a imagem do Ray Reconstruction é escalada sem denoiser, o que pode parecer mais ruidoso que o denoiser do
  próprio jogo; a página do Ray Regeneration diz o que está rodando. A caixa da aba Upscaling
```

New:

```text
  ajustes do Ray Regeneration aparecem, e o AMDNR Ray Denoiser (preview) tira o ruído dela, também nas APUs RDNA 3 / 3.5 (o AMD FSR Ray Regeneration é feito para RDNA 4); a página do Ray Regeneration diz o que está rodando. A caixa da aba Upscaling
```

**Round 4 - Feature bullet: "planned" and the RX 6000 box** - round-3 lines 650-652 -> final line 641. Old:

```text
  padrão; desmarque-a, ou defina `[FSR-RR] FfxDenoiserAllowPreRdna4=false`, e o jogo fica com o denoiser dele. Um
  denoiser próprio do AMDNR para RX 7000 está previsto. RX 6000 e anteriores só o recebem com `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (aba
  Upscaling: **Offer FSR Ray Regeneration on this GPU (restart)**). **Nitidez depois do RR** (0.3.4.1): quando o
```

New:

```text
  padrão; desmarque-a, ou defina `[FSR-RR] FfxDenoiserAllowPreRdna4=false`, e o jogo fica com o denoiser dele. RX 6000 e anteriores só o recebem como opção, com `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (aba Upscaling: **Offer Ray Regeneration on this GPU (restart)**, com a etiqueta "preview"), também com o AMDNR Ray Denoiser. **Nitidez depois do RR** (0.3.4.1): quando o
```

**Round 4 - FAQ "You cannot find the Ray Regeneration settings?"** - round-3 lines 831-839 -> final line 820. Old:

```text
sessão de jogo depois que ele rodou); a linha **Ray Regeneration** da aba Neural então oferece um botão **Open Ray
Regeneration**. Enquanto ele não está rodando, a aba não é desenhada e essa linha da aba Neural diz por que: o jogo
não ligou o Ray Reconstruction, o Ray Regeneration desistiu deste título e por que, ou há quantos segundos ele rodou
pela última vez. Numa RX 7000 mais uma linha esmaecida acrescenta que ali ele é oferecido como preview: o denoiser da AMD
não tem provedor para RDNA 3, então assim que o jogo liga o Ray Reconstruction a imagem dele é escalada sem denoiser, e
`[FSR-RR] FfxDenoiserAllowPreRdna4=false` (ou a caixa da aba Upscaling **Preview: Ray Regeneration on this card (restart)**,
desmarcada) desliga a preview para o jogo ficar com o denoiser dele. Em RX 6000 e anteriores ela diz que ele não é oferecido nessa
GPU, que a AMD publica o denoiser para RDNA 4, e que `[FSR-RR] FfxDenoiserAllowPreRdna4=true` o oferece de todo
jeito. Para fazê-lo rodar, nas
```

New:

```text
sessão de jogo depois que ele rodou); desde o 0.3.5.3 a aba Neural não tem nenhuma linha de Ray Regeneration. Enquanto ele não está rodando, a aba não é desenhada; se o Ray Regeneration estiver desligado neste título ou não conseguir iniciar nesta GPU, a linha laranja abaixo do runtime na aba Neural diz isso. Na RX 7000 e nas APUs RDNA 3 / 3.5 o Ray Regeneration é oferecido como preview, ligado por padrão, e o AMDNR Ray Denoiser (preview) tira o ruído dele (o AMD FSR Ray Regeneration é feito para RDNA 4); `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (ou a caixa da aba Upscaling **Preview: Ray Regeneration on this card (restart)**, desmarcada) desliga a preview para o jogo ficar com o denoiser dele. Em RX 6000 e anteriores ele não é oferecido por padrão: a caixa da aba Upscaling **Offer Ray Regeneration on this GPU (restart)** (`[FSR-RR] FfxDenoiserAllowPreRdna4=true`) o oferece, com o AMDNR Ray Denoiser (preview). Para fazê-lo rodar, nas
```

**Round 4 - Grain FAQ: "AMDNR's own denoiser is 0.3.6 work"** - round-3 lines 862-863 -> final line 843. Old:

```text
é suportado, para que ele fique com o denoiser dele (depois de reiniciar). O denoiser próprio do AMDNR é trabalho do
0.3.6.
```

New:

```text
é suportado, para que ele fique com o denoiser dele (depois de reiniciar). Desde o 0.3.5.3 a mesma linha também oferece o denoiser próprio do AMDNR, o AMDNR Ray Denoiser (preview), que o Automatic usa em todas as placas, menos na RX 9000.
```

### README.ru.md (round 4)

**Round 4 - "New in 0.3.5" note: RX 7000 without a denoiser** - round-3 lines 30-31 -> final line 30. Old:

```text
> как preview, включённый по умолчанию (у денойзера AMD нет провайдера для RDNA 3, поэтому картинка апскейлится без
> шумоподавления; флажок на вкладке Upscaling или `[FSR-RR] FfxDenoiserAllowPreRdna4=false` выключает его, и игра
```

New:

```text
> как preview, включённый по умолчанию (с 0.3.5.3 шум там убирает AMDNR Ray Denoiser (preview); флажок на вкладке Upscaling или `[FSR-RR] FfxDenoiserAllowPreRdna4=false` выключает его, и игра
```

**Round 4 - The other tabs: the Denoiser backend row** - round-3 lines 554-555 -> final line 553. Old:

```text
каждой видеокарты и каждого API), строка **Denoiser backend** (Automatic / Off — Off сообщает игре, что Ray
Reconstruction не поддерживается, и она оставляет свой денойзер; после перезапуска), настройки, More Ray Regeneration
```

New:

```text
каждой видеокарты и каждого API), строка **Denoiser backend** (Automatic / AMD FSR Ray Regeneration / AMDNR Ray Denoiser (preview) / Off (the game's own): Automatic — денойзер AMD на RX 9000 и AMDNR Ray Denoiser на всех остальных видеокартах, первые три переключаются прямо во время игры, а Off сообщает игре, что Ray Reconstruction не поддерживается, и она оставляет свой денойзер, после перезапуска), настройки, More Ray Regeneration
```

**Round 4 - Feature bullet, first line: RX 7000 without a denoiser** - round-3 line 623 -> final line 621. Old:

```text
- **FSR Ray Regeneration** — на RX 9000 (RDNA 4); на RX 7000 (RDNA 3) как preview, включённый по умолчанию, без денойзера (см. ниже); только в играх с DLSS Ray Reconstruction (Cyberpunk 2077,
```

New:

```text
- **FSR Ray Regeneration** — на RX 9000 (RDNA 4); на RX 7000 (RDNA 3) как preview, включённый по умолчанию, с AMDNR Ray Denoiser в роли денойзера (см. ниже); только в играх с DLSS Ray Reconstruction (Cyberpunk 2077,
```

**Round 4 - Feature bullet: Neural-tab pointer and RrBackend** - round-3 lines 628-632 -> final line 626. Old:

```text
  Ray Regeneration; вкладка Neural указывает на неё и, пока Ray Regeneration не работает, сохраняет приглушённую
  строку, которая говорит почему (игра не включила Ray Reconstruction, драйвер отказал в денойзере на этой видеокарте,
  Ray Regeneration отказался от этой игры и почему, или когда он работал в последний раз). Строка **Denoiser backend**
  на этой вкладке (`[FSR-RR] RrBackend = auto | off`) может сообщить игре, что Ray Reconstruction не поддерживается, и
  тогда она оставит свой денойзер (при следующем запуске игры). В игре на **Vulkan** Ray Reconstruction — "not
```

New:

```text
  Ray Regeneration; с 0.3.5.3 на вкладке Neural нет строки Ray Regeneration, только оранжевая строка под рантаймом, когда Ray Regeneration выключен в игре или не смог запуститься на этой видеокарте. Строка **Denoiser backend** на этой вкладке (`[FSR-RR] RrBackend` = auto / amd / amdnr / off) предлагает Automatic (AMD FSR Ray Regeneration на RX 9000, AMDNR Ray Denoiser (preview) на всех остальных видеокартах), AMD FSR Ray Regeneration, AMDNR Ray Denoiser (preview) и Off (the game's own); первые три переключаются прямо во время игры, а Off сообщает игре, что Ray Reconstruction не поддерживается, и тогда она оставит свой денойзер (при следующем запуске игры). В игре на **Vulkan** Ray Reconstruction — "not
```

**Round 4 - Feature bullet: RX 7000 "no provider for RDNA 3"** - round-3 lines 640-642 -> final line 634. Old:

```text
  Reconstruction, и настройки Ray Regeneration показываются, но у денойзера AMD нет провайдера для RDNA 3, поэтому
  денойзер отказывается запускаться, и картинка Ray Reconstruction апскейлится без шумоподавления, что может
  выглядеть шумнее собственного денойзера игры; страница Ray Regeneration говорит, что работает. Флажок на вкладке
```

New:

```text
  Reconstruction, и настройки Ray Regeneration показываются, а шум убирает AMDNR Ray Denoiser (preview), и на APU RDNA 3 / 3.5 тоже (AMD FSR Ray Regeneration сделан для RDNA 4); страница Ray Regeneration говорит, что работает. Флажок на вкладке
```

**Round 4 - Feature bullet: "planned" and the RX 6000 box** - round-3 lines 645-647 -> final line 637. Old:

```text
  Собственный денойзер AMDNR для RX 7000 запланирован. RX 6000 и более
  старые карты получают его только с `[FSR-RR] FfxDenoiserAllowPreRdna4=true`
  (вкладка Upscaling: **Offer FSR Ray Regeneration on this GPU (restart)**). **Резкость после RR**
```

New:

```text
  RX 6000 и более старые карты получают его только по желанию, с `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (вкладка Upscaling: **Offer Ray Regeneration on this GPU (restart)**, с пометкой "preview"), тоже с AMDNR Ray Denoiser. **Резкость после RR**
```

**Round 4 - FAQ "You cannot find the Ray Regeneration settings?"** - round-3 lines 825-832 -> final line 815. Old:

```text
работал); строка **Ray Regeneration** на вкладке Neural тогда предлагает кнопку **Open Ray Regeneration**. Пока он не
работает, вкладка не рисуется, а эта строка на вкладке Neural говорит почему: игра не включила Ray Reconstruction, Ray
Regeneration отказался от этой игры и почему, или сколько секунд назад он работал в последний раз. На RX 7000 ещё
одна приглушённая строка добавляет, что там он предлагается как preview: у денойзера AMD нет провайдера для RDNA 3,
поэтому как только игра включает Ray Reconstruction, её картинка апскейлится без шумоподавления, а
`[FSR-RR] FfxDenoiserAllowPreRdna4=false` (или снятый флажок на вкладке Upscaling
**Preview: Ray Regeneration on this card (restart)**) выключает preview, и игра остаётся со своим денойзером. На RX 6000 и старше она говорит, что на этой видеокарте он не предлагается, что AMD
выпускает денойзер для RDNA 4, и что `[FSR-RR] FfxDenoiserAllowPreRdna4=true` предлагает его всё равно. Чтобы он
```

New:

```text
работал); с 0.3.5.3 на вкладке Neural нет строки Ray Regeneration. Пока он не работает, вкладка не рисуется; если Ray Regeneration выключен в этой игре или не смог запуститься на этой видеокарте, об этом говорит оранжевая строка под рантаймом на вкладке Neural. На RX 7000 и APU RDNA 3 / 3.5 Ray Regeneration предлагается как preview, включённый по умолчанию, а шум убирает AMDNR Ray Denoiser (preview) (AMD FSR Ray Regeneration сделан для RDNA 4); `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (или снятый флажок на вкладке Upscaling **Preview: Ray Regeneration on this card (restart)**) выключает preview, и игра остаётся со своим денойзером. На RX 6000 и старше он по умолчанию не предлагается: флажок на вкладке Upscaling **Offer Ray Regeneration on this GPU (restart)** (`[FSR-RR] FfxDenoiserAllowPreRdna4=true`) включает его, с AMDNR Ray Denoiser (preview). Чтобы он
```

**Round 4 - Grain FAQ: "AMDNR's own denoiser is 0.3.6 work"** - round-3 lines 857-858 -> final line 840. Old:

```text
Reconstruction не поддерживается, и она оставляет свой денойзер (после перезапуска). Собственный денойзер AMDNR —
работа для 0.3.6.
```

New:

```text
Reconstruction не поддерживается, и она оставляет свой денойзер (после перезапуска). С 0.3.5.3 та же строка предлагает и собственный денойзер AMDNR — AMDNR Ray Denoiser (preview), который Automatic выбирает на всех видеокартах, кроме RX 9000.
```

### README.zh-CN.md (round 4)

**Round 4 - "New in 0.3.5" note: RX 7000 without a denoiser** - round-3 line 26 -> final line 26. Old:

```text
> preview 形式提供并默认开启（AMD 的降噪器没有面向 RDNA 3 的提供程序，因此画面在不带降噪的情况下被升采样；Upscaling 选项卡中的
```

New:

```text
> preview 形式提供并默认开启（自 0.3.5.3 起，在那里由 AMDNR Ray Denoiser（preview）降噪；Upscaling 选项卡中的
```

**Round 4 - The other tabs: the Denoiser backend row** - round-3 lines 487-488 -> final line 487. Old:

```text
紧跟在 Upscaling 之后：状态行（按显卡、按 API）、**Denoiser backend** 行（Automatic / Off——Off 会告诉游戏光线重建不受
支持，于是游戏保留自己的降噪器；重启后生效）、各项控件、More Ray Regeneration options，以及它自己的 Diagnostics 区块，
```

New:

```text
紧跟在 Upscaling 之后：状态行（按显卡、按 API）、**Denoiser backend** 行（Automatic / AMD FSR Ray Regeneration / AMDNR Ray Denoiser (preview) / Off (the game's own)：Automatic 在 RX 9000 上用 AMD 的降噪器，在其他所有显卡上用 AMDNR Ray Denoiser；前三项可在游戏运行时切换；Off 会告诉游戏光线重建不受支持，于是游戏保留自己的降噪器，重启后生效）、各项控件、More Ray Regeneration options，以及它自己的 Diagnostics 区块，
```

**Round 4 - Feature bullet, first line: RX 7000 without a denoiser** - round-3 line 541 -> final line 540. Old:

```text
- **FSR Ray Regeneration** —— 在 RX 9000（RDNA 4）上提供；在 RX 7000（RDNA 3）上作为 preview 提供，默认开启，不带降噪器（见下文）；仅在使用 DLSS 光线重建的游戏中（Cyberpunk 2077、Alan Wake 2），且游戏
```

New:

```text
- **FSR Ray Regeneration** —— 在 RX 9000（RDNA 4）上提供；在 RX 7000（RDNA 3）上作为 preview 提供，默认开启，由 AMDNR Ray Denoiser 降噪（见下文）；仅在使用 DLSS 光线重建的游戏中（Cyberpunk 2077、Alan Wake 2），且游戏
```

**Round 4 - Feature bullet: Neural-tab pointer and RrBackend** - round-3 lines 544-547 -> final line 543. Old:

```text
  紧跟在 Upscaling 之后，在 Ray Regeneration 于游戏中运行时绘制；Neural 选项卡会指向它，并在 Ray Regeneration 没有运行时
  保留那行说明原因的浅色文字（游戏没有开启光线重建、驱动在这块显卡上拒绝了降噪器、Ray Regeneration 放弃了这款游戏以及
  原因，或它上一次运行的时间）。该选项卡的 **Denoiser backend** 行（`[FSR-RR] RrBackend = auto | off`）可以告诉游戏光线
  重建不受支持，于是游戏保留自己的降噪器（在下次启动游戏时生效）。在 **Vulkan** 游戏中，光线重建按设计为 "not
```

New:

```text
  紧跟在 Upscaling 之后，在 Ray Regeneration 于游戏中运行时绘制；自 0.3.5.3 起 Neural 选项卡上已没有 Ray Regeneration 行，只有当 Ray Regeneration 在某款游戏中被关闭或无法在这块 GPU 上启动时，运行时下方的橙色提示行会说明。该选项卡的 **Denoiser backend** 行（`[FSR-RR] RrBackend` = auto / amd / amdnr / off）提供 Automatic（RX 9000 上为 AMD FSR Ray Regeneration，其他所有显卡上为 AMDNR Ray Denoiser (preview)）、AMD FSR Ray Regeneration、AMDNR Ray Denoiser (preview) 和 Off (the game's own)；前三项可在游戏运行时切换，Off 会告诉游戏光线重建不受支持，于是游戏保留自己的降噪器（在下次启动游戏时生效）。在 **Vulkan** 游戏中，光线重建按设计为 "not
```

**Round 4 - Feature bullet: RX 7000 "no provider for RDNA 3"** - round-3 lines 553-554 -> final line 549. Old:

```text
  （0.3.5.1；0.3.5 未默认提供）：游戏可以开启光线重建，Ray Regeneration 的设置也会显示，但 AMD 的降噪器没有面向 RDNA 3
  的提供程序，因此降噪器会拒绝启动，光线重建画面在不带降噪的情况下被升采样，看起来可能比游戏自己的降噪更嘈杂；
```

New:

```text
  （0.3.5.1；0.3.5 未默认提供）：游戏可以开启光线重建，Ray Regeneration 的设置也会显示，并由 AMDNR Ray Denoiser（preview）降噪，在 RDNA 3 / 3.5 APU 上也是如此（AMD FSR Ray Regeneration 是为 RDNA 4 打造的）；
```

**Round 4 - Feature bullet: "planned" and the RX 6000 box** - round-3 lines 557-559 -> final line 552. Old:

```text
  游戏便保留自己的降噪器。AMDNR 自己的 RX 7000 降噪器已在计划中。
  RX 6000 及更早的显卡只有设为 `[FSR-RR] FfxDenoiserAllowPreRdna4=true` 才会提供（Upscaling
  选项卡：**Offer FSR Ray Regeneration on this GPU (restart)**）。**RR 之后的锐化**（0.3.4.1）：当游戏没有传入锐化值时，
```

New:

```text
  游戏便保留自己的降噪器。RX 6000 及更早的显卡只作为可选项提供，需设为 `[FSR-RR] FfxDenoiserAllowPreRdna4=true`（Upscaling 选项卡：**Offer Ray Regeneration on this GPU (restart)**，带 "preview" 标签），同样由 AMDNR Ray Denoiser 降噪。**RR 之后的锐化**（0.3.4.1）：当游戏没有传入锐化值时，
```

**Round 4 - FAQ "You cannot find the Ray Regeneration settings?"** - round-3 lines 699-705 -> final line 692. Old:

```text
之后，在 Ray Regeneration 于游戏中运行时绘制（一旦运行过，本次会话内会一直保留）；此时 Neural 选项卡中的 **Ray
Regeneration** 一行会提供一个 **Open Ray Regeneration** 按钮。它没有运行时，该选项卡不会绘制，Neural 选项卡中的那一行
会说明原因：游戏没有开启光线重建、Ray Regeneration 放弃了这款游戏以及原因，或者它上一次运行是多少秒前。在 RX 7000 上，
还有一行浅色文字会补上：那里以 preview 形式提供它，AMD 的降噪器没有面向 RDNA 3 的提供程序，因此一旦游戏开启光线重建，其画面就会
在不带降噪的情况下被升采样，而 `[FSR-RR] FfxDenoiserAllowPreRdna4=false`（或取消勾选 Upscaling 选项卡中的
**Preview: Ray Regeneration on this card (restart)**）会关闭该 preview，让游戏保留自己的降噪器。在 RX 6000 及更早的显卡上则说明：
这块显卡上不提供它，AMD 只为 RDNA 4 发布该降噪器，而 `[FSR-RR] FfxDenoiserAllowPreRdna4=true` 仍可让它提供。
```

New:

```text
之后，在 Ray Regeneration 于游戏中运行时绘制（一旦运行过，本次会话内会一直保留）；自 0.3.5.3 起 Neural 选项卡上已没有 Ray Regeneration 行。它没有运行时，该选项卡不会绘制；如果 Ray Regeneration 在这款游戏中被关闭或无法在这块 GPU 上启动，Neural 选项卡中运行时下方的橙色提示行会说明。在 RX 7000 以及 RDNA 3 / 3.5 APU 上，Ray Regeneration 以 preview 形式提供并默认开启，由 AMDNR Ray Denoiser（preview）降噪（AMD FSR Ray Regeneration 是为 RDNA 4 打造的）；`[FSR-RR] FfxDenoiserAllowPreRdna4=false`（或取消勾选 Upscaling 选项卡中的 **Preview: Ray Regeneration on this card (restart)**）会关闭该 preview，让游戏保留自己的降噪器。在 RX 6000 及更早的显卡上默认不提供：Upscaling 选项卡中的 **Offer Ray Regeneration on this GPU (restart)**（`[FSR-RR] FfxDenoiserAllowPreRdna4=true`）可让它提供，同样由 AMDNR Ray Denoiser（preview）降噪。
```

**Round 4 - Grain FAQ: "AMDNR's own denoiser is 0.3.6 work"** - round-3 line 721 -> final line 708. Old:

```text
行可以设为 Off，它会告诉游戏光线重建不受支持，于是游戏保留自己的降噪器（重启后生效）。AMDNR 自己的降噪器属于 0.3.6 的工作。
```

New:

```text
行可以设为 Off，它会告诉游戏光线重建不受支持，于是游戏保留自己的降噪器（重启后生效）。自 0.3.5.3 起，同一行还提供 AMDNR 自己的降噪器 AMDNR Ray Denoiser（preview），Automatic 在除 RX 9000 以外的所有显卡上都使用它。
```


## 5. Not changed - for the owner (round 4 sweep: doubtful, listed instead of changed)

1. **Credits miss lmxxf 0.41.** The pak adds lmxxf 0.41's kernels in 0.3.5.3 (CHANGELOG), and the new NOTICE credits
   "lmxxf 0.41 by Kien (MIT)", but every README's credits line still names only "lmxxf 0.37 and 0.39 kernels"
   (README.md "- **lmxxf 0.37 and 0.39 kernels**"). Attribution, so the owner's wording.
2. **"0.3.5 on RX 9000: lmxxf 0.37 by Kien (MIT) is on by default"** (README.md lmxxf section, and the "New in 0.3.5"
   note): labelled 0.3.5 and true then; 0.3.5.3's default is 0.41. No 0.3.5.3 line exists to supersede it.
3. **Satisfactory FAQ** ("The game's Ray Reconstruction is on but the Neural tab says "Ray Regeneration is off in this
   title"?": empty camera matrices, "it stays off for the whole session"): 0.3.5.3 runs Ray Regeneration in games that
   give no camera (Hogwarts Legacy, Satisfactory) per the CHANGELOG. Not a default; likely out of date.
4. **Vulkan** ("Ray Reconstruction ... answered "not supported" by design", Linux / Proton list and the feature bullet):
   still true by default, but 0.3.5.3 adds the opt-in `[FSR-RR] RrVulkan=true`.
5. **RX 6000 NR** ("RX 6000 (RDNA 2) is planned for 0.3.6", handheld section and the requirements list): the
   CHANGELOG fixes RX 6000 runtime messages and HIP error 126, so RX 6000 players run NR with danielblnc 0.6.0; whether
   the README says RX 6000 is supported is the owner's call (the launcher manifest row is still commented).
6. **The grain FAQ's "No denoiser default ... changed in 0.3.4.2"** is about 0.3.4.2 and true; 0.3.5.3 does change five
   Ray Regeneration denoiser defaults (AMD's own values).
7. **The 0.3.6 line of the version list** still plans "the AMDNR denoiser (ARD) preview", which 0.3.5.3 ships.
8. No README text contradicts the other listed defaults (the NVIDIA look: the README never names a default look;
   FSR 4 (INT8) on RX 6000: not mentioned; the reflection ray length: not mentioned; XeSS pacing at 3X+: the README's
   "Extra pacing" sentence is about 6X+ and still true).
9. **History left as it is:** the "New in 0.3.4" note, the 0.3.4 line of the version list, the inherited Screen-space
   GI credits line, and "Screen GI" in the Anywhere paragraph's list of rows refused inside Anywhere (still true).

## 6. Scratch files

`C:\Users\Administrator\AppData\Local\Temp\rel0353readme\`: spec.txt / build.py (round 1), spec2.txt / build2.py
(round 2), spec3.txt / build3.py (round 3), round1\ and round2\ (the earlier outputs), report*.json, templates, notes,
gen scripts. Not needed by the owner; safe to delete.
