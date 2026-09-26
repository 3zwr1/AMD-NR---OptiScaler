# AMDNR — DLSS 5 Neural Rendering على AMD (نسخة OptiScaler) — v0.3.3.2

[English](README.md) | [中文](README.zh-CN.md) | [Português](README.pt-BR.md) | [Español](README.es.md) | **العربية** | [Français](README.fr.md) | [Italiano](README.it.md) | [Русский](README.ru.md) | [Polski](README.pl.md)

<div dir="rtl">

> **نحتاج إلى دعمك.** انضم إلى سيرفر Discord — <https://discord.gg/AMDNR> — للحصول على
> المساعدة، والإبلاغ عن الأخطاء، والحصول على النسخ التجريبية؛ فكل بلاغ مرفق بملف سجل (log) يجعل النسخة القادمة أفضل.

تقنية DLSS 5 Neural Rendering تعمل على بطاقات AMD الرسومية، وهي مدمجة في OptiScaler لتعمل في أي
لعبة Direct3D 12 يرتبط بها OptiScaler أصلًا. وإلى جانب التمريرة العصبية ستجد: تشغيل النموذج بالتناوب
(model interleave) لزيادة كبيرة في معدل الإطارات، وتركيب التعديل المتبقي (residual composition)، وتوليد
إطارات XeSS مفتوحًا حتى 6X (وحتى 10X اختياريًا في ألعاب D3D12)، وتقنية FSR Ray Regeneration للألعاب
التي تستخدم DLSS Ray Reconstruction.

**سيرفر Discord: <https://discord.gg/AMDNR>** — الدعم، والإبلاغ عن الأخطاء (`#bug-report`)، والنسخ
التجريبية.

**ادعم المشروع: <https://ko-fi.com/3zinr>**

> **بيئة تشغيل danielblnc هي من عمل Daniel Blanco.** بيئة التشغيل العصبية لـ AMD الموجودة في `Runtime.zip`
> (`dlssnr_amd_pass1..3.dll`) هي **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> يوزّعها AMDNR دون أي تعديل وبإذن منه؛ وهي ليست من عمل AMDNR. رجاءً ادعم مشروعه.
> وتجد الشكر الكامل لجميع المساهمين الآخرين في نهاية هذه الصفحة.

---

## دليل تثبيت AMDNR - OptiScaler

التثبيت بسيط جدًا.

### 1. حمّل الملفات

حمّل هذين الملفين من GitHub (<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>):

* `AMDNR-vX.X.X.zip`
* `Runtime.zip`

### 2. فك ضغط الملفين

فك ضغط محتويات ملفي `.zip` كليهما.

### 3. انسخ كل شيء إلى مجلد اللعبة

أولًا، انسخ جميع الملفات من `AMDNR-vX.X.X` إلى المجلد الرئيسي للعبة — وهو المجلد نفسه الذي يوجد
فيه ملف `.exe` الخاص باللعبة.

بعد ذلك، كرر الأمر نفسه مع جميع الملفات من `Runtime`.

### 4. غيّر اسم OptiScaler.dll

داخل مجلد اللعبة، ابحث عن:

`OptiScaler.dll`

وغيّر اسمه إلى:

`dxgi.dll`

الاسم `dxgi.dll` هو الخيار الموصى به.

إذا لم تعمل اللعبة أو لم يتم تحميل المود، فجرّب بدلًا من ذلك تغيير اسم `OptiScaler.dll` إلى أحد
الأسماء التالية:

* `d3d12.dll`
* `winmm.dll`
* `version.dll`
* `dbghelp.dll`

جرّب اسمًا واحدًا في كل مرة. لا تنشئ عدة نسخ من `OptiScaler.dll`.

> **لعبة Resident Evil Requiem (ونسخة الديمو منها) تحتاج إلى REFramework.** هذا متطلب معروف وليس خطأً في AMDNR: يعتمد عليه OptiScaler
> لتجاوز حماية Capcom ضد التلاعب (anti-tamper) ([ويكي OptiScaler](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-9-Requiem)). وبدونه تنهار اللعبة بعد 15-60 ثانية
> من تشغيلها ("An unhandled exception occurred"). ضع الملف `dinput8.dll` الموجود في `REFramework.zip` من أحدث نسخة nightly
> (<https://github.com/praydog/REFramework-nightly/releases>) بجانب `dxgi.dll`، وغيّر مفتاح قائمة REFramework (إلى Delete مثلًا)، لأن مفتاحه هو Insert أيضًا.
> بعد أي تحديث للعبة، توقّع حدوث انهيارات إلى أن يتم تحديث REFramework. ومن المرجح أن PRAGMATA وMonster Hunter Wilds وOnimusha تحتاج إليه أيضًا (غير مؤكد).

### 5. شغّل اللعبة

يعمل المفتاح `HOME` على تشغيل Neural Rendering وإيقافه أثناء اللعب (في كلتا بيئتي التشغيل؛ ويظهر إشعار
صغير يقول On / Off). يمكنك تغيير هذا المفتاح من الخانة المجاورة لمربع الاختيار Enable في تبويب Neural، أو من
Interface > Keybinds.

هذا كل شيء.

شغّل اللعبة بشكل طبيعي واضغط:

`INSERT`

سيفتح ذلك قائمة OptiScaler / AMDNR، حيث يمكنك ضبط المود كما تشاء.

### إذا لم يعمل

إذا ظلت اللعبة لا تعمل مع أي من الأسماء السابقة، فالرجاء الإبلاغ عن ذلك في قناة
`#bug-report` على Discord.

وعند الإبلاغ عن المشكلة، ارفع أيضًا أي ملفات `.log` قد تكون أُنشئت في المجلد الرئيسي
للعبة.

ملفات السجل هذه مهمة جدًا، وستساعدنا على تحديد المشكلة بشكل أسرع بكثير.

> غالبًا لا يكون ملف `.exe` في المكان الذي يشير إليه الاختصار. ألعاب Unreal تضعه داخل
> `<Game>\Binaries\Win64\`.

---

### بيئة تشغيل lmxxf (0.3.0، اختيارية)

يمكن لبيئة تشغيل عصبية ثانية (برخصة MIT، من تطوير lmxxf) أن تتولى التمريرة بدلًا من بيئة
danielblnc. تعمل على RDNA 4، وعلى RDNA 3 (RX 7000 وStrix Halo) عبر واجهة RDNA 3 الخلفية (backend) الخاصة
بـ AMDNR - وهي أبطأ هناك: ابدأ بضبط NR resolution على 67%. تحتاج إلى شيئين بجانب اللعبة:

1. الملف `LmxxfNrRuntime.dll` - موجود في هذا الأرشيف، بجانب `OptiScaler.dll` (يُنسخ مع بقية الملفات).
2. الملف `LmxxfNrRuntime.pak` (416 MB، مضمّن في ملف zip الخاص بـ AMDNR) بجانب `LmxxfNrRuntime.dll` - ويضم
   ملفات أوزان lmxxf ووحدات HIP وHLSL في ملف واحد مشفّر وموثَّق (authenticated). تفتحه بيئة التشغيل في الذاكرة؛ ولا
   يُستخرج أي شيء إلى القرص.

في أول تشغيل تُكتشف فيه بيئة تشغيل مثبتة دون أن يكون قد تم اختيار أي بيئة، تسألك القائمة أي بيئة تشغيل تريد
استخدامها (يُحفظ الاختيار في ملف ini عبر `[DlssNr] NrBackend = daniel | lmxxf`؛ ويمكنك تغييره من
Neural > Neural runtime، ويسري التغيير عند تشغيل اللعبة التالي). يُطبَّق تعديل lmxxf متأخرًا بإطار
واحد، محمولًا عبر متجهات الحركة، لذلك لا ينتظر الإطار الشبكة أبدًا (نحو 17 ms بدقة 1080p على
RX 9070 XT). ملف السجل الخاص بها هو `lmxxf_backend.log` بجانب اللعبة.

**التوافق (lmxxf).** لا ترى بيئة التشغيل إلا ما تراه DLSS، لذا فإن ما يختلف من لعبة لأخرى قائمة
قصيرة: صيغة الألوان وHDR، ومتجهات الحركة ومقياسها، والعمق واتجاهه، والقناع التفاعلي (reactive mask)،
وخامة التعريض (exposure texture)، وعلامة Reset، وموضع التمريرة (قبل Super Resolution، أو بعد Ray
Reconstruction). ما تم اختباره حتى الآن:

| اللعبة | الواجهة البرمجية (API) / الموضع | ملاحظات |
|---|---|---|
| Silent Hill 2 | D3D12، قبل SR | اللعبة المرجعية؛ تمت معالجة التخصيص المُبطَّن (padded) لخامة الألوان في Unreal |
| Forza Horizon 6 | D3D12، قبل SR | |
| Stray | D3D11 عبر جسر D3D12، قبل SR | |
| GTA V Enhanced | D3D12، قبل SR، HDR، قناع تفاعلي بقناة واحدة | أُصلح في 0.3.0: كان القناع يُقرأ على أنه "كل شيء تفاعلي" فلم يكن التعديل يُطبَّق أبدًا |
| أي لعبة تستخدم Ray Reconstruction | D3D12، بعد RR (تُكتب النتيجة مجددًا في المخرجات) | مدعوم منذ 0.3.0؛ لم يُؤكَّد بعد في أي لعبة |

إذا لم يظهر أي تأثير في لعبة ما: يحتوي `lmxxf_backend.log` على سطر `lmxxf inputs:` (الصيغ، والأحجام،
ومقياس الحركة، واتجاه العمق، والقناع، والتعريض) وعلى سطر `lmxxf stats @N:` كل 600 إطار (التعريض،
والسطوع المُدخل، وتعديل النموذج، والتعديل المحمول، وقيمة keep، ومتوسط التفاعلية، وطول المتجهات،
ونسبة الرفض). أرفق ملف السجل مع البلاغ؛ فهذان السطران عادةً ما يوضحان السبب.

مع lmxxf، يحتوي قسم Neural runtime على **Network history** (المدخل الزمني الخاص بالنموذج نفسه)،
ويحتوي Image look على مجموعة **lmxxf edit**: مُشكِّل التعديل (Edit detail وEdit colour وEdge guard:
تضخيم الجزء الدقيق من تعديل النموذج، ولونه مقابل تغيّر سطوعه، وتلاشي التعديل عبر حواف العمق)
و**Output smoothing** (تمريرة المشروع الأصلي (upstream) على المخرجات، وتحتاج إلى Network history).
أما Neural passes وResidual strength/limit وزيادة الحدة (sharpening) وDebug view 1 ومرشح Appearance
فتنطبق على كلتا بيئتي التشغيل.

الخيار **Full network** (Neural > Performance، `[DlssNr] LmxxfFullNetwork`، لـ lmxxf فقط) يشغّل جميع
كتل الشبكة الـ 71 بدلًا من تخطي الكتل 42 و43 و46: أدق قليلًا، وأبطأ بنحو 0.5 ms بدقة 1080p
(من 16.6 إلى 17.1 ms على RX 9070 XT). معطّل افتراضيًا.

## المتطلبات

- بطاقة رسوميات AMD مع تعريف (driver) حديث. تستخدم بيئة التشغيل العصبية HIP عبر التعريف؛ فلا حاجة
  إلى HIP SDK ولا إلى وضع المطوّر. بحسب الشريحة: سلسلة RX 9000 (RDNA 4) تشغّل كلتا بيئتي التشغيل؛
  وسلسلة RX 7000 (RDNA 3، للأجهزة المكتبية والمحمولة) تشغّل الاثنتين أيضًا - lmxxf عبر واجهة RDNA 3 الخلفية
  الخاصة بـ AMDNR، وبأداء أبطأ من RDNA 4؛ وStrix Halo (8060S / 8050S) تشغّل lmxxf؛ أما معالجات APU في
  أجهزة الألعاب المحمولة (Z1 Extreme / 780M، Z2 Extreme / 890M) وRDNA 2 (RX 6000، Steam Deck) فلا تدعمها
  أي من البيئتين.
  يوضح لك تبويب Neural ما يمكن لبطاقتك تشغيله.
- لعبة تعمل بـ Direct3D 12 أو Direct3D 11 أو Vulkan. المسار العصبي لـ AMD نفسه يعمل على D3D12؛ وتصل
  إليه ألعاب D3D11 وVulkan عبر جسر D3D12 في OptiScaler، ما يعني أن أداة رفع الدقة (upscaler) يجب أن
  تكون إحدى الواجهات الخلفية "w/Dx12" (`ffx_12`). اترك `Dx11Upscaler` / `VulkanUpscaler` على `auto`
  وستختارها هذه النسخة لك عند تفعيل العرض العصبي (neural rendering).
- نحو 2 GB من ذاكرة VRAM المتاحة عند دقة رسم (render resolution) من فئة 1080p.

## محتويات الأرشيفين

**AMDNR-vX.X.X.zip**

| الملف | ما هو |
|---|---|
| `OptiScaler.dll` | OptiScaler مع واجهة AMD الخلفية لـ DLSS-NR. غيّر اسمه كما يوضح الدليل. |
| `OptiScaler.ini` | الإعدادات. Neural Rendering مفعّل؛ والتسجيل (logging) مفعّل حتى يكون لديك ما ترفقه ببلاغ الخطأ. |
| `LmxxfNrRuntime.dll` | بيئة التشغيل العصبية lmxxf (مع kernels الإصدار 0.29 من lmxxf). لا تُستخدم إلا عند اختيارها؛ وتقرأ `LmxxfNrRuntime.pak` الموجود بجانبها، راجع قسم "بيئة تشغيل lmxxf". |
| `LmxxfNrRuntime.pak` | أوزان بيئة تشغيل lmxxf ووحدات HIP والـ shaders الخاصة بها في ملف واحد مشفّر (416 MB). لا تقرؤه إلا بيئة تشغيل lmxxf؛ ولا ضرر من إبقائه مع بيئة تشغيل danielblnc. |
| `OptiScaler\` | FSR وXeSS ومزيل التشويش FidelityFX وD3D12 Agility SDK التي يستخدمها OptiScaler. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | أداة dlssg-to-fsr3 من Nukem9، دون تعديل مع إعادة تسميتها: تُلبّى استدعاءات DLSS Frame Generation الصادرة من اللعبة بواسطة توليد الإطارات في FSR 3، وعلى Vulkan أيضًا (`FGNvngxReplacement=Nukems`). برخصة GPLv3، راجع `Licenses/`. |
| `Licenses\`، `LICENSE` | تراخيص الأطراف الثالثة، وإشعار AMDNR (`AMDNR_NOTICE.txt`)، وترخيص GPL-3.0 لهذه النسخة. |
| `SHA256SUMS.txt` | المجاميع الاختبارية (checksums) لكل ملف مرفق، في كلا الأرشيفين. |

**Runtime.zip**

| الملف | ما هو |
|---|---|
| `dlssnr_amd_pass1..3.dll` | بيئة التشغيل العصبية لـ AMD، الإصدار v0.3.1 من danielblnc، دون تعديل. ثلاث نسخ حتى يكون لكل تمريرة نسختها الخاصة في وضع التمريرات المتعددة (multi-pass). |
| `dlssnr_on_amd_weights.bin` | أوزان الشبكة التي تحمّلها بيئة التشغيل. |

## إعدادات يجدر بك معرفتها

افتح تبويب **Neural**. القيم الافتراضية هي أحدث ترتيب تم اختباره، لذا فالخطوة الأولى المفيدة هي
تغيير شيء واحد في كل مرة.

- **دقة NR (NR resolution)** — أداة التحكم الرئيسية في الموازنة بين الجودة والتكلفة. تحت 100% يعمل
  النموذج على صورة أصغر، ولا يُنقل إلا *تصحيحه* إلى الإطار بالدقة الكاملة، فيحتفظ الإطار بتفاصيله
  الأصلية. وفوق 100% تزداد التكلفة بمربع النسبة (150% تعني 2.25x). يتحرك شريط التمرير بخطوات قدرها
  5%: وكل حجم NR جديد قد يحتجز جزءًا من VRAM إلى أن يُعاد تشغيل اللعبة، لذا أعد تشغيل اللعبة بعد
  تغييرات كثيرة.
- **قوة التعديل المتبقي (Residual strength)** — مقدار ما يُطبَّق من تعديل النموذج؛ وفوق 1 يُضخّمه.
  هذا هو الإعداد الأكثر تأثيرًا في الصورة.
- **حد التعديل المتبقي (Residual limit)** — سقف لمقدار ما يمكن أن يتغير به البكسل الواحد. إذا ظهرت
  بقع غير متجانسة: **اخفضه**.
- **تناوب النموذج (Model interleave)** — يشغّل النموذج مرة كل إطارين لزيادة كبيرة في معدل الإطارات.
  أما الإطارات المتخطاة فيملؤها **Interleave preset**؛ والخيار *Guided fill v2* هو الافتراضي وهو الذي
  يجري العمل عليه حاليًا. ضبط توقيت نوعي الإطارات (pacing) تلقائي، ويعمل **Adaptive interleave**
  (مفعّل افتراضيًا) على تشغيل النموذج في كل إطار ما دامت الصورة تتحرك، بحيث لا يحدث التخطي - وما
  يرافقه من تشوهات بصرية - إلا عندما تكون الصورة ثابتة.
- **التمريرات العصبية (Neural passes)** — القيمتان 2 و3 تكدّسان النموذج فوق نفسه، مع عائد متناقص.
  مع lmxxf يبقى سجل الشبكة (history) في تمريرتها الأولى؛ والتمريرات الإضافية مجرد تحسين مكاني.
  أما danielblnc فيشغّل تمريرة واحدة في ألعاب Vulkan (وتوضح ذلك ملاحظة أسفل شريط التمرير).
- **تركيب الألوان (Colour composition)** (Neural > Image look، في كلتا بيئتي التشغيل) — الوضع *Classic*
  (الافتراضي) هو الصورة التي كانت لديك من قبل. أما *RenoDX (experimental)* فيشغّل تركيب الألوان الخاص
  بـ RenoDX بعد النموذج، كما يفعل مسار NVIDIA، ويضم: Composition detail وcolour، و**Highlight guard**
  ثنائي الاتجاه (2x افتراضيًا) يقيّد ناتج النموذج قياسًا بالصورة الأصلية، وعناصر تحكم اختيارية للبشرة /
  البيئة. في إطار من نوع display-referred (SDR) يعود إلى Classic، مع ملاحظة في القائمة. ولا تغيّره
  أنماط NR وإعداداتها المسبقة (presets).
- **توليد الإطارات معطّل في ملف ini جديد.** في تبويب Frame Gen: اختر FG Input (مثل "DLSSG via
  Streamline" في لعبة تدعم توليد الإطارات من DLSS) وFG Output (XeFG)، ثم فعّل **Active** في قسم
  Frame Generation (XeFG) واضغط Save Settings. إذا كان هذا الخيار مفعّلًا في ملف ini من الإصدار 0.1.0، فلن
  يُنقل عند تثبيت ملف ini الخاص بالإصدار 0.2.0.
- **توليد الإطارات المتعددة في XeFG (multi-frame generation)** — من 3X إلى 6X مدمج ومفعّل افتراضيًا
  (`XeFG\UnlockMFG`)، سواء لنسخة OptiScaler أو للنسخة الخاصة باللعبة نفسها. **احذف `XeFGUnlock.asi`**
  من `OptiScaler\plugins` إن كان لا يزال لديك: وجود نسختين من الباتش (patch) نفسه يؤدي إلى انهيار اللعبة.
  **الوصول حتى 10X اختياري** (لألعاب D3D12 فقط): اضبط *XeFG ceiling (restart)* تحت FG Output في
  تبويب Frame Gen (4X، أو 6X وهو الافتراضي، أو 8X أو 10X؛ `[XeFG] MaxInterpolatedFrames`)، ثم أعد تشغيل
  اللعبة، ثم اختر المضاعِف من قائمة MFG المنسدلة. فوق 6X يحتاج الأمر إلى مزوّد XeFG الخاص بـ OptiScaler
  مع تفعيل Extra pacing؛ أما نسخة XeSS 3 الخاصة باللعبة فتبقى عند 6X كحد أقصى. يتطلب 10X شاشة بتردد
  360 Hz أو أعلى وتحديد سقف لمعدل الإطارات يساوي معدل التحديث / 10؛ زمن الاستجابة (latency) مرتفع،
  ويحجز المزوّد نحو 128 MiB إضافية من VRAM بدقة 4K.
  لم يتم تأكيد 7X-10X في أي لعبة بعد: نرجو من المختبرين إرسال `OptiScaler.log`.
- **تقنية FSR Ray Regeneration** — مقتصرة افتراضيًا على RDNA 4 (RX 9000)؛ وتعمل فقط في الألعاب التي تستخدم
  DLSS Ray Reconstruction (Cyberpunk 2077، Alan Wake 2)، بشرط أن تكون اللعبة تشغّل DLSS (مع تفعيل spoofing)، وأن يكون تتبع الأشعة وRay
  Reconstruction مفعّلين في إعدادات اللعبة نفسها. عندها يعمل Neural Rendering بعدها، على مخرجاتها، وهذا
  يكلّف أكثر: اخفض NR resolution إذا انخفض معدل الإطارات. تظهر عناصر التحكم الخاصة بها (Neural >
  Quality > Ray Regeneration) فقط أثناء تشغيل اللعبة لـ Ray Reconstruction. أما **ملف تعريف تتبع
  المسار (path-traced profile)** (حبيبات أقل على الوجوه مع path tracing) فهو اختياري منذ 0.3.3.1:
  فعّله من هناك لتجربته في Resident Evil Requiem أو PRAGMATA. وفي المكان نفسه ستجد قوة bias mask، وعرض
  تصحيح (debug view) لـ RR، و**تنعيم البشرة (skin smoothing)** (تجريبي، للألعاب التي توفّر خامة توجيه SSS (SSS guide)؛
  معطّل افتراضيًا، لكنه مفعّل افتراضيًا في Resident Evil Requiem منذ 0.3.3.2).

## إذا حدث خطأ ما

يظهر الملف `OptiScaler.log` في مجلد اللعبة. أرفقه في `#bug-report`، واذكر اسم اللعبة ونوع بطاقة
الرسوميات. كما تكتب واجهة AMD الخلفية الملفين `amd_presr.log` و`amd_bridge.log`، وهما المفيدان عندما
تكون المشكلة في التمريرة العصبية تحديدًا. ويُحتفظ بسجل الجلسة السابقة باسم
`OptiScaler.previous.<exe>.log`؛ وبعد أي انهيار أرفقه أيضًا (سيذكر السجل الجديد حينها "no clean exit
recorded").

**هل يظهر NR frames 0/s، ويقول تبويب Neural أو `amd_presr.log` إن ملف DLL الخاص بالتمريرة نسخة لا
يشغّلها هذا الإصدار من AMDNR؟** ملفات `dlssnr_amd_pass1..3.dll` لديك هي نسخة من danielblnc لا يعرفها هذا الإصدار من AMDNR
(رُصدت مجموعة من الإصدار 0.2.16 متداولة بين اللاعبين)، أو أن أحد الملفات الثلاثة مفقود. منذ 0.3.3.2 يذكر تبويب Neural
اسم الملف وإصداره ويخبرك بما عليك فعله. استخدم `v0.4.0-Runtime.zip` (الأحدث) أو `Runtime.zip` (0.3.1)
من هذا الإصدار، على أن تكون ملفات DLL الثلاثة كلها من ملف zip نفسه: حجم `dlssnr_amd_pass1.dll` الموجود
في `v0.4.0-Runtime.zip` هو 10,027,008 بايت، وبصمة SHA256 الخاصة به تبدأ بـ `d62be3d8`. النسخ المدعومة:
0.2.17، 0.3.0، 0.3.1، 0.3.2، 0.3.3، 0.4.0، و0.4.1 / 0.4.2 قبل صدورهما. لا تثبّت برنامج الإعداد الخاص
بـ danielblnc ولا ملفاته `dxgi.dll` / `version.dll` / `winhttp.dll` بجانب AMDNR: فـ AMDNR يشغّل بيئة
التشغيل الخاصة به بالفعل.

**هل lmxxf لا يفعل شيئًا، أو يتوقف فورًا، على جهاز فيه رسوميات مدمجة؟** أُصلح في 0.3.3.2. على جهاز
مكتبي بمعالج Ryzen مع تفعيل رسومياته المدمجة، أو حاسوب محمول فيه معالج AMD APU مع بطاقة Radeon، أو جهاز
فيه بطاقتا رسوميات من AMD، فإن بطاقة الرسوميات التي تستخدمها اللعبة غالبًا لا تكون جهاز HIP رقم 0.
عندها كان lmxxf يفشل في أول إطار له (`hipErrorInvalidHandle (400)`، ثم "session is poisoned" في
`lmxxf_backend.log`) ويبقى متوقفًا. استبدل كلًّا من `OptiScaler.dll` (الملف الذي غيّرت اسمه، مثل
`dxgi.dll`) و`LmxxfNrRuntime.dll` بملفات الإصدار 0.3.3.2. لم يُختبر هذا بعد على جهاز كهذا: إذا ظل
lmxxf يتوقف، فتبويب Neural يخبرك الآن بالسبب؛ أرسل `lmxxf_backend.log` و`amd_bridge.log` (فهو يسرد
أجهزة HIP).

**هل تتوقف لعبة Vulkan (Indiana Jones and the Great Circle) عند البدء برسالة "Could not create the Vulkan
device (VK_ERROR_EXTENSION_NOT_PRESENT)"؟** أُصلح في 0.3.2: كان المسار العصبي الموروث من NVIDIA يطلب من
تعريف AMD امتدادين للجهاز (device extensions) خاصين بـ NVIDIA فقط. تصل ألعاب Vulkan إلى التمريرة العصبية
عبر جسر D3D12 في OptiScaler (راجع قسم المتطلبات).

**هل تجمّدت لعبة Vulkan مع lmxxf عند أول إطار NR؟** أُصلح في 0.3.3؛ توقّع تقطيعًا واحدًا (hitch) يدوم ثانية
تقريبًا عند بدء NR. وإذا توقفت جلسة Vulkan في أي وقت قبل أول استجابة من lmxxf، فسيعمل التشغيل التالي
ببيئة تشغيل danielblnc وسيخبرك تبويب Neural بالسبب؛ اضغط **Retry lmxxf** هناك (يحذف
`lmxxf_vk_launch.pending` الموجود بجانب `OptiScaler.dll`) لتجربة lmxxf مرة أخرى.

**هل توقف danielblnc لعدة ثوانٍ ثم أوقف NR، في لعبة Vulkan (Indiana Jones) مع ضبط Neural passes على
2-3؟** أُصلح في 0.3.3: في ألعاب Vulkan يشغّل تمريرة واحدة، وأُزيل انتظاره البالغ 80 ms بعد الإرسال
(post-submit). لا يزال أول إطار NR في كل جلسة يتوقف نحو 5 ثوانٍ؛ وتشرح ملاحظة أسفل خيار بيئة التشغيل
سطور السجل المتعلقة به. للمختبرين: من المتوقع أن يزيل `[DlssNr] AmdVkLateCopyWait=true` (تجريبي، معطّل
افتراضيًا، ولم يُختبر في أي لعبة بعد) هذا التوقف؛ أرسلوا `OptiScaler.log` و`amd_presr.log`
و`dlssnr_on_amd.log`.

**هل كان استهلاك lmxxf لذاكرة RAM يرتفع طوال مدة تشغيل NR؟** أُصلح في 0.3.3 (كان نحو 45 GB في الساعة
عند 60 NR fps). ما تبقّى: يحتجز danielblnc جزءًا من VRAM لكل حجم NR جديد يتجاوز نحو 1 MP (يقرّب الإصدار
0.3.3.2 أحجامه بخطوات قدرها 64 px عندما لا تكون عند 100%، لذا لا يوجد منها إلا القليل)؛ أعد تشغيل اللعبة
بعد تغييرات كثيرة مع danielblnc. ومنذ 0.3.3.2 لم يعد lmxxf يحتجز نحو 97 MB مع كل تغيير في NR resolution أو
في وضع DLSS: فهو ينشئ المخازن المؤقتة (buffers) الخاصة بشبكته مرة واحدة لكل حجم شبكة ثم يعيد استخدامها
(ويبقى قدر صغير يبلغ نحو 10-25 MB من VRAM مع كل تغيير).

**هل تفشل لعبة تستخدم Streamline عند البدء بالخطأ slInit error 0x18 (لوحظ مع NBA 2K27 على AMD)؟** يغلق
الإصدار 0.3.3 إحدى الطرق التي قد تسببه بها خطافات (hooks) إضافة Streamline في OptiScaler، لكن لم يُؤكَّد
أن هذا هو السبب في NBA 2K27. يسجّل `OptiScaler.log` الآن سطور `slInit returned ...` و`[SLINIT]`: أرسل
ملف السجل مع البلاغ.

**هل Ray Reconstruction مفعّل في اللعبة لكن تبويب Neural يقول "Ray Regeneration is off in this title"؟**
اللعبة لا توفّر ما تحتاج إليه FSR Ray Regeneration (في Satisfactory: لا توجد مصفوفات الكاميرا). يعمل رفع
الدقة FSR بدلًا منها ويأخذ NR موضعه المعتاد قبل SR؛ ولا يوجد في ملف ini ما يغيّر ذلك.

**هل تظهر في لعبة من ألعاب محرك Ubisoft Anvil (AC Black Flag Resynced، Shadows، Mirage) رسالة "DX12 Error
0x80070057"؟** هذه الألعاب تأتي بتقنية XeSS Frame Generation الخاصة بها. وهذه النسخة تترك لها هذه المهمة
(يتنحّى مخرج XeFG في OptiScaler هناك، ويوضح تبويب Frame Gen ذلك)؛ استخدم خيار XeSS FG الموجود في اللعبة
نفسها. إذا استمرت المشكلة، اضبط `[FrameGen] Enabled=false` و`[fakenvapi] ForceXeLL=false` ثم أبلغ عنها
مع ملف السجل.

**هل تنهار The Last of Us Part I عند الإقلاع؟** السبب هو تهيئة Streamline الخاصة باللعبة نفسها، وهي
مشكلة معروفة في OptiScaler: غيّر اسم `sl.common.dll` في مجلد اللعبة إلى `sl.common.dll.bak` واختر
**FSR 3.1** في إعدادات اللعبة بدلًا من DLSS.

الملاحظات الكاملة لكل إصدار: `CHANGELOG.md` (داخل ملف zip وفي المستودع).

## خارطة الطريق

- **0.3.3** (هذه النسخة) — lmxxf على RDNA 3 (RX 7000؛ عبر واجهة AMDNR الخلفية الخاصة)؛ تركيب الألوان
  RenoDX (تجريبي، اختياري) في كلتا بيئتي التشغيل؛ lmxxf: خيار Full network، وإصلاح تسريب RAM، وإصلاح
  ألعاب Vulkan (رفع كسول للأوزان (lazy weight upload) داخل جسر Vulkan)، وkernels الإصدار 0.29 (مطابقة
  بتًا ببت (bit-exact)، وأسرع)؛ danielblnc في ألعاب Vulkan: تمريرة Neural واحدة، ورسائل أوضح، وانتظار
  نسخ متأخر اختياري (late copy wait)؛ XeFG حتى 10X (اختياري، D3D12)؛ تحصين بدء تشغيل Streamline
  وتشخيصاته؛ ملف تعريف تتبع المسار (path-traced profile) وتنعيم البشرة في FSR Ray Regeneration؛ متانة أكبر في UE5.
- **0.3.2** — معالجة بلاغات 0.3.1: ألعاب Vulkan تبدأ وتعمل مع lmxxf، ومطابقة ألوان lmxxf مع ألوان
  danielblnc (التعريض التلقائي auto-exposure)، وقائمة اختيار بيئة التشغيل، وحالة Ray Reconstruction
  وضبطها؛ وأداة dlssg-to-fsr3 من Nukem9 ضمن ملف zip لتوليد الإطارات على Vulkan.
- **0.3.1** — إصلاحات من أولى بلاغات 0.3.0 (lmxxf لم يكن يعمل وحده أبدًا، وعدم ظهور أي أثر لـ NR في
  Where Winds Meet، والانهيار عند تغيير جودة DLSS، وGTA V Legacy) وإعدادات مسبقة لأنماط NR مع ثلاث
  خانات مخصصة.
- **0.3.0** — بيئة التشغيل العصبية **lmxxf** المبنية على HIP (RDNA 4) كبيئة تشغيل قابلة للاختيار إلى
  جانب بيئة danielblnc، وتأتي على شكل `LmxxfNrRuntime.dll` + `LmxxfNrRuntime.pak`: سجل الشبكة (network
  history)، وNeural passes حقيقية، ومُشكِّل التعديل، والتموضع بعد Ray Regeneration، وتشخيصات لكل لعبة،
  وإصلاح ذاتي. شكرًا جزيلًا لـ TheAutomatic، الذي يُبنى هذا الدمج على عمله في مشروع DLSS 5 AMD.
- **0.4.0** — برنامج AMDNR Launcher (تثبيت بيئات التشغيل وملف pak بنقرة واحدة، والتحديثات) ودعم
  الألعاب التي لا تملك أداة رفع دقة خاصة بها (من فئة Stray)، حيث يوفّر OptiScaler أداة رفع الدقة
  والتمريرة العصبية معًا.

---

## شكر وتقدير

هذه النسخة عمل ربط وتوصيل مبني على أعمال أشخاص آخرين. إذا وجدتها مفيدة، فالشكر يعود إلى أصحاب
المشاريع الأصلية (upstream).

- **TheAutomatic** — مشروع DLSS 5 AMD — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD — https://github.com/danielblnc/DLSS-NR-on-AMD (`Runtime.zip`، دون تعديل)
- **lmxxf** — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (بيئة تشغيل HIP، برخصة MIT)
- **c32w kernels** (0.3.3.2) — kernels خاصة بـ AMDNR تعمل بموجة واحدة (one-wave) على RDNA 4 لشبكة lmxxf، Copyright (c) 2026 3zwr1 (AMDNR)؛ والأفكار مستقاة من وثائق AMD العامة عن WMMA في RDNA 4 (GPUOpen، وأداة ROCm matrix instruction calculator)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3، دون تعديل)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (رياضيات تركيب الألوان، برخصة MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0)، وهو الأساس الذي بُني عليه فتح توليد الإطارات المتعددة المدمج في XeFG وضبط توقيته (pacing)
- **burak113** — المعالج المسبق (preprocessor) لـ FSR Ray Regeneration (فرع OptiScaler المسمى ffx-denoise-experimental، برخصة GPL-3.0)
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## حقوق النشر / الترخيص

مشروع AMDNR محمي بحقوق النشر: Copyright (c) 2026 3zwr1 (AMDNR). وهو تفرّع (fork) من OptiScaler،
ويُوزَّع بموجب ترخيص GPL-3.0 الموجود في `LICENSE`.

يخضع العمل الخاص بـ AMDNR لشرط إضافي بموجب البند 7(b) من GPL-3.0 (راجع
`Licenses/AMDNR_NOTICE.txt`): يجب على أي نسخة أو تفرّع أو عمل مشتق يستخدمه أن يحتفظ بإشعاراته وأن ينسب
الفضل إلى **AMDNR by 3zwr1** (<https://github.com/3zwr1/AMD-NR---OptiScaler>).

**حقوق نشر قائمة AMDNR.** قائمة AMDNR — تخطيطها وتصميمها ونصوصها والكود الذي أضافه AMDNR لها — محمية بحقوق النشر: Copyright (c) 2026 3zwr1 (AMDNR). وهي جزء من هذا التفرّع (fork) المرخّص بـ GPL-3.0، مع الشروط الإضافية التالية (GPL-3.0 section 7): (b) كل من يعيد استخدام أي جزء منها يجب أن يحتفظ بسطر حقوق النشر هذا وأن ينسب الفضل بشكل ظاهر إلى AMDNR by 3zwr1، في القائمة وفي ملف README؛ (c) لا يجوز لك تقديمها، أو تقديم نسخة معدّلة منها، على أنها من عملك؛ ويجب أن تُوسَم النسخ المعدّلة بوضوح على أنها غُيّرت؛ (e) لا تُمنح أي حقوق في اسم AMDNR أو شعاره؛ ولا يجوز للمشاريع الأخرى استخدامهما.

تبقى أعمال المشاريع الأصلية المذكورة أعلاه ملكًا لأصحابها، وتخضع لتراخيصهم الخاصة؛ ولا يدّعي AMDNR
أي حقوق نشر عليها.

سيُنشر الكود المصدري مع AMDNR 0.5.0.

## الجانب القانوني

تُوزَّع هذه النسخة بموجب ترخيص GPL-3.0 الموجود في `LICENSE`؛ وتراخيص مكتبات الأطراف الثالثة موجودة
في `Licenses\`. يُعاد توزيع بيئة التشغيل العصبية لـ AMD وأوزانها مع نسبتها إلى مؤلفيها الأصليين كما هو
مذكور أعلاه، وذلك للتسهيل فقط، دون ادعاء أي ملكية ودون تقديم أي ضمان.

الملف `nvngx_dlssnr.dll` الخاص بـ NVIDIA غير موجود في هذين الأرشيفين. لا شيء من هذا معتمد من NVIDIA أو
AMD أو أي ناشر ألعاب، ولا تابع لأي منها، ولا مدعوم منها. فهو يشغّل ميزة غير موثّقة بشكل مباشر. استخدمه
على مسؤوليتك الخاصة.

</div>
