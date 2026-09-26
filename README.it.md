# AMDNR — DLSS 5 Neural Rendering su AMD (build di OptiScaler) — v0.3.3.2

[English](README.md) | [中文](README.zh-CN.md) | [Português](README.pt-BR.md) | [Español](README.es.md) | [العربية](README.ar.md) | [Français](README.fr.md) | **Italiano** | [Русский](README.ru.md) | [Polski](README.pl.md)

> **Abbiamo bisogno del tuo supporto.** Unisciti al server Discord — <https://discord.gg/AMDNR> — per
> assistenza, segnalazioni di bug e build di test; ogni segnalazione con un log rende migliore la build successiva.

DLSS 5 Neural Rendering in esecuzione su GPU AMD, integrato in OptiScaler così da funzionare in
qualsiasi gioco Direct3D 12 che OptiScaler già aggancia. Oltre al pass neurale: model interleave per
un grande guadagno di frame rate, composizione residua, frame generation XeSS sbloccata fino a 6X
(fino a 10X opzionale nei giochi D3D12), e FSR Ray Regeneration per i giochi che usano DLSS Ray
Reconstruction.

**Discord: <https://discord.gg/AMDNR>** — supporto, segnalazioni di bug (`#bug-report`), build
di test.

**Supporta il progetto: <https://ko-fi.com/3zinr>**

> **Il runtime danielblnc è opera di Daniel Blanco.** Il runtime neurale AMD contenuto in `Runtime.zip`
> (`dlssnr_amd_pass1..3.dll`) è **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> AMDNR lo distribuisce senza modifiche, con il suo permesso; non è opera di AMDNR. Per favore, supporta il suo progetto.
> I crediti completi per tutti gli altri si trovano in fondo a questa pagina.

---

## AMDNR - Guida all'installazione di OptiScaler

L'installazione è piuttosto semplice.

### 1. Scarica i file

Scarica questi due file da GitHub (<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>):

* `AMDNR-vX.X.X.zip`
* `Runtime.zip`

### 2. Estrai entrambi i file

Estrai il contenuto di entrambi i file `.zip`.

### 3. Copia tutto nella cartella del gioco

Per prima cosa, copia tutti i file di `AMDNR-vX.X.X` nella cartella principale del gioco — la stessa
cartella in cui si trova il file `.exe` del gioco.

Poi fai lo stesso con tutti i file di `Runtime`.

### 4. Rinomina OptiScaler.dll

Nella cartella del gioco, cerca:

`OptiScaler.dll`

Rinominalo in:

`dxgi.dll`

`dxgi.dll` è l'opzione consigliata.

Se il gioco non si avvia o la mod non si carica, prova invece a rinominare `OptiScaler.dll` in uno
di questi:

* `d3d12.dll`
* `winmm.dll`
* `version.dll`
* `dbghelp.dll`

Prova un nome alla volta. Non creare più copie di `OptiScaler.dll`.

> **Resident Evil Requiem (e la sua demo) richiede REFramework.** È un requisito noto, non un bug di AMDNR: OptiScaler ne ha bisogno per
> superare l'anti-tamper di Capcom ([wiki di OptiScaler](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-9-Requiem)). Senza di esso, il gioco crasha 15-60 s
> dopo l'avvio ("An unhandled exception occurred"). Metti `dinput8.dll` preso da `REFramework.zip` dell'ultima nightly
> (<https://github.com/praydog/REFramework-nightly/releases>) accanto a `dxgi.dll`, e cambia il tasto del menu di REFramework (ad es. in Canc / Delete): anche REFramework usa Insert.
> Dopo un aggiornamento del gioco, aspettati crash finché REFramework non viene aggiornato. Probabilmente ne hanno bisogno anche PRAGMATA, Monster Hunter Wilds e Onimusha (non confermato).

### 5. Avvia il gioco

`HOME` attiva e disattiva il Neural Rendering mentre giochi (con entrambi i runtime; un piccolo avviso
mostra On / Off). Puoi riassegnarlo accanto alla casella Enable nella scheda Neural o in Interface > Keybinds.

Tutto qui.

Avvia il gioco normalmente e premi:

`INSERT`

Si aprirà il menu di OptiScaler / AMDNR, dove puoi configurare la mod come preferisci.

### Se non funziona

Se il gioco continua a non avviarsi con nessuno dei nomi indicati sopra, per favore segnalalo nel canale
`#bug-report` su Discord.

Quando segnali il problema, carica anche tutti i file `.log` eventualmente generati nella cartella
principale del gioco.

Questi log sono molto importanti e ci aiuteranno a identificare il problema molto più velocemente.

> Di solito il file `.exe` non si trova dove punta il collegamento. I giochi Unreal lo tengono in
> `<Game>\Binaries\Win64\`.

---

### Il runtime lmxxf (0.3.0, opzionale)

Un secondo runtime neurale (con licenza MIT, di lmxxf) può eseguire il pass al posto di quello di
danielblnc. Gira su RDNA 4, e su RDNA 3 (RX 7000, Strix Halo) tramite il backend RDNA 3 di AMDNR - lì è più lento:
parti con la NR resolution al 67%. Servono due cose accanto al gioco:

1. `LmxxfNrRuntime.dll` - in questo archivio, accanto a `OptiScaler.dll` (viene copiato insieme al resto).
2. `LmxxfNrRuntime.pak` (416 MB, incluso nello zip di AMDNR) accanto a `LmxxfNrRuntime.dll` - i file
   dei pesi di lmxxf, i moduli HIP e l'HLSL in un unico file cifrato e autenticato. Il runtime lo apre
   in memoria; niente viene estratto su disco.

Al primo avvio in cui viene trovato un runtime installato e non è stata ancora fatta una scelta, il
menu chiede quale usare (`[DlssNr] NrBackend = daniel | lmxxf` nell'ini la registra; Neural > Neural
runtime la cambia, al successivo avvio del gioco). La modifica di lmxxf viene applicata con un frame di
ritardo, trasportata dai motion vector, così il frame non aspetta mai la rete (circa 17 ms a 1080p
su una RX 9070 XT). Il suo log è `lmxxf_backend.log` accanto al gioco.

**Compatibilità (lmxxf).** Il runtime vede solo ciò che vede DLSS, quindi ciò che cambia da titolo a
titolo è una lista breve: formato del colore e HDR, motion vector e la loro scala, profondità e la sua
direzione, la reactive mask, la texture di esposizione, il flag Reset, e dove si colloca il pass
(prima della Super Resolution, o dopo la Ray Reconstruction). Testati finora:

| Titolo | API / posizione | Note |
|---|---|---|
| Silent Hill 2 | D3D12, prima di SR | titolo di riferimento; gestita l'allocazione colore con padding di Unreal |
| Forza Horizon 6 | D3D12, prima di SR | |
| Stray | D3D11 tramite il bridge D3D12, prima di SR | |
| GTA V Enhanced | D3D12, prima di SR, HDR, reactive mask a un canale | risolto nella 0.3.0: la mask veniva letta come "tutto reattivo" e la modifica non veniva mai applicata |
| Qualsiasi titolo con Ray Reconstruction | D3D12, dopo RR (riscritto nell'output) | supportato dalla 0.3.0; non ancora confermato in un gioco |

Se un titolo non mostra alcun effetto: `lmxxf_backend.log` contiene una riga `lmxxf inputs:` (formati,
dimensioni, scala del movimento, direzione della profondità, mask, esposizione) e una riga
`lmxxf stats @N:` ogni 600 frame (esposizione, luminosità in ingresso, la modifica del modello, la
modifica trasportata, keep, media reattiva, lunghezza dei vettori e frazione scartata). Allega il log
alla segnalazione; di solito quelle due righe dicono il perché.

Con lmxxf, il blocco Neural runtime ha **Network history** (l'input temporale proprio del modello),
e Image look ha il gruppo **lmxxf edit**: lo shaper della modifica (Edit detail, Edit colour, Edge
guard: guadagno sulla parte fine della modifica del modello, il suo colore rispetto alla sua
variazione di luminosità, e una dissolvenza della modifica in corrispondenza dei bordi di profondità) e
**Output smoothing** (il pass lato output del progetto upstream, richiede Network history). Neural passes, Residual
strength/limit, sharpening, Debug view 1 e il filtro Appearance valgono con entrambi i runtime.

**Full network** (Neural > Performance, `[DlssNr] LmxxfFullNetwork`, solo lmxxf) esegue tutti i 71
blocchi della rete invece di saltare il 42, il 43 e il 46: leggermente più fedele, circa 0.5 ms più
lento a 1080p (16.6 -> 17.1 ms su una RX 9070 XT). Disattivato di default.

## Requisiti

- Una GPU AMD con un driver aggiornato. Il runtime neurale usa HIP tramite il driver; non servono
  l'SDK HIP né la modalità sviluppatore. Per quanto riguarda i chip: RX 9000 (RDNA 4) esegue entrambi i runtime;
  anche RX 7000 (RDNA 3, desktop e mobile) li esegue entrambi - lmxxf tramite il backend RDNA 3 di
  AMDNR, più lento che su RDNA 4; Strix Halo (8060S / 8050S) esegue lmxxf; le APU per handheld
  (Z1 Extreme / 780M, Z2 Extreme / 890M) e RDNA 2 (RX 6000, Steam Deck) non sono supportate da nessuno
  dei due. La scheda Neural indica cosa può eseguire la tua GPU.
- Un gioco Direct3D 12, Direct3D 11 o Vulkan. Il percorso neurale AMD in sé è D3D12; i titoli D3D11 e
  Vulkan ci arrivano tramite il bridge D3D12 di OptiScaler, il che significa che l'upscaler deve essere
  uno dei backend "w/Dx12" (`ffx_12`). Lascia `Dx11Upscaler` / `VulkanUpscaler` su `auto` e questa
  build lo sceglie per te quando il neural rendering è attivo.
- Circa 2 GB di VRAM libera a risoluzioni di rendering di classe 1080p.

## Cosa contengono i due archivi

**AMDNR-vX.X.X.zip**

| File | Cos'è |
|---|---|
| `OptiScaler.dll` | OptiScaler con il backend AMD di DLSS-NR. Rinominalo come indicato nella guida. |
| `OptiScaler.ini` | Impostazioni. Il Neural Rendering è abilitato; il logging è attivo, così hai qualcosa da allegare a una segnalazione di bug. |
| `LmxxfNrRuntime.dll` | Il runtime neurale lmxxf (kernel lmxxf 0.29). Usato solo se scelto; legge `LmxxfNrRuntime.pak` accanto a sé, vedi "Il runtime lmxxf". |
| `LmxxfNrRuntime.pak` | Pesi, moduli HIP e shader del runtime lmxxf in un unico file cifrato (416 MB). Lo legge solo il runtime lmxxf; tenerlo insieme al runtime danielblnc non crea problemi. |
| `OptiScaler\` | FSR, XeSS, il denoiser FidelityFX e il D3D12 Agility SDK usati da OptiScaler. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | dlssg-to-fsr3 di Nukem9, non modificato e rinominato: le chiamate DLSS Frame Generation del gioco vengono gestite dalla frame generation di FSR 3, anche su Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, vedi `Licenses/`. |
| `Licenses\`, `LICENSE` | Licenze di terze parti, l'avviso di AMDNR (`AMDNR_NOTICE.txt`) e la licenza GPL-3.0 di questa build. |
| `SHA256SUMS.txt` | Checksum di ogni file distribuito, di entrambi gli archivi. |

**Runtime.zip**

| File | Cos'è |
|---|---|
| `dlssnr_amd_pass1..3.dll` | Il runtime neurale AMD, la v0.3.1 di danielblnc, non modificata. Tre copie, così il multi-pass ne ha una per ogni pass. |
| `dlssnr_on_amd_weights.bin` | I pesi della rete caricati dal runtime. |

## Impostazioni da conoscere

Apri la scheda **Neural**. I valori predefiniti sono la configurazione testata più recente, quindi la
prima mossa utile è cambiare una cosa alla volta.

- **NR resolution** — la leva principale tra qualità e costo. Sotto il 100% il modello lavora su
  un'immagine più piccola e solo la sua *correzione* viene riportata sul frame a piena risoluzione,
  così il frame mantiene il proprio dettaglio. Sopra il 100% il costo cresce con il quadrato (150%
  equivale a 2.25x). Lo slider si muove a scatti del 5%: ogni nuova dimensione NR può trattenere VRAM
  fino al riavvio del gioco, quindi riavvia il gioco dopo molte modifiche.
- **Residual strength** — quanta parte della modifica del modello viene applicata; sopra 1 la
  amplifica. È il controllo che cambia di più l'immagine.
- **Residual limit** — un tetto a quanto può variare un singolo pixel. Se vedi chiazze: **abbassalo**.
- **Model interleave** — esegue il modello un frame sì e uno no per un grande guadagno di frame rate.
  I frame saltati vengono riempiti dall'**Interleave preset**; *Guided fill v2* è il predefinito e
  quello su cui si sta lavorando attivamente. Il pacing dei due tipi di frame è automatico, e
  **Adaptive interleave** (attivo di default) esegue il modello su ogni frame mentre l'immagine è in
  movimento, così i salti - e i loro artefatti - avvengono solo quando l'immagine è ferma.
- **Neural passes** — 2 e 3 impilano il modello, con rendimenti decrescenti. Con lmxxf la history
  della rete resta quella del primo pass; i pass extra sono solo affinamento spaziale.
  danielblnc esegue 1 pass nei titoli Vulkan (lo indica una nota sotto lo slider).
- **Colour composition** (Neural > Image look, entrambi i runtime) — *Classic* (predefinito) è
  l'immagine che avevi prima. *RenoDX (experimental)* esegue la composizione colore di RenoDX dopo il
  modello, come fa il percorso NVIDIA: Composition detail e colour, un **Highlight guard** bidirezionale
  (2x di default) che limita la risposta del modello rispetto all'originale, e controlli opzionali per
  pelle / ambiente. Su un frame display-referred (SDR) torna a Classic, con una nota nel menu. Gli stili
  e i preset NR non lo toccano.
- **La frame generation è disattivata in un ini nuovo.** Scheda Frame Gen: scegli l'FG Input (ad es.
  "DLSSG via Streamline" in un gioco con la frame generation DLSS) e l'FG Output (XeFG), poi spunta
  **Active** nella sezione Frame Generation (XeFG) e premi Save Settings. Un ini della 0.1.0 che
  l'aveva attiva non viene mantenuto quando installi l'ini della 0.2.0.
- **Multi-frame generation XeFG** — da 3X a 6X è integrata e attiva di default (`XeFG\UnlockMFG`),
  sia per la copia di OptiScaler sia per quella del gioco. **Elimina `XeFGUnlock.asi`** da
  `OptiScaler\plugins` se ce l'hai ancora: due copie della stessa patch fanno crashare il gioco.
  **Fino a 10X va attivato manualmente** (solo giochi D3D12): imposta *XeFG ceiling (restart)* sotto FG Output
  nella scheda Frame Gen (4X, 6X predefinito, 8X o 10X; `[XeFG] MaxInterpolatedFrames`), riavvia, poi
  scegli il moltiplicatore nel menu a tendina MFG. Sopra 6X serve il provider XeFG di OptiScaler con
  Extra pacing attivo; la copia di XeSS 3 del gioco resta al massimo a 6X. 10X richiede un monitor da
  360 Hz o più e un limite di FPS pari a refresh / 10; la latenza è alta, e il provider riserva circa
  128 MiB di VRAM in più a 4K.
  7X-10X non è ancora confermato in un gioco: tester, per favore inviate `OptiScaler.log`.
- **FSR Ray Regeneration** — di default solo su RDNA 4 (RX 9000); solo nei giochi che usano DLSS Ray Reconstruction (Cyberpunk 2077,
  Alan Wake 2), con il gioco impostato su DLSS (spoofing attivo) e con ray tracing e Ray Reconstruction
  attivati nelle sue impostazioni. Il Neural Rendering viene quindi eseguito dopo di essa, sul suo
  output, il che costa di più: abbassa la NR resolution se il frame rate cala. I suoi controlli
  (Neural > Quality > Ray Regeneration) compaiono solo mentre il gioco sta usando la Ray
  Reconstruction. Il **profilo path-traced** (meno grana sui volti con il path tracing) va attivato
  manualmente dalla 0.3.3.1: spuntalo lì per provarlo in Resident Evil Requiem o PRAGMATA. Nello stesso punto ci
  sono l'intensità della bias mask, una vista di debug RR e lo **smoothing della pelle** (sperimentale,
  per i giochi che pubblicano una guida SSS; disattivato di default, ma attivo di default in Resident
  Evil Requiem dalla 0.3.3.2).

## Se qualcosa va storto

`OptiScaler.log` compare nella cartella del gioco. Allegalo in `#bug-report` e indica il gioco e
la GPU. Il backend AMD scrive anche `amd_presr.log` e `amd_bridge.log`, che sono quelli utili quando
è proprio il pass neurale a comportarsi male. Il log della sessione precedente viene conservato come
`OptiScaler.previous.<exe>.log`; dopo un crash, allega anche quello (in quel caso il nuovo log riporta "no
clean exit recorded").

**NR frames 0/s, e la scheda Neural o `amd_presr.log` dicono che la DLL del pass è una build che questo
AMDNR non gestisce?** Le tue `dlssnr_amd_pass1..3.dll` sono una build di danielblnc che questo AMDNR non
conosce (in giro è stato visto un set 0.2.16), oppure ne manca una delle tre. Dalla 0.3.3.2 la scheda
Neural indica il nome del file e la sua versione e dice cosa fare. Usa `v0.4.0-Runtime.zip` (il più
recente) o `Runtime.zip` (0.3.1) di questa release, prendendo tutte e tre le DLL dei pass dallo stesso
zip: il file `dlssnr_amd_pass1.dll` in `v0.4.0-Runtime.zip` pesa 10,027,008 byte, con SHA256 che inizia
per `d62be3d8`. Build supportate: 0.2.17, 0.3.0, 0.3.1, 0.3.2, 0.3.3, 0.4.0, e 0.4.1 / 0.4.2 già prima
della loro uscita. Non installare il setup di danielblnc né i suoi file `dxgi.dll` / `version.dll` /
`winhttp.dll` accanto ad AMDNR: AMDNR esegue già il suo runtime.

**lmxxf non fa nulla, o si ferma subito, su un PC con grafica integrata?** Risolto nella 0.3.3.2. Su un
Ryzen desktop con la grafica integrata attiva, un portatile con APU AMD e una Radeon, o un PC con due GPU
AMD, la GPU del gioco spesso non è il device HIP 0. lmxxf allora falliva al primo frame
(`hipErrorInvalidHandle (400)`, poi "session is poisoned" in `lmxxf_backend.log`) e restava disattivato.
Sostituisci sia `OptiScaler.dll` (il file che hai rinominato, ad es. `dxgi.dll`) sia `LmxxfNrRuntime.dll`
con i file della 0.3.3.2. Non ancora testato su un PC del genere: se lmxxf si ferma ancora, la scheda
Neural ora dice perché; invia `lmxxf_backend.log` e `amd_bridge.log` (elenca i device HIP).

**Un gioco Vulkan (Indiana Jones and the Great Circle) si interrompe all'avvio con "Could not create the
Vulkan device (VK_ERROR_EXTENSION_NOT_PRESENT)"?** Risolto nella 0.3.2: il percorso neurale NVIDIA
ereditato chiedeva al driver AMD due estensioni di dispositivo esclusive NVIDIA. I titoli Vulkan arrivano
al pass neurale tramite il bridge D3D12 di OptiScaler (vedi Requisiti).

**lmxxf bloccava un gioco Vulkan al primo frame NR?** Risolto nella 0.3.3; aspettati un singolo scatto
di circa 1 s all'avvio di NR. Se una sessione Vulkan si interrompe prima della prima risposta di lmxxf,
l'avvio successivo usa il runtime di danielblnc e la scheda Neural spiega perché; premi lì **Retry lmxxf**
(rimuove `lmxxf_vk_launch.pending` accanto a `OptiScaler.dll`) per riprovare lmxxf.

**danielblnc si bloccava per alcuni secondi e poi interrompeva NR, in un gioco Vulkan (Indiana Jones) con
2-3 Neural passes?** Risolto nella 0.3.3: nei titoli Vulkan esegue 1 pass, e la sua attesa di 80 ms dopo
il submit è stata rimossa. Il primo frame NR di una sessione causa ancora una pausa di circa 5 s; una nota sotto
la scelta del runtime spiega le relative righe di log. Tester: `[DlssNr] AmdVkLateCopyWait=true`
(sperimentale, disattivato di default, non ancora testato in un gioco) dovrebbe eliminare quella pausa;
inviate `OptiScaler.log`, `amd_presr.log` e `dlssnr_on_amd.log`.

**L'uso della RAM di lmxxf cresceva per tutto il tempo in cui NR era attivo?** Risolto nella 0.3.3 (erano
circa 45 GB all'ora a 60 fps NR). Cosa resta: danielblnc trattiene VRAM per ogni nuova dimensione NR oltre
circa 1 MP (fuori dal 100% la 0.3.3.2 arrotonda le sue dimensioni a 64 px, quindi ce ne sono solo poche);
con danielblnc, riavvia il gioco dopo molte modifiche. Dalla 0.3.3.2 lmxxf non trattiene più circa 97 MB
a ogni cambio di NR resolution o di modalità DLSS: crea i buffer della rete una sola volta per ogni
dimensione della rete e li riutilizza (resta solo un piccolo residuo di circa 10-25 MB di VRAM per ogni cambio).

**Un gioco Streamline fallisce all'avvio con l'errore slInit 0x18 (visto con NBA 2K27 su AMD)?** La 0.3.3
chiude uno dei modi in cui gli hook dei plugin Streamline di OptiScaler potevano causarlo, ma non è
confermato che sia la causa in NBA 2K27. `OptiScaler.log` ora registra le righe `slInit returned ...` e
`[SLINIT]`: invia il log con la segnalazione.

**La Ray Reconstruction del gioco è attiva ma la scheda Neural dice "Ray Regeneration is off in this
title"?** Il gioco non espone ciò di cui FSR Ray Regeneration ha bisogno (Satisfactory: nessuna matrice
della camera). Al suo posto gira l'upscaling FSR e NR prende la sua normale posizione pre-SR; nessuna
impostazione dell'ini può cambiarlo.

**Un gioco Ubisoft Anvil (AC Black Flag Resynced, Shadows, Mirage) mostra "DX12 Error 0x80070057"?**
Questi giochi hanno la propria XeSS Frame Generation. Questa build la lascia a loro (lì l'output XeFG di
OptiScaler si fa da parte e la scheda Frame Gen lo indica); usa l'opzione XeSS FG del gioco. Se succede
ancora, imposta `[FrameGen] Enabled=false` e `[fakenvapi] ForceXeLL=false` e segnalalo allegando il log.

**The Last of Us Part I crasha all'avvio?** La causa è l'inizializzazione di Streamline del gioco stesso, un
problema noto di OptiScaler: rinomina `sl.common.dll` nella cartella del gioco in `sl.common.dll.bak` e
scegli **FSR 3.1** nelle impostazioni del gioco invece di DLSS.

Note complete per ogni versione: `CHANGELOG.md` (nello zip e nel repository).

## Roadmap

- **0.3.3** (questa build) — lmxxf su RDNA 3 (RX 7000; backend proprio di AMDNR); composizione colore
  RenoDX (sperimentale, opzionale) su entrambi i runtime; lmxxf: opzione Full network, leak di RAM
  risolto, fix per i titoli Vulkan (caricamento lazy dei pesi dentro il bridge Vulkan), kernel 0.29
  (bit-exact, più veloci); danielblnc nei titoli Vulkan: 1 Neural pass, messaggi più chiari, un'attesa
  di copia tardiva opzionale; XeFG fino a 10X (opzionale, D3D12); irrobustimento e diagnostica
  dell'avvio di Streamline; profilo path-traced e smoothing della pelle per FSR Ray Regeneration; robustezza su UE5.
- **0.3.2** — le segnalazioni sulla 0.3.1: i titoli Vulkan si avviano e girano con lmxxf, colori di
  lmxxf allineati a quelli di danielblnc (auto-exposure), il menu a tendina del runtime, stato e tuning
  della Ray Reconstruction; dlssg-to-fsr3 di Nukem9 nello zip per la frame generation su Vulkan.
- **0.3.1** — fix dalle prime segnalazioni sulla 0.3.0 (lmxxf da solo non partiva mai, l'NR silenzioso
  di Where Winds Meet, il crash al cambio della qualità DLSS, GTA V Legacy) e preset di stile NR con
  tre slot personalizzati.
- **0.3.0** — il runtime neurale HIP **lmxxf** (RDNA 4) come runtime selezionabile accanto a quello di
  danielblnc, distribuito come `LmxxfNrRuntime.dll` + `LmxxfNrRuntime.pak`: network history, veri
  Neural passes, lo shaper della modifica, il posizionamento dopo Ray Regeneration, diagnostica per
  titolo e auto-riparazione. Un grande grazie a TheAutomatic, sul cui lavoro al progetto DLSS 5 AMD si
  basa questa integrazione.
- **0.4.0** — l'AMDNR Launcher (installazione con un clic dei runtime e del pak, aggiornamenti) e il
  supporto per i titoli senza un proprio upscaler (tipo Stray), dove OptiScaler fornisce insieme
  l'upscaler e il pass neurale.

---

## Crediti

Questa build si limita a collegare tra loro i lavori di altre persone. Se la trovi utile, i
ringraziamenti vanno ai progetti upstream.

- **TheAutomatic** — DLSS 5 AMD project — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD — https://github.com/danielblnc/DLSS-NR-on-AMD (`Runtime.zip`, non modificato)
- **lmxxf** — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (il runtime HIP, MIT)
- **c32w kernels** (0.3.3.2) — i kernel RDNA 4 a wave singola propri di AMDNR per la rete di lmxxf, Copyright (c) 2026 3zwr1 (AMDNR); idee tratte dalla documentazione pubblica di AMD su RDNA 4 WMMA (GPUOpen, ROCm matrix instruction calculator)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, non modificato)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (matematica della composizione colore, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), la base dello sblocco multi-frame XeFG integrato e del suo pacing
- **burak113** — il preprocessore di FSR Ray Regeneration (branch di OptiScaler ffx-denoise-experimental, GPL-3.0)
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## Copyright / Licenza

AMDNR è Copyright (c) 2026 3zwr1 (AMDNR). È un fork di OptiScaler, distribuito sotto la licenza GPL-3.0
contenuta in `LICENSE`.

Il lavoro proprio di AMDNR è soggetto a un termine aggiuntivo ai sensi della sezione 7(b) della GPL-3.0
(vedi `Licenses/AMDNR_NOTICE.txt`): qualsiasi copia, fork o opera derivata che lo utilizzi deve
mantenerne gli avvisi e accreditare **AMDNR by 3zwr1** (<https://github.com/3zwr1/AMD-NR---OptiScaler>).

**Copyright del menu AMDNR.** Il menu AMDNR — il suo layout, il design, i testi e il codice che AMDNR ha aggiunto per esso — è Copyright (c) 2026 3zwr1 (AMDNR). Fa parte di questo fork GPL-3.0, con questi termini aggiuntivi (GPL-3.0 section 7): (b) chiunque ne riutilizzi una qualsiasi parte deve mantenere questa riga di copyright e accreditare in modo visibile AMDNR by 3zwr1, nel menu e nel README; (c) non è consentito presentarlo, né presentarne una copia modificata, come opera propria; le versioni modificate devono essere chiaramente contrassegnate come modificate; (e) non viene concesso alcun diritto sul nome o sul logo AMDNR; altri progetti non possono usarli.

Il lavoro upstream citato sopra resta dei rispettivi autori, sotto le loro licenze; AMDNR non
rivendica alcun copyright su di esso.

Il codice sorgente sarà pubblicato con AMDNR 0.5.0.

## Note legali

Questa build è distribuita sotto la licenza GPL-3.0 contenuta in `LICENSE`; le licenze delle librerie
di terze parti si trovano in `Licenses\`. Il runtime neurale AMD e i suoi pesi sono ridistribuiti mantenendo
la loro paternità originale, come indicato nei crediti sopra, solo per comodità, senza rivendicarne la proprietà
e senza offrire alcuna garanzia.

Il file `nvngx_dlssnr.dll` di NVIDIA non è incluso in questi archivi. Niente di tutto ciò è approvato
o supportato da NVIDIA, AMD o da qualsiasi publisher di giochi, né è affiliato con loro. Controlla
direttamente una funzionalità non documentata. Usalo a tuo rischio e pericolo.
