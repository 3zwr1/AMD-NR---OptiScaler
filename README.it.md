# AMDNR — DLSS 5 Neural Rendering su AMD (build di OptiScaler) — v0.3.5.8 Public View

[English](README.md) | [中文](README.zh-CN.md) | [Português](README.pt-BR.md) | [Español](README.es.md) | [العربية](README.ar.md) | [Français](README.fr.md) | **Italiano** | [Русский](README.ru.md) | [Polski](README.pl.md)

> **Novità di 0.3.5.8 Public View — Launcher 0.3.5.36** (2026-10-23)
> 
> - Questa sezione descrive la versione attuale; la documentazione storica datata è conservata sotto. Download in fondo al riquadro.
> - Friendly use mantiene lo stile originale: NR, upscaling, FG e 1–3 passaggi dove supportati; mochi ne usa uno. Advanced offre ripristino con conferma e backup, applicato al riavvio. Classic carry è opzionale e disattivato per impostazione predefinita in lmxxf Model A e mochi A/B, separato dalla cronologia del modello e da Model interleave.
> - lmxxf 0.4.4 A/B, Daniel 0.8.0 e mochizuki 0.0.7. Il launcher scarica i componenti, ripara file mancanti e pulisce vecchie copie idonee dopo la verifica. Non estrarre tutti gli ZIP nel gioco. DLL NVIDIA e pesi NR non sono inclusi.
> - Correzioni agli input profondità/movimento e ai colori di mochi, al formato SNORM di RE3, alla durata delle risorse e alla sincronizzazione Model E in Anywhere. HelixSR 2.0.0 arriva dall’autore. FSR incluso: SR 4.1.1 / FG 4.0.1; SR 4.1.2 / FG 4.0.2 richiedono un driver che li fornisca.
> - Overlay Guide, Gallery più ampia con configurazione della singola immagine e preparazione/riparazione automatica. Server comunitario pubblicato; reazioni, menzioni e invio con Invio richiedono il launcher corrispondente. Conservazione dei messaggi per 48 ore mobili.
> - Non tutti i crash, sfarfallii, sfocature, ghosting e problemi RR/MFG sono risolti. Mochi RDNA 3 rimane rinviato. Restano la verifica finale in gioco e la consegna del sorgente corrispondente. Invia COLLECT LOGS.
> 
> [AMDNR-v0.3.5.8.zip](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/download/Alpha0.3.5.8/AMDNR-v0.3.5.8.zip) · [AMDNR-Anywhere-0.3.5.8.zip](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/download/Alpha0.3.5.8/AMDNR-Anywhere-0.3.5.8.zip) · [AMDNR-Mochi-0.0.7-component.zip](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/download/Alpha0.3.5.8/AMDNR-Mochi-0.0.7-component.zip) · [v0.8.0-Runtime.zip](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/download/Alpha0.3.5.8/v0.8.0-Runtime.zip) · [AMDNR-Launcher.exe](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/download/Alpha0.3.5.8/AMDNR-Launcher.exe)
> 
> [CHANGELOG.md](CHANGELOG.md) · [0.3.5.8](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/tag/Alpha0.3.5.8)


> **Abbiamo bisogno del tuo supporto.** Unisciti al server Discord — <https://discord.gg/AMDNR> — per
> assistenza, segnalazioni di bug e build di test; ogni segnalazione con un log rende migliore la build successiva.

DLSS 5 Neural Rendering in esecuzione su GPU AMD, integrato in OptiScaler così da funzionare in
qualsiasi gioco Direct3D 12 che OptiScaler già aggancia. Oltre al pass neurale: model interleave per
un grande guadagno di frame rate, composizione residua, frame generation XeSS sbloccata fino a 6X
(fino a 10X opzionale nei giochi D3D12), e FSR Ray Regeneration per i giochi che usano DLSS Ray
Reconstruction. Dalla 0.3.5, **AMDNR Anywhere** (preview) porta il Neural Rendering nei giochi che non hanno un
upscaler proprio, tramite l'AMDNR Launcher, senza scrivere nulla nella cartella del gioco (vedi "AMDNR Anywhere").
Dalla 0.3.5.6 l'elenco degli upscaler offre anche due scelte sperimentali e opzionali realizzate da altri autori,
**Model E** (HelixSR) e **Model L** (d4r), che l'AMDNR Launcher scarica dalle loro release (vedi "Model E e Model L").

**Discord: <https://discord.gg/AMDNR>** — supporto, segnalazioni di bug (`#bug-report`), build
di test.

**Supporta il progetto: <https://ko-fi.com/3zinr>**

> **Il runtime danielblnc è opera di Daniel Blanco.** Il runtime neurale AMD contenuto nei file `*Runtime.zip`
> (`dlssnr_amd_pass1..3.dll`) è **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> AMDNR lo distribuisce senza modifiche, con il suo permesso; non è opera di AMDNR. Per favore, supporta il suo progetto.
> I crediti completi per tutti gli altri si trovano in fondo a questa pagina.

> **Novità della 0.3.5.6** (2026-10-17, release Alpha0.3.5.6, con l'AMDNR Launcher 0.3.5.25):
> - **Due nuove scelte di upscaler, sperimentali e opzionali:** **Model E** (HelixSR di lonewolf0622) e **Model L** (d4r
>   di countervolts), che il launcher scarica dalle release dei loro autori quando li scegli. AMDNR non ospita né
>   distribuisce i loro file né quelli di NVIDIA. Model L è solo per RX 9070 / RX 9070 XT e gira con il runtime lmxxf o
>   con il Neural Rendering spento; il runtime di danielblnc si abbina a Model E. Leggi "Model E e Model L" prima di
>   attivarli.
> - **Full Model Look è lo stile NR predefinito:** un aspetto più pieno su entrambi i runtime, eseguito dopo l'upscaling
>   sulle schede che possono permetterselo - il controllo del costo e la protezione TDR lo riportano prima dell'upscaling
>   dove devono, e lo dicono. Scegliere un aspetto in NR style imposta anche il suo Neural placement e Full network, e il
>   menu ti avvisa quando una tua impostazione di placement trattiene l'aspetto.
> - **Più dettaglio sui volti**, sempre attivo, su entrambi i runtime (`[DlssNr] AmdFaceDetail=false` lo disattiva).
> - **Meno ghosting con lmxxf:** le scie dietro gli oggetti in movimento sono più corte, prima e dopo l'upscaling, e il
>   lampo dopo un cambio di scena è forte circa la metà. Il ghosting è ridotto, non eliminato.
> - **RX 7000 e handheld:** il Neural Rendering ora gira di default con una dimensione di rete più piccola (Fast mode di
>   lmxxf), che costa meno tempo per frame. `[DlssNr] AmdLmxxfFastMode=false` lo disattiva; le RX 9000 lo tengono spento.
> - **Controlli nuovi e rinominati:** Game exposure (Auto / On / Off), Anti-halo (prima Halo protection) e, su
>   danielblnc, **Refine passes** (i pass 2 e 3 girano con una dimensione di rete più piccola). **NR resolution** ora
>   arriva fino al 200% su entrambi i runtime.
> - **Il menu mostra solo ciò che il tuo runtime può usare:** un controllo che un runtime non può eseguire è nascosto su
>   quel runtime (la sua chiave ini continua a funzionare), e la voce del runtime indica la versione di lmxxf che gira
>   davvero (ad es. `lmxxf 0.41`).
> - **Correzioni:** i falsi "timeout" di danielblnc (testato in Silent Hill 2) - un'attesa che si arrende non spegne più
>   il Neural Rendering, il testo secondo cui il driver aveva resettato la GPU è sparito (non c'era stato alcun reset), e
>   la nota "runtime failed" viene scritta solo dopo un errore reale. FSR 3 / FSR 4 non dovrebbero più ripiegare su
>   FSR 2.1.2 quando un'altra scelta di upscaler fallisce o non è pronta: la tua scelta FSR viene mantenuta. La memoria
>   della frame generation XeFG viene conteggiata correttamente (compresa l'impostazione 6X).
> - **Sostituisci insieme tutti i file dello zip** (`OptiScaler.dll` e `LmxxfNrRuntime.dll` devono venire dalla stessa
>   release); utenti del launcher: si aggiorna da solo. Cosa resta aperto: "Limiti noti (0.3.5.6)". Dettagli:
>   `CHANGELOG.md`.

> **Tra la 0.3.5 e la 0.3.5.6** (dalla 0.3.5.1 alla 0.3.5.5R4): danielblnc 0.6.0 è gestito ed è ora il runtime
> danielblnc consigliato (gira anche su RX 6000, con l'SDK HIP di AMD installato); lmxxf 0.41 di Kien (MIT) è il set
> predefinito su RX 9000; l'NVIDIA look (lo stile NR predefinito dalla 0.3.5.3 alla 0.3.5.5R4); una protezione contro i
> blocchi che abbassa o ferma NR dopo un errore GPU conteggiato; l'AMDNR Launcher ridisegnato (tredici lingue, ASK AI);
> e molte correzioni di crash dalle vostre segnalazioni. Dettagli: `CHANGELOG.md` e le note di release su GitHub.

> **Novità della 0.3.5:** **AMDNR Anywhere** (preview): Neural Rendering per i giochi che non hanno un DLSS, XeSS o
> FSR 2 proprio - un solo pulsante **PLAY ANYWHERE** nell'AMDNR Launcher, niente viene scritto nella cartella del gioco;
> RX 9000 (RDNA 4) in questa release (vedi "AMDNR Anywhere"). **Ray Regeneration ha una scheda tutta sua**, subito dopo
> Upscaling, e le sue righe di stato dicono, per scheda video e per API, cosa è in funzione e perché no; su RX 7000, dalla 0.3.5.1, è di
> nuovo offerta come preview, attiva di default (dalla 0.3.5.3 lì il denoiser è l'AMDNR Ray Denoiser (preview); la casella della scheda Upscaling o `[FSR-RR] FfxDenoiserAllowPreRdna4=false` la spegne e il
> gioco tiene il proprio denoiser). **Il pass
> neurale può girare dopo l'upscaling** (`[DlssNr] AmdPlacement=post`, Neural > Performance > Neural placement; il
> predefinito `pre` non cambiava nella 0.3.5 - dalla 0.3.5.6 lo stile NR predefinito chiede `post`, vedi "Impostazioni da
> conoscere"). **La scheda Frame Gen dice perché non viene generato nulla** e nomina i cinque passaggi (vedi la
> voce delle FAQ sulla frame generation). **Più veloce su RX 7000 e sugli handheld di classe Z1 Extreme:** circa il 10%
> di tempo di rete in meno, stessa immagine bit per bit (il set di moduli della 0.3.5 in `LmxxfNrRuntime.pak`).
> **lmxxf 0.37 di Kien (MIT) è attivo di default su RX 9000:** circa il 20% di tempo di rete in meno su una RX 9070 XT
> (sperimentale su RX 9060 / 9060 XT; `[DlssNr] AmdLmxxfL37=false` lo disattiva; dalla 0.3.5.3 lì il set predefinito
> è lmxxf 0.41). Sugli handheld RDNA 3,
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
la 0.3.5.6 è il tag Alpha0.3.5.6):

* `AMDNR-vX.X.X.zip` (per la 0.3.5.6: `AMDNR-v0.3.5.6.zip`), con il runtime lmxxf completo.
* Per il runtime danielblnc, uno zip di runtime: `v0.6.0-Runtime.zip` (consigliato; RX 9000, RX 7000 e, con l'SDK HIP
  di AMD installato, RX 6000) dalla release Alpha0.3.5.5R4; `v0.5.1-Runtime.zip` (Alpha0.3.5), `v0.5.0-Runtime.zip`
  (Alpha0.3.4.2; su alcune schede può bloccare alcuni giochi), `v0.4.3-Runtime.zip` (Alpha0.3.4.2 e Alpha0.3.4.1),
  `v0.4.1-Runtime.zip` e `v0.4.0-Runtime.zip` (Alpha0.3.4.1) restano accettati su RX 9000 e RX 7000. Il runtime lmxxf
  è in `AMDNR-vX.X.X.zip` e non richiede alcuno zip di runtime su RX 7000 e RX 9000; le APU per handheld usano solo
  lmxxf. L'AMDNR Launcher sceglie lo zip giusto per la tua GPU. Vedi "Cosa contengono gli archivi".
* Niente da scaricare a mano per Model E o Model L: l'AMDNR Launcher li scarica dalle release dei loro autori quando
  li scegli (vedi "Model E e Model L").

### 2. Estrai entrambi i file

Estrai il contenuto di entrambi i file `.zip`.

### 3. Copia tutto nella cartella del gioco

Per prima cosa, copia tutti i file di `AMDNR-vX.X.X` nella cartella principale del gioco — la stessa
cartella in cui si trova il file `.exe` del gioco.

Poi fai lo stesso con tutti i file dello zip di runtime (ad es. `v0.6.0-Runtime` su RX 9000, RX 7000 e RX 6000).

> **Aggiorni da un AMDNR precedente?** Copia di nuovo tutto sovrascrivendo. **Sostituisci questi tre file insieme:**
> `OptiScaler.dll` (sostituisci il file che hai rinominato, ad es. `dxgi.dll`, con il nuovo rinominato allo
> stesso modo), `LmxxfNrRuntime.dll` e `LmxxfNrRuntime.pak` (440 MB). Non mescolarli con copie precedenti: nella 0.3.5.6
> `OptiScaler.dll` passa a `LmxxfNrRuntime.dll` nuovi dati del frame, e un runtime più vecchio gira senza di essi. Puoi
> tenere il tuo `OptiScaler.ini`: le nuove impostazioni usano i valori predefiniti. I file del tuo zip di runtime
> danielblnc restano come sono (il Doctor del launcher ti propone quello consigliato per la tua scheda). L'AMDNR
> Launcher lo fa per te: UPDATE ALL, oppure REPAIR / UPDATE su un gioco che mostra "Update available" (il launcher
> aggiorna prima sé stesso).

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

Entrambi i runtime condividono un'unica scheda Neural (vedi "Il menu" più sotto). Dalla 0.3.5.6 un controllo che il
runtime attivo non può eseguire è nascosto su quel runtime, con un conteggio (la sua chiave ini continua a
funzionare). Solo lmxxf: **Full network**,
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

**Fast mode** (Neural > Performance, `[DlssNr] AmdLmxxfFastMode`, lmxxf) fa girare la rete un livello di dimensione
più in basso (1080 -> 900, 900 -> 720; 720 -> 576 e 576 -> 360 dove le dimensioni piccole sono attive): circa il 29% di
tempo di rete in meno a 1080p (RX 9070 XT, misurato fuori da un gioco), con i dettagli fini un po' più morbidi. **Dalla
0.3.5.6 è attivo di default su RX 7000, Radeon 8060S / 8050S e sulle APU per handheld** e disattivato di default su
RX 9000; `false` (o la spunta) lo disattiva, `true` lo attiva, e la spunta mostra cosa gira. Le build di danielblnc che
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
  0.3.3.2. Dalla 0.3.5.2 anche le RX 9000 si agganciano ai livelli di default.
- **RX 9000:** `[DlssNr] AmdLmxxfTierSnap` (lì attivo di default dalla 0.3.5.2; prima opzionale) porta la dimensione
  NR di lmxxf a una dimensione di rete a ogni risoluzione di render: alcune dimensioni scendono di un livello (1440p
  FSR Quality, 1707x960 -> la dimensione 900: rete 14.08 -> 9.96 ms per esecuzione su una RX 9070 XT, misurato fuori da
  un gioco, immagine un po' più morbida), altre crescono dentro il loro livello (80% di un render 1080p -> 1600x900:
  stesso costo, un po' più di dettaglio). `false` mantiene le dimensioni della 0.3.3.2.
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
- **Dalla 0.3.5.3 il set predefinito su RX 9000 è lmxxf 0.41 di Kien (MIT)** (`[DlssNr] AmdLmxxfL41=false` lo
  disattiva); dalla 0.3.5.6 la voce Neural runtime indica il set che gira davvero. **Dalla 0.3.5.5R4 le RX 7000 e gli
  handheld di classe Z1 Extreme usano set di kernel più recenti**, stessa immagine bit per bit (verificata con gli hash
  sulle macchine dei tester).
- **0.3.5.6: Fast mode è attivo di default su RDNA 3** (RX 7000, Radeon 8060S / 8050S e le APU per handheld): la rete
  gira una dimensione sotto quella che userebbe la dimensione di render (un render 1080p gira alla dimensione 900), il
  che costa meno tempo per frame, con i dettagli fini un po' più morbidi. Le RX 9000 non cambiano (disattivato). Non ci
  sono ancora segnalazioni da giochi su queste GPU: per favore inviaci le tue. `[DlssNr] AmdLmxxfFastMode=false` (o la
  spunta Fast mode) lo disattiva.

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
  danielblnc non gira sulle APU per handheld. Le schede desktop RX 6000 (RDNA 2) usano il runtime 0.6.0 di danielblnc
  (vedi "Requisiti"; lì lmxxf è previsto per la 0.3.6); Steam Deck e le altre APU RDNA 2 non sono supportate.
- **Cosa fa da solo** (solo finché il tuo ini non ha un valore proprio): la dimensione della rete è limitata a 576
  (1024x576; dalla 0.3.5.5R3, prima era 360p), e dalla 0.3.5.6 Fast mode è attivo di default, quindi la rete gira una
  dimensione più in basso, a 360p (640x360); il modello gira un frame ogni 4 (Model interleave; non salvato). Dalla
  0.3.5.5R3 i tuoi Neural passes vengono mantenuti (prima erano fissati a 1); il costo NR li mette in conto.
- **La velocità, onestamente:** Per
  confronto: una RX 7800 XT (60 compute unit) richiede 34.4 ms per esecuzione della rete alla dimensione 720;
  questi chip ne hanno da 12 a 16 e girano a frequenze più basse. Aspettati un forte calo di frame rate anche a
  360p con il modello un frame ogni 4, un po' di ghosting per l'interleave lungo, e un aspetto più morbido che su
  una GPU desktop. Il costo NR alla fine della riga di stato della scheda Neural (e in Diagnostics) mostra il numero
  reale sul tuo dispositivo.
- **Impostazioni:**
  - Più nitido ma più lento: `[DlssNr] AmdLmxxfFastMode=false` (la dimensione di rete 1024x576;
    `[DlssNr] AmdLmxxfTierCap=360` mantiene la dimensione più piccola anche con Fast mode spento).
  - Con Fast mode attivo, un render a 720p o 800p con la NR resolution al 100% gira già alla dimensione 360p, quindi
    una NR resolution più bassa non costa meno.
  - Model interleave su Off viene salvato come `[DlssNr] AmdInterleave=1` (anch'esso spento), così il valore
    predefinito dell'handheld non torna al prossimo avvio. Per spegnerlo a mano, scrivi 1, non 0.
  - Preset > **Handheld** (dalla 0.3.5.1) imposta NR resolution all'80%, Dynamic NR disattivato, il modello ogni 4 fotogrammi con Edit accumulation, 1 Neural pass, Full network attivo, Fast mode attivo e Before upscaling, più i suoi valori di aspetto (Residual limit 0.32, Temporal stability 0.70, Sharpening 0.10, Output smoothing 0.65 e qualcun altro; il tooltip del pulsante li elenca tutti). Il pulsante compare solo su queste APU; anche Quality, Balanced e Performance qui mantengono la dimensione di rete per handheld (il menu lo dice).
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
- **Usa insieme i file di una stessa release:** sostituisci insieme `OptiScaler.dll`, `LmxxfNrRuntime.dll` e
  `LmxxfNrRuntime.pak`; il runtime rifiuta un handheld quando `OptiScaler.dll` è più
  vecchio della 0.3.4 ("this handheld needs OptiScaler.dll 0.3.4 or newer").
- **Tester con un handheld:** chiedi su Discord il kit di test per handheld (`handheld-test.zip`). Il suo
  `run_probe.bat` misura la rete sul tuo dispositivo e scrive `handheld_result.txt` (il tuo nome utente di Windows
  viene mascherato).

## AMDNR Anywhere (preview, novità della 0.3.5)

**Cos'è.** Neural Rendering per i giochi che non hanno un DLSS, XeSS o FSR 2 proprio - e niente viene scritto nella
cartella del gioco. AMDNR gira dentro un host di cattura della finestra: l'host cattura la finestra del gioco, la scala
al tuo schermo, e il Neural Rendering gira sull'immagine catturata; il nostro menu viene disegnato dentro
l'host (il tuo tasto del menu, `INSERT` di default) con una scheda **Anywhere** tutta sua. L'host è
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)**, nella build propria di AMDNR: il suo `Magpie.exe` è ricompilato con le modifiche di AMDNR (l'offerta del sorgente, un diff rispetto al tag del fork e le opzioni di build sono accanto, in `Anywhere-host-source/`) e il pacchetto non contiene file NVIDIA. L'AMDNR Launcher lo scarica dalla release GitHub di AMDNR (circa 130 MB per la 0.3.5.6, una volta per ogni versione dell'host).

**Come si usa.** Nell'AMDNR Launcher un gioco senza upscaler mostra **PLAY ANYWHERE** al posto di INSTALL. Premilo: il
launcher scarica l'host (la prima volta), avvia il gioco, e l'host ne cattura la finestra. Esegui il gioco **in finestra
o in finestra senza bordi**, non a schermo intero esclusivo, e premi il tuo tasto del menu per il menu di AMDNR dentro
l'host. Un gioco con un upscaler proprio mantiene la normale via INSTALL: Anywhere è per i giochi che non ne hanno uno.

**Le impostazioni dell'host stanno nel menu**, nelle Host settings della scheda Anywhere (anche il launcher ha un suo
pannello, vedi sotto): la dimensione della finestra del gioco (720p / 900p / 1080p - un consiglio su cosa impostare nel gioco; l'host cattura
qualunque finestra il gioco apra), l'elenco delle fasi (V1: un solo pass FSR 3 verso lo schermo; V2: FSR 3 a 1x, poi un
pass di riempimento), il livello NR (Auto / 720 / 900 / 1080), VRR, il frame pacing e il **frame rate dell'host**
(Default = la frequenza di aggiornamento del tuo schermo, al massimo 60; Auto = la frequenza che la rete ha sostenuto
nell'ultima sessione di gioco; da 30 a 120; Display refresh = nessun limite). Si applicano al **prossimo** PLAY
ANYWHERE, e il launcher mostra un riepilogo di sola lettura accanto al pulsante. Nell'`OptiScaler.ini` dell'host sono
`[DlssNr] AnywhereWindow`, `AnywhereEffect`, `AnywhereNrTier`, `AnywhereVrr`, `AnywherePacing` e `AnywhereHostFps`.
Limita anche il gioco, con il suo limitatore, a 60-90 fps: l'host può mostrare solo i frame che il gioco ha disegnato,
e ogni frame dell'host esegue la rete una volta. **Dall'AMDNR Launcher 0.3.5.25 anche il launcher ha un pannello di
impostazioni Anywhere** (il runtime NR, Neural passes, la dimensione neurale, FSR Auto / FSR 4 / FSR 3 / Off - Off
mantiene NR -, la frame generation (sperimentale), il movimento per l'upscaling, la nitidezza, e Model E / Model L
(sperimentali; con il Neural Rendering dentro Anywhere sono ancora in fase di test)); scrive lo stesso ini dell'host e
si applica al prossimo PLAY ANYWHERE.

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
lascia Model interleave spento - l'host lo rifiuta). L'host di cattura è la build propria di AMDNR, scaricata dalla release GitHub di AMDNR. Segnalazioni: lo zip di **Save report** dal menu dentro l'host (il suo titolo indica il gioco
scalato), oppure il COLLECT LOGS del launcher.

## Model E e Model L (sperimentali, opzionali, novità della 0.3.5.6)

Due scelte di Super Resolution (upscaler) in più, accanto a FSR e XeSS. **Non sono opera di AMDNR né modelli di AMDNR,
e nessuno dei due è nello zip di AMDNR.** Quando ne scegli uno nell'AMDNR Launcher (GAME SETTINGS di un gioco > **SR
model**: Auto / Model E / Model L), il launcher mostra una volta l'avviso di quel modello, ti chiede di accettarlo e
scarica i file dalla release del suo autore, verificati con l'hash. AMDNR non ospita né distribuisce i loro file né
alcun file NVIDIA. Entrambi sono sperimentali: aspettati malfunzionamenti, e invia una segnalazione quando qualcosa va
storto. **Auto** mantiene la normale via del gioco (e toglie i file di Model L dalla cartella del gioco).

**Model E - HelixSR di lonewolf0622** (<https://github.com/lonewolf0622/HelixSR>), sotto la sua licenza.
- HelixSR costruisce la sua rete sul tuo PC a partire dal file DLSS di NVIDIA, che il launcher scarica dal GitHub di
  NVIDIA stessa (o prende dal tuo gioco); ciò che costruisce resta sul tuo PC.
- Nel gioco: Upscaling > **HelixSR (Model E) - restart required** (giochi D3D12; `[Upscalers] FfxSRProvider=helix_e`).
  Ha effetto al prossimo avvio del gioco.
- Funziona con **entrambi** i runtime NR, danielblnc e lmxxf. Nei nostri test ha girato in un gioco su una RX 9070 XT.

**Model L - d4r di countervolts** (<https://github.com/countervolts/d4r>), sotto la licenza di d4r.
- **Cos'è:** d4r esegue **il runtime DLSS di NVIDIA, sotto l'EULA di NVIDIA, tramite ZLUDA / HIP** su una scheda AMD.
  Il launcher scarica il pacchetto d4r dalla release GitHub di d4r e lo dispone nella cartella del gioco come
  `AMDNR-SR-L\`; AMDNR non ospita né distribuisce alcun file NVIDIA. **L'autore di d4r non l'ha testato su Windows.**
- **Solo RX 9070 / RX 9070 XT** (testato su una RX 9070 XT; la RX 9060 XT e le schede più vecchie vengono rifiutate), e
  nel gioco **solo con giochi D3D12**.
- **Gira con il runtime lmxxf, o con il Neural Rendering spento. Il runtime di danielblnc si abbina a Model E:** con
  danielblnc come runtime NR la riga Model L è in grigio con la frase "Model L runs with the lmxxf runtime (or with
  Neural Rendering off); danielblnc's runtime pairs with Model E. Pick lmxxf under Neural Rendering first." (in
  italiano: "Model L gira con il runtime lmxxf (o con il Neural Rendering spento); il runtime di danielblnc si abbina a
  Model E. Scegli prima lmxxf sotto Neural Rendering."), e un gioco avviato con entrambi usa invece il tuo upscaler FSR.
- Nel gioco: Upscaling > **Model L (d4r) - restart required**, in grigio con il motivo quando qui non può girare (non è
  un gioco D3D12, non è stato preparato nel launcher, non è una RX 9070 / 9070 XT, il runtime è danielblnc, o serve
  ancora un riavvio). Sceglierlo scrive `[DlssNr] AmdSrModel=L` (scegliere un altro upscaler toglie quella riga); ha
  effetto al prossimo avvio del gioco.
- **Sperimentale:** la sua velocità non è ancora ottimizzata e può essere molto più lento di FSR. Se non riesce ad
  avviarsi, il gioco dovrebbe mantenere il tuo upscaler FSR (non FSR 2.1.2), e `OptiScaler.log` dice perché in una riga
  `[MODELL`.

Dentro **AMDNR Anywhere** entrambi sono offerti nelle impostazioni Anywhere del launcher; lì, insieme al Neural
Rendering, sono ancora in fase di test.

## Limiti noti (0.3.5.6)

- **Il ghosting con lmxxf è ridotto, non eliminato:** una scia più corta può ancora seguire gli oggetti in rapido
  movimento.
- **Fast mode su RX 7000 e handheld** è un nuovo valore predefinito, senza ancora segnalazioni da giochi: per favore
  inviaci le tue. `[DlssNr] AmdLmxxfFastMode=false` ripristina la dimensione di rete precedente.
- **Il Full Model Look dopo l'upscaling costa di più**, perché la rete riceve l'immagine alla dimensione dello schermo.
  Dove il controllo del costo o il registro TDR di questo gioco dicono che la scheda non può permetterselo, gira prima
  dell'upscaling e la riga di stato Neural lo dice; anche un tuo `[DlssNr] AmdPlacement=pre` lo tiene prima
  dell'upscaling (lo si dice una volta: scegli di nuovo Full Model Look in NR style, o cancella la riga).
- **Ripiego su FSR 2.2:** in alcuni giochi D3D11 e Vulkan un upscaler fallito può ancora ripiegare su FSR 2.2.
- **Model L** è solo per RX 9070 / RX 9070 XT e D3D12, non si abbina al runtime di danielblnc, è sperimentale e può
  essere molto più lento di FSR; d4r non è stato testato dal suo autore su Windows.
- **Model E e Model L dentro AMDNR Anywhere**, insieme al Neural Rendering, sono ancora in fase di test.
- **danielblnc su RX 7000:** la modalità same-frame del suo runtime costa molto tempo per frame su RDNA 3, per come è
  progettata; lì usa lmxxf.
- **Linux / Proton:** il Neural Rendering lì non gira (vedi sotto).

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

1. Scarica `AMDNR-vX.X.X.zip` dalla pagina delle release (per la 0.3.5.6: `AMDNR-v0.3.5.6.zip`). Gli zip del runtime
   danielblnc (`v0.6.0-Runtime.zip` e gli altri) servono solo al Neural Rendering, quindi su Linux non ti servono
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
  l'SDK HIP né la modalità sviluppatore (tranne su RX 6000, vedi sotto). Per quanto riguarda i chip:
  - RX 9000 (RDNA 4): entrambi i runtime.
  - RX 7000 (RDNA 3, desktop e mobile): entrambi i runtime - lmxxf tramite il backend RDNA 3 di AMDNR, più lento
    che su RDNA 4 (il livello di dimensione della rete e, dalla 0.3.5.6, Fast mode sono attivi di default, vedi sopra).
  - Strix Halo (Radeon 8060S / 8050S): lmxxf.
  - APU per handheld con 12+ compute unit (Z1 Extreme / Z2 / 780M, Z2 Extreme / 890M / 880M): lmxxf,
    sperimentale e lento. Z1 (4 CU), 760M / 740M e 860M / 840M: non supportate.
  - RX 6000 (RDNA 2, desktop): il runtime 0.6.0 di danielblnc (`v0.6.0-Runtime.zip`), con l'SDK HIP di AMD (HIP 7.2)
    installato, che mette il runtime HIP di cui le RX 6000 hanno bisogno in una cartella propria (AMDNR la cerca lì);
    lmxxf non gira ancora su RX 6000 (previsto per la 0.3.6). Steam Deck e APU RDNA 2: non supportate (per il Neural
    Rendering; per gli upscaler sotto Proton vedi "Linux / Proton").

  La scheda Neural indica cosa può eseguire la tua GPU (passa il mouse sulle voci dei runtime, o guarda la riga
  GPU in Diagnostics).
- **AMDNR Anywhere** (preview): Windows, una scheda RX 9000 (RDNA 4) in questa release, e l'AMDNR Launcher, che
  scarica l'host di cattura; il gioco gira in finestra o in finestra senza bordi. Vedi "AMDNR Anywhere".
- **Model E / Model L** (sperimentali, opzionali): l'AMDNR Launcher, che li scarica, e un gioco D3D12; Model L richiede
  anche una RX 9070 / RX 9070 XT e il runtime lmxxf (o il Neural Rendering spento). Vedi "Model E e Model L".
- Un gioco Direct3D 12, Direct3D 11 o Vulkan. Il percorso neurale AMD in sé è D3D12; i titoli D3D11 e
  Vulkan ci arrivano tramite il bridge D3D12 di OptiScaler, il che significa che l'upscaler deve essere
  uno dei backend "w/Dx12" (`ffx_12`). Lascia `Dx11Upscaler` / `VulkanUpscaler` su `auto` e questa
  build lo sceglie per te quando il neural rendering è attivo. Con il Neural Rendering attivo, l'elenco di Upscaling li chiama "... w/Dx12 - Neural".
- Circa 2 GB di VRAM libera a risoluzioni di rendering di classe 1080p.

## Cosa contengono gli archivi

**AMDNR-vX.X.X.zip**

| File | Cos'è |
|---|---|
| `OptiScaler.dll` | OptiScaler con il backend AMD di DLSS-NR (AMDNR 0.3.5.6). Rinominalo come indicato nella guida. |
| `OptiScaler.ini` | Impostazioni. Il Neural Rendering è abilitato; il logging è attivo, così hai qualcosa da allegare a una segnalazione di bug. |
| `LmxxfNrRuntime.dll` | Il runtime neurale lmxxf (0.3.5.6: un controllo, sulla history della rete, della provenienza di ogni pixel, che accorcia le scie di ghosting, e un frame di reset che non lampeggia più così forte; deve venire dalla stessa release di `OptiScaler.dll`. 0.3.5: verifica ogni modulo HIP del pak rispetto all'elenco di digest del pak stesso prima di usarlo, nomina entrambe le parti quando nessun adattatore HIP corrisponde alla GPU del gioco, mantiene i passi di risoluzione dinamica senza ricostruire la rete, e non legge più le variabili d'ambiente di lmxxf che cambiano l'immagine; i kernel di lmxxf, compresi quelli di lmxxf 0.31, i kernel c32w di AMDNR, le dimensioni di rete piccole e la maschera nativa dei personaggi). Usato solo se scelto; legge `LmxxfNrRuntime.pak` accanto a sé, vedi "Il runtime lmxxf". |
| `LmxxfNrRuntime.pak` | Pesi, moduli HIP e shader del runtime lmxxf in un unico file cifrato (465 MB; 0.3.5: il set di moduli per RX 7000 e per gli handheld di classe Z1 Extreme - Z1 Extreme, Z2, Radeon 780M - è circa il 10% più veloce, stessa immagine; le RX 9000 ricevono i moduli di lmxxf 0.37 / 0.39 di Kien (MIT) accanto al loro set di base invariato; i moduli per Z2 Extreme / 890M / 880M e Strix Halo sono invariati). Lo legge solo il runtime lmxxf; tenerlo insieme al runtime danielblnc non crea problemi. |
| `OptiScaler\` | FSR, XeSS, il denoiser FidelityFX e il D3D12 Agility SDK usati da OptiScaler. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | dlssg-to-fsr3 di Nukem9, non modificato e rinominato: le chiamate DLSS Frame Generation del gioco vengono gestite dalla frame generation di FSR 3, anche su Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, vedi `Licenses/`. |
| `Licenses\`, `LICENSE` | Licenze di terze parti, l'avviso di AMDNR (`AMDNR_NOTICE.txt`) e la licenza GPL-3.0 di questa build. |
| `SHA256SUMS.txt` | Checksum di ogni file di questo zip e dei file degli zip del runtime danielblnc che elenca. |

**Non in questo zip** (l'AMDNR Launcher scarica ciascuno di essi verificandone l'hash, solo quando lo scegli): gli zip
del runtime danielblnc (sotto); l'host di cattura di AMDNR Anywhere (la build propria di AMDNR, dalla release GitHub di
AMDNR); HelixSR di Model E e d4r di Model L, dalle release dei loro autori (vedi "Model E e Model L"); e OptiPatcher,
dalle release del progetto OptiScaler (GAME SETTINGS).

**Gli zip del runtime danielblnc** (DLSS-NR on AMD by Daniel Blanco, non modificato, con il suo permesso; usane uno)

Quale usare: `v0.6.0-Runtime.zip` (consigliato) dalla release Alpha0.3.5.5R4 - su RX 9000 e RX 7000, ed è l'unico per
le RX 6000; `v0.5.1-Runtime.zip` (Alpha0.3.5), `v0.5.0-Runtime.zip` (Alpha0.3.4.2), `v0.4.3-Runtime.zip` (Alpha0.3.4.2
e Alpha0.3.4.1), `v0.4.1-Runtime.zip` e `v0.4.0-Runtime.zip` (Alpha0.3.4.1) restano accettati su RX 9000 e RX 7000. Il
runtime lmxxf non richiede alcuno zip di runtime su RX 7000 e RX 9000; le APU per handheld usano solo lmxxf. L'AMDNR
Launcher offre per la tua scheda 0.6.0 (consigliato, elencato per primo), 0.5.1, 0.5.0, 0.4.3, 0.4.1 e 0.4.0, e lo
sceglie per te.

| Zip | Release | Runtime danielblnc |
|---|---|---|
| `v0.6.0-Runtime.zip` | Alpha0.3.5.5R4 | 0.6.0, **consigliato su RX 9000 e RX 7000, ed è quello per le RX 6000** (con l'SDK HIP di AMD installato); le impostazioni del runtime danielblnc funzionano con questo runtime |
| `v0.5.1-Runtime.zip` | Alpha0.3.5 | 0.5.1, ancora accettato; le impostazioni del runtime danielblnc funzionano con questo runtime |
| `v0.5.0-Runtime.zip` | Alpha0.3.4.2 | 0.5.0, ancora accettato (su alcune schede può bloccare alcuni giochi: uno più recente è più sicuro); le impostazioni del runtime danielblnc funzionano con questo runtime |
| `v0.4.3-Runtime.zip` | Alpha0.3.4.2 (e Alpha0.3.4.1) | 0.4.3, ancora accettato; le impostazioni del runtime danielblnc funzionano con questo runtime |
| `v0.4.1-Runtime.zip` | Alpha0.3.4.1 (e Alpha0.3.4) | 0.4.1, ancora accettato. Network style, Tone curve, Black lift e Game exposure sono in grigio con questo runtime |
| `v0.4.0-Runtime.zip` | Alpha0.3.4.1 (e Alpha0.3.4) | 0.4.0, ancora accettato; le impostazioni del runtime danielblnc funzionano con questo runtime |
| `v0.3.3-Runtime.zip` | [Alpha0.3.4](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/tag/Alpha0.3.4) | 0.3.3, ritirato: non più consigliato. Funziona ancora se lo hai già; le impostazioni del runtime danielblnc funzionano con questo runtime |
| `Runtime.zip` | Alpha0.3.4 | 0.3.1; le impostazioni del runtime danielblnc sono in grigio con questo runtime |

**danielblnc 0.6.0 è il runtime danielblnc consigliato** (il launcher lo elenca per primo per la tua scheda). Dalla
0.3.5.5R4 AMDNR esegue il suo runtime in modalità same-frame: con AMDNR la sua modalità async completava tre job e poi
più nulla, quindi un `Async=1` nel suo `dlssnr_on_amd.ini` viene eseguito come same-frame e lo si segnala una volta
(`[DlssNr] AmdDanielAsync=true` lo lascia passare). Dalla 0.3.5.6 il runtime di danielblnc si abbina a Model E, non a
Model L. Una build danielblnc più recente della 0.6.0 non viene gestita da questa release.

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
- **Neural runtime** (danielblnc / lmxxf, con la versione che gira, ad es. `danielblnc 0.6.0` o `lmxxf 0.41`; dalla 0.3.5.6 la voce di lmxxf indica il set di kernel lmxxf che gira, non la release di AMDNR) con una parola di stato: running, restart the game to switch, not
  installed, not for this GPU o stopped. Sotto, il credito del runtime attivo e una riga di stato, ad es.
  `Running - 1920x1080 at 100% - NR 62/s - model 62/s - 15.3 ms` (l'ultimo numero è il costo NR), e una riga **Live** chiusa con più
  dettagli. Quando qualcosa
  richiede la tua attenzione segue una riga arancione, con un pulsante quando c'è una soluzione (Retry lmxxf,
  Switch to danielblnc, Open Upscaling). Nello stato predefinito non ce n'è nessuna.
- **Preset**: Quality / Balanced / Performance impostano la NR resolution a 100 / 85 / 70% e spengono Dynamic NR;
  nient'altro. Sulle APU per handheld c'è un quarto pulsante, **Handheld** (vedi "APU per handheld"). **NR style**
  (prima i tre aspetti - **Full Model Look**, il predefinito dalla 0.3.5.6, **NVIDIA look** e **AMDNR soft** - poi
  quattro stili con nome; vedi "Impostazioni da conoscere"), e **Style slots** (Store / Apply / Clear).
- **Performance**: **Neural placement** (Before / After upscaling, novità della 0.3.5; vedi "Impostazioni da
  conoscere"), NR resolution (%) con il suo costo (dal 25 al 200% dalla 0.3.5.6), Neural passes, **Refine passes**
  (danielblnc, novità della 0.3.5.6), Full network, Fast mode, Dynamic NR resolution, Model interleave
  (Interleave preset e la riga del pacing compaiono sotto mentre è attivo).
- **Quality**: Residual strength, Residual limit, Temporal stability, Sharpening (CAS), e **More quality options**
  (Network history - un'unica casella per entrambi i runtime -, Output smoothing, Stability mode, Residual
  temporal, Residual edge fade, Still-surface steadiness).
- **Image look**: Colour composition, Detail e Colour strength, e tre sezioni richiudibili: **Model strength**
  (Tone e Structure intensity, Character structure, Edit detail / colour, Edge guard, Native character mask, e
  Network style, Tone curve e Black lift di danielblnc), **Exposure and highlights** (Auto-exposure, il suo limite
  delle alte luci, Highlight colour guard, **Game exposure** Auto / On / Off) e **Appearance filter** (con la parola
  off / on dopo il nome; il suo slider **Anti-halo** si chiamava Halo protection prima della 0.3.5.6). Un "default" o
  "custom" tenue dopo il nome di una sezione richiudibile indica se hai cambiato qualcosa al suo interno.
- **Ray Regeneration**: non è più in questa scheda dalla 0.3.5.3; i suoi controlli e il suo stato sono su una scheda **Ray Regeneration** tutta sua (subito dopo Upscaling). Qui solo la riga arancione sotto il runtime può dire "Ray Regeneration is off in this title" o "Ray Regeneration could not start on this GPU" (passa il mouse sopra per il motivo).
- **Screen-space GI** (novità della 0.3.5.3): **AMDNR Screen-space GI**, disattivato di default, con un "by 3zwr1" tenue dopo il nome; sotto, mentre è attivo: GI quality (Low / Medium / High / Ultra; Auto ne sceglie uno per la tua GPU finché non ne premi uno, e la sua etichetta mostra il tempo GPU misurato, o una stima finché la GI non è partita), Bounce light, Ambient occlusion, Radius, Object thickness, Camera FOV, **More GI options** (Bounce colour, Sky light, Multi-bounce, FOV axis, Colour encoding, Debug view) e Reset GI. Su ogni GPU AMD, con o senza runtime NR; dentro AMDNR Anywhere è in grigio con il motivo.
- **La riga degli strumenti**, chiusa all'avvio: **Diagnostics** (Network output, Debug view, Edit shaper A/B, NR cost,
  le letture di ghosting e di auto-regolazione, la riga GPU, **Save report**; la vista di debug RR è nella scheda Ray
  Regeneration dalla 0.3.5) e **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy); **Experimental** compare solo finché il tuo ini tiene attivo lo Screen-space GI ereditato e ritirato (`[AmdRtgi] Enabled`), con la sola casella, etichettata "retired".

Dalla 0.3.5.6 un controllo che il runtime attivo non può eseguire è nascosto su quel runtime, con un conteggio (ad es.
Refine passes e Tone intensity su lmxxf; Edit detail, Edit colour, Edge guard, Output smoothing e Full network su
danielblnc); la sua chiave ini continua a funzionare. Un controllo che funziona solo in certe condizioni resta, in
grigio con il motivo (ad es. Residual limit, Debug view, Neural passes); cambiare runtime non sposta nessun'altra riga.

**Le altre schede:** Upscaling inizia con l'upscaler, una riga di stato e Render resolution (i vecchi Upscale Ratio
Override e Output Scaling); su una scheda video non NVIDIA "DLSS w/Dx12" non compare più. Dalla 0.3.5.6 l'elenco degli
upscaler contiene anche **HelixSR (Model E) - restart required** (giochi D3D12) e **Model L (d4r) - restart required**
(in grigio con il motivo dove non può girare; vedi "Model E e Model L"). **Ray Regeneration** (novità
della 0.3.5) segue Upscaling mentre Ray Regeneration è in funzione nel titolo: le righe di stato (per scheda video e per
API), la riga **Denoiser backend** (Automatic / AMD FSR Ray Regeneration / AMDNR Ray Denoiser (preview) / Off (the game's own): Automatic è quello di AMD su RX 9000 e l'AMDNR Ray Denoiser su tutte le altre schede, i primi tre si cambiano a gioco in corso, e Off dice al gioco che la Ray Reconstruction non è supportata, così tiene il proprio denoiser, dopo un riavvio), i controlli, More Ray Regeneration options, e un blocco Diagnostics tutto
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

- **NR style** (in cima alla scheda Neural, `[DlssNr] AmdNrLook = auto | 0 | 1`, entrambi i runtime) — le prime tre
  voci sono aspetti completi. **Full Model Look** (`auto`, il predefinito dalla 0.3.5.6): la rete dell'NVIDIA look
  (Style 0, tutti i 71 blocchi) con intensità del modello più forti - Structure intensity 2 su entrambi i runtime, Edit
  detail 1.5 ed Edit colour 1.25 su lmxxf - senza residual limit né edge guard, Temporal stability 0.5, mentre l'output
  smoothing, la network history e le altre protezioni di stabilità restano attive; chiede Neural placement After
  upscaling finché non hai impostato tu il placement (vedi Neural placement più sotto). **NVIDIA look** (`0`, il
  predefinito dalla 0.3.5.3 alla 0.3.5.5R4): il più vicino all'output di NVIDIA stessa, Before upscaling. **AMDNR
  soft** (`1`): l'aspetto AMDNR precedente, più morbido e più stabile. Un aspetto fornisce i suoi valori solo finché
  non hai impostato tu quei controlli, e con Model interleave le protezioni mantengono i valori precedenti. Scegliere un
  aspetto nel menu a tendina scrive anche il suo Neural placement (Full Model Look: After upscaling; gli altri due:
  Before) e Full network come valori tuoi. I quattro stili con nome che seguono impostano i loro slider sopra l'aspetto
  in uso.
- **NR resolution** — la leva principale tra qualità e costo. Sotto il 100% il modello lavora su
  un'immagine più piccola e solo la sua *correzione* viene riportata sul frame a piena risoluzione,
  così il frame mantiene il proprio dettaglio. Sopra il 100% il costo cresce con il quadrato (150%
  equivale a 2.25x, 200% a 4x). Dalla 0.3.5.6 lo slider va dal 25% al 200% su entrambi i runtime (prima si fermava al
  150%); la rete di lmxxf arriva comunque al massimo a 1920x1080, e la riga lo dice quando è quello il limite. Lo
  slider si muove a scatti del 5%: ogni nuova dimensione NR può trattenere VRAM
  fino al riavvio del gioco, quindi riavvia il gioco dopo molte modifiche.
  Il costo accanto segna 1.00x al 100%; con lmxxf è il prezzo del livello di dimensione della rete su cui gira (il
  suo tooltip indica il livello). I pulsanti Preset lo impostano a 100 / 85 / 70%.
- **Neural placement** (Neural > Performance, `[DlssNr] AmdPlacement = pre | post`, novità della 0.3.5, entrambi i
  runtime) — dove gira il pass neurale. `pre` (il predefinito della chiave stessa, e ciò che eseguiva ogni build prima
  della 0.3.5.6) modifica l'immagine a risoluzione di render che l'upscaler sta per leggere. `post` modifica invece
  l'immagine finita dell'upscaler, a risoluzione di visualizzazione: più nitido, perché l'upscaler non rifiltra più la
  modifica, e più costoso - con lmxxf la rete gira alla dimensione dello schermo fino al suo tetto di 1920x1080, quindi
  uno schermo 1080p paga il livello più alto a qualsiasi NR resolution, e uno schermo 1440p o 4K riceve una modifica a
  1080 righe riportata su (danielblnc non ha questo tetto, quindi per lui `post` costa di più) - un po' meno tollerante
  in movimento, e l'HUD è compreso se il gioco lo compone prima dell'upscaling. **Dalla 0.3.5.6, finché la riga è vuota
  o `auto`, il Full Model Look predefinito chiede `post` su ogni scheda**; il controllo del costo, il registro TDR di
  questo gioco e la protezione TDR in tempo reale ne valutano il prezzo, e lo riportano a `pre` per la sessione dove la
  scheda non può permetterselo (la riga di stato Neural e `amd_bridge.log` dicono perché). Un `pre` impostato da te (la
  riga, il preset Handheld o il launcher) tiene l'aspetto prima dell'upscaling; la riga di stato lo dice, e scegliere di
  nuovo Full Model Look in NR style scrive `post`. Rifiutato (si torna a `pre`, una riga in `amd_bridge.log`) dentro
  AMDNR Anywhere, in modalità immagine finale, e una volta che Ray Regeneration è entrata in funzione nel titolo. La
  riga NR resolution mostra entrambe le dimensioni mentre `post` è attivo.
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
- **Refine passes** (Neural > Performance, `[DlssNr] AmdRefineMode = full | refine`, solo danielblnc, novità della
  0.3.5.6) — la modalità di pass propria di AMDNR. *Full* (il predefinito) esegue ogni pass alla dimensione piena.
  *Refine* esegue il pass 1 alla dimensione piena e i pass 2 e 3 una dimensione di rete più piccola sulla risposta del
  pass precedente, poi riporta la loro modifica sull'immagine a dimensione piena: ogni pass gira ancora, quelli extra
  costano meno. Nessun effetto con 1 pass. Il runtime di lmxxf costruisce una sola dimensione di rete per sessione,
  quindi esegue Refine come Full e lì la riga è nascosta.
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
- **Più dettaglio sui volti** (novità della 0.3.5.6, entrambi i runtime, sempre attivo) — finché Character structure
  segue Structure intensity (la spunta predefinita), volti e pelle ricevono 1.5 volte la structure della scena, al
  massimo 4 (con il Full Model Look: 2.00 -> 3.00); nessun pass in più e nessun costo in più. Un Character structure
  impostato da te ha sempre la precedenza, e il valore in grigio nel menu mostra il numero aumentato.
  `[DlssNr] AmdFaceDetail=false` (solo nell'ini) lo disattiva.
- **Game exposure** (Image look > Exposure and highlights, `[DlssNr] AmdUseGameExposure = auto | 1 | 0`, entrambi i
  runtime) — dalla 0.3.5.6 le sue voci sono **Auto / On / Off**. Auto usa il valore di luminosità del gioco stesso
  quando il gioco ne pubblica uno; On lo passa sempre alla rete; Off non lo fa mai, e subentra l'auto-exposure del
  runtime - prova Off se NR rende un gioco troppo scuro o troppo luminoso. Agisce al frame successivo del modello.
- **Anti-halo** (Image look > Appearance filter, `[AmdLook] AntiHalo`, 0..1, predefinito 0.901; prima della 0.3.5.6 si
  chiamava Halo protection, stessa chiave) — limita la modifica del filtro accanto ai bordi forti, così che non si formi
  un anello chiaro o scuro; più alto = bordi più sicuri, dettaglio un po' più morbido accanto a essi. Richiede che
  l'Effect mix del filtro sia sopra 0.
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
- **FSR Ray Regeneration** — su RX 9000 (RDNA 4); su RX 7000 (RDNA 3) come preview, attiva di default, con l'AMDNR Ray Denoiser come denoiser (vedi sotto); solo nei giochi che usano DLSS Ray Reconstruction (Cyberpunk 2077,
  Alan Wake 2), con il gioco impostato su DLSS (spoofing attivo) e con ray tracing e Ray Reconstruction
  attivati nelle sue impostazioni. Il Neural Rendering viene quindi eseguito dopo di essa, sul suo
  output, il che costa di più: abbassa la NR resolution se il frame rate cala. Dalla 0.3.5 i suoi controlli sono su
  una scheda **Ray Regeneration** tutta loro, subito dopo Upscaling, disegnata mentre Ray Regeneration è in funzione
  nel titolo; dalla 0.3.5.3 la scheda Neural non ha più una riga di Ray Regeneration, solo la riga arancione sotto il runtime quando Ray Regeneration è spenta in un titolo o non è potuta partire su questa GPU. La riga **Denoiser backend** della scheda (`[FSR-RR] RrBackend` = auto / amd / amdnr / off) offre Automatic (AMD FSR Ray Regeneration su RX 9000, l'AMDNR Ray Denoiser (preview) su tutte le altre schede), AMD FSR Ray Regeneration, AMDNR Ray Denoiser (preview) e Off (the game's own); i primi tre si cambiano a gioco in corso, e Off dice al gioco che la Ray Reconstruction non è supportata, così tiene il proprio denoiser (al prossimo avvio del gioco). In un titolo **Vulkan** la Ray
  Reconstruction è "not supported" per scelta (il denoiser è D3D12) e la scheda lo dice. Il **profilo path-traced**
  (meno grana sui volti con il path tracing) va attivato manualmente dalla 0.3.3.1: spuntalo lì per provarlo in
  Resident Evil Requiem o PRAGMATA. La stessa scheda ha l'intensità della bias mask e lo **smoothing della pelle**
  (sperimentale, per i giochi che pubblicano una guida SSS; disattivato di default, ma attivo di default in Resident
  Evil Requiem dalla 0.3.3.2); i controlli di regolazione temporale sono in *More Ray Regeneration options*, e la
  vista di debug RR e il numero del rumore (grana in ingresso e in uscita, sfarfallio a camera ferma) sono nel blocco
  Diagnostics della scheda. Su RX 7000 (RDNA 3) Ray Regeneration è offerta come preview
  (0.3.5.1; la 0.3.5 non la offriva di default): il gioco può attivare la Ray Reconstruction e le impostazioni di Ray
  Regeneration compaiono, e l'AMDNR Ray Denoiser (preview) le toglie il rumore, anche sulle APU RDNA 3 / 3.5 (AMD FSR Ray Regeneration è fatta per RDNA 4); la pagina Ray Regeneration dice cosa gira. La casella della scheda Upscaling
  **Preview: Ray Regeneration on this card (restart)** (con il tag "preview - on by default") è spuntata di default;
  togli la spunta, o imposta `[FSR-RR] FfxDenoiserAllowPreRdna4=false`, e il gioco tiene il proprio denoiser. Le RX 6000 e precedenti la ricevono solo come opzione, con `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (scheda Upscaling: **Offer Ray Regeneration on this GPU (restart)**, con il tag "preview"), anche lì con l'AMDNR Ray Denoiser. **Sharpening dopo RR** (0.3.4.1): quando il
  gioco non invia alcun valore di nitidezza, AMDNR applica uno sharpening di 0.25 dopo RR su Windows (0 su Linux /
  Proton); per disattivarlo: Image > Sharpness, spunta Override, slider a 0. Dalla 0.3.4.2 quel numero è una chiave dell'ini
  tutta sua, `[Sharpness] RrDefaultSharpness` (stesso valore predefinito 0.25): mettici 0.15, 0.10 o 0 senza toccare
  Override, e un valore che il tuo ini ha conservato sotto `[Sharpness] Sharpness` con Override disattivato viene
  segnalato nel menu come in attesa.
- **AMDNR Screen-space GI by 3zwr1** (disattivato di default; Neural > Screen-space GI, o `[AmdGi] Enabled=true`) - la luce rimbalzata e l'occlusione ambientale in screen space di AMDNR, dalla profondità del gioco, prima di NR, dell'upscaler e dell'interfaccia; funziona con NR attivo o spento; Low / Medium / High / Ultra (Auto per GPU), circa 0.6 ms in High con un render 1080p su una RX 9070 XT (misurato). È screen space: manca la luce che arriva da fuori schermo. Vedi `CHANGELOG.md`.
- **Model E / Model L** (scheda Upscaling, novità della 0.3.5.6) — scelte di upscaler sperimentali realizzate da altri
  autori e scaricate dall'AMDNR Launcher; Model L gira con lmxxf o con NR spento. Vedi "Model E e Model L".
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
tutte e tre le DLL dei pass dallo stesso zip: `v0.6.0-Runtime.zip` dalla release Alpha0.3.5.5R4 (176,366,023 byte,
con SHA256 che inizia per `d2bb1702`; su RX 9000, RX 7000 e RX 6000); `v0.5.1-Runtime.zip` (Alpha0.3.5),
`v0.5.0-Runtime.zip` (Alpha0.3.4.2; 149,550,553 byte, con SHA256 che inizia per `7a49ab0e`), `v0.4.3-Runtime.zip`
(Alpha0.3.4.2 e Alpha0.3.4.1; 116,484,918 byte, con SHA256 che inizia per `07dd7774`), `v0.4.1-Runtime.zip` e `v0.4.0-Runtime.zip`
(Alpha0.3.4.1) restano accettati (il file `dlssnr_amd_pass1.dll` in `v0.4.1-Runtime.zip` pesa 9,916,928 byte, con
SHA256 che inizia per `823063eb`; in `v0.4.0-Runtime.zip`: 10,027,008 byte, `d62be3d8`). Build supportate: 0.2.17,
0.3.0, 0.3.1, 0.3.2, 0.3.3, 0.4.0 e gli zip di runtime citati sopra, fino alla 0.6.0. Non installare il setup di
danielblnc né i suoi file `dxgi.dll` / `version.dll` /
`winhttp.dll` accanto ad AMDNR: AMDNR esegue già il suo runtime. **Una build danielblnc più recente della 0.6.0 non
viene gestita da questa release:** la scheda Neural nomina il file e la sua versione e lo dice. Con la 0.6.0, la 0.5.1,
la 0.5.0, la 0.4.3, la 0.4.1 e la 0.4.0 le impostazioni esclusive di danielblnc (Network style, Tone curve, Black lift,
Game exposure, Fast mode) funzionano; dalla 0.3.5.5R4 un `Async=1` in `dlssnr_on_amd.ini` viene eseguito come
same-frame (`[DlssNr] AmdDanielAsync=true` lo lascia passare; vedi "Cosa contengono gli archivi").

**danielblnc: NR si fermava dopo qualche "timeout", o lo schermo diceva che il driver video aveva resettato la GPU?**
Risolto nella 0.3.5.6 (testato in Silent Hill 2). La causa è stata trovata: con i file di Model L nella cartella del
gioco, il runtime di danielblnc riceveva la copia di HIP propria di Model L, e la sua attesa non vedeva mai finire il
job. Ora AMDNR dà l'HIP del driver a ogni chiamante al di fuori di Model L. Un'attesa che si arrende viene segnalata
come tale e non spegne più il Neural Rendering; non c'era stato alcun reset del driver, e quel testo è sparito; e la
nota "runtime failed" che legge il Doctor del launcher viene scritta solo dopo un errore reale. Il runtime di
danielblnc si abbina a Model E; per Model L usa lmxxf.

**Hai scelto FSR 3 / FSR 4 e il gioco usava FSR 2.1.2?** Dalla 0.3.5.6 una scelta di upscaler fallita o non pronta
(Model E, Model L o un'altra) dovrebbe mantenere la tua scelta FSR (FSR 4 / FSR 3.1) invece di ripiegare su FSR 2.1.2
(D3D12); FSR 2.1.2 gira solo quando lo scegli tu, o quando nessun'altra libreria FSR può girare, e il log lo dice. In
alcuni giochi D3D11 e Vulkan un upscaler fallito può ancora ripiegare su FSR 2.2. Se lo vedi ancora, invia lo zip del
report.

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
gioco una volta che è entrata in funzione); dalla 0.3.5.3 la scheda Neural non ha più una riga di Ray Regeneration. Mentre non è in funzione, la scheda non viene disegnata; se Ray Regeneration è spenta in questo titolo o non è potuta partire su questa GPU, lo dice la riga arancione sotto il runtime della scheda Neural. Su RX 7000 e sulle APU RDNA 3 / 3.5 Ray Regeneration è offerta come preview, attiva di default, e l'AMDNR Ray Denoiser (preview) le toglie il rumore (AMD FSR Ray Regeneration è fatta per RDNA 4); `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (o la casella della scheda Upscaling **Preview: Ray Regeneration on this card (restart)**, senza spunta) spegne la preview, così il gioco tiene il proprio denoiser. Su RX 6000 e precedenti non viene offerta di default: la casella della scheda Upscaling **Offer Ray Regeneration on this GPU (restart)** (`[FSR-RR] FfxDenoiserAllowPreRdna4=true`) la offre, con l'AMDNR Ray Denoiser (preview). Per farla partire,
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
Reconstruction non è supportata, così tiene il proprio denoiser (dopo un riavvio). Dalla 0.3.5.3 la stessa riga offre anche il denoiser proprio di AMDNR, l'AMDNR Ray Denoiser (preview), che Automatic usa su tutte le schede tranne le RX 9000.

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

- **0.3.5.6** (questa build) — **Model E** (HelixSR di lonewolf0622) e **Model L** (d4r di countervolts) come scelte di
  upscaler sperimentali e opzionali scaricate dalle release dei loro autori (Model L: RX 9070 / 9070 XT, con lmxxf o con
  NR spento); il Full Model Look come stile NR predefinito, dopo l'upscaling dove la scheda può permetterselo; più
  dettaglio sui volti; meno ghosting con lmxxf; Fast mode di default su RX 7000 e sugli handheld; Game exposure
  (Auto / On / Off) e Anti-halo; NR resolution fino al 200%; un menu che nasconde ciò che un
  runtime non può eseguire; Refine passes su danielblnc; la correzione dei falsi timeout di danielblnc; FSR 3 / 4
  mantenuto invece di FSR 2.1.2; il conteggio della memoria di XeFG; AMDNR Launcher 0.3.5.25.
- **Dalla 0.3.5.1 alla 0.3.5.5R4** — danielblnc 0.6.0 (anche su RX 6000, con l'SDK HIP di AMD); lmxxf 0.41 su RX 9000;
  l'NVIDIA look; l'AMDNR Ray Denoiser (preview) e AMDNR Screen-space GI (0.3.5.3); una protezione contro i blocchi dopo
  errori GPU conteggiati; correzioni di crash dalle vostre segnalazioni; l'AMDNR Launcher ridisegnato con ASK AI. Vedi
  `CHANGELOG.md`.
- **0.3.5** — **AMDNR Anywhere** (preview, RX 9000, tramite il launcher); la scheda Ray Regeneration con
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
- **0.3.6** — le RX 6000 (RDNA 2) sui kernel propri di AMDNR (il runtime 0.6.0 di danielblnc gira già lì): dietro un
  gate hardware, con il livello 360 e Model interleave come preview su Navi 21; AMDNR Anywhere su RX 7000 una volta
  testato lì. (L'AMDNR Ray Denoiser, il denoiser proprio di AMDNR dietro la chiamata Ray Reconstruction, è già
  distribuito come preview dalla 0.3.5.3.)
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
- **kernel di lmxxf 0.37, 0.39 e 0.41** in `LmxxfNrRuntime.pak` (i moduli lmxxf037-*, lmxxf039-* e lmxxf041-* per RX 9000) e il loro codice di lancio in `LmxxfNrRuntime.dll` — lmxxf 0.37 / 0.39 / 0.41 by Kien (MIT), distribuiti così come li ha compilati lmxxf; la parte di AMDNR è il caricamento come un unico gruppo con pin, i pin SHA-256, i default per GPU, gli interruttori di disattivazione e i fallback
- **c32w kernels** (0.3.3.2) — i kernel RDNA 4 a wave singola propri di AMDNR per la rete di lmxxf, Copyright (c) 2026 3zwr1 (AMDNR); idee tratte dalla documentazione pubblica di AMD su RDNA 4 WMMA (GPUOpen, ROCm matrix instruction calculator)
- **Il backend RDNA 3 di AMDNR** (0.3.3; le build per handheld gfx1103 / gfx1150 nella 0.3.4), la politica dei livelli di dimensione della rete e le dimensioni di rete piccole (0.3.4) — Copyright (c) 2026 3zwr1 (AMDNR)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Dagherbou / OptiScaler_DLSSNR** — https://github.com/Dagherbou/OptiScaler_DLSSNR
- **wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass** — https://github.com/wilsjo2/OptiScaler-DLSSNR-PreSR-Multipass
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, non modificato)
- **HelixSR** — lonewolf0622 — https://github.com/lonewolf0622/HelixSR (Model E, 0.3.5.6: scaricato dall'AMDNR Launcher dalla sua release e usato sotto la sua licenza; non distribuito da AMDNR; costruisce la sua rete sul tuo PC a partire dal file DLSS di NVIDIA, che il launcher scarica dal GitHub di NVIDIA stessa o prende dal tuo gioco)
- **d4r** — countervolts — https://github.com/countervolts/d4r (Model L, 0.3.5.6: il runtime DLSS di NVIDIA sotto l'EULA di NVIDIA, eseguito tramite **ZLUDA** (vosen, https://github.com/vosen/ZLUDA) e HIP; scaricato dall'AMDNR Launcher dalla release di d4r, sotto la licenza di d4r; non distribuito da AMDNR, non testato dal suo autore su Windows)
- **OptiPatcher** — dalle release del progetto OptiScaler, https://github.com/optiscaler/OptiPatcher (scaricato dalle GAME SETTINGS dell'AMDNR Launcher quando lo richiedi; non distribuito da AMDNR)
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** — l'host di cattura della finestra dentro cui gira AMDNR Anywhere, nella build propria di AMDNR pubblicata sulle release GitHub di AMDNR (`Magpie.exe` ricompilato con le modifiche di AMDNR; l'offerta del sorgente, un diff rispetto al tag del fork e le opzioni di build accanto, in `Anywhere-host-source/`; nessun file NVIDIA) — https://github.com/Blinue/Magpie (il fork: https://github.com/SAOG0721/Magpie)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (matematica della composizione colore, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), la base dello sblocco multi-frame XeFG integrato e del suo pacing
- **Zach Hembree (DarkHelmet)** — FSR Ray Regeneration per OptiScaler, l'origine del percorso Ray Regeneration di AMDNR, proseguito da **burak113**, dal cui branch AMDNR ha fatto il port (branch di OptiScaler ffx-denoise-experimental, GPL-3.0)
- **Screen-space GI** (l'effetto ereditato; ritirato dal menu in 0.3.4, `[AmdRtgi] Enabled` nell'ini) — un effetto che AMDNR ha ereditato dalla linea OptiScaler-AMD-PreSR; il merito è dei suoi autori originali. Richiede la cartella `experimental_lighting` del pacchetto danielblnc, che AMDNR non distribuisce.
- **AMDNR Screen-space GI by 3zwr1** - lavoro proprio di AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), scritto a partire da ricerche pubblicate (elencate in `CHANGELOG.md` e `Licenses/AMDNR_NOTICE.txt`).
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## AMDNR Launcher

**AMDNR Launcher** (nuovo nella 0.3.4) è un programma per Windows 10 / 11 che può girare anche sotto Proton su Linux
(sperimentale, non ancora testato da noi: vedi "Linux / Proton"). Installa e aggiorna AMDNR gioco per gioco: trova i
tuoi giochi (Steam, Epic, l'app Xbox, Ubisoft Connect, l'app EA, GOG, Rockstar, Battle.net e Amazon Games), sceglie
il nome della DLL, scarica la build e il runtime danielblnc che scegli (e, quando li scegli, Model E e Model L dalle
release dei loro autori), controlla ogni installazione con il suo Doctor, esegue AMDNR Anywhere per i giochi senza un upscaler proprio (PLAY ANYWHERE) e si aggiorna da solo. Scarica
`AMDNR-Launcher.exe`, l'AMDNR Launcher dalla release più recente, dalla pagina delle release:
<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>

**Novità del Launcher 0.3.5.25** (nella release Alpha0.3.5.6; aggiorna prima sé stesso, poi i tuoi giochi):

- **SR model** nelle GAME SETTINGS di un gioco: Auto / Model E / Model L (vedi "Model E e Model L"). La prima scelta di
  un modello mostra il suo avviso (ACCEPT / Cancel, Cancel è il predefinito); poi il launcher scarica i suoi file dalla
  release dell'autore, verificati con l'hash. Model L si abbina al runtime lmxxf (o al Neural Rendering spento); con il
  runtime di danielblnc il launcher lo rifiuta e dice perché. Auto toglie di nuovo i file di Model L.
- Righe **Game exposure** (Auto / On / Off) e **Anti-halo** nelle GAME SETTINGS, scritte solo in una build di AMDNR
  che le legge.
- **Impostazioni Anywhere** nella pagina ANYWHERE del launcher: runtime NR, passes, dimensione neurale, FSR (Auto /
  FSR 4 / FSR 3 / Off - Off mantiene NR), frame generation (sperimentale), movimento per l'upscaling, nitidezza e SR
  model; si applicano al prossimo PLAY ANYWHERE.
- Aspetto **Most FPS**: fa girare la rete una dimensione più piccola (lmxxf), con una dimensione neurale più piccola;
  nessun frame neurale viene saltato (Refine passes su danielblnc dove gira più di un pass).
- **Public Chat** nel Community Lab: i messaggi sono pubblici e controllati dall'AI, e possono essere tradotti
  automaticamente nella lingua del tuo launcher; il team può rimuovere i messaggi.
- Un runtime che scegli per un gioco viene mantenuto per quel gioco.

**Novità del Launcher 0.3.5.1** (nella release Alpha0.3.5; aggiorna prima sé stesso, poi i tuoi giochi):

- **PLAY ANYWHERE** su un gioco senza un upscaler proprio (vedi "AMDNR Anywhere"): il launcher scarica l'host di
  cattura dalla release GitHub di AMDNR, avvia il gioco ed esegue il Neural Rendering sulla sua finestra, con un
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
- **Dodici lingue**: turco, coreano e ungherese si aggiungono alle nove qui sotto (ora tredici: anche l'uzbeco).
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

- Nove lingue (dodici dalla 0.3.5.1, ora tredici): inglese, arabo, cinese (semplificato), francese, spagnolo,
  portoghese, italiano, russo e polacco, più turco, coreano, ungherese e uzbeco. Il
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

Il file `nvngx_dlssnr.dll` di NVIDIA non è incluso in questi archivi, e nessun altro file NVIDIA lo è. Il pacchetto d4r
di Model L (il runtime DLSS di NVIDIA sotto l'EULA di NVIDIA) e il file DLSS di NVIDIA da cui Model E costruisce la sua
rete vengono scaricati dall'AMDNR Launcher solo quando li scegli, dalla release di d4r e dal GitHub di NVIDIA stessa (o
dal tuo gioco); AMDNR non li ospita né li distribuisce, e Model E e Model L sono opera dei loro autori, non di AMDNR.
AMDNR non è affiliato con AMD o NVIDIA né approvato da esse. Niente di tutto ciò è approvato
o supportato da NVIDIA, AMD o da qualsiasi publisher di giochi, né è affiliato con loro. Controlla
direttamente una funzionalità non documentata. Usalo a tuo rischio e pericolo.
