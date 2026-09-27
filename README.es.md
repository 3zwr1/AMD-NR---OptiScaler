# AMDNR — DLSS 5 Neural Rendering en AMD (build de OptiScaler) — v0.3.4.1

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

> **El runtime de danielblnc es obra de Daniel Blanco.** El runtime neural AMD de los archivos `*Runtime.zip`
> (`dlssnr_amd_pass1..3.dll`) es **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> AMDNR lo distribuye sin modificar, con su permiso; no es obra de AMDNR. Por favor, apoya su proyecto.
> Los créditos completos de todos los demás están al final de esta página.

> **Novedades de 0.3.4.1 (hotfix):** Ray Regeneration es menos suave en Windows (enfoque de 0.25 cuando el juego no
> envía ningún valor de nitidez; para desactivarlo: Image > Sharpness, marca Override, control deslizante a 0); ya
> no aparece la falsa ventana emergente "Upscaler failed to run!" en Control Resonant; en Linux / Proton el menú
> funciona (confirmado por un jugador, también con la generación de fotogramas activada) y Ray Regeneration no añade
> ahí enfoque por defecto (ver "Linux / Proton"). **AMDNR Launcher 0.3.4.1**, hecho a partir de tus comentarios en
> Discord: nueve idiomas, búsqueda, favoritos, ocultar, renombrar, CHOOSE GAME .EXE, PLAY, un UNINSTALL completo y
> más (ver "AMDNR Launcher"). Neural Rendering no cambia respecto a 0.3.4 (mismo runtime y mismo pak): si vienes de
> 0.3.4, reemplaza solo `OptiScaler.dll`; si usas el launcher, él lo actualiza por ti. Detalles: `CHANGELOG.md`.

> **Novedades de 0.3.4:** un menú nuevo (la pestaña Neural rehecha, el mismo aspecto en todas las pestañas y un
> botón **Save report** que comprime tus logs para un reporte); lmxxf es más rápido en RX 7000 (1440p FSR Quality:
> 73.3 -> 52.2 ms por pasada de la red en una RX 7800 XT, tiempo de red medido fuera de un juego) y en RX 9070 / 9070 XT (kernels de lmxxf 0.31);
> lmxxf funciona en APU de consolas portátiles (experimental; la prueba de un tester, en un juego: unos 29 fps en Shadow of the Tomb Raider en una ROG Ally);
> un **Fast mode** opcional para lmxxf; **AMDNR Screen GI**, la GI en espacio de pantalla propia de AMDNR (preview,
> desactivada por defecto); y muchas correcciones. Reemplaza `OptiScaler.dll`,
> `LmxxfNrRuntime.dll` y `LmxxfNrRuntime.pak` a la vez. Detalles: `CHANGELOG.md`.

---

## AMDNR - Guía de instalación de OptiScaler

La instalación es bastante sencilla. **En Windows, el AMDNR Launcher hace todo esto por ti** (ver "AMDNR Launcher"
más abajo). A mano:

### 1. Descarga los archivos

Descarga estos archivos de la última release en GitHub (<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>;
0.3.4.1 es la etiqueta Alpha0.3.4.1):

* `AMDNR-vX.X.X.zip` (para 0.3.4.1: `AMDNR-v0.3.4.1.zip`), con el runtime lmxxf completo.
* Para el runtime de danielblnc, un zip de runtime según tu GPU: en **RX 9000**, `v0.4.1-Runtime.zip` (el más
  nuevo) o `v0.4.0-Runtime.zip` de Alpha0.3.4.1; en **RX 7000**, `v0.3.3-Runtime.zip`, que sigue en la
  [release Alpha0.3.4](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/tag/Alpha0.3.4). El runtime lmxxf va
  en `AMDNR-vX.X.X.zip` y no necesita ningún zip de runtime en RX 7000 y RX 9000; las APU de consolas portátiles
  usan solo lmxxf. El AMDNR Launcher elige el zip adecuado para tu GPU. Ver "Qué hay en los archivos".

### 2. Extrae ambos archivos

Extrae el contenido de los dos archivos `.zip`.

### 3. Copia todo a la carpeta del juego

Primero, copia todos los archivos de `AMDNR-vX.X.X` a la carpeta raíz del juego — la misma carpeta
donde está el `.exe` del juego.

Después, haz lo mismo con todos los archivos del zip de runtime (p. ej. `v0.4.1-Runtime` en RX 9000,
`v0.3.3-Runtime` en RX 7000).

> **¿Actualizas desde un AMDNR anterior?** Vuelve a copiarlo todo y sobrescribe. En 0.3.4 cambiaron tres archivos
> a la vez: `OptiScaler.dll` (sustituye el archivo que renombraste, p. ej. `dxgi.dll`, por el nuevo renombrado igual),
> `LmxxfNrRuntime.dll` y `LmxxfNrRuntime.pak` (440 MB, nuevo en 0.3.4). No los mezcles con copias
> anteriores. Puedes conservar tu `OptiScaler.ini`: los ajustes nuevos usan sus valores por defecto.
> **De 0.3.4 a 0.3.4.1 solo cambió `OptiScaler.dll`:** reemplaza ese único archivo; `LmxxfNrRuntime.dll`,
> `LmxxfNrRuntime.pak` y los archivos de tu zip de runtime se quedan como están. El AMDNR Launcher lo hace por ti:
> pulsa REPAIR / UPDATE en un juego que muestre "Update available".

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

**Lo más fácil: Save report.** Si el menú se abre, pulsa **Save report** (la última fila de Neural > Diagnostics, o la primera de Advanced > Logging). Escribe un zip,
`AMDNR-report-<exe del juego>-<fecha>.zip`, en la carpeta del juego (en el Escritorio si la carpeta del juego es de
solo lectura, si no en `%TEMP%`), con `report.txt`, los logs y los archivos ini, y el menú muestra dónde quedó. Tu
nombre de usuario de Windows y el nombre del PC se sustituyen por marcadores; un nombre dentro de una ruta del juego
fuera de `C:\Users\` no. Adjunta el zip en `#bug-report`.

> El `.exe` normalmente no está donde apunta el acceso directo. Los juegos Unreal lo guardan en
> `<Game>\Binaries\Win64\`.

---

### El runtime lmxxf (0.3.0, opcional)

Un segundo runtime neural (licencia MIT, de lmxxf) puede ejecutar el pase en lugar del de
danielblnc. RDNA 4 lo ejecuta de forma nativa; RDNA 3 (RX 7000, Strix Halo) lo ejecuta mediante el backend
RDNA 3 de AMDNR por 3zwr1 - más lento ahí, ver "RX 7000" más abajo: empieza con NR resolution al 70% o menos.
Las APU de consolas portátiles también lo ejecutan, de forma experimental (ver "APU de consolas portátiles" más
abajo). Necesita dos cosas junto al juego:

1. `LmxxfNrRuntime.dll` - en este archivo, junto a `OptiScaler.dll` (se copia con el resto).
2. `LmxxfNrRuntime.pak` (440 MB, incluido en el zip de AMDNR) junto a `LmxxfNrRuntime.dll` - los
   pesos, módulos HIP y HLSL de lmxxf en un solo archivo cifrado y autenticado. El runtime lo abre
   en memoria; nada se desempaqueta en disco.

En el primer inicio que encuentra un runtime instalado y ninguna elección hecha, el menú pregunta
cuál usar (`[DlssNr] NrBackend = daniel | lmxxf` en el ini lo registra; Neural > Neural runtime lo
cambia, en el siguiente inicio del juego). La edición de lmxxf se aplica un fotograma después,
transportada por los vectores de movimiento, así que el fotograma nunca espera a la red (unos
14.1 ms de tiempo de red a 1080p en una RX 9070 XT). Su log es `lmxxf_backend.log` junto al juego.

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

Ambos runtimes comparten una sola pestaña Neural (ver "El menú" más abajo). Los controles que el runtime
activo no tiene aparecen en gris con una etiqueta corta, u ocultos con un recuento. Solo lmxxf: **Full
network**, **Output smoothing** (Quality > More quality options, necesita Network history), **Edit detail**,
**Edit colour** y **Edge guard** (Image look > Model strength: ganancia sobre la parte fina de la edición del
modelo, su color frente a su cambio de brillo, y un desvanecimiento de la edición en los bordes de
profundidad) y el tope de altas luces de la autoexposición. Nuevo en lmxxf en 0.3.4: Network output,
Encoding, Residual edge fade, Game exposure, Fast mode, la lectura del ritmo del interleave y la
máscara de personajes nativa del modelo con Structure intensity y Character structure (cada
cambio reconstruye la red: una pausa de alrededor de 1 s).

**Full network** (Neural > Performance, `[DlssNr] LmxxfFullNetwork`, solo lmxxf) ejecuta los 71
bloques de la red en lugar de saltarse el 42, el 43 y el 46: algo más fiel, unos 0.5 ms más lento a
1080p (16.6 -> 17.1 ms en una RX 9070 XT, medido en 0.3.3). Desactivado por defecto.

**Fast mode** (Neural > Performance, `[DlssNr] AmdLmxxfFastMode`, lmxxf, opcional, desactivado por defecto) ejecuta
la red un nivel de tamaño por debajo (1080 -> 900, 900 -> 720): alrededor de un 29% menos de tiempo de red a 1080p
(RX 9070 XT, medido fuera de un juego), con el detalle fino algo más suave. Las builds de danielblnc que tienen su
propio Fast mode muestran también ahí una fila Fast mode (`[DlssNr] AmdDanielFastMode`); los runtimes de los zips de
runtime de esta release no lo tienen, así que la fila está oculta.

### RX 7000 (RDNA 3): más rápido con el nivel de tamaño de red (nuevo en 0.3.4)

La red de lmxxf funciona a unos pocos tamaños fijos (niveles): 720 (1280x720), 900 (1600x900) y 1080
(1920x1080), más 576 y 360 (nuevos, usados en consolas portátiles). Un nivel cuesta lo mismo sea cual sea la
parte que llena la imagen. En RDNA 3 (RX 7000, Radeon 8060S / 8050S y las APU portátiles) el tamaño de NR de
lmxxf se ajusta ahora por defecto a un nivel: baja al nivel inferior siguiente cuando está más cerca de él (más
barato), si no crece hasta llenar su propio nivel (mismo coste, algo más de detalle), nunca por encima del
tamaño del propio fotograma.

Tiempo de red por pasada en una RX 7800 XT (medido por un tester con la sonda de lmxxf; solo la red, media
de 30 pasadas; el tiempo del nivel 900 se midió a 1600x900):

| Ajuste del juego | 0.3.3.2 | 0.3.4 en RX 7000 |
|---|---|---|
| 1440p, FSR Quality (render 1706x960), NR 100% | nivel 1080: 73.3 ms | nivel 900: 52.2 ms |
| Render 1080p, NR 85% | nivel 1080: 73.2 ms | nivel 900: 52.2 ms |
| Render 1080p, NR 70% | nivel 900: 52.2 ms | nivel 720: 34.4 ms |
| Render 1080p, NR 80% | nivel 900: 52.2 ms | nivel 900, lleno: 52.2 ms (más detalle) |
| Render 1080p, NR 100% | nivel 1080: 73.2 ms | sin cambios |

- En el juego la ganancia por fotograma mostrado es menor: con Model interleave la red corre cada 2
  fotogramas, y el juego tiene su propio coste. Aún sin medir en un juego.
- La red ve una imagen algo más pequeña (a 1440p Quality, alrededor de un 6% menos de píxeles por lado), así
  que el detalle fino puede quedar algo más suave. `[DlssNr] AmdLmxxfTierSnap=false` vuelve a los tamaños de
  0.3.3.2. RX 9000 conserva los tamaños de 0.3.3.2 salvo que lo pongas en `true`.
- Empieza con NR resolution al 70% o menos (el nivel 720 con un render de 1080p; el preset Performance es
  70%). El coste junto a NR resolution se calcula según el nivel en el que corre la red; su tooltip nombra el
  nivel.

### APU de consolas portátiles (experimental, nuevo en 0.3.4)

lmxxf funciona en APU de consolas portátiles con 12 o más unidades de cómputo, mediante el backend RDNA 3 de
AMDNR por 3zwr1: **Z1 Extreme, Z2 y Radeon 780M** (gfx1103), **Z2 Extreme, Radeon 890M y 880M** (gfx1150). Es
experimental y lento. La fila Neural runtime dice "experimental" tras el crédito
RDNA 3.
Primeros resultados de un tester (ROG Ally, Z1 Extreme): la sonda de lmxxf fuera de un juego, 54.7 ms por pasada de
la red al tamaño 360p, 110.9 ms a 576p; en un juego, la prueba de un tester (Shadow of the Tomb Raider, 1280x720 con XeSS, preset Handheld),
62 ms por pasada de la red de media a 360p con el modelo cada 4.º fotograma, unos 29 fps con NR activado.

- **No soportadas:** Z1 y Radeon 740M (4 unidades de cómputo), Radeon 760M (8), Radeon 860M / 840M. El runtime
  de danielblnc no funciona en APU portátiles. RX 6000 (RDNA 2) está previsto para 0.4.0; la Steam Deck y otras
  APU RDNA 2 no están soportadas.
- **Lo que hace por sí solo** (solo mientras tu ini no tenga un valor propio): la red corre a su tamaño más
  pequeño, 360p (640x360), y el modelo corre cada 4 fotogramas (Model interleave; no se guarda). Neural passes
  sigue en 1.
- **Velocidad, con honestidad:** Como
  referencia: una RX 7800 XT (60 unidades de cómputo) necesita 34.4 ms por pasada de la red al tamaño 720; estos
  chips tienen de 12 a 16 y funcionan a relojes más bajos. Espera una gran pérdida de fotogramas incluso a 360p
  con el modelo cada 4 fotogramas, algo de ghosting por el interleave largo, y un aspecto más suave que en una
  GPU de escritorio. El coste de NR al final de la línea de estado de la pestaña Neural (y en Diagnostics)
  muestra el número real en tu dispositivo.
- **Ajustes:**
  - Más nítido pero más lento: `[DlssNr] AmdLmxxfTierCap=576` (el tamaño de red 1024x576).
  - Con un render de 720p u 800p, NR resolution al 100% ya alimenta el tamaño 360p, así que una NR resolution
    más baja no lo abarata.
  - Model interleave en Off se guarda como `[DlssNr] AmdInterleave=1` (también apagado), para que el valor por
    defecto de la portátil no vuelva en el siguiente inicio. Para apagarlo a mano, escribe 1, no 0.
  - Preset > **Handheld** ajusta NR resolution al 100%, Dynamic NR desactivado, el modelo cada 4.º fotograma, 1 Neural pass y Full network desactivado. El botón solo aparece en estas APU; Quality, Balanced y Performance también mantienen aquí el tamaño de red de 360p (el menú lo indica).
- **FSR 4:** en estos chips (y en las Radeon 780M / 760M / 740M en general) FSR 4 INT8 ya no se activa por sí solo:
  con `Dx12Upscaler=auto` el upscaler es XeSS, y FSR 3.1 sigue siendo FSR 3.1. `[FSR] Fsr4ForceModel=2` aún lo
  fuerza (experimental).
- **Shadow of the Tomb Raider** (y los juegos que crean su dispositivo D3D12 dos veces) ya no se cierra al arrancar el
  upscaler (corregido en 0.3.4).
- **Controlador:** usa el controlador Adrenalin propio de AMD. lmxxf necesita HIP (`amdhip64_7.dll`), que
  algunos controladores de fabricantes de portátiles omiten; `amd_bridge.log` dice entonces que HIP no está
  disponible.
- **Usa juntos los archivos de 0.3.4** (0.3.4.1 solo cambia `OptiScaler.dll`): solo el pak de 0.3.4 tiene los
  módulos para portátiles, y el `LmxxfNrRuntime.dll` de 0.3.4 rechaza una portátil cuando `OptiScaler.dll` es
  anterior a 0.3.4 ("this handheld needs OptiScaler.dll 0.3.4 or newer").
- **Testers con una consola portátil:** pide en Discord el kit de prueba para portátiles (`handheld-test.zip`).
  Su `run_probe.bat` mide la red en tu dispositivo y escribe `handheld_result.txt` (tu nombre de usuario de
  Windows queda oculto).

## Linux / Proton (Steam Deck, Linux de escritorio)

AMDNR funciona bajo Proton y Wine como una build de OptiScaler. **Neural Rendering no funciona en Linux (solo
Windows):** los dos runtimes de NR necesitan AMD HIP, y Proton y Wine no ofrecen HIP. Con NR activado bajo Proton, NR
no se ejecuta, y la pestaña Neural no siempre dice por qué. Es lo esperado, no un cierre ni una instalación rota. El
AMDNR Launcher es un programa de Windows que puede funcionar bajo Proton (experimental, aún no lo hemos probado, ver
abajo); la instalación a mano funciona sin él.

**Qué funciona:** los upscalers FSR (FSR 3.1, y FSR 4 en las GPU y controladores que lo soportan), el menú (`INSERT`)
y **Save report**. Un jugador confirmó en Steam Proton (RX 9070 XT, vkd3d-proton, Resident Evil Requiem) que el juego
arranca, el menú se abre y toma el control del ratón, y Save report funciona, también con la generación de fotogramas
activada.

**Ray Regeneration y generación de fotogramas:**
- Problema conocido: Ray Regeneration puede mostrar manchas rosas / magenta en Proton; el enfoque por defecto ahora está desactivado ahí, pero si aún las ves usa FSR sin Ray Regeneration (FSR 4 en RX 9000) y envía un Save report.
  En Proton, AMDNR no añade enfoque después de Ray Regeneration cuando el juego no envía ningún valor de nitidez (en
  Windows añade 0.25): Image > Sharpness muestra "RR default 0 (off on Proton)", y Override sigue fijando tu propio
  valor.
- **La generación de fotogramas** ahora se activa sin cierres, pero los contadores de fps cuentan también los
  fotogramas generados: con un límite de 60 fps o V-Sync a 60 Hz son 30 fotogramas reales, y se ve como 30. Déjala
  desactivada en Proton por ahora (`[FrameGen] FGOutput=nofg`), o úsala solo cuando el juego llegue a unos 60 fps sin
  ella, en una pantalla de más de 60 Hz.

**Requisitos:** un Proton o Wine actual (probado: Proton 11, que es Wine 11), con el juego en vkd3d-proton (D3D12) o
DXVK (D3D11), lo predeterminado en Proton. Las versiones anteriores no están probadas.

**El AMDNR Launcher en Linux (experimental, aún no lo hemos probado).** El launcher es el mismo programa de Windows,
`AMDNR-Launcher.exe` (autocontenido: no hay que instalar .NET ni ningún otro runtime). Bajo Wine / Proton detecta
Wine y muestra un aviso con lo que hay que hacer. También busca tu biblioteca de Steam de Linux a través de la unidad
`Z:` de Wine (`~/.steam/steam` y `~/.local/share/Steam`, y las carpetas de biblioteca de `libraryfolders.vdf`). Aún
no lo hemos probado nosotros: si lo pruebas, cuéntanos en Discord si funciona. Para probarlo:

1. En Steam, añade `AMDNR-Launcher.exe` como juego que no es de Steam (**Juegos > Añadir un juego que no es de Steam
   a mi biblioteca**; en inglés: **Games > Add a Non-Steam Game to My Library**).
2. En sus **Propiedades > Compatibilidad** (en inglés: **Properties > Compatibility**), fuerza una versión de Proton
   (Proton Experimental) y luego inícialo desde Steam.
3. Si el juego no está en **LIBRARY** (BIBLIOTECA), pulsa **ADD** (AÑADIR) y elige la carpeta del juego (tus
   carpetas de Linux están en la unidad `Z:`); si el launcher elige un `.exe` equivocado, usa **CHOOSE GAME .EXE**
   (ELEGIR EL .EXE DEL JUEGO).
4. Selecciona el juego y pulsa **INSTALL** (INSTALAR).
5. En **Propiedades > General > Opciones de lanzamiento** del juego (en inglés: **Properties > General > Launch
   Options**), escribe `WINEDLLOVERRIDES="dxgi=n,b" %command%` (si el launcher usó otro nombre de DLL para el
   juego, pon ese nombre en lugar de `dxgi`), y después sigue con los pasos 5 y 6 de la instalación a mano de abajo
   (inicia el juego desde Steam; el botón **PLAY** del launcher está desactivado bajo Wine / Proton).

**Instalación a mano** (sin el launcher):

1. Descarga `AMDNR-vX.X.X.zip` de la página de releases (para 0.3.4.1: `AMDNR-v0.3.4.1.zip`). Los zips del runtime de
   danielblnc (`v0.4.1-Runtime.zip` y los demás) solo los usa Neural Rendering, así que no los necesitas en Linux
   (copiar uno no hace daño).
2. Extrae el zip y copia todo en la carpeta del juego, junto al `.exe` del juego.
3. Renombra `OptiScaler.dll` a `dxgi.dll`.
4. En Steam, abre **Propiedades > General > Opciones de lanzamiento** del juego (en inglés: **Properties > General >
   Launch Options**) y escribe:

   ```
   WINEDLLOVERRIDES="dxgi=n,b" %command%
   ```

   Esto le dice a Wine que cargue el `dxgi.dll` de la carpeta del juego en lugar del suyo; sin ello AMDNR no carga.
   Si usaste otro nombre (por ejemplo `winmm.dll` o `version.dll`), pon ese nombre en lugar de `dxgi`, p. ej.
   `WINEDLLOVERRIDES="winmm=n,b" %command%`. Lutris, Heroic y Bottles: añade el mismo override (`dxgi` =
   `native,builtin`) en los DLL overrides o en las variables de entorno del runner.
5. En `OptiScaler.ini`, pon `[FrameGen] FGOutput=nofg` (generación de fotogramas desactivada, ver arriba).
6. Inicia el juego y pulsa `INSERT` para abrir el menú. Configura ahí el upscaler.

**El menú.** En 0.3.4, con la generación de fotogramas activada, el menú podía abrirse sin recibir la entrada del
ratón ni del teclado, o no abrirse. 0.3.4.1 vincula el menú a la ventana del juego; un jugador confirmó en Proton que
el menú se abre y toma el control del ratón, también con la generación de fotogramas activada. Si aun así te pasa en
tu equipo, AMDNR muestra el aviso "Menu window lost". Entonces, en `OptiScaler.ini`, pon `[FrameGen] FGOutput=nofg`;
si el menú sigue sin responder, pon también `[Menu] OverlayMenu=false` (el menú clásico, que no depende de la ventana
overlay).

**HDR.** AMDNR no activa HDR bajo Proton. El HDR depende de tu configuración de Proton y del escritorio: una build de
Proton con soporte HDR y una sesión que pueda mostrar HDR (por ejemplo gamescope, o un escritorio Wayland con HDR
activado). Si el HDR funciona en el juego sin AMDNR, sigue funcionando con AMDNR; si la opción HDR del juego aparece
en gris, el arreglo está en tu configuración de Proton o del escritorio.

**Para reportar un problema en Linux:** usa el botón **Save report** del menú (deja `[Log] LogToFile=true`, el valor
por defecto, para que el reporte tenga el log de esta sesión); el reporte muestra si el juego corrió bajo Wine/Proton,
vkd3d-proton o DXVK. Por favor, añade tu distribución, GPU, versión de Mesa y versión de Proton.

## Requisitos

- Windows 10 u 11 (64 bits) para Neural Rendering. El AMDNR Launcher es un programa de Windows que puede funcionar
  bajo Proton (experimental, aún no lo hemos probado). Bajo Linux / Proton, AMDNR funciona como una build de
  OptiScaler sin NR (ver "Linux / Proton").
- Una GPU AMD con un controlador actual. El runtime neural usa HIP a través del controlador; no
  hace falta el SDK de HIP ni el modo desarrollador. Qué chips:
  - RX 9000 (RDNA 4): ambos runtimes.
  - RX 7000 (RDNA 3, escritorio y portátil): ambos runtimes - lmxxf mediante el backend RDNA 3 de AMDNR, más
    lento que en RDNA 4 (el nivel de tamaño de red está activado por defecto, ver arriba).
  - Strix Halo (Radeon 8060S / 8050S): lmxxf.
  - APU de consolas portátiles con 12+ unidades de cómputo (Z1 Extreme / Z2 / 780M, Z2 Extreme / 890M /
    880M): lmxxf, experimental y lento. Z1 (4 CU), 760M / 740M y 860M / 840M: no soportadas.
  - RX 6000 (RDNA 2): aún no soportada, prevista para 0.4.0. Steam Deck y APU RDNA 2: no soportadas (para
    Neural Rendering; para los upscalers bajo Proton, ver "Linux / Proton").

  La pestaña Neural dice qué puede ejecutar tu GPU (pasa el ratón por las entradas de runtime, o mira la
  línea GPU en Diagnostics).
- Un juego Direct3D 12, Direct3D 11 o Vulkan. La ruta neural de AMD es D3D12; los títulos D3D11 y
  Vulkan la alcanzan a través del puente D3D12 de OptiScaler, lo que significa que el upscaler debe
  ser uno de los backends "w/Dx12" (`ffx_12`). Deja `Dx11Upscaler` / `VulkanUpscaler` en `auto` y
  esta build lo elige por ti cuando el renderizado neural está activo. Con Neural Rendering activado, la lista de Upscaling los nombra "... w/Dx12 - Neural".
- Unos 2 GB de VRAM libre a resoluciones de renderizado de clase 1080p.

## Qué hay en los archivos

**AMDNR-vX.X.X.zip**

| Archivo | Qué es |
|---|---|
| `OptiScaler.dll` | OptiScaler con el backend AMD de DLSS-NR (AMDNR 0.3.4.1). Renómbralo como dice la guía. |
| `OptiScaler.ini` | Ajustes. Neural Rendering está activado; el registro está encendido para que un reporte tenga algo que adjuntar. |
| `LmxxfNrRuntime.dll` | El runtime neural lmxxf (0.3.4, sin cambios en 0.3.4.1: los kernels de lmxxf, incluidos los de lmxxf 0.31, los kernels c32w de AMDNR, los tamaños de red pequeños y la máscara de personajes nativa). Se usa solo cuando se elige; lee `LmxxfNrRuntime.pak` junto a él, ver "El runtime lmxxf". |
| `LmxxfNrRuntime.pak` | Los pesos, módulos HIP y shaders del runtime lmxxf en un archivo cifrado (440 MB, sin cambios en 0.3.4.1; nuevo en 0.3.4: los módulos para consolas portátiles y los kernels de lmxxf 0.31). Solo lo lee el runtime lmxxf; es inofensivo mantenerlo con el runtime de danielblnc. |
| `OptiScaler\` | FSR, XeSS, el denoiser FidelityFX y el D3D12 Agility SDK que usa OptiScaler. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | El dlssg-to-fsr3 de Nukem9, sin modificar y renombrado: las llamadas de DLSS Frame Generation del juego servidas por la generación de fotogramas de FSR 3, también en Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, ver `Licenses/`. |
| `Licenses\`, `LICENSE` | Licencias de terceros, el aviso de AMDNR (`AMDNR_NOTICE.txt`) y la licencia GPL-3.0 de esta build. |
| `SHA256SUMS.txt` | Sumas de verificación de cada archivo de este zip, y de los archivos de los zips del runtime de danielblnc que figuran en él. |

**Los zips del runtime de danielblnc** (DLSS-NR on AMD by Daniel Blanco, sin modificar, con su permiso; usa uno)

Cuál usar: en **RX 9000**, `v0.4.1-Runtime.zip` (o `v0.4.0-Runtime.zip`) de Alpha0.3.4.1; en **RX 7000**,
`v0.3.3-Runtime.zip` de la
[release Alpha0.3.4](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/tag/Alpha0.3.4). El runtime lmxxf no
necesita ningún zip de runtime en RX 7000 y RX 9000; las APU de consolas portátiles usan solo lmxxf.

| Zip | Release | Runtime de danielblnc |
|---|---|---|
| `v0.4.1-Runtime.zip` | Alpha0.3.4.1 (y Alpha0.3.4) | 0.4.1, el más nuevo, para RX 9000. Network style, Tone curve, Black lift y Game exposure aparecen en gris con él |
| `v0.4.0-Runtime.zip` | Alpha0.3.4.1 (y Alpha0.3.4) | 0.4.0, para RX 9000; los ajustes del runtime de danielblnc funcionan con él |
| `v0.3.3-Runtime.zip` | [Alpha0.3.4](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/tag/Alpha0.3.4) | 0.3.3, para RX 7000; los ajustes del runtime de danielblnc funcionan con él |
| `Runtime.zip` | Alpha0.3.4 | 0.3.1; los ajustes del runtime de danielblnc aparecen en gris con él |

Cada uno contiene:

| Archivo | Qué es |
|---|---|
| `dlssnr_amd_pass1..3.dll` | El runtime neural AMD, sin modificar. Tres copias para que el multipase tenga una por pase. |
| `dlssnr_on_amd_weights.bin` | Los pesos de la red que carga el runtime. |
| `danielblnc_ATTRIBUTION.txt` | El crédito de Daniel Blanco y los términos bajo los que AMDNR distribuye su runtime. |

## El menú (nuevo en 0.3.4)

Pulsa `INSERT`. Todas las pestañas tienen el mismo aspecto: pestañas de texto, una fila de cabecera con
Discord y GitHub (abre esta página), una fila de créditos (el nombre de Daniel Blanco abre su página de GitHub), la línea **Components** (cuántos de
los siete componentes de OptiScaler están activos; púlsala para ver la lista), y un pie con Menu Scale, Save Settings y Close. La ayuda se abre al pasar el ratón por la
etiqueta de un control.

**La pestaña Neural, de arriba abajo:**

- **Enable Neural Rendering** y su tecla (el botón, p. ej. `Home`: púlsalo y luego pulsa otra tecla para
  reasignarla).
- **Neural runtime** (danielblnc / lmxxf, con la versión exacta de tus archivos, p. ej. `lmxxf 0.3.4`) con una palabra de estado: running, restart the game to switch, not
  installed, not for this GPU o stopped. Debajo, el crédito del runtime activo y una línea de estado, p. ej.
  `Running - 1920x1080 at 100% - NR 62/s - model 62/s - 15.3 ms` (el último número es el coste de NR), y una fila **Live** cerrada con
  más detalle. Cuando
  algo requiere tu atención sigue una línea naranja, con un botón cuando hay arreglo (Retry lmxxf, Switch to
  danielblnc, Open Upscaling). En el estado por defecto no hay ninguna.
- **Preset**: Quality / Balanced / Performance ponen NR resolution al 100 / 85 / 70% y apagan Dynamic NR; nada
  más. En APU de consolas portátiles hay un cuarto botón, **Handheld** (ver "APU de consolas portátiles"). **NR
  style**, y **Style slots** (Store / Apply / Clear).
- **Performance**: NR resolution (%) con su coste, Neural passes, Full network, Fast mode, Dynamic NR resolution, Model
  interleave (Interleave preset y la línea de ritmo aparecen debajo mientras está activado).
- **Quality**: Residual strength, Residual limit, Temporal stability, Sharpening (CAS), y **More quality
  options** (Network history - una sola casilla para ambos runtimes -, Output smoothing, Stability mode,
  Residual temporal, Residual edge fade, Still-surface steadiness).
- **Image look**: Colour composition, Detail y Colour strength, y tres desplegables: **Model strength** (Tone y
  Structure intensity, Character structure, Edit detail / colour, Edge guard, Native character mask, y Network
  style, Tone curve y Black lift de danielblnc), **Exposure and highlights** (Auto-exposure, su tope de altas
  luces, Highlight colour guard, Game exposure) y **Appearance filter** (con su palabra off / on tras el nombre).
  Un "default" o "custom" tenue tras el nombre de un desplegable indica si cambiaste algo dentro.
- **Ray Regeneration**: su propia sección, visible solo mientras el juego usa FSR Ray Regeneration.
- **La fila de herramientas**, cerrada al inicio: **Diagnostics** (Network output, Debug view, la vista de
  depuración de RR, Edit shaper A/B, NR cost, las lecturas de ghosting y de autoajuste, la línea GPU, **Save report**), **Runtime
  options** (Encoding, Every-frame NR, NR slots, Highlight proxy) y **Experimental** (AMDNR Screen-space GI, en preview).

Un control que el runtime activo no tiene aparece en gris con una etiqueta corta (p. ej. "not in lmxxf yet") u
oculto con un recuento ("3 danielblnc-only options hidden"); cambiar de runtime no mueve ninguna otra fila.

**Las otras pestañas:** Upscaling empieza con el upscaler, una línea de estado y Render resolution (los antiguos
Upscale Ratio Override y Output Scaling); en una tarjeta que no es NVIDIA ya no aparece "DLSS w/Dx12". Image
contiene Sharpness, Textures, Init Flags y el Magnifier. Frame Gen empieza con FG Input y FG Output. Interface
tiene el overlay de FPS y Keybinds (un botón por tecla). Advanced empieza con Active Quirks, luego Display (V-Sync),
Compatibility y Logging. Los ajustes, las claves y lo que escribe Save Settings no cambian, salvo donde
`CHANGELOG.md` lo indica.

## Ajustes que conviene conocer

Abre la pestaña **Neural**. Los valores por defecto son la configuración probada más reciente, así
que el primer movimiento útil es cambiar una cosa a la vez.

- **NR resolution** — la palanca principal de calidad/coste. Por debajo del 100% el modelo trabaja
  sobre una imagen más pequeña y solo su *corrección* se lleva de vuelta al fotograma a resolución
  completa, así que el fotograma conserva su propio detalle. Por encima del 100% el coste crece con
  el cuadrado (150% es 2.25x). El control se mueve en pasos del 5%: cada tamaño de NR nuevo puede
  retener VRAM hasta que reinicies el juego, así que reinícialo tras muchos cambios.
  El coste junto a él marca 1.00x al 100%; en lmxxf es el precio del nivel de tamaño de red en el que corre (su
  tooltip nombra el nivel). Los botones Preset lo ponen al 100 / 85 / 70%.
- **Residual strength** — cuánto de la edición del modelo se aplica; por encima de 1 amplifica. Es
  el control que más cambia la imagen.
- **Residual limit** — un techo a cuánto puede moverse un píxel. ¿Manchas por zonas? **Bájalo**.
- **Model interleave** — ejecuta el modelo cada dos fotogramas para una gran ganancia de
  fotogramas. Los fotogramas omitidos los rellena el **Interleave preset**; *Edit accumulation*
  (preset 10, en ambos runtimes) es el predeterminado: cada fotograma es su propia imagen más la
  corrección que arrastra el modelo, así que no se conserva ninguna imagen anterior. *Guided fill v2*
  (preset 6, danielblnc) y *Classic carry* (lmxxf) son los rellenos anteriores. El ritmo de los dos
  tipos de fotograma es automático en danielblnc y está apagado en lmxxf (`[DlssNr] AmdInterleavePacing` de
  0 a 1 marca el ritmo en ambos, a costa de fotogramas); una línea tenue bajo el preset muestra la medición.
  Adaptive interleave está desactivado en esta build.
- **Neural passes** — 2 y 3 apilan el modelo, con rendimientos decrecientes. Bajo lmxxf, la
  historia de la red sigue siendo su primer pase; los pases extra son solo refinamiento espacial.
  danielblnc ejecuta 1 pase en los títulos Vulkan (un aviso bajo el control lo indica).
- **Colour composition** (Neural > Image look, en ambos runtimes) — *Classic* (predeterminado) es
  la imagen que ya tenías. *RenoDX (experimental)* ejecuta la composición de color de RenoDX después
  del modelo, como hace la ruta NVIDIA: Composition detail y colour, un **Highlight guard** en ambos
  sentidos (2x por defecto) que acota la respuesta del modelo frente al original, y controles
  opcionales de piel / entorno. En un fotograma display-referred (SDR), con Network output o con Encoding
  sRGB / Gamma 2.2 vuelve a Classic en ambos runtimes; el aviso del menú ofrece entonces un botón que quita el
  bloqueo. Los estilos y presets de NR no lo tocan.
- **Native character mask** (Image look > Model strength, `[DlssNr] AutoMask`, activado por defecto) — el
  tratamiento propio del modelo para caras y piel. Desmarcarlo ahora actúa en ambos runtimes (en lmxxf
  reconstruye la red: una pausa de alrededor de 1 s); en lmxxf, Structure intensity y Character
  structure ahora también actúan.
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
- **FSR Ray Regeneration** — RX 9000 y RX 7000 (RDNA 4 y RDNA 3) por defecto; solo en juegos que usan DLSS Ray Reconstruction (Cyberpunk 2077,
  Alan Wake 2), con el juego ejecutando DLSS (spoofing activado), trazado de rayos y Ray
  Reconstruction activados en sus propios ajustes. Neural Rendering se ejecuta entonces después,
  sobre su salida, lo que cuesta más: baja la NR resolution si caen los fotogramas. Sus controles
  tienen su propia sección, **Neural > Ray Regeneration**, visible solo mientras el juego usa Ray
  Reconstruction. El **perfil path-traced** (menos grano en las caras con path tracing) es opcional desde
  0.3.3.1: márcalo ahí para probarlo en Resident Evil Requiem o PRAGMATA. La misma sección tiene la
  intensidad de la bias mask y el **suavizado de piel** (experimental, para juegos que publican una guía
  SSS; desactivado por defecto, pero activado por defecto en Resident Evil Requiem desde 0.3.3.2); los
  controles de ajuste temporal están en *More Ray Regeneration options*, y la vista de depuración de RR en
  Diagnostics. En RX 7000 (RDNA 3) FSR Ray Regeneration vuelve a ofrecerse por defecto (0.3.3.2 lo ofrecía solo en
  RDNA 4). AMD lo publica solo para RDNA 4: si el controlador lo rechaza, el juego recibe FSR sin el eliminador de
  ruido. `[FSR-RR] FfxDenoiserAllowPreRdna4=false` lo limita a RDNA 4; RX 6000 y anteriores lo reciben solo con `true`
  (pestaña Upscaling: **Offer FSR Ray Regeneration on this GPU (restart)**). **Enfoque después de RR** (0.3.4.1):
  cuando el juego no envía ningún valor de nitidez, AMDNR aplica un enfoque de 0.25 después de RR en Windows (0 en
  Linux / Proton); para desactivarlo: Image > Sharpness, marca Override, control deslizante a 0.
- **AMDNR Screen GI** (preview, nuevo en 0.3.4, desactivado por defecto; Neural > Experimental, o `[AmdGi] Enabled=true`) — la luz rebotada y la oclusión ambiental en espacio de pantalla propias de AMDNR, a partir de la profundidad del juego, antes de NR y del upscaler; funciona con NR activado o desactivado; alrededor de 1 ms en High con un render de 1080p en una RX 9070 XT (medido fuera de un juego). Es espacio de pantalla: falta la luz que viene de fuera de la pantalla. Ver `CHANGELOG.md`.
- **Save report** (Neural > Diagnostics, o Advanced > Logging) — un zip con todos los logs y los archivos ini para un reporte; ver "Si
  no funciona" más arriba.

## Si algo sale mal

`OptiScaler.log` aparece en la carpeta del juego. Adjúntalo en `#bug-report`, y di qué juego y qué
GPU; **Save report** (Neural > Diagnostics, o Advanced > Logging) lo comprime junto con todo lo demás. El backend AMD también
escribe `amd_presr.log` y `amd_bridge.log`, que son los útiles cuando lo que falla es específicamente el pase
neural. Los logs de las tres últimas sesiones se conservan como `OptiScaler.previous.<exe>.log` (el más
reciente), `OptiScaler.previous-1.<exe>.log` y `OptiScaler.previous-2.<exe>.log` (`[Log] KeepPreviousLogs`; 1
conserva solo uno, como antes). Tras un cierre inesperado, adjúntalos también: el log nuevo dice entonces "no
clean exit recorded" (desde 0.3.4 ya no tras una salida normal).

**¿NR frames 0/s, y la pestaña Neural o `amd_presr.log` dicen que la DLL del pase es una build que este
AMDNR no maneja?** Tus `dlssnr_amd_pass1..3.dll` son una build de danielblnc que este AMDNR no conoce (se ha
visto circular un conjunto 0.2.16), o falta una de las tres. Desde 0.3.3.2 la pestaña Neural nombra el
archivo y su versión y dice qué hacer. Usa el runtime de tu GPU, con las tres DLL de pase del mismo zip: en
**RX 9000**, `v0.4.1-Runtime.zip` (el más nuevo) o `v0.4.0-Runtime.zip` de esta release (la `dlssnr_amd_pass1.dll`
de `v0.4.1-Runtime.zip` tiene 9,916,928 bytes y su SHA256 empieza por `823063eb`; en `v0.4.0-Runtime.zip`:
10,027,008 bytes, `d62be3d8`); en **RX 7000**, `v0.3.3-Runtime.zip` de la
[release Alpha0.3.4](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/tag/Alpha0.3.4) (su
`dlssnr_amd_pass1.dll`: 7,607,296 bytes, con un SHA256 que empieza por `907b30a6`). Builds
soportadas: 0.2.17, 0.3.0, 0.3.1, 0.3.2, 0.3.3, 0.4.0, 0.4.1, y 0.4.x antes de su publicación. No instales el instalador propio de danielblnc ni su
`dxgi.dll` / `version.dll` / `winhttp.dll` junto a AMDNR: AMDNR ya ejecuta su runtime.

**¿lmxxf no hace nada, o se detiene enseguida, en un PC con gráficos integrados?** Corregido en 0.3.3.2. En
un Ryzen de escritorio con los gráficos integrados activados, un portátil con APU AMD y una Radeon, o un PC
con dos GPU AMD, la GPU del juego a menudo no es el dispositivo HIP 0. lmxxf fallaba entonces en su primer
fotograma (`hipErrorInvalidHandle (400)`, y luego "session is poisoned" en `lmxxf_backend.log`) y se quedaba
apagado. Reemplaza tanto `OptiScaler.dll` (el archivo que renombraste, p. ej. `dxgi.dll`) como
`LmxxfNrRuntime.dll` por los de 0.3.3.2 o posteriores. Aún sin probar en un PC así: si lmxxf sigue deteniéndose, la
pestaña Neural ahora dice por qué; envía `lmxxf_backend.log` y `amd_bridge.log` (lista los dispositivos HIP).

**¿La línea de estado de lmxxf dice `c32w=off:nofile` en una RX 9070 / 9070 XT?** Una carpeta antigua
`DLSS5-AMD\native-game-tiled-assets` junto al `.exe` del juego (de una instalación anterior de lmxxf) se usa en
lugar de `LmxxfNrRuntime.pak`. No tiene los kernels c32w, así que lmxxf va a la velocidad de antes. Borra o
renombra la carpeta `DLSS5-AMD`: el pak contiene todo lo que lmxxf necesita. Un `LmxxfNrRuntime.pak` anterior a
0.3.3.2 da el mismo estado; sustitúyelo por el de esta versión.
`fk=fff-` en la misma línea significa lo mismo (un pak antiguo o una carpeta suelta): lmxxf sigue
funcionando, a la velocidad de antes.

**danielblnc: ¿el estilo de NR sigue cambiando cuando la NR resolution deja el 100%?** Sigue abierto en 0.3.4 y
0.3.4.1, y el valor por defecto no cambia. Al 100%, Residual strength 0.99 da el 99% de 1.00 (corregido en 0.3.3.2); fuera
del 100% (también en los pasos de Dynamic NR y los presets Balanced / Performance), strength, limit y edge fade
siguen actuando sobre el resultado completo, así que el aspecto puede cambiar. 0.3.4 añade un A/B para encontrar
la corrección adecuada: Neural > Diagnostics > **Edit shaper (A/B, not saved)** con Literal, F1 y F2, más Only
below 100% y Carry cap (solo danielblnc; Save Settings no lo guarda; las claves del ini son
`[DlssNr] AmdEditShaper`, `AmdEditShaperLimit`, `AmdEditShaperScope` y `AmdEditShaperCarryCap`). Si uno de ellos hace que
el 85% se vea como el 100% en tu juego, cuéntanoslo en Discord con capturas. lmxxf no está afectado.

**¿El menú se abría y cerraba dos veces por pulsación, o el teclado y el ratón dejaban de funcionar en todo el
escritorio con el menú abierto (Assetto Corsa)?** Corregido en 0.3.4: una segunda pulsación de la tecla del menú
o de NR en menos de 400 ms se ignora (`[Hotfix] MenuToggleDebounceMs`, 0 = el comportamiento anterior), y con el
menú abierto se salta el hook de teclado o ratón de bajo nivel del juego, pero la tecla sigue llegando a Windows
(`[Hotfix] MenuLowLevelHookPassThrough=false` = el comportamiento anterior). Aún sin confirmar en Assetto Corsa:
si sigue ocurriendo, envía el zip del reporte. Un reporte más reciente, el ratón que no funciona en el menú en
Assetto Corsa, aún se está investigando; no está corregido en 0.3.4.1.

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
this title"?** El juego no publica lo que FSR Ray Regeneration necesita: su plugin DLSS pasa matrices de cámara
vacías (Satisfactory), que el Ray Reconstruction de NVIDIA trata como opcionales y FSR Ray Regeneration necesita.
El escalado FSR corre en su lugar y NR toma su posición habitual antes del SR; la pestaña Upscaling dice lo
mismo. Desde 0.3.4 se queda apagado toda la sesión en un título Unreal con esta firma. Desactiva Ray
Reconstruction en el juego y restaura los ajustes de denoiser propios del motor.

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

- **0.3.4.1** (esta build) — hotfix: enfoque de Ray Regeneration cuando el juego no envía ningún valor de nitidez
  (Windows; ninguno por defecto en Linux / Proton), la falsa ventana emergente "Upscaler failed to run!" de Control
  Resonant, el menú de Linux / Proton se vincula a la ventana del juego, Save report nombra vkd3d-proton / DXVK;
  AMDNR Launcher 0.3.4.1 (nueve idiomas, búsqueda, favoritos, ocultar, renombrar, CHOOSE GAME .EXE, PLAY, un
  UNINSTALL completo); NR sin cambios.
- **0.3.4** — el menú nuevo (la pestaña Neural rehecha, el mismo aspecto en todas las pestañas,
  Save report); lmxxf más rápido en RX 7000 (el nivel de tamaño de red por defecto) y en RX 9070 /
  9070 XT (kernels de lmxxf 0.31); lmxxf en APU de consolas portátiles (experimental; nuevos tamaños
  de red 360p y 576p); lmxxf gana Network output, Encoding, Residual edge fade, la máscara de personajes nativa y un Fast mode opcional; AMDNR Screen GI (preview); ajustes del runtime de danielblnc (Network style, Tone curve, Black lift, Game exposure) y
  un guardián del color de las altas luces; ajuste y diagnósticos de Ray Regeneration; correcciones de la entrada del menú en Assetto Corsa, de Shadow of the Tomb Raider, Marvel's Midnight Suns y The
  Last of Us Part II, de la salida limpia y de los logs.
- **0.3.3.x** — lmxxf en RDNA 3 (RX 7000; backend propio de AMDNR); composición de color
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
- **0.4.0** — RX 6000
  (RDNA 2), y soporte para títulos sin upscaler propio (clase Stray), donde OptiScaler aporta el upscaler y el
  pase neural juntos.

---

## Créditos

Esta build es un trabajo de cableado sobre el trabajo de otras personas. Si te resulta útil, el
agradecimiento pertenece a upstream.

- **TheAutomatic** — DLSS 5 AMD project — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD by Daniel Blanco — https://github.com/danielblnc/DLSS-NR-on-AMD (los archivos `*Runtime.zip`, sin modificar)
- **lmxxf** (Kien) — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (el port de la red, los kernels y el runtime HIP, MIT)
- **TheAutomatic** — `LmxxfNrRuntime.cpp`, `LmxxfNrApi.h`, `LmxxfProductionOptions.h`: portions contributed to lmxxf by TheAutomatic (MIT)
- **kernels de lmxxf 0.31** en `LmxxfNrRuntime.pak` (the ViT projection (lmxxf031-vit-wide-deep), the C512 QKV and mix kernels (lmxxf031-c512-m32-mh, lmxxf031-c512-m32-deep) and one-wave-per-head attention (lmxxf031-c64-wave2)) — de lmxxf (Kien, MIT), compilados por AMDNR a partir de las fuentes y la receta de compilación de lmxxf; la parte de AMDNR es la carga, los pines SHA-256, el filtrado por GPU y las alternativas
- **kernels c32w** (0.3.3.2) — kernels RDNA 4 de una wave propios de AMDNR para la red de lmxxf, Copyright (c) 2026 3zwr1 (AMDNR); ideas de la documentación pública de WMMA de RDNA 4 de AMD (GPUOpen, ROCm matrix instruction calculator)
- **El backend RDNA 3 de AMDNR** (0.3.3; las builds para portátiles gfx1103 / gfx1150 en 0.3.4), la política de niveles de tamaño de red y los tamaños de red pequeños (0.3.4) — Copyright (c) 2026 3zwr1 (AMDNR)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Dagherbou / OptiScaler_DLSSNR** — https://github.com/Dagherbou/OptiScaler_DLSSNR
- **wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass** — https://github.com/wilsjo2/OptiScaler-DLSSNR-PreSR-Multipass
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, sin modificar)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (matemática de la composición de color, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), la base del desbloqueo multifotograma de XeFG integrado y de su ritmo
- **burak113** — el preprocesador de FSR Ray Regeneration (rama de OptiScaler ffx-denoise-experimental, GPL-3.0)
- **Screen-space GI** (el efecto heredado; retirado del menú en 0.3.4, `[AmdRtgi] Enabled` en el ini) — un efecto que AMDNR heredó del linaje OptiScaler-AMD-PreSR; el mérito es de sus autores originales. Necesita la carpeta `experimental_lighting` del paquete de danielblnc, que AMDNR no distribuye.
- **AMDNR Screen GI** (preview de 0.3.4) — obra propia de AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), escrita a partir de artículos publicados (Therrien, Levesque y Gilet 2023; Jimenez et al. 2016; Schied et al. 2017; y los demás que se citan en `CHANGELOG.md` y `Licenses/AMDNR_NOTICE.txt`)
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## AMDNR Launcher

**AMDNR Launcher** (nuevo en 0.3.4) es un programa de Windows 10 / 11 que también puede funcionar bajo Proton en
Linux (experimental, aún no lo hemos probado: ver "Linux / Proton"). Instala y actualiza AMDNR juego por juego:
encuentra tus juegos (Steam, Epic, la app de Xbox, Ubisoft Connect, la app de EA, GOG, Rockstar, Battle.net y Amazon
Games), elige el nombre de la DLL, descarga la build y el runtime de danielblnc que elijas, comprueba cada instalación
con su Doctor y se actualiza a sí mismo. Descarga `AMDNR-Launcher.exe` (0.3.4.1) de la release Alpha0.3.4.1:
<https://github.com/3zwr1/AMD-NR---OptiScaler/releases/download/Alpha0.3.4.1/AMDNR-Launcher.exe>

**Novedades del Launcher 0.3.4.1: lo pediste, lo hicimos** (a partir de los primeros comentarios en Discord):

- Nueve idiomas: inglés, árabe, chino (simplificado), francés, español, portugués, italiano, ruso y polaco. El
  launcher sigue el idioma de tu Windows (si no es uno de estos, inglés); elige otro en LANGUAGE (IDIOMA) o en
  SETTINGS (AJUSTES), donde también se ofrece la primera vez que lo abres. Los resultados del Doctor, los mensajes
  de instalación y el reporte de COLLECT LOGS (RECOPILAR REGISTROS) se quedan en inglés para que el soporte pueda
  leerlos.
- Busca en la biblioteca; favoritos (una estrella, y los juegos marcados primero); oculta juegos (HIDDEN los vuelve
  a mostrar); renombra un juego.
- CHOOSE GAME .EXE: elige tú el exe del juego cuando el launcher eligió uno equivocado o ninguno. Cyberpunk 2077 y
  The Witcher 3 (REDengine) ahora se encuentran en la carpeta correcta sin necesidad de ello.
- PLAY y OPEN FOLDER en la página de cada juego; STORES activa y desactiva tiendas enteras; las carpetas que añades a
  mano siguen en la lista, también mientras su unidad está desconectada, hasta que las quitas.
- UNINSTALL pregunta primero y quita todo lo que colocó el mod, incluido lo que escribió mientras el juego corría
  (logs, cachés, volcados de cierre, reportes sin terminar); conserva los zips de Save report terminados y una DLL
  con el nombre del proxy que ya no es un OptiScaler (la del propio juego).
- COLLECT LOGS en cualquier juego, instalado o no, con un informe del escaneo.
- Las consolas portátiles y las APU (ROG Ally Z1 Extreme y otras APU Ryzen) se reconocen, con una nota de que AMDNR
  en ellas aún está en pruebas.
- Linux (experimental, aún no lo hemos probado): el mismo exe de Windows puede funcionar bajo Proton, muestra ahí un
  aviso con lo que hay que hacer y también busca juegos en tu biblioteca de Steam de Linux; los pasos están en
  "Linux / Proton". Cuéntanos en Discord si funciona.

Su código fuente está en `Launcher/OpenSource/` del repositorio de GitHub de este proyecto, con su propia licencia,
`Launcher/OpenSource/LICENSE.txt`. **No** está cubierto por la licencia GPL-3.0 (`LICENSE`) de este repositorio: es
código fuente disponible (source-available), todos los derechos reservados, Copyright (c) 2026 3zwr1 (AMDNR). El
manifiesto del launcher es `Launcher/manifest.json`. Consulta también la sección 7 de `Licenses/AMDNR_NOTICE.txt`.

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
