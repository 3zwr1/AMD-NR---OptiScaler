# AMDNR — DLSS 5 Neural Rendering su AMD (build di OptiScaler) — v0.3.5

[English](README.md) | [中文](README.zh-CN.md) | [Português](README.pt-BR.md) | [Español](README.es.md) | [العربية](README.ar.md) | [Français](README.fr.md) | **Italiano** | [Русский](README.ru.md) | [Polski](README.pl.md)

> **Abbiamo bisogno del tuo supporto.** Unisciti al server Discord — <https://discord.gg/AMDNR> — per
> assistenza, segnalazioni di bug e build di test; ogni segnalazione con un log rende migliore la build successiva.

DLSS 5 Neural Rendering in esecuzione su GPU AMD, integrato in OptiScaler così da funzionare in
qualsiasi gioco Direct3D 12 che OptiScaler già aggancia. Oltre al pass neurale: model interleave per
un grande guadagno di frame rate, composizione residua, frame generation XeSS sbloccata fino a 6X
(fino a 10X opzionale nei giochi D3D12), e FSR Ray Regeneration per i giochi che usano DLSS Ray
Reconstruction. Dalla 0.3.5, **AMDNR Anywhere** (preview) porta il Neural Rendering nei giochi che non hanno un
upscaler proprio, tramite l'AMDNR Launcher, senza scrivere nulla nella cartella del gioco (vedi "AMDNR Anywhere").

**Discord: <https://discord.gg/AMDNR>** — supporto, segnalazioni di bug (`#bug-report`), build
di test.

**Supporta il progetto: <https://ko-fi.com/3zinr>**

> **Il runtime danielblnc è opera di Daniel Blanco.** Il runtime neurale AMD contenuto nei file `*Runtime.zip`
> (`dlssnr_amd_pass1..3.dll`) è **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> AMDNR lo distribuisce senza modifiche, con il suo permesso; non è opera di AMDNR. Per favore, supporta il suo progetto.
> I crediti completi per tutti gli altri si trovano in fondo a questa pagina.

> **Novità della 0.3.5:** **AMDNR Anywhere** (preview): Neural Rendering per i giochi che non hanno un DLSS, XeSS o
> FSR 2 proprio - un solo pulsante **PLAY ANYWHERE** nell'AMDNR Launcher, niente viene scritto nella cartella del gioco;
> RX 9000 (RDNA 4) in questa release (vedi "AMDNR Anywhere"). **Ray Regeneration ha una scheda tutta sua**, subito dopo
> Upscaling, e le sue righe di stato dicono, per scheda video e per API, cosa è in funzione e perché no; su RX 7000 non è più offerta
> di default (il gioco tiene il proprio denoiser), con un'opzione sperimentale non supportata. **Il pass
> neurale può girare dopo l'upscaling** (`[DlssNr] AmdPlacement=post`, Neural > Performance > Placement; il predefinito
> `pre` non cambia). **La scheda Frame Gen dice perché non viene generato nulla** e nomina i cinque passaggi (vedi la
> voce delle FAQ sulla frame generation). **Più veloce su RX 7000 e sugli handheld di classe Z1 Extreme:** circa il 10%
> di tempo di rete in meno, stessa immagine bit per bit (il set di moduli della 0.3.5 in `LmxxfNrRuntime.pak`).
> **lmxxf 0.37 di Kien (MIT) è attivo di default su RX 9000:** circa il 20% di tempo di rete in meno su una RX 9070 XT
> (sperimentale su RX 9060 / 9060 XT; `[DlssNr] AmdLmxxfL37=false` lo disattiva). Sugli handheld RDNA 3,
> **FSR 4 (INT8)** è disponibile come opzione sperimentale, e gli Style slots personalizzati ora conservano l'intero
> aspetto. I giochi con risoluzione dinamica non ricostruiscono più la rete a ogni passo, la
> modifica trasportata non sparisce più quando ti muovi su un handheld con Model interleave, Uncharted: Legacy of
> Thieves non va più in crash su RX 9000, e molte altre correzioni. **Cambiano tutti e tre i file: sostituisci
> `OptiScaler.dll`, `LmxxfNrRuntime.dll` e `LmxxfNrRuntime.pak` insieme; utenti del launcher: si aggiorna da solo.**
> Dettagli: `CHANGELOG.md`.

> **Novità della 0.3.4.2 (hotfix):** il menu in Assetto Corsa: il tasto del menu lo apre o chiude una sola volta per
> pressione, i clic più brevi di un frame non vanno più persi, e il selettore del runtime risponde a `1` / `2` / `Enter` /
> `Esc`, ha una X nella barra del titolo e chiudere il menu vale come "Decide later". Il selettore del runtime non apre
> più il menu da solo (al suo posto un avviso), la sezione **Ray Regeneration** della scheda Neural non si nasconde più
> — c'è sempre, e una riga in grigio dice perché non è in funzione — e il testo su Wine / Proton dice che Ray
> Regeneration lì è un problema noto. AMDNR **accetta anche un layout di runtime danielblnc in
> più**, quindi una build danielblnc più recente potrà funzionare qui senza alcun aggiornamento di AMDNR.
> **Neural Rendering è identico byte per byte alla 0.3.4.1 a parte quell'unica riga di layout accettato** (il
> passaggio neurale, entrambi i runtime e il pak
> sono invariati): venendo dalla 0.3.4.1 o dalla 0.3.4, sostituisci solo `OptiScaler.dll`; utenti del launcher: si
> aggiorna da solo. Dettagli: `CHANGELOG.md`.

> **Novità della 0.3.4.1 (hotfix):** Ray Regeneration è meno morbida su Windows (uno sharpening di 0.25 quando il gioco
> non ne invia; per disattivarlo: Image > Sharpness, spunta Override, slider a 0); niente più falso popup "Upscaler
> failed to run!" in Control Resonant; su Linux / Proton il menu funziona (confermato da un giocatore, anche con la
> frame generation attiva) e lì Ray Regeneration non aggiunge alcuno sharpening predefinito (vedi "Linux / Proton").
> **AMDNR Launcher 0.3.4.1**, realizzato a partire dai vostri feedback su Discord: nove lingue, ricerca, preferiti,
> nascondi, rinomina, CHOOSE GAME .EXE, PLAY, un UNINSTALL completo e altro (vedi "AMDNR Launcher"). Il Neural
> Rendering è invariato rispetto alla 0.3.4 (stesso runtime e stesso pak): se vieni dalla 0.3.4, sostituisci solo
> `OptiScaler.dll`; chi usa il launcher: l'aggiornamento lo fa lui per te. Dettagli: `CHANGELOG.md`.

> **Novità della 0.3.4:** un nuovo menu (la scheda Neural rifatta, lo stesso stile in tutte le schede e un pulsante
> **Save report** che comprime i tuoi log in uno zip per la segnalazione); lmxxf è più veloce su RX 7000 (1440p FSR
> Quality: 73.3 -> 52.2 ms per esecuzione della rete su una RX 7800 XT, tempo di rete misurato fuori da un gioco) e su RX 9070 / 9070 XT (kernel
> di lmxxf 0.31); lmxxf gira sulle APU per handheld (sperimentale; la prova di un tester, in un gioco: circa 29 fps in Shadow of the Tomb Raider su una ROG
> Ally); un **Fast mode** opzionale per lmxxf; **AMDNR Screen GI**, la GI in screen space di AMDNR (preview,
> disattivata di default); e molte correzioni. Sostituisci
> `OptiScaler.dll`, `LmxxfNrRuntime.dll` e `LmxxfNrRuntime.pak` insieme. Dettagli: `CHANGELOG.md`.

---

## AMDNR - Guida all'installazione di OptiScaler

L'installazione è piuttosto semplice. **Su Windows, l'AMDNR Launcher fa tutto questo per te** (vedi "AMDNR Launcher"
più sotto). A mano:

### 1. Scarica i file

Scarica questi file dall'ultima release su GitHub (<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>;
la 0.3.5 è il tag Alpha0.3.5):

* `AMDNR-vX.X.X.zip` (per la 0.3.5: `AMDNR-v0.3.5.zip`), con il runtime lmxxf completo.
* Per il runtime danielblnc, uno zip di runtime: su **RX 9000 e su RX 7000**, `v0.5.0-Runtime.zip`
  (consigliato) dalla release Alpha0.3.4.2; `v0.4.3-Runtime.zip` (Alpha0.3.4.2 e Alpha0.3.4.1), `v0.4.1-Runtime.zip` e
  `v0.4.0-Runtime.zip` (Alpha0.3.4.1) restano accettati. Il runtime lmxxf è in
  `AMDNR-vX.X.X.zip` e non richiede alcuno zip di runtime su RX 7000 e RX 9000; le APU per handheld usano solo
  lmxxf. L'AMDNR Launcher sceglie lo zip giusto per la tua GPU. Vedi "Cosa contengono gli archivi".

### 2. Estrai entrambi i file

Estrai il contenuto di entrambi i file `.zip`.

### 3. Copia tutto nella cartella del gioco

Per prima cosa, copia tutti i file di `AMDNR-vX.X.X` nella cartella principale del gioco — la stessa
cartella in cui si trova il file `.exe` del gioco.

Poi fai lo stesso con tutti i file dello zip di runtime (ad es. `v0.5.0-Runtime` su RX 9000 e su RX 7000).

> **Aggiorni da un AMDNR precedente?** Copia di nuovo tutto sovrascrivendo. **Nella 0.3.5 sono cambiati tre file
> insieme:** `OptiScaler.dll` (sostituisci il file che hai rinominato, ad es. `dxgi.dll`, con il nuovo rinominato allo
> stesso modo), `LmxxfNrRuntime.dll` e `LmxxfNrRuntime.pak` (440 MB). Non mescolarli con copie precedenti. Puoi
> tenere il tuo `OptiScaler.ini`: le nuove impostazioni usano i valori predefiniti. I file del tuo zip di runtime
> danielblnc restano come sono. L'AMDNR Launcher lo fa per te: UPDATE ALL, oppure REPAIR / UPDATE su un gioco che
> mostra "Update available" (il launcher aggiorna prima sé stesso).

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
* `winhttp.dll`
* `wininet.dll`

Prova un nome alla volta. Non creare più copie di `OptiScaler.dll`. Questi sono i nomi con cui il mod si carica (più
`OptiScaler.asi` con un loader ASI); `d3d11.dll` non è tra questi.

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

**Il modo più semplice: Save report.** Se il menu si apre, fai clic su **Save report** (l'ultima riga di Neural >
Diagnostics, o la prima di Advanced > Logging). Scrive uno zip, `AMDNR-report-<exe del gioco>-<data>.zip`, nella cartella del gioco (sul Desktop se la
cartella del gioco è di sola lettura, altrimenti in `%TEMP%`), con `report.txt`, i log e i file ini, e il menu
mostra dove è finito. Il tuo nome utente di Windows e il nome del PC vengono sostituiti da segnaposto; un nome
all'interno di un percorso del gioco fuori da `C:\Users\` no. Allega lo zip in `#bug-report`. Il **COLLECT LOGS**
dell'AMDNR Launcher scrive lo stesso zip per qualsiasi gioco e, dalla 0.3.5.1, aggiunge il log del crash e il dump più
recente dopo un crash.

> Di solito il file `.exe` non si trova dove punta il collegamento. I giochi Unreal lo tengono in
> `<Game>\Binaries\Win64\`.

---

### Il runtime lmxxf (0.3.0, opzionale)

Un secondo runtime neurale (con licenza MIT, di lmxxf) può eseguire il pass al posto di quello di
danielblnc. Gira in modo nativo su RDNA 4; su RDNA 3 (RX 7000, Strix Halo) passa per il backend RDNA 3 di AMDNR
di 3zwr1 - lì è più lento, vedi "RX 7000" più sotto: parti con la NR resolution al 70% o meno. Gira anche sulle
APU per handheld, in via sperimentale (vedi "APU per handheld" più sotto). Servono due cose accanto al gioco:

1. `LmxxfNrRuntime.dll` - in questo archivio, accanto a `OptiScaler.dll` (viene copiato insieme al resto).
2. `LmxxfNrRuntime.pak` (440 MB, incluso nello zip di AMDNR) accanto a `LmxxfNrRuntime.dll` - i file
   dei pesi di lmxxf, i moduli HIP e l'HLSL in un unico file cifrato e autenticato. Il runtime lo apre
   in memoria; niente viene estratto su disco.

Al primo avvio in cui viene trovato un runtime installato e non è stata ancora fatta una scelta, il
menu chiede quale usare (`[DlssNr] NrBackend = daniel | lmxxf` nell'ini la registra; Neural > Neural
runtime la cambia, al successivo avvio del gioco). La modifica di lmxxf viene applicata con un frame di
ritardo, trasportata dai motion vector, così il frame non aspetta mai la rete (circa 14.1 ms di tempo
di rete a 1080p su una RX 9070 XT). Il suo log è `lmxxf_backend.log` accanto al gioco.

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

Entrambi i runtime condividono un'unica scheda Neural (vedi "Il menu" più sotto). I controlli che il runtime
attivo non ha sono in grigio con una breve etichetta, o nascosti con un conteggio. Solo lmxxf: **Full network**,
**Output smoothing** (Quality > More quality options, richiede Network history), **Edit detail**, **Edit
colour** ed **Edge guard** (Image look > Model strength: guadagno sulla parte fine della modifica del modello, il
suo colore rispetto alla sua variazione di luminosità, e una dissolvenza della modifica in corrispondenza dei
bordi di profondità) e il limite delle alte luci dell'auto-exposure. Novità di lmxxf nella 0.3.4: Network
output, Encoding, Residual edge fade, Game exposure, Fast mode, la lettura del pacing dell'interleave e la
maschera nativa dei personaggi del modello con Structure intensity e Character structure
(ogni modifica ricostruisce la rete: uno scatto di circa 1 s).

**Full network** (Neural > Performance, `[DlssNr] LmxxfFullNetwork`, solo lmxxf) esegue tutti i 71
blocchi della rete invece di saltare il 42, il 43 e il 46: leggermente più fedele, circa 0.5 ms più
lento a 1080p (16.6 -> 17.1 ms su una RX 9070 XT, misurato nella 0.3.3). Disattivato di default.

**Fast mode** (Neural > Performance, `[DlssNr] AmdLmxxfFastMode`, lmxxf, opzionale, disattivato di default) fa
girare la rete un livello di dimensione più in basso (1080 -> 900, 900 -> 720): circa il 29% di tempo di rete in meno
a 1080p (RX 9070 XT, misurato fuori da un gioco), con i dettagli fini un po' più morbidi. Le build di danielblnc che
hanno un proprio Fast mode mostrano lì anche una riga Fast mode (`[DlssNr] AmdDanielFastMode`); i runtime negli zip
di runtime di questa release non ce l'hanno, quindi la riga è nascosta.

### RX 7000 (RDNA 3): più veloce con il livello di dimensione della rete (novità della 0.3.4)

La rete di lmxxf gira a poche dimensioni fisse (livelli): 720 (1280x720), 900 (1600x900) e 1080 (1920x1080),
più 576 e 360 (nuove, usate sugli handheld). Un livello costa lo stesso qualunque parte ne riempia l'immagine. Su
RDNA 3 (RX 7000, Radeon 8060S / 8050S e le APU per handheld) la dimensione NR di lmxxf ora si aggancia di default
a un livello: scende al livello inferiore quando è più vicina a quello (costa meno), altrimenti cresce fino a
riempire il proprio livello (stesso costo, un po' più di dettaglio), mai oltre la dimensione del frame stesso.

Tempo di rete per esecuzione su una RX 7800 XT (misurato da un tester con la sonda di lmxxf; solo la rete, media
di 30 esecuzioni; il tempo del livello 900 è stato misurato a 1600x900):

| Impostazione del gioco | 0.3.3.2 | 0.3.4 su RX 7000 |
|---|---|---|
| 1440p, FSR Quality (render 1706x960), NR 100% | livello 1080: 73.3 ms | livello 900: 52.2 ms |
| Render 1080p, NR 85% | livello 1080: 73.2 ms | livello 900: 52.2 ms |
| Render 1080p, NR 70% | livello 900: 52.2 ms | livello 720: 34.4 ms |
| Render 1080p, NR 80% | livello 900: 52.2 ms | livello 900, riempito: 52.2 ms (più dettaglio) |
| Render 1080p, NR 100% | livello 1080: 73.2 ms | invariato |

- In gioco il guadagno per frame mostrato è minore: con Model interleave la rete gira un frame sì e uno no, e il
  gioco ha il suo costo. Non ancora misurato in un gioco.
- La rete vede un'immagine un po' più piccola (a 1440p Quality circa il 6% di pixel in meno per lato), quindi il
  dettaglio fine può risultare un po' più morbido. `[DlssNr] AmdLmxxfTierSnap=false` ripristina le dimensioni della
  0.3.3.2. Le RX 9000 mantengono le dimensioni della 0.3.3.2 a meno che tu non lo imposti a `true`.
- **RX 9000:** `[DlssNr] AmdLmxxfTierSnap=true` (lì disattivato di default) porta la dimensione NR di lmxxf a una
  dimensione di rete a ogni risoluzione di render: alcune dimensioni scendono di un livello (1440p FSR Quality,
  1707x960 -> la dimensione 900: rete 14.08 -> 9.96 ms per esecuzione su una RX 9070 XT, misurato fuori da un gioco,
  immagine un po' più morbida), altre crescono dentro il loro livello (80% di un render 1080p -> 1600x900: stesso
  costo, un po' più di dettaglio). Se non impostato, le RX 9000 mantengono le dimensioni della 0.3.3.2.
- Parti con la NR resolution al 70% o meno (il livello 720 con un render a 1080p; il preset Performance è 70%). Il
  costo accanto a NR resolution si basa sul livello su cui gira la rete; il suo tooltip indica il livello.
- **0.3.5: circa il 10% di tempo di rete in meno su RX 7000**, stessa immagine bit per bit: il set di moduli della
  0.3.5 in `LmxxfNrRuntime.pak` (misurato e verificato con gli hash da un tester su una RX 7800 XT; la tabella qui
  sopra è quella della 0.3.4).
- **0.3.5 su RX 9000: lmxxf 0.37 di Kien (MIT) è attivo di default** - circa il 20% di tempo di rete in meno su una
  RX 9070 XT alla dimensione 1080 (14.0 -> circa 11.3 ms, misurato fuori da un gioco; l'immagine non è identica bit
  per bit a quella della 0.3.4.2). Anche sulle RX 9060 / 9060 XT è attivo, insieme ai kernel c32w e FastK, come
  esperimento (non ancora provato su quella scheda). Per disattivarli: `[DlssNr] AmdLmxxfL37=false`,
  `AmdLmxxfC32w=false`, `AmdLmxxfFastK=false`.

### APU per handheld (sperimentale, novità della 0.3.4)

lmxxf gira sulle APU per handheld con 12 o più compute unit, tramite il backend RDNA 3 di AMDNR di 3zwr1:
**Z1 Extreme, Z2 e Radeon 780M** (gfx1103), **Z2 Extreme, Radeon 890M e 880M** (gfx1150). È sperimentale e lento. La riga Neural runtime mostra "experimental" dopo il credito RDNA 3.
Primi risultati di un tester (ROG Ally, Z1 Extreme): la sonda di lmxxf fuori da un gioco, 54.7 ms per esecuzione
della rete alla dimensione 360p, 110.9 ms a 576p; in un gioco, la prova di un tester (Shadow of the Tomb Raider, 1280x720 con XeSS, preset
Handheld), 62 ms per esecuzione della rete in media a 360p con il modello ogni 4 fotogrammi, circa 29 fps con NR attivo.
**La 0.3.5 toglie circa il 10% a quei numeri sulla classe Z1 Extreme** (Z1 Extreme, Z2, Radeon 780M: circa 50 ms a
360p, circa 105 ms a 576p, stessa immagine bit per bit, verificata con gli hash da un tester); i moduli per Z2 Extreme /
890M / 880M sono invariati.

- **Non supportate:** Z1 e Radeon 740M (4 compute unit), Radeon 760M (8), Radeon 860M / 840M. Il runtime di
  danielblnc non gira sulle APU per handheld. Le RX 6000 (RDNA 2) sono previste per la 0.3.6; Steam Deck e le altre
  APU RDNA 2 non sono supportate.
- **Cosa fa da solo** (solo finché il tuo ini non ha un valore proprio): la rete gira alla sua dimensione più
  piccola, 360p (640x360), e il modello gira un frame ogni 4 (Model interleave; non salvato). Neural passes resta
  a 1.
- **La velocità, onestamente:** Per
  confronto: una RX 7800 XT (60 compute unit) richiede 34.4 ms per esecuzione della rete alla dimensione 720;
  questi chip ne hanno da 12 a 16 e girano a frequenze più basse. Aspettati un forte calo di frame rate anche a
  360p con il modello un frame ogni 4, un po' di ghosting per l'interleave lungo, e un aspetto più morbido che su
  una GPU desktop. Il costo NR alla fine della riga di stato della scheda Neural (e in Diagnostics) mostra il numero
  reale sul tuo dispositivo.
- **Impostazioni:**
  - Più nitido ma più lento: `[DlssNr] AmdLmxxfTierCap=576` (la dimensione di rete 1024x576).
  - Con un render a 720p o 800p, la NR resolution al 100% alimenta già la dimensione 360p, quindi una NR
    resolution più bassa non costa meno.
  - Model interleave su Off viene salvato come `[DlssNr] AmdInterleave=1` (anch'esso spento), così il valore
    predefinito dell'handheld non torna al prossimo avvio. Per spegnerlo a mano, scrivi 1, non 0.
  - Preset > **Handheld** imposta NR resolution al 100%, Dynamic NR disattivato, il modello ogni 4 fotogrammi, 1 Neural pass e Full network disattivato. Il pulsante compare solo su queste APU; anche Quality, Balanced e Performance qui mantengono la dimensione di rete 360p (il menu lo dice).
- **FSR 4:** FSR 4 (INT8) è disponibile come opzione sperimentale sugli handheld RDNA 3 (scheda Upscaling); non validato da AMD.
  La casella **FSR 4 (INT8) - Experimental on this GPU (restart)** scrive `[FSR] Fsr4ForceModel=2`; non si attiva mai da solo
  (con `Dx12Upscaler=auto` l'upscaler è XeSS). Circa 1,5-3 ms per fotogramma su una Z1 Extreme (una stima); con NR,
  FSR 4 o la dimensione 576, non entrambi.
- **Shadow of the Tomb Raider** (e i giochi che creano il loro device D3D12 due volte) non va più in crash all'avvio
  dell'upscaler (corretto in 0.3.4).
- **Driver:** usa il driver Adrenalin di AMD. lmxxf ha bisogno di HIP (`amdhip64_7.dll`), che alcuni driver dei
  produttori di handheld non includono; `amd_bridge.log` dice allora che HIP non è disponibile.
- **Muoversi con Model interleave (risolto nella 0.3.5):** la modifica trasportata spariva appena ti muovevi
  ("l'effetto sparisce quando mi muovo"): con Model interleave la rete gira su frame di durata diversa, e la protezione
  del trasporto presupponeva frame uguali. Ora legge i vettori del frame precedente alla durata di questo frame
  (entrambi i runtime).
- **Usa insieme i file della 0.3.5:** la 0.3.5 cambia tutti e tre i file (`OptiScaler.dll`, `LmxxfNrRuntime.dll` e
  `LmxxfNrRuntime.pak`), quindi sostituiscili insieme; il runtime rifiuta un handheld quando `OptiScaler.dll` è più
  vecchio della 0.3.4 ("this handheld needs OptiScaler.dll 0.3.4 or newer").
- **Tester con un handheld:** chiedi su Discord il kit di test per handheld (`handheld-test.zip`). Il suo
  `run_probe.bat` misura la rete sul tuo dispositivo e scrive `handheld_result.txt` (il tuo nome utente di Windows
  viene mascherato).

## AMDNR Anywhere (preview, novità della 0.3.5)

**Cos'è.** Neural Rendering per i giochi che non hanno un DLSS, XeSS o FSR 2 proprio - e niente viene scritto nella
cartella del gioco. AMDNR gira dentro un host di cattura della finestra: l'host cattura la finestra del gioco, la scala
al tuo schermo con FSR 3, e il Neural Rendering gira sull'immagine catturata; il nostro menu viene disegnato dentro
l'host (il tuo tasto del menu, `INSERT` di default) con una scheda **Anywhere** tutta sua. L'host è
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR**:
l'AMDNR Launcher lo scarica dalla release del suo autore (467 MB, una sola volta).

**Come si usa.** Nell'AMDNR Launcher un gioco senza upscaler mostra **PLAY ANYWHERE** al posto di INSTALL. Premilo: il
launcher scarica l'host (la prima volta), avvia il gioco, e l'host ne cattura la finestra. Esegui il gioco **in finestra
o in finestra senza bordi**, non a schermo intero esclusivo, e premi il tuo tasto del menu per il menu di AMDNR dentro
l'host. Un gioco con un upscaler proprio mantiene la normale via INSTALL: Anywhere è per i giochi che non ne hanno uno.

**Le impostazioni dell'host stanno nel menu**, nelle Host settings della scheda Anywhere, non nel launcher: la
dimensione della finestra del gioco (720p / 900p / 1080p - un consiglio su cosa impostare nel gioco; l'host cattura
qualunque finestra il gioco apra), l'elenco delle fasi (V1: un solo pass FSR 3 verso lo schermo; V2: FSR 3 a 1x, poi un
pass di riempimento), il livello NR (Auto / 720 / 900 / 1080), VRR, il frame pacing e il **frame rate dell'host**
(Default = la frequenza di aggiornamento del tuo schermo, al massimo 60; Auto = la frequenza che la rete ha sostenuto
nell'ultima sessione di gioco; da 30 a 120; Display refresh = nessun limite). Si applicano al **prossimo** PLAY
ANYWHERE, e il launcher mostra un riepilogo di sola lettura accanto al pulsante. Nell'`OptiScaler.ini` dell'host sono
`[DlssNr] AnywhereWindow`, `AnywhereEffect`, `AnywhereNrTier`, `AnywhereVrr`, `AnywherePacing` e `AnywhereHostFps`.
Limita anche il gioco, con il suo limitatore, a 60-90 fps: l'host può mostrare solo i frame che il gioco ha disegnato,
e ogni frame dell'host esegue la rete una volta.

**Cosa l'host non può dare alla rete.** Una finestra catturata non ha profondità, motion vector, jitter né esposizione
propri; l'host stima il movimento. Perciò la scheda Anywhere indica cosa viene stimato; le righe della scheda Neural
che lì non possono agire sono nascoste o rifiutate con un motivo (Ray Regeneration, Screen GI e Model interleave -
spenderebbe la modifica trasportata su un movimento stimato); e una riga di stato nella pagina Anywhere dice quando la
rete supera il budget di frame dell'host e cosa abbassare. **Una finestra di gioco da 1920x1080 o più piccola è il caso
esatto al pixel:** una finestra più grande viene prima ridotta al tetto della rete e poi riportata su con l'upscaling,
e la pagina lo dice, con la quota dei pixel dello schermo che la rete ha visto. La riga NR resolution mostra la
dimensione reale della rete dentro l'host.

**Stato: preview.** Solo RX 9000 (RDNA 4) in questa release; le RX 7000 seguiranno una volta testato lì. Può restare un
leggero sfarfallio o qualche scatto nei movimenti rapidi (abbassa il livello NR a 720, limita il gioco a 60-90 fps,
lascia Model interleave spento - l'host lo rifiuta). L'host di cattura viene scaricato dalla release GitHub del suo
autore, non dalla nostra. Segnalazioni: lo zip di **Save report** dal menu dentro l'host (il suo titolo indica il gioco
scalato), oppure il COLLECT LOGS del launcher.

## Linux / Proton (Steam Deck, Linux desktop)

AMDNR gira sotto Proton e Wine come build di OptiScaler. **Il Neural Rendering non gira su Linux (solo Windows):**
entrambi i runtime NR richiedono l'HIP del driver AMD per Windows, che Proton e Wine non forniscono. Con NR attivato
sotto Proton, NR non gira, e dalla 0.3.5 la scheda Neural e il report lo dicono (invece di "Idle"). È il comportamento
previsto, non un crash né un'installazione difettosa. L'AMDNR Launcher è un programma Windows che può girare sotto
Proton (sperimentale, non ancora testato da noi, vedi sotto); l'installazione a mano funziona senza il launcher.

**Cosa funziona:** gli upscaler FSR (FSR 3.1, e FSR 4 sulle GPU e i driver che lo supportano), il menu (`INSERT`) e
**Save report**. Un giocatore ha confermato su Steam Proton (RX 9070 XT, vkd3d-proton, Resident Evil Requiem) che il
gioco si avvia, il menu si apre e riceve l'input del mouse, e Save report funziona, anche con la frame generation
attiva.

**Ray Regeneration e frame generation:**
- Problema noto: Ray Regeneration può mostrare chiazze rosa / magenta su Proton; lo sharpening predefinito ora lì è disattivato, ma se le vedi ancora usa il semplice FSR (FSR 4 su RX 9000) e invia un Save report.
  Su Proton, AMDNR non aggiunge sharpening dopo Ray Regeneration quando il gioco non ne invia (su Windows aggiunge
  0.25): Image > Sharpness mostra "RR default 0 (off on Proton)", e Override imposta comunque il tuo valore.
- **La frame generation** ora si attiva senza crash, ma i contatori degli fps contano anche i frame generati: con un
  limite a 60 fps o con il V-Sync a 60 Hz i frame reali sono 30, e l'immagine sembra a 30 fps. Per ora lasciala
  disattivata su Proton (`[FrameGen] FGOutput=nofg`), oppure usala solo quando il gioco arriva a circa 60 fps senza di
  essa, su uno schermo a più di 60 Hz.
- **Titoli Vulkan** (giochi RTX Remix, id Tech 8), su Proton come su Windows: Ray Reconstruction e la frame generation
  propria di AMDNR rispondono "not supported" per scelta (il denoiser è D3D12, e i buffer del ray tracing restano sul
  device Vulkan); dalla 0.3.5 le schede Ray Regeneration e Frame Gen lo dicono, invece di chiederti di attivarle in un
  gioco che le mostra in grigio. La super resolution DLSS funziona tramite il bridge.
- Dalla 0.3.5, `[Spoofing] Dxgi=true` sotto Proton (la via di aggiornamento a FSR 4) non provoca più un errore all'avvio.

**Requisiti:** un Proton o un Wine recente (testato: Proton 11, cioè Wine 11), con il gioco su vkd3d-proton (D3D12) o
DXVK (D3D11), che è il predefinito di Proton. Le versioni più vecchie non sono testate.

**L'AMDNR Launcher su Linux (sperimentale, non ancora testato da noi).** Il launcher è lo stesso programma Windows,
`AMDNR-Launcher.exe` (autonomo: non serve installare .NET né altri runtime). Sotto Wine / Proton riconosce Wine e
mostra un avviso con cosa fare. Cerca anche la tua libreria Steam di Linux tramite l'unità `Z:` di Wine
(`~/.steam/steam` e `~/.local/share/Steam`, e le cartelle della libreria elencate in `libraryfolders.vdf`). Non
l'abbiamo ancora testato noi stessi: se lo provi, facci sapere su Discord se funziona. Per provarlo:

1. In Steam, aggiungi `AMDNR-Launcher.exe` come gioco non di Steam: **Giochi > Aggiungi un gioco non di Steam alla
   mia libreria** (Games > Add a Non-Steam Game to My Library).
2. Nelle sue **Proprietà > Compatibilità** (Properties > Compatibility), forza una versione di Proton (Proton
   Experimental), poi avvialo da Steam.
3. Se il gioco non è in **LIBRARY** (LIBRERIA), premi **ADD** (AGGIUNGI) e scegli la cartella del gioco (le tue
   cartelle di Linux sono sull'unità `Z:`); se il launcher sceglie il `.exe` sbagliato, usa
   **CHOOSE GAME .EXE** (SCEGLI IL .EXE DEL GIOCO).
4. Seleziona il gioco e premi **INSTALL** (INSTALLA).
5. Nelle **Proprietà > Generali > Opzioni di avvio** del gioco (Properties > General > Launch Options), inserisci
   `WINEDLLOVERRIDES="dxgi=n,b" %command%` (se il launcher ha usato un altro nome di DLL per il gioco, metti quel
   nome al posto di `dxgi`), poi prosegui con i passaggi 5 e 6 dell'installazione a mano qui sotto (avvia il gioco
   da Steam; il pulsante **PLAY** del launcher è disattivato sotto Wine / Proton).

**Installazione a mano** (senza il launcher):

1. Scarica `AMDNR-vX.X.X.zip` dalla pagina delle release (per la 0.3.5: `AMDNR-v0.3.5.zip`). Gli zip del runtime
   danielblnc (`v0.5.0-Runtime.zip` e gli altri) servono solo al Neural Rendering, quindi su Linux non ti servono
   (copiarne uno non fa danni).
2. Estrai lo zip e copia tutto nella cartella del gioco, accanto al file `.exe` del gioco.
3. Rinomina `OptiScaler.dll` in `dxgi.dll`.
4. In Steam, apri **Proprietà > Generali > Opzioni di avvio** del gioco (Properties > General > Launch Options) e
   inserisci:

   ```
   WINEDLLOVERRIDES="dxgi=n,b" %command%
   ```

   Questo dice a Wine di caricare il `dxgi.dll` nella cartella del gioco invece del proprio; senza, AMDNR non si
   carica. Se hai usato un altro nome (ad esempio `winmm.dll` o `version.dll`), metti quel nome al posto di `dxgi`,
   ad es. `WINEDLLOVERRIDES="winmm=n,b" %command%`. Lutris, Heroic e Bottles: aggiungi lo stesso override (`dxgi` =
   `native,builtin`) negli override delle DLL o nelle variabili d'ambiente del runner.
5. In `OptiScaler.ini`, imposta `[FrameGen] FGOutput=nofg` (frame generation disattivata, vedi sopra).
6. Avvia il gioco e premi `INSERT` per aprire il menu. Configura lì l'upscaler.

**Il menu.** Nella 0.3.4, con la frame generation attiva, il menu poteva aprirsi senza ricevere l'input di mouse o
tastiera, o non aprirsi affatto. La 0.3.4.1 aggancia il menu alla finestra del gioco; un giocatore ha confermato su
Proton che il menu si apre e riceve l'input del mouse, anche con la frame generation attiva. Se sul tuo sistema
succede ancora, AMDNR mostra l'avviso "Menu window lost". In quel caso, in `OptiScaler.ini`, imposta
`[FrameGen] FGOutput=nofg`; se il menu continua a non rispondere, imposta anche `[Menu] OverlayMenu=false` (il menu
classico, che non dipende dalla finestra di overlay).

**HDR.** AMDNR non attiva l'HDR sotto Proton. L'HDR dipende dalla tua configurazione di Proton e del desktop: una
build di Proton con supporto HDR e una sessione in grado di mostrare l'HDR (ad esempio gamescope, o un desktop
Wayland con l'HDR attivo). Se l'HDR funziona nel gioco senza AMDNR, continua a funzionare con AMDNR; se l'opzione HDR
del gioco è in grigio, la soluzione sta nella configurazione di Proton o del desktop.

**Segnalare un problema su Linux:** usa il pulsante **Save report** del menu (lascia `[Log] LogToFile=true`, il
valore predefinito, così il report contiene il log di questa sessione); il report indica se il gioco girava sotto
Wine/Proton, vkd3d-proton o DXVK. Aggiungi la tua distribuzione, la GPU, la versione di Mesa e quella di Proton.

## Requisiti

- Windows 10 o 11 (64 bit) per il Neural Rendering. L'AMDNR Launcher è un programma Windows che può girare sotto
  Proton (sperimentale, non ancora testato da noi). Sotto Linux / Proton AMDNR gira come build di OptiScaler senza NR
  (vedi "Linux / Proton").
- Una GPU AMD con AMD Software: Adrenalin Edition 26.9.1 o più recente. Il runtime neurale usa HIP tramite il driver; non servono
  l'SDK HIP né la modalità sviluppatore. Per quanto riguarda i chip:
  - RX 9000 (RDNA 4): entrambi i runtime.
  - RX 7000 (RDNA 3, desktop e mobile): entrambi i runtime - lmxxf tramite il backend RDNA 3 di AMDNR, più lento
    che su RDNA 4 (il livello di dimensione della rete è attivo di default, vedi sopra).
  - Strix Halo (Radeon 8060S / 8050S): lmxxf.
  - APU per handheld con 12+ compute unit (Z1 Extreme / Z2 / 780M, Z2 Extreme / 890M / 880M): lmxxf,
    sperimentale e lento. Z1 (4 CU), 760M / 740M e 860M / 840M: non supportate.
  - RX 6000 (RDNA 2): non ancora supportate, previste per la 0.3.6. Steam Deck e APU RDNA 2: non supportate (per il
    Neural Rendering; per gli upscaler sotto Proton vedi "Linux / Proton").

  La scheda Neural indica cosa può eseguire la tua GPU (passa il mouse sulle voci dei runtime, o guarda la riga
  GPU in Diagnostics).
- **AMDNR Anywhere** (preview): Windows, una scheda RX 9000 (RDNA 4) in questa release, e l'AMDNR Launcher, che
  scarica l'host di cattura; il gioco gira in finestra o in finestra senza bordi. Vedi "AMDNR Anywhere".
- Un gioco Direct3D 12, Direct3D 11 o Vulkan. Il percorso neurale AMD in sé è D3D12; i titoli D3D11 e
  Vulkan ci arrivano tramite il bridge D3D12 di OptiScaler, il che significa che l'upscaler deve essere
  uno dei backend "w/Dx12" (`ffx_12`). Lascia `Dx11Upscaler` / `VulkanUpscaler` su `auto` e questa
  build lo sceglie per te quando il neural rendering è attivo. Con il Neural Rendering attivo, l'elenco di Upscaling li chiama "... w/Dx12 - Neural".
- Circa 2 GB di VRAM libera a risoluzioni di rendering di classe 1080p.

## Cosa contengono gli archivi

**AMDNR-vX.X.X.zip**

| File | Cos'è |
|---|---|
| `OptiScaler.dll` | OptiScaler con il backend AMD di DLSS-NR (AMDNR 0.3.5). Rinominalo come indicato nella guida. |
| `OptiScaler.ini` | Impostazioni. Il Neural Rendering è abilitato; il logging è attivo, così hai qualcosa da allegare a una segnalazione di bug. |
| `LmxxfNrRuntime.dll` | Il runtime neurale lmxxf (0.3.5: verifica ogni modulo HIP del pak rispetto all'elenco di digest del pak stesso prima di usarlo, nomina entrambe le parti quando nessun adattatore HIP corrisponde alla GPU del gioco, mantiene i passi di risoluzione dinamica senza ricostruire la rete, e non legge più le variabili d'ambiente di lmxxf che cambiano l'immagine; i kernel di lmxxf, compresi quelli di lmxxf 0.31, i kernel c32w di AMDNR, le dimensioni di rete piccole e la maschera nativa dei personaggi). Usato solo se scelto; legge `LmxxfNrRuntime.pak` accanto a sé, vedi "Il runtime lmxxf". |
| `LmxxfNrRuntime.pak` | Pesi, moduli HIP e shader del runtime lmxxf in un unico file cifrato (440 MB; 0.3.5: il set di moduli per RX 7000 e per gli handheld di classe Z1 Extreme - Z1 Extreme, Z2, Radeon 780M - è circa il 10% più veloce, stessa immagine; le RX 9000 ricevono i moduli di lmxxf 0.37 di Kien (MIT) accanto al loro set di base invariato; i moduli per Z2 Extreme / 890M / 880M e Strix Halo sono invariati). Lo legge solo il runtime lmxxf; tenerlo insieme al runtime danielblnc non crea problemi. |
| `OptiScaler\` | FSR, XeSS, il denoiser FidelityFX e il D3D12 Agility SDK usati da OptiScaler. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | dlssg-to-fsr3 di Nukem9, non modificato e rinominato: le chiamate DLSS Frame Generation del gioco vengono gestite dalla frame generation di FSR 3, anche su Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, vedi `Licenses/`. |
| `Licenses\`, `LICENSE` | Licenze di terze parti, l'avviso di AMDNR (`AMDNR_NOTICE.txt`) e la licenza GPL-3.0 di questa build. |
| `SHA256SUMS.txt` | Checksum di ogni file di questo zip e dei file degli zip del runtime danielblnc che elenca. |

**Gli zip del runtime danielblnc** (DLSS-NR on AMD by Daniel Blanco, non modificato, con il suo permesso; usane uno)

Quale usare: su **RX 9000 e su RX 7000**, `v0.5.0-Runtime.zip` (consigliato) dalla release Alpha0.3.4.2;
`v0.4.3-Runtime.zip` (Alpha0.3.4.2 e Alpha0.3.4.1), `v0.4.1-Runtime.zip` e `v0.4.0-Runtime.zip` (Alpha0.3.4.1)
restano accettati. Il runtime lmxxf non richiede alcuno zip di runtime su RX 7000 e RX 9000; le APU per handheld
usano solo lmxxf. L'AMDNR Launcher offre 0.5.0 (consigliato), 0.4.3, 0.4.1 e 0.4.0, e lo sceglie per te.

| Zip | Release | Runtime danielblnc |
|---|---|---|
| `v0.5.0-Runtime.zip` | Alpha0.3.4.2 | 0.5.0, **consigliato su RX 9000 e RX 7000**; le impostazioni del runtime danielblnc funzionano con questo runtime, e dalla 0.3.5 la tua chiave `Async` nel suo `dlssnr_on_amd.ini` gli arriva |
| `v0.4.3-Runtime.zip` | Alpha0.3.4.2 (e Alpha0.3.4.1) | 0.4.3, ancora accettato; le impostazioni del runtime danielblnc funzionano con questo runtime |
| `v0.4.1-Runtime.zip` | Alpha0.3.4.1 (e Alpha0.3.4) | 0.4.1, ancora accettato. Network style, Tone curve, Black lift e Game exposure sono in grigio con questo runtime |
| `v0.4.0-Runtime.zip` | Alpha0.3.4.1 (e Alpha0.3.4) | 0.4.0, ancora accettato; le impostazioni del runtime danielblnc funzionano con questo runtime |
| `v0.3.3-Runtime.zip` | [Alpha0.3.4](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/tag/Alpha0.3.4) | 0.3.3, ritirato: non più consigliato. Funziona ancora se lo hai già; le impostazioni del runtime danielblnc funzionano con questo runtime |
| `Runtime.zip` | Alpha0.3.4 | 0.3.1; le impostazioni del runtime danielblnc sono in grigio con questo runtime |

**danielblnc 0.5.0 è il runtime danielblnc consigliato dalla 0.3.5** (gira anche sulla 0.3.4.2). La 0.3.5 inoltre
passa al runtime la tua chiave `Async` (o la più vecchia `Inline`) da `dlssnr_on_amd.ini` invece di forzare la modalità
same-frame; senza nessuna delle due chiavi un'installazione predefinita non cambia. Una build danielblnc più recente
della 0.5.0 non viene gestita da questa release.

Ognuno contiene:

| File | Cos'è |
|---|---|
| `dlssnr_amd_pass1..3.dll` | Il runtime neurale AMD, non modificato. Tre copie, così il multi-pass ne ha una per ogni pass. |
| `dlssnr_on_amd_weights.bin` | I pesi della rete caricati dal runtime. |
| `danielblnc_ATTRIBUTION.txt` | Il credito di Daniel Blanco e le condizioni alle quali AMDNR distribuisce il suo runtime. |

## Il menu (novità della 0.3.4)

Premi `INSERT`. Tutte le schede hanno lo stesso stile: schede di testo, una riga di intestazione con Discord e
GitHub (apre questa pagina), una riga di crediti (il nome di Daniel Blanco apre la sua pagina GitHub), la riga **Components** (quanti dei sette
componenti di OptiScaler sono attivi; cliccala per l'elenco), e un piè di pagina con Menu Scale, Save Settings e Close. L'aiuto si apre passando il mouse
sull'etichetta di un controllo.

**La scheda Neural, dall'alto in basso:**

- **Enable Neural Rendering** e il suo tasto (il pulsante, ad es. `Home`: cliccalo, poi premi un altro tasto per
  riassegnarlo).
- **Neural runtime** (danielblnc / lmxxf, con la versione esatta dei tuoi file, ad es. `lmxxf 0.3.4`) con una parola di stato: running, restart the game to switch, not
  installed, not for this GPU o stopped. Sotto, il credito del runtime attivo e una riga di stato, ad es.
  `Running - 1920x1080 at 100% - NR 62/s - model 62/s - 15.3 ms` (l'ultimo numero è il costo NR), e una riga **Live** chiusa con più
  dettagli. Quando qualcosa
  richiede la tua attenzione segue una riga arancione, con un pulsante quando c'è una soluzione (Retry lmxxf,
  Switch to danielblnc, Open Upscaling). Nello stato predefinito non ce n'è nessuna.
- **Preset**: Quality / Balanced / Performance impostano la NR resolution a 100 / 85 / 70% e spengono Dynamic NR;
  nient'altro. Sulle APU per handheld c'è un quarto pulsante, **Handheld** (vedi "APU per handheld"). **NR style**, e
  **Style slots** (Store / Apply / Clear).
- **Performance**: **Placement** (prima / dopo l'upscaling, novità della 0.3.5; vedi "Impostazioni da conoscere"), NR
  resolution (%) con il suo costo, Neural passes, Full network, Fast mode, Dynamic NR resolution, Model interleave
  (Interleave preset e la riga del pacing compaiono sotto mentre è attivo).
- **Quality**: Residual strength, Residual limit, Temporal stability, Sharpening (CAS), e **More quality options**
  (Network history - un'unica casella per entrambi i runtime -, Output smoothing, Stability mode, Residual
  temporal, Residual edge fade, Still-surface steadiness).
- **Image look**: Colour composition, Detail e Colour strength, e tre sezioni richiudibili: **Model strength**
  (Tone e Structure intensity, Character structure, Edit detail / colour, Edge guard, Native character mask, e
  Network style, Tone curve e Black lift di danielblnc), **Exposure and highlights** (Auto-exposure, il suo limite
  delle alte luci, Highlight colour guard, Game exposure) e **Appearance filter** (con la parola off / on dopo il nome). Un "default" o "custom" tenue dopo il nome di una sezione
  richiudibile indica se hai cambiato qualcosa al suo interno.
- **Ray Regeneration**: dalla 0.3.5 un rimando. Mentre Ray Regeneration è in funzione nel titolo, i suoi controlli sono
  su una scheda **Ray Regeneration** tutta loro (subito dopo Upscaling) e questa riga offre un pulsante **Open Ray
  Regeneration**; mentre non è in funzione, la riga dice perché (il gioco non ha attivato la Ray Reconstruction, il
  driver ha rifiutato il denoiser su questa scheda video, Ray Regeneration ha rinunciato a questo titolo e perché, o
  quando è stata eseguita l'ultima volta).
- **La riga degli strumenti**, chiusa all'avvio: **Diagnostics** (Network output, Debug view, Edit shaper A/B, NR cost,
  le letture di ghosting e di auto-regolazione, la riga GPU, **Save report**; la vista di debug RR è nella scheda Ray
  Regeneration dalla 0.3.5), **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy) ed
  **Experimental** (AMDNR Screen-space GI, in preview).

Un controllo che il runtime attivo non ha è in grigio con una breve etichetta (ad es. "not in lmxxf yet") o
nascosto con un conteggio ("3 danielblnc-only options hidden"); cambiare runtime non sposta nessun'altra riga.

**Le altre schede:** Upscaling inizia con l'upscaler, una riga di stato e Render resolution (i vecchi Upscale Ratio
Override e Output Scaling); su una scheda video non NVIDIA "DLSS w/Dx12" non compare più. **Ray Regeneration** (novità
della 0.3.5) segue Upscaling mentre Ray Regeneration è in funzione nel titolo: le righe di stato (per scheda video e per
API), la riga **Denoiser backend** (Automatic / Off - Off dice al gioco che la Ray Reconstruction non è supportata, così
tiene il proprio denoiser; dopo un riavvio), i controlli, More Ray Regeneration options, e un blocco Diagnostics tutto
suo con la vista di debug RR e il numero del rumore (grana in ingresso e in uscita, sfarfallio a camera ferma); non
viene mai disegnata dentro AMDNR Anywhere. Image contiene Sharpness, Textures, Init Flags e il Magnifier. Frame Gen
inizia con FG Input e FG Output e, dalla 0.3.5, con una riga che nomina il passaggio che manca ancora prima che venga
generato qualcosa. Interface ha l'overlay degli FPS e Keybinds (un pulsante per tasto). Advanced inizia con Active
Quirks, poi Display (V-Sync), Compatibility e Logging. Dentro AMDNR Anywhere il menu mostra una scheda **Anywhere** (la
riga di cattura e di rete, le impostazioni dell'host) al posto delle schede Frame Gen e Advanced. Le impostazioni, le
chiavi e ciò che scrive Save Settings non cambiano, tranne dove lo dice `CHANGELOG.md`.

## Impostazioni da conoscere

Apri la scheda **Neural**. I valori predefiniti sono la configurazione testata più recente, quindi la
prima mossa utile è cambiare una cosa alla volta.

- **NR resolution** — la leva principale tra qualità e costo. Sotto il 100% il modello lavora su
  un'immagine più piccola e solo la sua *correzione* viene riportata sul frame a piena risoluzione,
  così il frame mantiene il proprio dettaglio. Sopra il 100% il costo cresce con il quadrato (150%
  equivale a 2.25x). Lo slider si muove a scatti del 5%: ogni nuova dimensione NR può trattenere VRAM
  fino al riavvio del gioco, quindi riavvia il gioco dopo molte modifiche.
  Il costo accanto segna 1.00x al 100%; con lmxxf è il prezzo del livello di dimensione della rete su cui gira (il
  suo tooltip indica il livello). I pulsanti Preset lo impostano a 100 / 85 / 70%.
- **Placement** (Neural > Performance, `[DlssNr] AmdPlacement = pre | post`, novità della 0.3.5, entrambi i runtime) —
  dove gira il pass neurale. `pre` (il predefinito, e ciò che faceva ogni build precedente) modifica l'immagine a
  risoluzione di render che l'upscaler sta per leggere. `post` modifica invece l'immagine finita dell'upscaler, a
  risoluzione di visualizzazione: più nitido, perché l'upscaler non rifiltra più la modifica, e più costoso - la rete
  gira alla dimensione dello schermo fino al suo tetto di 1920x1080, quindi uno schermo 1080p paga il livello più alto
  a qualsiasi NR resolution, e uno schermo 1440p o 4K riceve una modifica a 1080 righe riportata su - un po' meno
  tollerante in movimento, e l'HUD è compreso se il gioco lo compone prima dell'upscaling. Rifiutato (si torna a
  `pre`, una riga in `amd_bridge.log`) dentro AMDNR Anywhere, in modalità immagine finale, e una volta che Ray
  Regeneration è entrata in funzione nel titolo. La riga NR resolution mostra entrambe le dimensioni mentre `post` è
  attivo.
- **Residual strength** — quanta parte della modifica del modello viene applicata; sopra 1 la
  amplifica. È il controllo che cambia di più l'immagine.
- **Residual limit** — un tetto a quanto può variare un singolo pixel. Se vedi chiazze: **abbassalo**.
- **Model interleave** — esegue il modello un frame sì e uno no per un grande guadagno di frame rate.
  I frame saltati vengono riempiti dall'**Interleave preset**; il predefinito è *Edit accumulation*
  (preset 10, entrambi i runtime): ogni frame è l'immagine di quel frame più la correzione portata dal
  modello, quindi nessuna immagine precedente viene trattenuta. *Guided fill v2* (preset 6, danielblnc) e
  *Classic carry* (lmxxf) sono i riempimenti precedenti. Il pacing dei due tipi di frame è automatico con
  danielblnc e spento con lmxxf (`[DlssNr] AmdInterleavePacing` tra 0 e 1 li cadenza entrambi, al costo di
  qualche fps); una riga attenuata sotto il preset mostra la misura. Adaptive interleave è disattivato in questa
  build.
- **Neural passes** — 2 e 3 impilano il modello, con rendimenti decrescenti. Con lmxxf la history
  della rete resta quella del primo pass; i pass extra sono solo affinamento spaziale.
  danielblnc esegue 1 pass nei titoli Vulkan (lo indica una nota sotto lo slider).
- **Colour composition** (Neural > Image look, entrambi i runtime) — *Classic* (predefinito) è
  l'immagine che avevi prima. *RenoDX (experimental)* esegue la composizione colore di RenoDX dopo il
  modello, come fa il percorso NVIDIA: Composition detail e colour, un **Highlight guard** bidirezionale
  (2x di default) che limita la risposta del modello rispetto all'originale, e controlli opzionali per
  pelle / ambiente. Su un frame display-referred (SDR), con Network output o con Encoding sRGB / Gamma 2.2
  torna a Classic su entrambi i runtime; la nota del menu offre allora un pulsante che toglie il blocco. Gli
  stili e i preset NR non lo toccano.
- **Native character mask** (Image look > Model strength, `[DlssNr] AutoMask`, attivo di default) — il
  trattamento proprio del modello per volti e pelle. Toglierne la spunta ora agisce su entrambi i runtime (con
  lmxxf ricostruisce la rete: uno scatto di circa 1 s); con lmxxf ora agiscono anche Structure
  intensity e Character structure.
- **La frame generation è disattivata in un ini nuovo**, e servono cinque passaggi: FG Input e FG Output nella scheda
  Frame Gen (ad es. "DLSSG via Streamline" in un gioco con la frame generation DLSS, e XeFG), **Save Settings**, un
  riavvio completo del gioco, la frame generation **propria** del gioco attivata, poi **Active** spuntato sotto Frame
  Generation. Dalla 0.3.5 la scheda e il log nominano il passaggio che manca. L'elenco completo, con cosa spegnere nel
  gioco, è nelle FAQ qui sotto ("Frame generation: nessun guadagno di fps?").
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
- **FSR Ray Regeneration** — su RX 9000 (RDNA 4); su RX 7000 (RDNA 3) solo come opzione sperimentale, non supportata (vedi sotto); solo nei giochi che usano DLSS Ray Reconstruction (Cyberpunk 2077,
  Alan Wake 2), con il gioco impostato su DLSS (spoofing attivo) e con ray tracing e Ray Reconstruction
  attivati nelle sue impostazioni. Il Neural Rendering viene quindi eseguito dopo di essa, sul suo
  output, il che costa di più: abbassa la NR resolution se il frame rate cala. Dalla 0.3.5 i suoi controlli sono su
  una scheda **Ray Regeneration** tutta loro, subito dopo Upscaling, disegnata mentre Ray Regeneration è in funzione
  nel titolo; la scheda Neural vi rimanda e, mentre Ray Regeneration non è in funzione, mantiene la riga in grigio che
  dice perché (il gioco non ha attivato la Ray Reconstruction, il driver ha rifiutato il denoiser su questa scheda
  video, Ray Regeneration ha rinunciato a questo titolo e perché, o quando è stata eseguita l'ultima volta). La riga
  **Denoiser backend** della scheda (`[FSR-RR] RrBackend = auto | off`) può dire al gioco che la Ray Reconstruction
  non è supportata, così tiene il proprio denoiser (al prossimo avvio del gioco). In un titolo **Vulkan** la Ray
  Reconstruction è "not supported" per scelta (il denoiser è D3D12) e la scheda lo dice. Il **profilo path-traced**
  (meno grana sui volti con il path tracing) va attivato manualmente dalla 0.3.3.1: spuntalo lì per provarlo in
  Resident Evil Requiem o PRAGMATA. La stessa scheda ha l'intensità della bias mask e lo **smoothing della pelle**
  (sperimentale, per i giochi che pubblicano una guida SSS; disattivato di default, ma attivo di default in Resident
  Evil Requiem dalla 0.3.3.2); i controlli di regolazione temporale sono in *More Ray Regeneration options*, e la
  vista di debug RR e il numero del rumore (grana in ingresso e in uscita, sfarfallio a camera ferma) sono nel blocco
  Diagnostics della scheda. Su RX 7000 (RDNA 3) Ray Regeneration non è supportata
  e, dalla 0.3.5, non è più offerta di default: il denoiser di AMD non ha un provider per RDNA 3, quindi il gioco tiene
  il proprio denoiser. La casella della scheda Upscaling **Experimental: Ray Regeneration on this card (restart)**
  (con il tag "experimental - not supported") serve solo per i test: se la spunti, il denoiser si rifiuta di partire e
  il gioco riceve FSR senza denoiser, che può risultare più rumoroso del denoiser del gioco. È previsto un denoiser
  tutto di AMDNR per le RX 7000. Le RX 6000 e precedenti la ricevono solo con `[FSR-RR] FfxDenoiserAllowPreRdna4=true`
  (scheda Upscaling: **Offer FSR Ray Regeneration on this GPU (restart)**). **Sharpening dopo RR** (0.3.4.1): quando il
  gioco non invia alcun valore di nitidezza, AMDNR applica uno sharpening di 0.25 dopo RR su Windows (0 su Linux /
  Proton); per disattivarlo: Image > Sharpness, spunta Override, slider a 0. Dalla 0.3.4.2 quel numero è una chiave dell'ini
  tutta sua, `[Sharpness] RrDefaultSharpness` (stesso valore predefinito 0.25): mettici 0.15, 0.10 o 0 senza toccare
  Override, e un valore che il tuo ini ha conservato sotto `[Sharpness] Sharpness` con Override disattivato viene
  segnalato nel menu come in attesa.
- **AMDNR Screen GI** (preview, nuovo in 0.3.4, disattivato di default; Neural > Experimental, o `[AmdGi] Enabled=true`) — la luce rimbalzata e l'occlusione ambientale in screen space di AMDNR, dalla profondità del gioco, prima di NR e dell'upscaler; funziona con NR attivo o spento; circa 1 ms in High con un render 1080p su una RX 9070 XT (misurato fuori da un gioco). È screen space: manca la luce che arriva da fuori schermo. Vedi `CHANGELOG.md`.
- **Save report** (Neural > Diagnostics, o Advanced > Logging) — uno zip con tutti i log e i file ini per una segnalazione; vedi "Se
  non funziona" sopra.

## Se qualcosa va storto

`OptiScaler.log` compare nella cartella del gioco. Allegalo in `#bug-report` e indica il gioco e
la GPU; **Save report** (Neural > Diagnostics, o Advanced > Logging) lo comprime insieme a tutto il resto. Il backend AMD scrive
anche `amd_presr.log` e `amd_bridge.log`, che sono quelli utili quando è proprio il pass neurale a comportarsi
male. I log delle ultime tre sessioni vengono conservati come `OptiScaler.previous.<exe>.log` (il più recente),
`OptiScaler.previous-1.<exe>.log` e `OptiScaler.previous-2.<exe>.log` (`[Log] KeepPreviousLogs`; 1 ne conserva
uno solo, come prima). Dopo un crash, allega anche quelli: il nuovo log riporta allora "no clean exit recorded"
(dalla 0.3.4 non più dopo un'uscita normale).

**NR frames 0/s, e la scheda Neural o `amd_presr.log` dicono che la DLL del pass è una build che questo
AMDNR non gestisce?** Le tue `dlssnr_amd_pass1..3.dll` sono una build di danielblnc che questo AMDNR non
conosce (in giro è stato visto un set 0.2.16), oppure ne manca una delle tre. Dalla 0.3.3.2 la scheda
Neural indica il nome del file e la sua versione e dice cosa fare. Usa il runtime consigliato, prendendo
tutte e tre le DLL dei pass dallo stesso zip: su **RX 9000 e su RX 7000**, `v0.5.0-Runtime.zip` dalla release
Alpha0.3.4.2 (149,550,553 byte, con SHA256 che inizia per `7a49ab0e`); `v0.4.3-Runtime.zip` (Alpha0.3.4.2 e
Alpha0.3.4.1; 116,484,918 byte, con SHA256 che inizia per `07dd7774`), `v0.4.1-Runtime.zip` e `v0.4.0-Runtime.zip`
(Alpha0.3.4.1) restano accettati (il file `dlssnr_amd_pass1.dll` in `v0.4.1-Runtime.zip` pesa 9,916,928 byte, con
SHA256 che inizia per `823063eb`; in `v0.4.0-Runtime.zip`: 10,027,008 byte, `d62be3d8`). Build supportate: 0.2.17,
0.3.0, 0.3.1, 0.3.2, 0.3.3, 0.4.0 e gli zip di runtime citati sopra, fino alla 0.5.0. Non installare il setup di
danielblnc né i suoi file `dxgi.dll` / `version.dll` /
`winhttp.dll` accanto ad AMDNR: AMDNR esegue già il suo runtime. **Una build danielblnc più recente della 0.5.0 non
viene gestita da questa release:** la scheda Neural nomina il file e la sua versione e lo dice. Con la 0.5.0, la 0.4.3,
la 0.4.1 e la 0.4.0 le impostazioni esclusive di danielblnc (Network style, Tone curve, Black lift, Game exposure, Fast
mode) funzionano, e dalla 0.3.5 la tua chiave `Async` in `dlssnr_on_amd.ini` arriva al runtime (vedi "Cosa contengono
gli archivi").

**lmxxf non fa nulla, o si ferma subito, su un PC con grafica integrata?** Risolto nella 0.3.3.2. Su un
Ryzen desktop con la grafica integrata attiva, un portatile con APU AMD e una Radeon, o un PC con due GPU
AMD, la GPU del gioco spesso non è il device HIP 0. lmxxf allora falliva al primo frame
(`hipErrorInvalidHandle (400)`, poi "session is poisoned" in `lmxxf_backend.log`) e restava disattivato.
Sostituisci sia `OptiScaler.dll` (il file che hai rinominato, ad es. `dxgi.dll`) sia `LmxxfNrRuntime.dll`
con i file della 0.3.3.2 o successiva. Non ancora testato su un PC del genere: se lmxxf si ferma ancora, la scheda
Neural ora dice perché; invia `lmxxf_backend.log` e `amd_bridge.log` (elenca i device HIP).

**Un PC con grafica integrata e una Radeon (un Ryzen desktop con la GPU integrata attiva, o un portatile): NR non parte
mai, e la scheda Neural o la riga GPU nomina la GPU integrata?** Il passaggio neurale di AMDNR gira sulla GPU con cui il
gioco disegna. Se Windows ha avviato il gioco sulla GPU integrata, NR non gira affatto sulla tua Radeon. Assegna il
gioco alla GPU dedicata: Impostazioni di Windows > Sistema > Schermo > Grafica, aggiungi l'`.exe` del gioco, Opzioni,
Prestazioni elevate; poi riavvia il gioco e controlla la riga GPU in Neural > Diagnostics, che nomina l'adattatore su
cui gira NR (`OptiScaler.log` ha una riga `AMD neural: NR runs on ...` quando quell'adattatore non è la GPU
principale). Visto in Starfield su un Ryzen desktop. Dalla 0.3.5 la riga della scheda Neural dice quale dei tre casi
è - nessun runtime ancora, NR in esecuzione **sulla GPU integrata** (un'APU che un runtime accetta, come una Radeon
780M accanto a una scheda Radeon: NR gira lì, molto più lentamente che sulla scheda), oppure un runtime su un altro
adattatore - e `amd_bridge.log` elenca ogni adattatore una volta.

**La riga di stato di lmxxf dice `c32w=off:nofile` su una RX 9070 / 9070 XT?** Una vecchia cartella
`DLSS5-AMD\native-game-tiled-assets` accanto all'`.exe` del gioco (rimasta da una precedente installazione di
lmxxf) viene usata al posto di `LmxxfNrRuntime.pak`. Non contiene i kernel c32w, quindi lmxxf gira alla vecchia
velocità. Elimina o rinomina la cartella `DLSS5-AMD`: il pak contiene tutto ciò che serve a lmxxf. Un
`LmxxfNrRuntime.pak` precedente alla 0.3.3.2 mostra lo stesso stato; sostituiscilo con quello di questa release.
`fk=fff-` nella stessa riga significa la stessa cosa (un vecchio pak o una cartella sciolta): lmxxf gira
comunque, alla vecchia velocità.

**danielblnc: lo stile NR cambia ancora quando la NR resolution si allontana dal 100%?** Ancora aperto dalla 0.3.4 alla 0.3.5,
e il valore predefinito non cambia. Al 100%, Residual strength 0.99 dà il 99% di 1.00 (risolto nella 0.3.3.2);
lontano dal 100% (anche nei passi di Dynamic NR e nei preset Balanced / Performance) strength, limit ed edge fade
agiscono ancora sull'intero risultato, quindi l'aspetto può cambiare. La 0.3.4 aggiunge un A/B per trovare la
correzione giusta: Neural > Diagnostics > **Edit shaper (A/B, not saved)** con Literal, F1 e F2, più Only below
100% e Carry cap (solo danielblnc; Save Settings non lo salva; le chiavi dell'ini sono `[DlssNr] AmdEditShaper`,
`AmdEditShaperLimit`, `AmdEditShaperScope` e `AmdEditShaperCarryCap`). Se uno di questi fa sembrare l'85% come il
100% nel tuo gioco, faccelo sapere su Discord con degli screenshot. lmxxf non è interessato.

**Il menu si apriva e si chiudeva due volte a ogni pressione, o tastiera e mouse smettevano di funzionare su tutto il
desktop con il menu aperto (Assetto Corsa)?** Risolto nella 0.3.4: una seconda pressione del tasto del menu o di NR
entro 400 ms viene ignorata (`[Hotfix] MenuToggleDebounceMs`, 0 = il comportamento precedente), e con il menu aperto
l'hook di tastiera o mouse a basso livello del gioco viene saltato ma il tasto arriva comunque a Windows
(`[Hotfix] MenuLowLevelHookPassThrough=false` = il comportamento precedente). Non ancora confermato in Assetto
Corsa: se succede ancora, invia lo zip del report.

**Il menu si apriva da solo sul selettore del runtime, i clic non facevano nulla, o il tasto del menu lo nascondeva solo
finché restava premuto (Assetto Corsa)?** Risolto nella 0.3.4.2: il tasto del menu lo apre o chiude una sola volta per
pressione fisica (un messaggio di tasto che arriva in ritardo viene ignorato), i clic e i tasti del menu più brevi di
un frame vengono riprodotti, il selettore del runtime risponde a `1` / `2` / `Enter` / `Esc` e alla X della sua barra
del titolo, e chiudere il menu vale come "Decide later"; il selettore non apre più il menu da solo. Non
ancora confermato dal giocatore di Assetto Corsa: se succede ancora, invia lo zip del report. Per riavere il tasto del
menu e i clic della 0.3.4.1: aggiungi `DiagInputHooksSkip=presslatch,clickreplay` sotto `[Hotfix]` nel tuo
`OptiScaler.ini` (nessuna chiave nuova; l'ini incluso descrive la riga solo in un commento).
La 0.3.5 aggiunge due cose per Assetto Corsa: AMDNR si fa da parte davanti a un `nvngx.dll` estraneo nella cartella del
gioco e protegge la creazione del device D3D11On12, e un gioco DX11 che non presenta mai tramite D3D12 riceve una coda
D3D12 di bootstrap per il pass neurale (`[DlssNr] AmdBootstrapQueue`, auto). Non ancora confermato da un giocatore di
Assetto Corsa.

**Uncharted: Legacy of Thieves Collection andava in crash pochi secondi dopo l'avvio su RX 9000 con Neural Rendering
attivo?** Risolto nella 0.3.5: il gioco esegue il suo lavoro su piccole fiber da 192 KiB, e la prima inizializzazione
di HIP (il driver compila i suoi kernel ausiliari dentro il gioco) faceva traboccare quello stack al primo frame NR.
Il primo uso di HIP da parte del bridge neurale ora gira su uno stack grande tutto suo (`[DlssNr] BigStackCall`, auto).
Se per la 0.3.4.2 avevi impostato lì `[DlssNr] Enabled=false`, riattivalo. Non ancora confermato da un giocatore con
RX 9000: se succede ancora, invia lo zip del report.

**Un gioco con risoluzione dinamica (The Last of Us Part II) sfarfalla con il Neural Rendering attivo?** Risolto nella
0.3.5: il gioco cambiava la sua dimensione di render centinaia di volte al minuto, e ogni passo ricostruiva la rete e
azzerava la sua history. Un passo che resta dentro l'allocazione ora viene mantenuto (niente assestamento, niente
riscaldamento, niente azzeramento della history, entrambi i runtime); un frame più grande o un calo reale rialloca
ancora. La riga delle statistiche del report conta i passi mantenuti. Non ancora confermato in quel gioco.

**Frame generation: nessun guadagno di fps, o "restart the game" per sempre?** In quasi tutte le segnalazioni la frame
generation semplicemente non era stata attivata - spenta, non rotta. Servono cinque passaggi, e dalla 0.3.5 la scheda
Frame Gen e il log nominano quello che manca:

1. Scheda Frame Gen: scegli **entrambi**, FG Input e FG Output. Un gioco DX12 con una frame generation propria: la sua
   DLSS FG o FSR 3.1 FG è l'input (giochi con FSR 3.1 FG: "FSR 3.1 FG", non "FSR 3.0 FG"); nessuna FG nel gioco: FG
   Input = OptiFG (Upscaler), HUD fix attivo. DX11: solo OptiFG. Vulkan: nessun output FSR FG / XeFG (usa quella del
   gioco).
2. **Save Settings**.
3. **Chiudi completamente il gioco e riavvialo** - la FG non può attivarsi a partita in corso.
4. Nelle opzioni grafiche del gioco **attiva** la sua frame generation: DLSS Frame Generation (con DLSS come upscaler)
   per l'input DLSSG, la frame generation FSR (con FSR) per l'input FSR 3.1 FG.
5. Riapri il menu, scheda Frame Gen, spunta **Active** sotto Frame Generation. Non viene generato nulla finché quella
   casella non è spuntata (XeFG può chiedere un riavvio in più).

Poi **spegni** nel gioco: lo schermo intero esclusivo (XeFG richiede la finestra senza bordi), il V-Sync e i limiti di
frame (o limita al doppio dei tuoi fps di base), e la frame generation XeSS propria del gioco se ce l'ha (un solo
generatore di frame per finestra; un gioco che carica la propria XeSS FG riceve una nota nella scheda). Nessun guadagno
di fps = la FG è spenta - il log dice `... Enabled is off ...: no frames are generated. Frame generation off, not
broken.`; metà degli fps = un limite o il V-Sync trattiene i frame generati. Se fallisce ancora, premi **Save report**
dopo il fallimento e pubblica lo zip con il gioco, la coppia che hai scelto, quale opzione di frame generation del gioco
era attiva, finestra senza bordi o schermo intero, HDR attivo o no, e cosa hai visto. Le FAQ complete sono fissate nel
canale di supporto su Discord.

**Altre mod (RED4ext di Cyberpunk 2077, Cyber Engine Tweaks) o ReShade accanto ad AMDNR?** Dalla 0.3.5 il report e il
log nominano i proxy loader delle altre mod (`Mod loaders:` in `report.txt`; RED4ext è `winmm.dll`, Cyber Engine Tweaks
è `version.dll` tramite un loader ASI) e avvertono quando AMDNR occupa il nome di un loader noto di questo gioco senza
concatenarlo. L'AMDNR Launcher non prende né sposta mai il file di un loader: sceglie un altro nome di proxy (Cyberpunk
2077: `dxgi.dll`), e il suo Doctor nomina ogni loader che un INSTALL precedente ha messo da parte (`AMDNR_backup`).
ReShade accanto ad AMDNR viene riconosciuto dalla risorsa di versione del modulo o dagli export add-on di ReShade, mai
da un nome di file, e viene nominato nel report e nel log (`[Game] ReShade detected: <module>`); niente viene caricato,
agganciato o bloccato, e far condividere ai due il device D3D12 è progettato, non ancora realizzato.

**`No HIP adapter matches D3D12 LUID` nella scheda Neural o in `amd_bridge.log`?** Dalla 0.3.5 la riga nomina entrambe
le parti - l'adattatore D3D12 del gioco (nome, LUID), ogni device HIP (ordinale, nome, gfx, LUID) - e la causa probabile
con cosa fare: il gioco gira sulla GPU integrata o su un'altra scheda (Impostazioni di Windows > Sistema > Schermo >
Grafica: assegna il gioco alla GPU dedicata), un runtime HIP senza identità (un `amdhip64_7.dll` vagante accanto al
gioco: rimuovilo), nessun device HIP (installa il driver Adrenalin di AMD), la stessa scheda sotto un'altra identità,
oppure un adattatore non AMD. Entrambi i runtime scrivono lo stesso testo.

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

**Non trovi le impostazioni di Ray Regeneration?** Dalla 0.3.5 sono su una scheda **Ray Regeneration** tutta loro,
subito dopo Upscaling, disegnata mentre Ray Regeneration è in funzione nel titolo (e mantenuta per tutta la sessione di
gioco una volta che è entrata in funzione); la riga **Ray Regeneration** della scheda Neural offre allora un pulsante
**Open Ray Regeneration**. Mentre non è in funzione, la scheda non viene disegnata e quella riga della scheda Neural
dice perché: il gioco non ha attivato la Ray Reconstruction, Ray Regeneration ha rinunciato a questo titolo e perché, o
da quanti secondi è stata eseguita l'ultima volta. Su una RX 7000 un'ulteriore riga in grigio aggiunge che lì non viene
offerta: il denoiser di AMD non ha un provider per RDNA 3, quindi il gioco tiene il proprio denoiser; nella scheda
Upscaling esiste un'opzione sperimentale (**Experimental: Ray Regeneration on this card (restart)**), ma non è
supportata. Su RX 6000
e precedenti dice che non viene offerta su quella GPU, che AMD pubblica il denoiser per RDNA 4, e che
`[FSR-RR] FfxDenoiserAllowPreRdna4=true` la offre comunque. Per farla partire,
nelle impostazioni grafiche del gioco: scegli **DLSS** come upscaler (non FSR, non XeSS), attiva il **ray tracing** o il
path tracing e attiva la **Ray Reconstruction** (DLSS-RR); la scheda Upscaling mostrerà allora "FSR Ray Regeneration".
Quale impostazione cambiare per quale problema è nella guida alle impostazioni di Ray Regeneration
**RR-BEST-SETTINGS.md** (non è nello zip).

**Ray Regeneration sembra granulosa o rumorosa?** Giudicala prima con il **Neural Rendering disattivato** (togli la spunta a **Enable Neural Rendering** in cima alla scheda Neural, premi Home mentre giochi, oppure metti
`[DlssNr] Enabled=false`): il passaggio neurale viene eseguito dopo Ray Regeneration,
sul suo output, quindi uno screenshot fatto con NR attivo non dice nulla sul denoiser. Poi, in base al tipo di grana —
grana che striscia in una scena ferma, puntini luminosi, grana sui volti, scie dietro i personaggi in movimento — le
impostazioni da provare sono in quella stessa guida, **RR-BEST-SETTINGS.md**. **Nella 0.3.4.2 nessun valore
predefinito del denoiser e nessuno dello sharpening è cambiato**: i numeri sono quelli della 0.3.4.1. Quello che è
cambiato: lo sharpening che AMDNR aggiunge dopo Ray Regeneration ora è una chiave dell'ini tutta sua,
`[Sharpness] RrDefaultSharpness` (stesso valore predefinito 0.25), quindi 0.15, 0.10 o 0 è una modifica dell'ini e
non una nuova build. Controlla il tuo
ini prima di inseguire la grana con il cursore della nitidezza: un valore sotto `[Sharpness] Sharpness` non fa nulla
mentre `OverrideSharpness` è disattivato, e si applica nell'istante in cui spunti **Override** nel menu - ed è per questo che Image > Sharpness ora lo segnala
("ini Sharpness 1.00 waits for Override"). Due cose che
non nascondiamo: una parte della grana è il campionamento dei raggi del gioco stesso — il denoiser di AMD non è fatto
per riparare rumore che arriva correlato, e un gioco che offre la DLSS Ray Reconstruction spegne il proprio denoiser e
ci consegna il segnale grezzo — e la grana che striscia in una scena ferma ha dalla nostra parte una causa strutturale
che nessun cursore elimina del tutto. Quella è un problema noto. La 0.3.5 le dà un numero: il blocco Diagnostics della
scheda Ray Regeneration mostra la grana in ingresso e in uscita e lo sfarfallio a camera ferma (misurati mentre la
scheda è aperta), e la riga **Denoiser backend** della scheda può essere impostata su Off, che dice al gioco che la Ray
Reconstruction non è supportata, così tiene il proprio denoiser (dopo un riavvio). Il denoiser proprio di AMDNR è
lavoro per la 0.3.6.

**La Ray Reconstruction del gioco è attiva ma la scheda Neural dice "Ray Regeneration is off in this
title"?** Il gioco non espone ciò di cui FSR Ray Regeneration ha bisogno: il suo plugin DLSS passa matrici della
camera vuote (Satisfactory), che la Ray Reconstruction di NVIDIA tratta come opzionali e di cui FSR Ray
Regeneration ha bisogno. Al suo posto gira l'upscaling FSR e NR prende la sua normale posizione pre-SR; lo dice
anche la scheda Upscaling. Dalla 0.3.4 resta spenta per tutta la sessione in un titolo Unreal con questa firma.
Disattiva la Ray Reconstruction nel gioco e ripristina le impostazioni di denoiser del motore.

**Un gioco Ubisoft Anvil (AC Black Flag Resynced, Shadows, Mirage) mostra "DX12 Error 0x80070057"?**
Questi giochi hanno la propria XeSS Frame Generation. Dalla 0.3.5 la scheda Frame Gen mostra lì una nota di consiglio
(lascia spenta la XeSS FG del gioco, altrimenti due generatori condividono una finestra) e l'output XeFG di AMDNR gira
comunque; la via più semplice è l'opzione XeSS FG del gioco con la frame generation di AMDNR lasciata spenta. Se succede
ancora, imposta `[FrameGen] Enabled=false` e `[fakenvapi] ForceXeLL=false` e segnalalo allegando il log.

**The Last of Us Part I crasha all'avvio?** La causa è l'inizializzazione di Streamline del gioco stesso, un
problema noto di OptiScaler: rinomina `sl.common.dll` nella cartella del gioco in `sl.common.dll.bak` e
scegli **FSR 3.1** nelle impostazioni del gioco invece di DLSS.

Note complete per ogni versione: `CHANGELOG.md` (nello zip e nel repository).

## Roadmap

- **0.3.5** (questa build) — **AMDNR Anywhere** (preview, RX 9000, tramite il launcher); la scheda Ray Regeneration con
  righe di stato per scheda video e per API, la riga Denoiser backend Off e il numero del rumore; il pass neurale dopo
  l'upscaling (`AmdPlacement`); la scheda Frame Gen e il log dicono perché non viene generato nulla; lmxxf 0.37 di
  Kien (MIT) su RX 9000; il set di moduli della 0.3.5 (circa il 10% più veloce su RX 7000 e sugli handheld di classe
  Z1 Extreme, stessa immagine); i passi di
  risoluzione dinamica mantenuti senza ricostruire la rete; la correzione del trasporto sugli handheld; la cartella
  condivisa dei runtime (`AmdRuntimePath`); il rifiuto dell'adattatore HIP che nomina entrambe le parti; i loader delle
  altre mod e ReShade nominati nel report; i processi anti-cheat e di segnalazione dei crash lasciati in pace;
  correzioni per Uncharted (RX 9000), Assetto Corsa, F1 25 e Kingdom Come: Deliverance II (Game Pass), Control Resonant
  con la frame generation, Tainted Grail: The Fall of Avalon e Dead Space, GTA V Enhanced, Half-Life 2 RTX e altri
  titoli Vulkan, e i testi su Linux / Proton; AMDNR Launcher 0.3.5.1.
- **0.3.4.2** — hotfix: il menu di Assetto Corsa (il tasto del menu lo apre o chiude una sola volta per
  pressione, i clic più brevi di un frame vengono riprodotti, il selettore del runtime risponde ai tasti e chiudere il
  menu vale come "Decide later"), il selettore del runtime non apre più il menu da solo, la sezione Ray Regeneration è
  sempre nella scheda Neural e dice perché non è in funzione, un layout di runtime danielblnc in più accettato,
  correzioni del testo su Wine /
  Proton, aggiunte al README (nomi proxy, Uncharted, PC ibridi, `AmdLmxxfTierSnap` su RX 9000, le due voci di FAQ su
  Ray Regeneration); NR identico byte per
  byte alla 0.3.4.1 a parte quell'unica riga di layout accettato.
- **0.3.4.1** — hotfix: lo sharpening di Ray Regeneration quando il gioco non ne invia (Windows; di
  default nessuno su Linux / Proton), il falso popup "Upscaler failed to run!" di Control Resonant, il menu su Linux /
  Proton agganciato alla finestra del gioco, Save report che indica vkd3d-proton / DXVK; AMDNR Launcher 0.3.4.1
  (nove lingue, ricerca, preferiti, nascondi, rinomina, CHOOSE GAME .EXE, PLAY, un UNINSTALL completo); NR invariato.
- **0.3.4** — il nuovo menu (la scheda Neural rifatta, lo stesso stile in tutte le schede, Save
  report); lmxxf più veloce su RX 7000 (il livello di dimensione della rete di default) e su RX 9070 /
  9070 XT (kernel di lmxxf 0.31); lmxxf sulle APU per handheld (sperimentale; nuove dimensioni di rete
  360p e 576p); lmxxf ottiene Network output, Encoding, Residual edge fade, la maschera nativa dei personaggi e un Fast mode opzionale; AMDNR Screen GI (preview); le impostazioni del runtime danielblnc (Network style, Tone curve, Black lift, Game exposure) e una
  protezione del colore delle alte luci; regolazione e diagnostica di Ray Regeneration; fix dell'input del menu in Assetto Corsa, di Shadow of the Tomb Raider, Marvel's Midnight Suns e The Last of Us
  Part II, dell'uscita pulita e dei log.
- **0.3.3.x** — lmxxf su RDNA 3 (RX 7000; backend proprio di AMDNR); composizione colore
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
- **0.3.6** — le RX 6000 (RDNA 2): i kernel propri di AMDNR dietro un gate hardware, con il livello 360 e Model
  interleave come preview su Navi 21; la preview del denoiser AMDNR (ARD), un denoiser proprio di AMDNR dietro la
  chiamata Ray Reconstruction per le schede su cui il denoiser di AMD rifiuta di girare; AMDNR Anywhere su RX 7000 una
  volta testato lì.
- **0.4.0** — il livello di rete 1440p; Ray Regeneration nei titoli Vulkan tramite il bridge; Neural Rendering
  cross-adapter (il gioco su una scheda, la rete sulla scheda AMD); Anywhere oltre la preview (i titoli senza un
  proprio upscaler, dove OptiScaler fornisce insieme l'upscaler e il pass neurale).
- **Più avanti** — AMDNR su qualsiasi finestra (il desktop).

---

## Crediti

Questa build si limita a collegare tra loro i lavori di altre persone. Se la trovi utile, i
ringraziamenti vanno ai progetti upstream.

- **TheAutomatic** — DLSS 5 AMD project — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD by Daniel Blanco — https://github.com/danielblnc/DLSS-NR-on-AMD (i file `*Runtime.zip`, non modificati)
- **lmxxf** (Kien) — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (il port della rete, i kernel e il runtime HIP, MIT)
- **TheAutomatic** — `LmxxfNrRuntime.cpp`, `LmxxfNrApi.h`, `LmxxfProductionOptions.h`: portions contributed to lmxxf by TheAutomatic (MIT)
- **kernel di lmxxf 0.31** in `LmxxfNrRuntime.pak` (the ViT projection (lmxxf031-vit-wide-deep), the C512 QKV and mix kernels (lmxxf031-c512-m32-mh, lmxxf031-c512-m32-deep) and one-wave-per-head attention (lmxxf031-c64-wave2)) — di lmxxf (Kien, MIT), compilati da AMDNR dai sorgenti e dalla ricetta di build di lmxxf; la parte di AMDNR è il caricamento, i pin SHA-256, il filtro per GPU e i fallback
- **kernel di lmxxf 0.37** in `LmxxfNrRuntime.pak` (i moduli lmxxf037-* per RX 9000) e il loro codice di lancio in `LmxxfNrRuntime.dll` — lmxxf 0.37 by Kien (MIT), distribuiti così come li ha compilati lmxxf; la parte di AMDNR è il caricamento come un unico gruppo con pin, i pin SHA-256, i default per GPU, gli interruttori di disattivazione e i fallback
- **c32w kernels** (0.3.3.2) — i kernel RDNA 4 a wave singola propri di AMDNR per la rete di lmxxf, Copyright (c) 2026 3zwr1 (AMDNR); idee tratte dalla documentazione pubblica di AMD su RDNA 4 WMMA (GPUOpen, ROCm matrix instruction calculator)
- **Il backend RDNA 3 di AMDNR** (0.3.3; le build per handheld gfx1103 / gfx1150 nella 0.3.4), la politica dei livelli di dimensione della rete e le dimensioni di rete piccole (0.3.4) — Copyright (c) 2026 3zwr1 (AMDNR)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Dagherbou / OptiScaler_DLSSNR** — https://github.com/Dagherbou/OptiScaler_DLSSNR
- **wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass** — https://github.com/wilsjo2/OptiScaler-DLSSNR-PreSR-Multipass
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, non modificato)
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR** — l'host di cattura della finestra dentro cui gira AMDNR Anywhere — https://github.com/Blinue/Magpie (il fork: https://github.com/SAOG0721/Magpie)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (matematica della composizione colore, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), la base dello sblocco multi-frame XeFG integrato e del suo pacing
- **Zach Hembree (DarkHelmet)** — FSR Ray Regeneration per OptiScaler, l'origine del percorso Ray Regeneration di AMDNR, proseguito da **burak113**, dal cui branch AMDNR ha fatto il port (branch di OptiScaler ffx-denoise-experimental, GPL-3.0)
- **Screen-space GI** (l'effetto ereditato; ritirato dal menu in 0.3.4, `[AmdRtgi] Enabled` nell'ini) — un effetto che AMDNR ha ereditato dalla linea OptiScaler-AMD-PreSR; il merito è dei suoi autori originali. Richiede la cartella `experimental_lighting` del pacchetto danielblnc, che AMDNR non distribuisce.
- **AMDNR Screen GI** (preview 0.3.4) — lavoro proprio di AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), scritto a partire da articoli pubblicati (Therrien, Levesque e Gilet 2023; Jimenez et al. 2016; Schied et al. 2017; e gli altri elencati in `CHANGELOG.md` e `Licenses/AMDNR_NOTICE.txt`)
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## AMDNR Launcher

**AMDNR Launcher** (nuovo nella 0.3.4) è un programma per Windows 10 / 11 che può girare anche sotto Proton su Linux
(sperimentale, non ancora testato da noi: vedi "Linux / Proton"). Installa e aggiorna AMDNR gioco per gioco: trova i
tuoi giochi (Steam, Epic, l'app Xbox, Ubisoft Connect, l'app EA, GOG, Rockstar, Battle.net e Amazon Games), sceglie
il nome della DLL, scarica la build e il runtime danielblnc che scegli, controlla ogni installazione con il suo
Doctor, esegue AMDNR Anywhere per i giochi senza un upscaler proprio (PLAY ANYWHERE) e si aggiorna da solo. Scarica
`AMDNR-Launcher.exe`, l'AMDNR Launcher dalla release più recente, dalla pagina delle release:
<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>

**Novità del Launcher 0.3.5.1** (nella release Alpha0.3.5; aggiorna prima sé stesso, poi i tuoi giochi):

- **PLAY ANYWHERE** su un gioco senza un upscaler proprio (vedi "AMDNR Anywhere"): il launcher scarica l'host di
  cattura dalla release del suo autore, avvia il gioco ed esegue il Neural Rendering sulla sua finestra, con un
  riepilogo di sola lettura delle impostazioni dell'host accanto al pulsante (le impostazioni vere e proprie sono nella
  scheda Anywhere del menu). RX 9000 in questa release. I giochi in cui la mod non può caricarsi (32 bit, DirectX 9 /
  OpenGL senza upscaler) vengono rifiutati a INSTALL con una riga chiara e ricevono l'offerta di Anywhere.
- **UPDATE ALL** e aggiornamento all'avvio; mirror dei pacchetti; RETRY / OPEN DOWNLOAD / IMPORT PACKAGE quando un
  download fallisce.
- **COLLECT LOGS** attiva il gestore dei crash per una sola esecuzione e porta con sé `amdnr_crash.log`, il dump più
  recente e il `dlssnr_on_amd.ini` di danielblnc.
- **PLAY** avvia i giochi Xbox / Microsoft Store tramite il loro app id.
- **Un solo PLAY** con un selettore del percorso (l'upscaler del gioco o AMDNR Anywhere, memorizzato per gioco) e
  l'API grafica (DX9 / 10 / 11 / 12 / Vulkan / OpenGL) su ogni pagina di gioco.
- **Dodici lingue**: turco, coreano e ungherese si aggiungono alle nove qui sotto.
- **I giochi Rockstar si avviano tramite il loro store** (Steam, Epic o il Rockstar Games Launcher), mai dal loro exe.
- **AMDNR Anywhere**: il gioco gira con una priorità GPU inferiore alla normale, così l'host e il Neural Rendering
  passano per primi; l'icona dell'host di cattura nell'area di notifica è nascosta.
- **Resident Evil 2 / 3 / 4 (2023) / Village**: un avviso di configurazione (richiedono il plugin di upscaler di
  PureDark con REFramework). Altri giochi senza DLSS, XeSS o FSR 2+ vengono segnati come non supportati, con il
  motivo, prima di qualsiasi download.
- **FSR 4 (INT8)** come opzione sperimentale nella pagina del gioco degli handheld RDNA 3.
- **I loader delle altre mod non vengono mai presi né spostati** (RED4ext, Cyber Engine Tweaks e simili): il launcher
  sceglie un altro nome di proxy e il suo Doctor nomina ogni loader che un INSTALL precedente ha messo da parte.
- **GTA V Enhanced**: la pagina del gioco riporta la riga del percorso (l'FSR 3.1 scelto nel gioco è l'input; il Neural
  Rendering gira prima di esso) e una nota su `settings.xml`.

**Novità del Launcher 0.3.4.1: ce l'avete chiesto, l'abbiamo realizzato** (dai primi feedback su Discord):

- Nove lingue (dodici dalla 0.3.5.1): inglese, arabo, cinese (semplificato), francese, spagnolo, portoghese, italiano,
  russo e polacco, più turco, coreano e ungherese. Il
  launcher segue la lingua di Windows (o l'inglese, se non è tra queste); puoi sceglierne un'altra da LANGUAGE
  (LINGUA) o in SETTINGS (IMPOSTAZIONI), dove la scelta viene proposta anche al primo avvio. I risultati del
  Doctor, i messaggi di installazione e il report di COLLECT LOGS (RACCOGLI LOG) restano in inglese, così chi fa
  assistenza può leggerli.
- Ricerca nella libreria; preferiti (una stella, e i giochi con la stella in cima); nascondere i giochi (HIDDEN li
  mostra di nuovo); rinominare un gioco.
- CHOOSE GAME .EXE: scegli tu l'exe del gioco quando il launcher ha scelto quello sbagliato o nessuno. Cyberpunk 2077
  e The Witcher 3 (REDengine) ora vengono trovati nella cartella giusta anche senza.
- PLAY e OPEN FOLDER nella pagina di ogni gioco; STORES attiva e disattiva interi store; le cartelle che aggiungi a
  mano restano nell'elenco, anche mentre il loro disco è scollegato, finché non le rimuovi.
- UNINSTALL chiede prima conferma e rimuove tutto ciò che la mod ha messo, compreso ciò che ha scritto mentre il
  gioco girava (log, cache, crash dump, report non completati); conserva gli zip di Save report completati e una DLL
  con il nome del proxy che non è più un OptiScaler (quella del gioco stesso).
- COLLECT LOGS su qualsiasi gioco, installato o no, con un report della scansione.
- Handheld e APU (ROG Ally Z1 Extreme e altre APU Ryzen) vengono riconosciuti, con una nota che dice che AMDNR su di
  essi è ancora in fase di test.
- Linux (sperimentale, non ancora testato da noi): lo stesso exe per Windows può girare sotto Proton, lì mostra un
  avviso con cosa fare e cerca i giochi anche nella tua libreria Steam di Linux; i passaggi sono in "Linux / Proton".
  Facci sapere su Discord se funziona.

Il suo codice sorgente è in `Launcher/OpenSource/` del repository GitHub di questo progetto, con una licenza propria,
`Launcher/OpenSource/LICENSE.txt`. **Non** è coperto dalla licenza GPL-3.0 (`LICENSE`) di questo repository: è
source-available, tutti i diritti riservati, Copyright (c) 2026 3zwr1 (AMDNR). Il manifest del launcher è
`Launcher/manifest.json`. Vedi anche la sezione 7 di `Licenses/AMDNR_NOTICE.txt`.

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
