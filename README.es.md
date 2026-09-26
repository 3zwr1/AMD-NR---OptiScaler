# AMDNR — DLSS 5 Neural Rendering en AMD (build de OptiScaler) — v0.3.3.2

[English](README.md) | [中文](README.zh-CN.md) | [Português](README.pt-BR.md) | **Español** | [العربية](README.ar.md) | [Français](README.fr.md) | [Italiano](README.it.md) | [Русский](README.ru.md) | [Polski](README.pl.md)

> **Necesitamos tu apoyo.** Únete al servidor de Discord — <https://discord.gg/AMDNR> — para
> ayuda, reportes de errores y builds de prueba; cada reporte con un log hace mejor la siguiente build.

DLSS 5 Neural Rendering funcionando en GPUs AMD, integrado en OptiScaler para que funcione en
cualquier juego Direct3D 12 que OptiScaler ya engancha. Sobre el pase neural: model interleave para
una gran ganancia de fotogramas, composición residual, generación de fotogramas XeSS desbloqueada
hasta 6X (hasta 10X opcional en juegos D3D12), y FSR Ray Regeneration para los juegos que usan DLSS
Ray Reconstruction.

**Discord: <https://discord.gg/AMDNR>** — soporte, reportes de errores (`#bug-report`), builds
de prueba.

**Apoya el proyecto: <https://ko-fi.com/3zinr>**

> **El runtime de danielblnc es obra de Daniel Blanco.** El runtime neural AMD de `Runtime.zip`
> (`dlssnr_amd_pass1..3.dll`) es **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> AMDNR lo distribuye sin modificar, con su permiso; no es obra de AMDNR. Por favor, apoya su proyecto.
> Los créditos completos de todos los demás están al final de esta página.

---

## AMDNR - Guía de instalación de OptiScaler

La instalación es bastante sencilla.

### 1. Descarga los archivos

Descarga estos dos archivos desde GitHub (<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>):

* `AMDNR-vX.X.X.zip`
* `Runtime.zip`

### 2. Extrae ambos archivos

Extrae el contenido de los dos archivos `.zip`.

### 3. Copia todo a la carpeta del juego

Primero, copia todos los archivos de `AMDNR-vX.X.X` a la carpeta raíz del juego — la misma carpeta
donde está el `.exe` del juego.

Después, haz lo mismo con todos los archivos de `Runtime`.

### 4. Renombra OptiScaler.dll

Dentro de la carpeta del juego, busca:

`OptiScaler.dll`

Renómbralo a:

`dxgi.dll`

`dxgi.dll` es la opción recomendada.

Si el juego no arranca o el mod no carga, prueba renombrar `OptiScaler.dll` a uno de estos:

* `d3d12.dll`
* `winmm.dll`
* `version.dll`
* `dbghelp.dll`

Prueba un nombre a la vez. No crees varias copias de `OptiScaler.dll`.

> **Resident Evil Requiem (y su demo) necesita REFramework.** Es un requisito conocido, no un error de AMDNR: OptiScaler depende de él
> para saltarse el anti-tamper de Capcom ([wiki de OptiScaler](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-9-Requiem)). Sin él, el juego se cierra
> 15-60 s después de arrancar ("An unhandled exception occurred"). Copia `dinput8.dll` de `REFramework.zip`, del último nightly
> (<https://github.com/praydog/REFramework-nightly/releases>), junto a `dxgi.dll`, y cambia la tecla del menú de REFramework (p. ej. a Supr / Delete): también es Insert.
> Tras una actualización del juego habrá cierres hasta que se actualice REFramework. Probablemente PRAGMATA, Monster Hunter Wilds y Onimusha también lo necesitan (sin confirmar).

### 5. Inicia el juego

`HOME` enciende y apaga Neural Rendering mientras juegas (ambos runtimes; un aviso pequeño dice
On / Off). Puedes reasignarla junto a la casilla Enable en la pestaña Neural o en Interface > Keybinds.

Eso es todo.

Inicia el juego normalmente y pulsa:

`INSERT`

Esto abre el menú de OptiScaler / AMDNR, donde puedes configurar el mod como prefieras.

### Si no funciona

Si el juego sigue sin arrancar con ninguno de los nombres anteriores, repórtalo en el canal
`#bug-report` de Discord.

Al reportar el problema, sube también cualquier archivo `.log` que se haya generado en la carpeta
raíz del juego.

Esos logs son muy importantes y nos ayudan a identificar el problema mucho más rápido.

> El `.exe` normalmente no está donde apunta el acceso directo. Los juegos Unreal lo guardan en
> `<Game>\Binaries\Win64\`.

---

### El runtime lmxxf (0.3.0, opcional)

Un segundo runtime neural (licencia MIT, de lmxxf) puede ejecutar el pase en lugar del de
danielblnc. RDNA 4, y RDNA 3 (RX 7000, Strix Halo) mediante el backend RDNA 3 de AMDNR - más lento
ahí: empieza con NR resolution al 67%. Necesita dos cosas junto al juego:

1. `LmxxfNrRuntime.dll` - en este archivo, junto a `OptiScaler.dll` (se copia con el resto).
2. `LmxxfNrRuntime.pak` (416 MB, incluido en el zip de AMDNR) junto a `LmxxfNrRuntime.dll` - los
   pesos, módulos HIP y HLSL de lmxxf en un solo archivo cifrado y autenticado. El runtime lo abre
   en memoria; nada se desempaqueta en disco.

En el primer inicio que encuentra un runtime instalado y ninguna elección hecha, el menú pregunta
cuál usar (`[DlssNr] NrBackend = daniel | lmxxf` en el ini lo registra; Neural > Neural runtime lo
cambia, en el siguiente inicio del juego). La edición de lmxxf se aplica un fotograma después,
transportada por los vectores de movimiento, así que el fotograma nunca espera a la red (unos 17 ms
a 1080p en una RX 9070 XT). Su log es `lmxxf_backend.log` junto al juego.

**Compatibilidad (lmxxf).** El runtime solo ve lo que ve DLSS, así que lo que varía por título es
una lista corta: formato de color y HDR, vectores de movimiento y su escala, profundidad y su
dirección, la máscara reactiva, la textura de exposición, la bandera Reset, y dónde se sitúa el
pase (antes de Super Resolution, o después de Ray Reconstruction). Probado hasta ahora:

| Título | API / posición | Notas |
|---|---|---|
| Silent Hill 2 | D3D12, antes de SR | título de referencia; manejada la asignación de color con relleno de Unreal |
| Forza Horizon 6 | D3D12, antes de SR | |
| Stray | D3D11 a través del puente D3D12, antes de SR | |
| GTA V Enhanced | D3D12, antes de SR, HDR, máscara reactiva de un canal | corregido en 0.3.0: la máscara se leía como "todo reactivo" y la edición nunca llegaba |
| Cualquier título con Ray Reconstruction | D3D12, después de RR (escrita de vuelta en la salida) | soportado desde 0.3.0; aún no confirmado en un juego |

Si un título no muestra efecto: `lmxxf_backend.log` tiene una línea `lmxxf inputs:` (formatos,
tamaños, escala de movimiento, dirección de profundidad, máscara, exposición) y una línea
`lmxxf stats @N:` cada 600 fotogramas (exposición, brillo alimentado, la edición del modelo, la
edición transportada, keep, media reactiva, longitud de vectores y fracción rechazada). Adjunta el
log a un reporte; esas dos líneas suelen decir por qué.

Bajo lmxxf, el bloque Neural runtime tiene **Network history** (la propia entrada temporal del
modelo), e Image look tiene el grupo **lmxxf edit**: el modelador de la edición (Edit detail,
Edit colour, Edge guard: ganancia sobre la parte fina de la edición del modelo, su color frente a su
cambio de brillo, y un desvanecimiento de la edición en los bordes de profundidad) y **Output
smoothing** (el pase de salida de upstream, necesita Network history). Neural passes, Residual
strength/limit, el enfoque, Debug view 1 y el filtro Appearance se aplican en ambos runtimes.

**Full network** (Neural > Performance, `[DlssNr] LmxxfFullNetwork`, solo lmxxf) ejecuta los 71
bloques de la red en lugar de saltarse el 42, el 43 y el 46: algo más fiel, unos 0.5 ms más lento a
1080p (16.6 -> 17.1 ms en una RX 9070 XT). Desactivado por defecto.

## Requisitos

- Una GPU AMD con un controlador actual. El runtime neural usa HIP a través del controlador; no
  hace falta el SDK de HIP ni el modo desarrollador. Qué chips: RX 9000 (RDNA 4) ejecuta ambos
  runtimes; RX 7000 (RDNA 3, escritorio y portátil) también ambos - lmxxf mediante el backend RDNA 3 de
  AMDNR, más lento que en RDNA 4; Strix Halo (8060S / 8050S) ejecuta lmxxf; las APU de consolas
  portátiles (Z1 Extreme / 780M, Z2 Extreme / 890M) y RDNA 2 (RX 6000, Steam Deck) no están
  soportadas por ninguno. La pestaña Neural dice qué puede ejecutar tu GPU.
- Un juego Direct3D 12, Direct3D 11 o Vulkan. La ruta neural de AMD es D3D12; los títulos D3D11 y
  Vulkan la alcanzan a través del puente D3D12 de OptiScaler, lo que significa que el upscaler debe
  ser uno de los backends "w/Dx12" (`ffx_12`). Deja `Dx11Upscaler` / `VulkanUpscaler` en `auto` y
  esta build lo elige por ti cuando el renderizado neural está activo.
- Unos 2 GB de VRAM libre a resoluciones de renderizado de clase 1080p.

## Qué hay en los dos archivos

**AMDNR-vX.X.X.zip**

| Archivo | Qué es |
|---|---|
| `OptiScaler.dll` | OptiScaler con el backend AMD de DLSS-NR. Renómbralo como dice la guía. |
| `OptiScaler.ini` | Ajustes. Neural Rendering está activado; el registro está encendido para que un reporte tenga algo que adjuntar. |
| `LmxxfNrRuntime.dll` | El runtime neural lmxxf (kernels de lmxxf 0.29). Se usa solo cuando se elige; lee `LmxxfNrRuntime.pak` junto a él, ver "El runtime lmxxf". |
| `LmxxfNrRuntime.pak` | Los pesos, módulos HIP y shaders del runtime lmxxf en un archivo cifrado (416 MB). Solo lo lee el runtime lmxxf; es inofensivo mantenerlo con el runtime de danielblnc. |
| `OptiScaler\` | FSR, XeSS, el denoiser FidelityFX y el D3D12 Agility SDK que usa OptiScaler. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | El dlssg-to-fsr3 de Nukem9, sin modificar y renombrado: las llamadas de DLSS Frame Generation del juego servidas por la generación de fotogramas de FSR 3, también en Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, ver `Licenses/`. |
| `Licenses\`, `LICENSE` | Licencias de terceros, el aviso de AMDNR (`AMDNR_NOTICE.txt`) y la licencia GPL-3.0 de esta build. |
| `SHA256SUMS.txt` | Sumas de verificación de cada archivo distribuido, de ambos archivos. |

**Runtime.zip**

| Archivo | Qué es |
|---|---|
| `dlssnr_amd_pass1..3.dll` | El runtime neural AMD, v0.3.1 de danielblnc, sin modificar. Tres copias para que el multipase tenga una por pase. |
| `dlssnr_on_amd_weights.bin` | Los pesos de la red que carga el runtime. |

## Ajustes que conviene conocer

Abre la pestaña **Neural**. Los valores por defecto son la configuración probada más reciente, así
que el primer movimiento útil es cambiar una cosa a la vez.

- **NR resolution** — la palanca principal de calidad/coste. Por debajo del 100% el modelo trabaja
  sobre una imagen más pequeña y solo su *corrección* se lleva de vuelta al fotograma a resolución
  completa, así que el fotograma conserva su propio detalle. Por encima del 100% el coste crece con
  el cuadrado (150% es 2.25x). El control se mueve en pasos del 5%: cada tamaño de NR nuevo puede
  retener VRAM hasta que reinicies el juego, así que reinícialo tras muchos cambios.
- **Residual strength** — cuánto de la edición del modelo se aplica; por encima de 1 amplifica. Es
  el control que más cambia la imagen.
- **Residual limit** — un techo a cuánto puede moverse un píxel. ¿Manchas por zonas? **Bájalo**.
- **Model interleave** — ejecuta el modelo cada dos fotogramas para una gran ganancia de
  fotogramas. Los fotogramas omitidos los rellena el **Interleave preset**; *Guided fill v2* es el
  predeterminado y el que está en desarrollo activo. El ritmo de los dos tipos de fotograma es
  automático, y **Adaptive interleave** (activado por defecto) ejecuta el modelo en cada fotograma
  mientras la imagen se mueve, de modo que las omisiones - y sus artefactos - solo ocurren cuando
  la imagen está quieta.
- **Neural passes** — 2 y 3 apilan el modelo, con rendimientos decrecientes. Bajo lmxxf, la
  historia de la red sigue siendo su primer pase; los pases extra son solo refinamiento espacial.
  danielblnc ejecuta 1 pase en los títulos Vulkan (un aviso bajo el control lo indica).
- **Colour composition** (Neural > Image look, en ambos runtimes) — *Classic* (predeterminado) es
  la imagen que ya tenías. *RenoDX (experimental)* ejecuta la composición de color de RenoDX después
  del modelo, como hace la ruta NVIDIA: Composition detail y colour, un **Highlight guard** en ambos
  sentidos (2x por defecto) que acota la respuesta del modelo frente al original, y controles
  opcionales de piel / entorno. En un fotograma display-referred (SDR) vuelve a Classic, con un
  aviso en el menú. Los estilos y presets de NR no lo tocan.
- **La generación de fotogramas está apagada en un ini nuevo.** Pestaña Frame Gen: elige el FG
  Input (p. ej. "DLSSG via Streamline" en un juego con generación de fotogramas DLSS) y el FG
  Output (XeFG), luego marca **Active** en la sección Frame Generation (XeFG) y pulsa Save
  Settings. Un ini de 0.1.0 que la tenía activada no se conserva al instalar el ini de 0.2.0.
- **Generación multifotograma XeFG** — 3X a 6X viene integrada y activada por defecto
  (`XeFG\UnlockMFG`), para la copia de OptiScaler y la del propio juego. **Borra `XeFGUnlock.asi`**
  de `OptiScaler\plugins` si aún lo tienes: dos copias del mismo parche hacen que el juego se cierre.
  **Hasta 10X es opcional** (solo juegos D3D12): elige *XeFG ceiling (restart)* bajo FG Output en la
  pestaña Frame Gen (4X, 6X por defecto, 8X o 10X; `[XeFG] MaxInterpolatedFrames`), reinicia y luego
  elige el multiplicador en el desplegable MFG. Por encima de 6X necesita el proveedor XeFG propio de
  OptiScaler con Extra pacing activado; la copia de XeSS 3 del propio juego se queda en 6X como
  máximo. 10X necesita una pantalla de 360 Hz o más y un límite de fotogramas a refresco / 10; la
  latencia es alta y el proveedor reserva unos 128 MiB más de VRAM a 4K. 7X-10X aún no está
  confirmado en un juego: si lo pruebas, envía `OptiScaler.log`.
- **FSR Ray Regeneration** — solo RDNA 4 (RX 9000) por defecto; solo en juegos que usan DLSS Ray Reconstruction (Cyberpunk 2077,
  Alan Wake 2), con el juego ejecutando DLSS (spoofing activado), trazado de rayos y Ray
  Reconstruction activados en sus propios ajustes. Neural Rendering se ejecuta entonces después,
  sobre su salida, lo que cuesta más: baja la NR resolution si caen los fotogramas. Sus controles
  (Neural > Quality > Ray Regeneration) solo aparecen mientras el juego usa Ray Reconstruction.
  El **perfil path-traced** (menos grano en las caras con path tracing) es opcional desde 0.3.3.1:
  márcalo ahí para probarlo en Resident Evil Requiem o PRAGMATA. En el mismo lugar están la intensidad
  de la bias mask, una vista de depuración de RR y el **suavizado de piel** (experimental, para juegos
  que publican una guía SSS; desactivado por defecto, pero activado por defecto en Resident Evil
  Requiem desde 0.3.3.2).

## Si algo sale mal

`OptiScaler.log` aparece en la carpeta del juego. Adjúntalo en `#bug-report`, y di qué juego y qué
GPU. El backend AMD también escribe `amd_presr.log` y `amd_bridge.log`, que son los útiles cuando
lo que falla es específicamente el pase neural. El log de la sesión anterior se conserva como
`OptiScaler.previous.<exe>.log`; tras un cierre inesperado, adjúntalo también (el log nuevo dice
entonces "no clean exit recorded").

**¿NR frames 0/s, y la pestaña Neural o `amd_presr.log` dicen que la DLL del pase es una build que este
AMDNR no maneja?** Tus `dlssnr_amd_pass1..3.dll` son una build de danielblnc que este AMDNR no conoce (se ha
visto circular un conjunto 0.2.16), o falta una de las tres. Desde 0.3.3.2 la pestaña Neural nombra el
archivo y su versión y dice qué hacer. Usa `v0.4.0-Runtime.zip` (la más nueva) o `Runtime.zip` (0.3.1) de
esta release, las tres DLL de pase del mismo zip: la `dlssnr_amd_pass1.dll` de `v0.4.0-Runtime.zip` tiene
10,027,008 bytes y su SHA256 empieza por `d62be3d8`. Builds soportadas: 0.2.17, 0.3.0, 0.3.1, 0.3.2, 0.3.3,
0.4.0, y 0.4.1 / 0.4.2 antes de su publicación. No instales el instalador propio de danielblnc ni su
`dxgi.dll` / `version.dll` / `winhttp.dll` junto a AMDNR: AMDNR ya ejecuta su runtime.

**¿lmxxf no hace nada, o se detiene enseguida, en un PC con gráficos integrados?** Corregido en 0.3.3.2. En
un Ryzen de escritorio con los gráficos integrados activados, un portátil con APU AMD y una Radeon, o un PC
con dos GPU AMD, la GPU del juego a menudo no es el dispositivo HIP 0. lmxxf fallaba entonces en su primer
fotograma (`hipErrorInvalidHandle (400)`, y luego "session is poisoned" en `lmxxf_backend.log`) y se quedaba
apagado. Reemplaza tanto `OptiScaler.dll` (el archivo que renombraste, p. ej. `dxgi.dll`) como
`LmxxfNrRuntime.dll` por los de 0.3.3.2. Aún sin probar en un PC así: si lmxxf sigue deteniéndose, la
pestaña Neural ahora dice por qué; envía `lmxxf_backend.log` y `amd_bridge.log` (lista los dispositivos HIP).

**¿Un juego Vulkan (Indiana Jones and the Great Circle) se detiene al arrancar con "Could not create the
Vulkan device (VK_ERROR_EXTENSION_NOT_PRESENT)"?** Corregido en 0.3.2: la ruta neural heredada de
NVIDIA pedía al driver AMD dos extensiones de dispositivo exclusivas de NVIDIA. Los títulos Vulkan llegan
al pase neural por el puente D3D12 de OptiScaler (ver Requisitos).

**¿lmxxf congelaba un juego Vulkan en el primer fotograma de NR?** Corregido en 0.3.3; espera una pausa
única de unos 1 s cuando arranca NR. Si una sesión Vulkan se detiene antes de la primera respuesta de lmxxf,
el siguiente inicio usa el runtime de danielblnc y la pestaña Neural dice por qué; pulsa ahí **Retry lmxxf**
(borra `lmxxf_vk_launch.pending` junto a `OptiScaler.dll`) para volver a probar lmxxf.

**¿danielblnc se pausaba varios segundos y luego detenía NR en un juego Vulkan (Indiana Jones) con 2-3
Neural passes?** Corregido en 0.3.3: en los títulos Vulkan ejecuta 1 pase, y su espera de 80 ms tras el
envío ha desaparecido. El primer fotograma de NR de una sesión aún se pausa unos 5 s; un aviso bajo la
elección de runtime explica sus líneas de log. Testers: `[DlssNr] AmdVkLateCopyWait=true` (experimental,
desactivado por defecto, aún sin probar en un juego) debería eliminar esa pausa; envía `OptiScaler.log`,
`amd_presr.log` y `dlssnr_on_amd.log`.

**¿El uso de RAM de lmxxf subía mientras NR estaba activo?** Corregido en 0.3.3 (eran unos 45 GB por
hora a 60 fotogramas de NR por segundo). Lo que queda: danielblnc retiene VRAM por cada tamaño de NR
nuevo por encima de unos 1 MP (desde 0.3.3.2 sus tamaños se redondean a 64 px fuera del 100%, así que son
pocos); reinicia el juego tras muchos cambios con danielblnc. Desde 0.3.3.2, lmxxf ya no retiene unos 97 MB
por cada cambio de NR resolution o de modo DLSS: crea sus búferes de red una vez por tamaño de red y los
reutiliza (queda un resto pequeño de unos 10-25 MB de VRAM por cambio).

**¿Un juego con Streamline falla al arrancar con el error 0x18 de slInit (visto con NBA 2K27 en AMD)?**
0.3.3 cierra una forma en que los hooks de plugins de Streamline de OptiScaler podían causarlo, pero no
está confirmado que sea la causa en NBA 2K27. `OptiScaler.log` ahora registra líneas `slInit returned ...`
y `[SLINIT]`: envía el log con el reporte.

**¿El Ray Reconstruction del juego está activado pero la pestaña Neural dice "Ray Regeneration is off in
this title"?** El juego no publica lo que FSR Ray Regeneration necesita (Satisfactory: sin matrices de
cámara). El escalado FSR corre en su lugar y NR toma su posición habitual antes del SR; nada en el ini
cambia esto.

**¿Un juego de Ubisoft Anvil (AC Black Flag Resynced, Shadows, Mirage) muestra "DX12 Error 0x80070057"?**
Esos juegos llevan su propia generación de fotogramas XeSS. Esta build se la deja a ellos (la salida
XeFG de OptiScaler se retira ahí y la pestaña Frame Gen lo indica); usa la opción XeSS FG del propio
juego. Si sigue ocurriendo, pon `[FrameGen] Enabled=false` y `[fakenvapi] ForceXeLL=false` y repórtalo
con el log.

**¿The Last of Us Part I se cierra al arrancar?** Es la propia inicialización de Streamline del
juego, un problema conocido de OptiScaler: renombra `sl.common.dll` en la carpeta del juego a
`sl.common.dll.bak` y elige **FSR 3.1** en los ajustes del juego en lugar de DLSS.

Notas completas de cada versión: `CHANGELOG.md` (en el zip y en el repositorio).

## Hoja de ruta

- **0.3.3** (esta build) — lmxxf en RDNA 3 (RX 7000; backend propio de AMDNR); composición de color
  RenoDX (experimental, opcional) en ambos runtimes; lmxxf: opción Full network, fuga de RAM
  corregida, títulos Vulkan corregidos (carga diferida de pesos dentro del puente Vulkan), kernels de
  0.29 (idénticos bit a bit, más rápidos); danielblnc en títulos Vulkan: 1 Neural pass, mensajes más
  claros, una espera de copia tardía opcional; XeFG hasta 10X (opcional, D3D12); inicio de
  Streamline reforzado y diagnósticos; perfil path-traced y suavizado de piel de FSR Ray
  Regeneration; robustez en UE5.
- **0.3.2** — los reportes de 0.3.1: los títulos Vulkan arrancan y funcionan con lmxxf, colores
  de lmxxf alineados con danielblnc (autoexposición), el desplegable del runtime, estado y ajuste de Ray
  Reconstruction; el dlssg-to-fsr3 de Nukem9 en el zip para la generación de fotogramas en Vulkan.
- **0.3.1** — correcciones de los primeros reportes de 0.3.0 (lmxxf solo nunca
  funcionaba, el NR silencioso de Where Winds Meet, el cierre al cambiar la calidad de DLSS, GTA V
  Legacy) y presets de estilo de NR con tres ranuras personalizadas.
- **0.3.0** — el runtime neural HIP **lmxxf** (RDNA 4) como runtime seleccionable junto al de
  danielblnc, distribuido como `LmxxfNrRuntime.dll` + `LmxxfNrRuntime.pak`: historia de la red,
  Neural passes reales, el modelador de la edición, la posición después de Ray Regeneration,
  diagnósticos por título y autorreparación. Muchas gracias a TheAutomatic, sobre cuyo trabajo en
  el proyecto DLSS 5 AMD se construye esta integración.
- **0.4.0** — el AMDNR Launcher (instalación en un clic de los runtimes y el pak, actualizaciones) y
  soporte para títulos sin upscaler propio (clase Stray), donde OptiScaler aporta el upscaler y el
  pase neural juntos.

---

## Créditos

Esta build es un trabajo de cableado sobre el trabajo de otras personas. Si te resulta útil, el
agradecimiento pertenece a upstream.

- **TheAutomatic** — DLSS 5 AMD project — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD — https://github.com/danielblnc/DLSS-NR-on-AMD (`Runtime.zip`, sin modificar)
- **lmxxf** — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (el runtime HIP, MIT)
- **kernels c32w** (0.3.3.2) — kernels RDNA 4 de una wave propios de AMDNR para la red de lmxxf, Copyright (c) 2026 3zwr1 (AMDNR); ideas de la documentación pública de WMMA de RDNA 4 de AMD (GPUOpen, ROCm matrix instruction calculator)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, sin modificar)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (matemática de la composición de color, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), la base del desbloqueo multifotograma de XeFG integrado y de su ritmo
- **burak113** — el preprocesador de FSR Ray Regeneration (rama de OptiScaler ffx-denoise-experimental, GPL-3.0)
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## Copyright / Licencia

AMDNR es Copyright (c) 2026 3zwr1 (AMDNR). Es un fork de OptiScaler, distribuido bajo la licencia
GPL-3.0 en `LICENSE`.

El trabajo propio de AMDNR lleva un término adicional según la sección 7(b) de la GPL-3.0 (ver
`Licenses/AMDNR_NOTICE.txt`): toda copia, fork u obra derivada que lo use debe conservar sus avisos y
acreditar **AMDNR by 3zwr1** (<https://github.com/3zwr1/AMD-NR---OptiScaler>).

**Copyright del menú de AMDNR.** El menú de AMDNR — su disposición, su diseño, sus textos y el código que AMDNR añadió para él — es Copyright (c) 2026 3zwr1 (AMDNR). Forma parte de este fork bajo GPL-3.0, con estos términos adicionales (GPL-3.0 section 7): (b) quien reutilice cualquier parte de él debe conservar esta línea de copyright y acreditar de forma visible a AMDNR by 3zwr1, en el menú y en el README; (c) no puedes presentarlo, ni presentar una copia modificada, como obra tuya; las versiones modificadas deben indicar claramente que han sido modificadas; (e) no se concede ningún derecho sobre el nombre ni el logotipo de AMDNR; otros proyectos no pueden usarlos.

El trabajo de upstream acreditado arriba sigue siendo de sus autores, bajo sus propias licencias;
AMDNR no reclama copyright sobre él.

El código fuente se publicará con AMDNR 0.5.0.

## Legal

Esta build se distribuye bajo la licencia GPL-3.0 en `LICENSE`; las licencias de las bibliotecas de
terceros están en `Licenses\`. El runtime neural AMD y sus pesos se redistribuyen bajo su autoría
original tal como se acredita arriba, solo por comodidad, sin reclamar propiedad y sin ofrecer
garantía.

El `nvngx_dlssnr.dll` de NVIDIA no está en estos archivos. Nada de esto está respaldado, afiliado ni
soportado por NVIDIA, AMD ni ningún editor de juegos. Maneja directamente una función no documentada.
Úsalo bajo tu propio riesgo.
