> Actualización local: selector de calidad NR compartido con Friendly use; Handheld solo en dispositivos compatibles. Guías de profundidad/movimiento corregidas para mochi A/B y contador de salidas NR reales. Classic carry desactivado por defecto para lmxxf Model A y mochi A/B; sigue disponible en Neural > Quality. Historial de red independiente. Transporte de color corregido y shaders integrados al compilar. No se garantiza la eliminación de todos los artefactos.

# AMDNR 0.3.5.8

Candidato local completo; aún no publicado. Launcher 0.3.5.36.

Menú más claro, modo Friendly use con varias pasadas donde se admiten y restablecimiento con copia de seguridad al reiniciar. Descargas automáticas, reparación de archivos de Daniel, limpieza de cachés antiguas y mejor selección del proxy. Incluye la corrección del formato de movimiento de RE3 y correcciones específicas de memoria y cierres. Persisten problemas en algunos juegos; consulte KNOWN-LIMITS.md. El manual anterior se conserva abajo como referencia.

[CHANGELOG](CHANGELOG.md) · [Limits](KNOWN-LIMITS.md)


---

## Earlier manual (historical reference)

# AMDNR — DLSS 5 Neural Rendering en AMD (build de OptiScaler) — v0.3.5.7 Public View

[English](README.md) | [中文](README.zh-CN.md) | [Português](README.pt-BR.md) | **Español** | [العربية](README.ar.md) | [Français](README.fr.md) | [Italiano](README.it.md) | [Русский](README.ru.md) | [Polski](README.pl.md)

> **Necesitamos tu apoyo.** Únete al servidor de Discord — <https://discord.gg/AMDNR> — para
> ayuda, reportes de errores y builds de prueba; cada reporte con un log hace mejor la siguiente build.

DLSS 5 Neural Rendering funcionando en GPUs AMD, integrado en OptiScaler para que funcione en
cualquier juego Direct3D 12 que OptiScaler ya engancha. Sobre el pase neural: model interleave para
una gran ganancia de fotogramas, composición residual, generación de fotogramas XeSS desbloqueada
hasta 6X (hasta 10X opcional en juegos D3D12), y FSR Ray Regeneration para los juegos que usan DLSS
Ray Reconstruction. Desde 0.3.5, **AMDNR Anywhere** (preview) lleva Neural Rendering a los juegos que no tienen
ningún upscaler propio, a través del AMDNR Launcher, sin escribir nada en la carpeta del juego (ver "AMDNR Anywhere").
Desde 0.3.5.6, la lista de upscalers ofrece además dos opciones experimentales y opcionales hechas por otros autores,
**Model E** (HelixSR) y **Model L** (d4r), que el AMDNR Launcher descarga de sus propias releases (ver "Model E y
Model L").

**Discord: <https://discord.gg/AMDNR>** — soporte, reportes de errores (`#bug-report`), builds
de prueba.

**Apoya el proyecto: <https://ko-fi.com/3zinr>**

> **El runtime de danielblnc es obra de Daniel Blanco.** El runtime neural AMD de los archivos `*Runtime.zip`
> (`dlssnr_amd_pass1..3.dll`) es **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> AMDNR lo distribuye sin modificar, con su permiso; no es obra de AMDNR. Por favor, apoya su proyecto.
> Los créditos completos de todos los demás están al final de esta página.

## Novedades de 0.3.5.7 Public View

AMDNR Launcher **0.3.5.35** utiliza el canal público.
- Las actualizaciones del launcher se instalan en la pantalla del logotipo antes de abrir la interfaz, con progreso y recuperación de errores.
- Galería con imágenes más grandes y la configuración asociada a la imagen seleccionada, que se puede copiar; se indica cuando faltan los metadatos.
- Se recuerdan tamaño, posición y estado maximizado de la ventana, ajustándolos al área visible, incluso a 720p.
- Guide pasa a llamarse **Overlay Guide**, con aviso de desarrollo. FAQ permanece en **More**.
- Los mensajes del chat público caducan 48 horas después de su creación original en UTC, con control en servidor y cliente.
- Decodificación asíncrona de imágenes, prevención de solicitudes duplicadas de la Galería y guardado atómico de ajustes.

**Estado de los runtimes:** El paquete incluye la integración de lmxxf 0.4.4 de AMDNR para Model A/B; mochi 0.0.7 se ofrece como componente separado del launcher. Se probaron los cambios de modelo, el reinicio del historial temporal y los cambios de resolución en pruebas controladas. Esto no demuestra la solución de los problemas de todos los juegos. Mochi para RDNA 3 sigue aplazado a 0.3.5.9. Consulta CHANGELOG.md.

> **Novedades de 0.3.5.6** (2026-10-17, release Alpha0.3.5.6, con el AMDNR Launcher 0.3.5.25):
> - **Dos opciones nuevas de upscaler, experimentales y opcionales:** **Model E** (HelixSR de lonewolf0622) y **Model L**
>   (d4r de countervolts), que el launcher descarga de las propias releases de sus autores cuando los eliges. AMDNR no
>   aloja ni distribuye sus archivos ni los de NVIDIA. Model L es solo para RX 9070 / RX 9070 XT y funciona con el
>   runtime lmxxf o con Neural Rendering desactivado; el runtime de danielblnc va con Model E. Lee "Model E y Model L"
>   antes de activarlos.
> - **Full Model Look es el estilo de NR por defecto:** un aspecto más completo en ambos runtimes, ejecutado después del
>   escalado en las tarjetas que pueden permitírselo — el control de coste y la protección TDR lo devuelven a antes del
>   escalado donde hace falta, y lo dicen. Elegir un aspecto en NR style también fija su Neural placement y Full
>   network, y el menú te avisa cuando tu propio ajuste de placement frena el aspecto.
> - **Más detalle en las caras**, siempre activado, en ambos runtimes (`[DlssNr] AmdFaceDetail=false` lo desactiva).
> - **Menos ghosting con lmxxf:** los rastros detrás de los objetos en movimiento son más cortos, antes y después del
>   escalado, y el destello tras un corte de escena es aproximadamente la mitad de intenso. El ghosting se reduce, no
>   desaparece.
> - **RX 7000 y consolas portátiles:** Neural Rendering ahora corre por defecto un tamaño de red más pequeño (Fast mode
>   de lmxxf), lo que cuesta menos tiempo por fotograma. `[DlssNr] AmdLmxxfFastMode=false` lo desactiva; RX 9000 lo
>   mantiene desactivado.
> - **Controles nuevos y renombrados:** Game exposure (Auto / On / Off), Anti-halo (antes Halo protection) y, en
>   danielblnc, **Refine passes** (los pases 2 y 3 corren un tamaño de red más pequeño). **NR resolution** ahora llega
>   hasta el 200% en ambos runtimes.
> - **El menú muestra solo lo que tu runtime puede usar:** un control que un runtime no puede ejecutar se oculta en ese
>   runtime (su clave del ini sigue funcionando), y la entrada del runtime nombra la versión de lmxxf que de verdad se
>   ejecuta (p. ej. `lmxxf 0.41`).
> - **Correcciones:** los falsos "timeouts" de danielblnc (probado en Silent Hill 2) — una espera que se rinde ya no
>   apaga Neural Rendering, ha desaparecido el texto que afirmaba que el controlador había reiniciado la GPU (no hubo
>   ningún reinicio), y la nota "runtime failed" solo se escribe tras un fallo real. FSR 3 / FSR 4 ya no deberían volver
>   a FSR 2.1.2 cuando otra opción de upscaler falla o no está lista: se conserva tu propia elección de FSR. La memoria
>   de la generación de fotogramas XeFG se cuenta correctamente (incluido el ajuste 6X).
> - **Reemplaza todos los archivos del zip a la vez** (`OptiScaler.dll` y `LmxxfNrRuntime.dll` deben venir de la misma
>   release); usuarios del launcher: se actualiza solo. Lo que sigue abierto: "Límites conocidos (0.3.5.6)". Detalles:
>   `CHANGELOG.md`.

> **Entre 0.3.5 y 0.3.5.6** (de 0.3.5.1 a 0.3.5.5R4): AMDNR ya maneja danielblnc 0.6.0, que ahora es el runtime de
> danielblnc recomendado (también funciona en RX 6000, con el SDK de HIP de AMD instalado); lmxxf 0.41 de Kien (MIT) es
> el conjunto por defecto en RX 9000; el NVIDIA look (el estilo de NR por defecto de 0.3.5.3 a 0.3.5.5R4); una
> protección contra congelaciones que baja o detiene NR tras un fallo de GPU contabilizado; el AMDNR Launcher rediseñado
> (trece idiomas, ASK AI); y muchas correcciones de cierres a partir de tus reportes. Detalles: `CHANGELOG.md` y las
> notas de la release en GitHub.

> **Novedades de 0.3.5:** **AMDNR Anywhere** (preview): Neural Rendering para los juegos que no tienen DLSS, XeSS ni
> FSR 2 propios — un solo botón **PLAY ANYWHERE** en el AMDNR Launcher, sin escribir nada en la carpeta del juego;
> RX 9000 (RDNA 4) en esta release (ver "AMDNR Anywhere"). **Ray Regeneration tiene su propia pestaña**, justo
> después de Upscaling, y sus líneas de estado dicen, por tarjeta y por API, qué se ejecuta y por qué no; en RX 7000, desde 0.3.5.1, se ofrece
> de nuevo como preview, activa por defecto (desde 0.3.5.3 el AMDNR Ray Denoiser (preview) elimina ahí el ruido; la casilla de la pestaña Upscaling o `[FSR-RR] FfxDenoiserAllowPreRdna4=false`
> la apaga y el juego conserva su propio eliminador de ruido). **El pase
> neural puede ejecutarse después del escalado** (`[DlssNr] AmdPlacement=post`, Neural > Performance > Neural
> placement; el valor por defecto `pre` no cambió en 0.3.5 — desde 0.3.5.6 el estilo de NR por defecto pide `post`,
> ver "Ajustes que conviene conocer"). **La pestaña Frame Gen dice por qué no se genera nada** y nombra los cinco
> pasos (ver la entrada del FAQ sobre la generación de fotogramas). **Más rápido en RX 7000 y en las consolas
> portátiles de la clase Z1 Extreme:** alrededor de un 10% menos de tiempo de red, la misma imagen bit a bit (el
> conjunto de módulos de 0.3.5 en `LmxxfNrRuntime.pak`). **lmxxf 0.37 de Kien (MIT) viene activado por defecto en
> RX 9000:** alrededor de un 20% menos de tiempo de red en una RX 9070 XT (experimental en RX 9060 / 9060 XT;
> `[DlssNr] AmdLmxxfL37=false` lo desactiva; desde 0.3.5.3 el conjunto por defecto ahí es lmxxf 0.41). En las
> consolas portátiles RDNA 3, **FSR 4 (INT8)** es una opción
> experimental que se activa a mano, y las ranuras de estilo personalizadas ahora guardan el aspecto completo. Los
> juegos con resolución
> dinámica ya no reconstruyen la red en cada paso, la edición transportada ya no desaparece al moverte en una
> portátil con Model interleave, Uncharted: Legacy of Thieves ya no se cierra en RX 9000, y muchas correcciones más.
> **Cambian los tres archivos: reemplaza `OptiScaler.dll`, `LmxxfNrRuntime.dll` y `LmxxfNrRuntime.pak` a la vez;
> usuarios del launcher: se actualiza solo.** Detalles: `CHANGELOG.md`.

> **Novedades de 0.3.4.2 (hotfix):** el menú en Assetto Corsa: la tecla del menú lo abre o cierra una sola vez por
> pulsación, los clics más cortos que un fotograma ya no se pierden, y el selector de runtime responde a `1` / `2` /
> `Enter` / `Esc`, tiene una X en su barra de título y cerrar el menú cuenta como "Decide later". El selector de
> runtime ya no abre el menú por sí solo (un aviso en su lugar), la sección **Ray Regeneration** de la pestaña Neural
> ya no se esconde — siempre está ahí y una línea atenuada dice por qué no se está ejecutando — y el texto de Wine /
> Proton dice que Ray Regeneration es un problema conocido allí. AMDNR también **acepta un layout de runtime de
> danielblnc más**, así que una build más nueva de danielblnc puede funcionar aquí sin ninguna actualización de
> AMDNR. **Neural Rendering es idéntico byte a byte a 0.3.4.1 salvo esa única fila de layout
> aceptado** (el pase neural, ambos runtimes
> y el pak no cambian): viniendo de 0.3.4.1 o 0.3.4, reemplaza solo `OptiScaler.dll`; usuarios del launcher: se
> actualiza solo. Detalles: `CHANGELOG.md`.

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
0.3.5.6 es la etiqueta Alpha0.3.5.6):

* `AMDNR-vX.X.X.zip` (para 0.3.5.6: `AMDNR-v0.3.5.6.zip`), con el runtime lmxxf completo.
* Para el runtime de danielblnc, un zip de runtime: `v0.6.0-Runtime.zip` (recomendado; RX 9000, RX 7000 y, con el SDK
  de HIP de AMD instalado, RX 6000) de Alpha0.3.5.5R4; `v0.5.1-Runtime.zip` (Alpha0.3.5), `v0.5.0-Runtime.zip`
  (Alpha0.3.4.2; puede congelar algunos juegos en algunas tarjetas), `v0.4.3-Runtime.zip` (Alpha0.3.4.2 y
  Alpha0.3.4.1), `v0.4.1-Runtime.zip` y `v0.4.0-Runtime.zip` (Alpha0.3.4.1) siguen aceptados en RX 9000 y RX 7000. El
  runtime lmxxf va en `AMDNR-vX.X.X.zip` y no necesita ningún zip de runtime en RX 7000 y RX 9000; las APU de consolas
  portátiles usan solo lmxxf. El AMDNR Launcher elige el zip adecuado para tu GPU. Ver "Qué hay en los archivos".
* No hay nada que descargar a mano para Model E ni para Model L: el AMDNR Launcher los descarga de las propias releases
  de sus autores cuando los eliges (ver "Model E y Model L").

### 2. Extrae ambos archivos

Extrae el contenido de los dos archivos `.zip`.

### 3. Copia todo a la carpeta del juego

Primero, copia todos los archivos de `AMDNR-vX.X.X` a la carpeta raíz del juego — la misma carpeta
donde está el `.exe` del juego.

Después, haz lo mismo con todos los archivos del zip de runtime (p. ej. `v0.6.0-Runtime` en RX 9000, RX 7000 y RX 6000).

> **¿Actualizas desde un AMDNR anterior?** Vuelve a copiarlo todo y sobrescribe. **Reemplaza estos tres a la vez:**
> `OptiScaler.dll` (sustituye el archivo que renombraste, p. ej. `dxgi.dll`, por el nuevo renombrado igual),
> `LmxxfNrRuntime.dll` y `LmxxfNrRuntime.pak` (440 MB). No los mezcles con copias anteriores: en 0.3.5.6
> `OptiScaler.dll` le pasa a `LmxxfNrRuntime.dll` datos de fotograma nuevos, y un runtime anterior funciona sin ellos.
> Puedes conservar tu `OptiScaler.ini`: los ajustes nuevos usan sus valores por defecto. Los archivos de tu zip de
> runtime de danielblnc se quedan como están (el Doctor del launcher te ofrece el recomendado para tu tarjeta). El AMDNR
> Launcher lo hace por ti: UPDATE ALL, o REPAIR / UPDATE en un juego que muestre "Update available" (el launcher se
> actualiza primero a sí mismo).

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
* `winhttp.dll`
* `wininet.dll`

Prueba un nombre a la vez. No crees varias copias de `OptiScaler.dll`. Estos son los nombres con los que carga el mod
(más `OptiScaler.asi` con un cargador ASI); `d3d11.dll` no es uno de ellos.

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
fuera de `C:\Users\` no. Adjunta el zip en `#bug-report`. El **COLLECT LOGS** del AMDNR Launcher escribe el mismo zip
para cualquier juego y, desde 0.3.5.1, añade tras un cierre inesperado el log del cierre y el volcado más reciente.

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

Ambos runtimes comparten una sola pestaña Neural (ver "El menú" más abajo). Desde 0.3.5.6, un control que el
runtime activo no puede ejecutar se oculta en ese runtime, con un recuento (su clave del ini sigue funcionando). Solo lmxxf: **Full
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

**Fast mode** (Neural > Performance, `[DlssNr] AmdLmxxfFastMode`, lmxxf) ejecuta la red un nivel de tamaño por
debajo (1080 -> 900, 900 -> 720; 720 -> 576 y 576 -> 360 donde los tamaños pequeños están activados): alrededor de un
29% menos de tiempo de red a 1080p (RX 9070 XT, medido fuera de un juego), con el detalle fino algo más suave. **Desde
0.3.5.6 viene activado por defecto en RX 7000, Radeon 8060S / 8050S y las APU de consolas portátiles** y desactivado
por defecto en RX 9000; `false` (o la casilla) lo desactiva, `true` lo activa, y la casilla muestra lo que se ejecuta.
Las builds de danielblnc que tienen su
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
  0.3.3.2. Desde 0.3.5.2, RX 9000 también se ajusta a un nivel por defecto.
- **RX 9000:** `[DlssNr] AmdLmxxfTierSnap` (activado por defecto ahí desde 0.3.5.2; opcional antes) lleva el tamaño de
  NR de lmxxf a un tamaño de red en cada resolución de render: algunos tamaños bajan un nivel (1440p FSR Quality,
  1707x960 -> el tamaño 900: red 14.08 -> 9.96 ms por ejecución en una RX 9070 XT, medido fuera de un juego, imagen algo
  más suave), otros crecen dentro de su nivel (80% de un render 1080p -> 1600x900: mismo coste, algo más de detalle).
  `false` conserva los tamaños de 0.3.3.2.
- Empieza con NR resolution al 70% o menos (el nivel 720 con un render de 1080p; el preset Performance es
  70%). El coste junto a NR resolution se calcula según el nivel en el que corre la red; su tooltip nombra el
  nivel.
- **0.3.5: alrededor de un 10% menos de tiempo de red en RX 7000**, la misma imagen bit a bit: el conjunto de
  módulos de 0.3.5 en `LmxxfNrRuntime.pak` (medido y verificado por hash por un tester en una RX 7800 XT; la tabla
  de arriba es la de 0.3.4).
- **0.3.5 en RX 9000: lmxxf 0.37 de Kien (MIT) viene activado por defecto** - alrededor de un 20% menos de tiempo
  de red en una RX 9070 XT en el tamaño 1080 (14.0 -> unos 11.3 ms, medido fuera de un juego; la imagen no es
  idéntica bit a bit a la de 0.3.4.2). En RX 9060 / 9060 XT también viene activado, junto con los kernels c32w y
  FastK, como experimento (aún sin probar en esa tarjeta). Para desactivarlos: `[DlssNr] AmdLmxxfL37=false`,
  `AmdLmxxfC32w=false`, `AmdLmxxfFastK=false`.
- **Desde 0.3.5.3 el conjunto por defecto en RX 9000 es lmxxf 0.41 de Kien (MIT)** (`[DlssNr] AmdLmxxfL41=false` lo
  desactiva); desde 0.3.5.6 la entrada Neural runtime nombra el conjunto que de verdad se ejecuta. **Desde 0.3.5.5R4,
  RX 7000 y las consolas portátiles de la clase Z1 Extreme ejecutan conjuntos de kernels más nuevos**, con la misma
  imagen bit a bit (verificada por hash en los equipos de los testers).
- **0.3.5.6: Fast mode viene activado por defecto en RDNA 3** (RX 7000, Radeon 8060S / 8050S y las APU de consolas
  portátiles): la red corre un tamaño por debajo del que usaría el tamaño de render (un render de 1080p corre al tamaño
  900), lo que cuesta menos tiempo por fotograma, con el detalle fino algo más suave. RX 9000 no cambia (desactivado).
  Aún no hay reportes de juegos con estas GPU: por favor, envía el tuyo. `[DlssNr] AmdLmxxfFastMode=false` (o la
  casilla Fast mode) lo desactiva.

### APU de consolas portátiles (experimental, nuevo en 0.3.4)

lmxxf funciona en APU de consolas portátiles con 12 o más unidades de cómputo, mediante el backend RDNA 3 de
AMDNR por 3zwr1: **Z1 Extreme, Z2 y Radeon 780M** (gfx1103), **Z2 Extreme, Radeon 890M y 880M** (gfx1150). Es
experimental y lento. La fila Neural runtime dice "experimental" tras el crédito
RDNA 3.
Primeros resultados de un tester (ROG Ally, Z1 Extreme): la sonda de lmxxf fuera de un juego, 54.7 ms por pasada de
la red al tamaño 360p, 110.9 ms a 576p; en un juego, la prueba de un tester (Shadow of the Tomb Raider, 1280x720 con XeSS, preset Handheld),
62 ms por pasada de la red de media a 360p con el modelo cada 4.º fotograma, unos 29 fps con NR activado. **0.3.5
recorta alrededor de un 10% de esas cifras en la clase Z1 Extreme** (Z1 Extreme, Z2, Radeon 780M: unos 50 ms a 360p,
unos 105 ms a 576p, la misma imagen bit a bit, verificada por hash por un tester); los módulos de Z2 Extreme / 890M /
880M no cambian.

- **No soportadas:** Z1 y Radeon 740M (4 unidades de cómputo), Radeon 760M (8), Radeon 860M / 840M. El runtime
  de danielblnc no funciona en APU portátiles. Las tarjetas de escritorio RX 6000 (RDNA 2) ejecutan el runtime 0.6.0
  de danielblnc (ver "Requisitos"; lmxxf ahí está previsto para 0.3.6); la Steam Deck y otras APU RDNA 2 no están
  soportadas.
- **Lo que hace por sí solo** (solo mientras tu ini no tenga un valor propio): el tamaño de red tiene un tope de 576
  (1024x576; desde 0.3.5.5R3, antes era 360p), y desde 0.3.5.6 Fast mode viene activado por defecto, así que la red
  corre un tamaño por debajo, a 360p (640x360); el modelo corre cada 4 fotogramas (Model interleave; no se guarda).
  Desde 0.3.5.5R3 se conservan tus Neural passes (antes se fijaban en 1); el coste de NR los tiene en cuenta.
- **Velocidad, con honestidad:** Como
  referencia: una RX 7800 XT (60 unidades de cómputo) necesita 34.4 ms por pasada de la red al tamaño 720; estos
  chips tienen de 12 a 16 y funcionan a relojes más bajos. Espera una gran pérdida de fotogramas incluso a 360p
  con el modelo cada 4 fotogramas, algo de ghosting por el interleave largo, y un aspecto más suave que en una
  GPU de escritorio. El coste de NR al final de la línea de estado de la pestaña Neural (y en Diagnostics)
  muestra el número real en tu dispositivo.
- **Ajustes:**
  - Más nítido pero más lento: `[DlssNr] AmdLmxxfFastMode=false` (el tamaño de red 1024x576;
    `[DlssNr] AmdLmxxfTierCap=360` mantiene el tamaño más pequeño incluso con Fast mode desactivado).
  - Con Fast mode activado, un render de 720p u 800p con NR resolution al 100% ya corre al tamaño 360p, así que una NR
    resolution más baja no lo abarata.
  - Model interleave en Off se guarda como `[DlssNr] AmdInterleave=1` (también apagado), para que el valor por
    defecto de la portátil no vuelva en el siguiente inicio. Para apagarlo a mano, escribe 1, no 0.
  - Preset > **Handheld** (desde 0.3.5.1) ajusta NR resolution al 80%, Dynamic NR desactivado, el modelo cada 4.º fotograma con Edit accumulation, 1 Neural pass, Full network activado, Fast mode activado y Before upscaling, más sus propios valores de aspecto (Residual limit 0.32, Temporal stability 0.70, Sharpening 0.10, Output smoothing 0.65 y algunos más; el tooltip del botón los enumera todos). El botón solo aparece en estas APU; Quality, Balanced y Performance también mantienen aquí el tamaño de red de las portátiles (el menú lo indica).
- **FSR 4:** FSR 4 (INT8) está disponible como opción experimental en las portátiles RDNA 3 (pestaña Upscaling); no validado por AMD.
  La casilla **FSR 4 (INT8) - Experimental on this GPU (restart)** escribe `[FSR] Fsr4ForceModel=2`; nunca se activa por sí solo
  (con `Dx12Upscaler=auto` el upscaler es XeSS). Unos 1,5-3 ms por fotograma en una Z1 Extreme (estimación); con NR,
  FSR 4 o el tamaño 576, no ambos.
- **Shadow of the Tomb Raider** (y los juegos que crean su dispositivo D3D12 dos veces) ya no se cierra al arrancar el
  upscaler (corregido en 0.3.4).
- **Controlador:** usa el controlador Adrenalin propio de AMD. lmxxf necesita HIP (`amdhip64_7.dll`), que
  algunos controladores de fabricantes de portátiles omiten; `amd_bridge.log` dice entonces que HIP no está
  disponible.
- **Moverse con Model interleave (corregido en 0.3.5):** la edición transportada desaparecía en cuanto te movías
  ("el efecto se va al moverse"): con Model interleave la red corre sobre fotogramas de duración desigual, y la
  protección del transporte suponía fotogramas iguales. Ahora lee los vectores del fotograma anterior con la
  duración de este fotograma (ambos runtimes).
- **Usa juntos los archivos de una misma release:** reemplaza `OptiScaler.dll`, `LmxxfNrRuntime.dll` y
  `LmxxfNrRuntime.pak` a la vez; el runtime rechaza una portátil cuando `OptiScaler.dll` es
  anterior a 0.3.4 ("this handheld needs OptiScaler.dll 0.3.4 or newer").
- **Testers con una consola portátil:** pide en Discord el kit de prueba para portátiles (`handheld-test.zip`).
  Su `run_probe.bat` mide la red en tu dispositivo y escribe `handheld_result.txt` (tu nombre de usuario de
  Windows queda oculto).

## AMDNR Anywhere (preview, nuevo en 0.3.5)

**Qué es.** Neural Rendering para los juegos que no tienen DLSS, XeSS ni FSR 2 propios — y sin escribir nada en la
carpeta del juego. AMDNR se ejecuta dentro de un host de captura de ventana: el host captura la ventana del juego, la
escala a tu pantalla, y Neural Rendering se ejecuta sobre la imagen capturada; nuestro menú se dibuja dentro
del host (tu tecla del menú, `INSERT` por defecto) con su propia pestaña **Anywhere**. El host es
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)**, en la build propia de AMDNR: su `Magpie.exe` está recompilado con los cambios de AMDNR (la oferta de código fuente, un único diff respecto a la etiqueta del fork y las opciones de compilación están a su lado, en `Anywhere-host-source/`) y el paquete no lleva ningún archivo de NVIDIA. El AMDNR Launcher lo descarga de la release de AMDNR en GitHub (unos 130 MB para 0.3.5.6, una vez por versión del host).

**Cómo se usa.** En el AMDNR Launcher, un juego sin upscaler muestra **PLAY ANYWHERE** en lugar de INSTALL. Púlsalo:
el launcher descarga el host (la primera vez), inicia el juego, y el host captura su ventana. Ejecuta el juego **en
ventana o en ventana sin bordes**, no en pantalla completa exclusiva, y pulsa tu tecla del menú para abrir el menú de
AMDNR dentro del host. Un juego con upscaler propio conserva la ruta INSTALL normal: Anywhere es para los juegos que
no tienen ninguno.

**Los ajustes del host viven en el menú**, en Host settings de la pestaña Anywhere (el launcher también tiene su
propio panel, ver abajo): el tamaño de la ventana del juego (720p / 900p / 1080p — un consejo sobre qué poner en el juego; el host captura la ventana que
el juego abra, sea cual sea), la lista de etapas (V1: un pase de FSR 3 a la pantalla; V2: FSR 3 a 1x y después un
pase de relleno), el nivel de NR (Auto / 720 / 900 / 1080), VRR, el ritmo de fotogramas y la **frecuencia de
fotogramas del host** (Default = la frecuencia de refresco de tu pantalla, como máximo 60; Auto = la frecuencia que
la red sostuvo en la última sesión de juego; de 30 a 120; Display refresh = sin límite). Se aplican en el
**siguiente** PLAY ANYWHERE, y el launcher muestra un resumen de solo lectura junto al botón. En el `OptiScaler.ini`
propio del host son `[DlssNr] AnywhereWindow`, `AnywhereEffect`, `AnywhereNrTier`, `AnywhereVrr`, `AnywherePacing` y
`AnywhereHostFps`. Limita también el juego, con su propio limitador, a 60-90 fps: el host solo puede mostrar
fotogramas que el juego dibujó, y cada fotograma del host ejecuta la red una vez. **Desde el AMDNR Launcher 0.3.5.25,
el launcher también tiene un panel de ajustes de Anywhere** (el runtime de NR, Neural passes, el tamaño neural, FSR
Auto / FSR 4 / FSR 3 / Off — Off conserva NR —, la generación de fotogramas (experimental), el movimiento para el
escalado, la nitidez, y Model E / Model L (experimentales; con Neural Rendering dentro de Anywhere aún están en
pruebas)); escribe el mismo ini del host y se aplica en el siguiente PLAY ANYWHERE.

**Lo que el host no puede darle a la red.** Una ventana capturada no tiene profundidad, vectores de movimiento,
jitter ni exposición propios; el host estima el movimiento. Por eso la pestaña Anywhere nombra lo que se estima; las
filas de la pestaña Neural que no pueden actuar ahí se ocultan o se rechazan con un motivo (Ray Regeneration, Screen
GI y Model interleave — gastaría la edición transportada sobre un movimiento estimado); y una línea de estado en la
página Anywhere dice cuándo la red supera el presupuesto de fotograma del host y qué bajar. **Una ventana de juego de
1920x1080 o menor es el caso exacto al píxel:** una ventana mayor se reduce primero al techo de la red y se vuelve a
escalar, y la página lo dice, con la proporción de los píxeles de la pantalla que vio la red. La fila NR resolution
muestra el tamaño real de la red dentro del host.

**Estado: preview.** Solo RX 9000 (RDNA 4) en esta release; RX 7000 seguirá cuando esté probado ahí. Puede quedar un
ligero parpadeo o tirón en el movimiento rápido (baja el nivel de NR a 720, limita el juego a 60-90 fps, deja Model
interleave desactivado — el host lo rechaza). El host de captura es la build propia de AMDNR y se descarga de la release de AMDNR en GitHub. Reportes: el zip de **Save report** desde el menú dentro del host (su título nombra el juego
escalado), o el COLLECT LOGS del launcher.

## Model E y Model L (experimentales, opcionales, nuevos en 0.3.5.6)

Dos opciones más de Super Resolution (upscaler) junto a FSR y XeSS. **No son obra de AMDNR ni modelos de AMDNR, y
ninguno de los dos va en el zip de AMDNR.** Cuando eliges uno en el AMDNR Launcher (GAME SETTINGS de un juego > **SR
model**: Auto / Model E / Model L), el launcher muestra una vez el aviso de ese modelo, te pide que lo aceptes y
descarga los archivos de la propia release del autor, verificados por hash. AMDNR no aloja ni distribuye sus archivos
ni ningún archivo de NVIDIA. Los dos son experimentales: espera fallos, y envía un reporte cuando algo vaya mal.
**Auto** conserva la ruta normal del juego (y quita los archivos de Model L de la carpeta del juego).

**Model E - HelixSR de lonewolf0622** (<https://github.com/lonewolf0622/HelixSR>), bajo su propia licencia.
- HelixSR construye su red en tu PC a partir del archivo DLSS de NVIDIA, que el launcher descarga del propio GitHub de
  NVIDIA (o toma de tu juego); lo que construye se queda en tu PC.
- En el juego: Upscaling > **HelixSR (Model E) - restart required** (juegos D3D12; `[Upscalers] FfxSRProvider=helix_e`).
  Se aplica en el siguiente inicio del juego.
- Funciona con **ambos** runtimes de NR, danielblnc y lmxxf. En nuestras pruebas funcionó en un juego en una RX 9070 XT.

**Model L - d4r de countervolts** (<https://github.com/countervolts/d4r>), bajo la propia licencia de d4r.
- **Qué es:** d4r ejecuta **el runtime DLSS de NVIDIA, bajo el EULA de NVIDIA, a través de ZLUDA / HIP** en una
  tarjeta AMD. El launcher descarga el paquete de d4r de la propia release de d4r en GitHub y lo coloca en la carpeta
  del juego como `AMDNR-SR-L\`; AMDNR no aloja ni distribuye ningún archivo de NVIDIA. **El autor de d4r no lo ha
  probado en Windows.**
- **Solo RX 9070 / RX 9070 XT** (probado en una RX 9070 XT; la RX 9060 XT y las tarjetas anteriores se rechazan) y, en
  el juego, **solo juegos D3D12**.
- **Funciona con el runtime lmxxf, o con Neural Rendering desactivado. El runtime de danielblnc va con Model E:** con
  danielblnc como runtime de NR, la fila Model L aparece en gris ("Model L runs with the lmxxf runtime (or with Neural
  Rendering off); danielblnc's runtime pairs with Model E. Pick lmxxf under Neural Rendering first."; es decir: Model L
  funciona con el runtime lmxxf, o con Neural Rendering desactivado; el runtime de danielblnc va con Model E; elige
  primero lmxxf en Neural Rendering), y un juego iniciado con los dos ejecuta tu upscaler FSR en su lugar.
- En el juego: Upscaling > **Model L (d4r) - restart required**, en gris con su motivo cuando no puede funcionar aquí
  (no es un juego D3D12, no está preparado en el launcher, no es una RX 9070 / 9070 XT, el runtime de danielblnc, o aún
  hace falta un reinicio). Elegirlo escribe `[DlssNr] AmdSrModel=L` (elegir otro upscaler quita esa línea); se aplica
  en el siguiente inicio del juego.
- **Experimental:** su velocidad aún no está ajustada y puede ir mucho más lento que FSR. Si no puede arrancar, el
  juego debería conservar tu upscaler FSR (no FSR 2.1.2), y `OptiScaler.log` dice por qué en una línea `[MODELL`.

Dentro de **AMDNR Anywhere** los dos se ofrecen en los ajustes de Anywhere del launcher; junto con Neural Rendering ahí
aún están en pruebas.

## Límites conocidos (0.3.5.6)

- **El ghosting con lmxxf se reduce, no se elimina:** un rastro más corto aún puede seguir a los objetos que se mueven
  rápido.
- **Fast mode en RX 7000 y consolas portátiles** es un valor por defecto nuevo, todavía sin reportes de juegos: por
  favor, envía el tuyo. `[DlssNr] AmdLmxxfFastMode=false` devuelve el tamaño de red anterior.
- **El Full Model Look después del escalado cuesta más**, porque la red recibe la imagen al tamaño de la pantalla.
  Donde el control de coste o el historial de TDR de este juego dicen que la tarjeta no puede pagarlo, se ejecuta antes
  del escalado y la línea de estado de Neural lo dice; tu propio `[DlssNr] AmdPlacement=pre` también lo mantiene antes
  del escalado (se dice una vez: vuelve a elegir Full Model Look en NR style, o borra la línea).
- **Vuelta a FSR 2.2:** en algunos juegos D3D11 y Vulkan, un upscaler que falla aún puede volver a FSR 2.2.
- **Model L** es solo para RX 9070 / RX 9070 XT y D3D12, no va con el runtime de danielblnc, es experimental y puede ir
  mucho más lento que FSR; su autor no ha probado d4r en Windows.
- **Model E y Model L dentro de AMDNR Anywhere**, junto con Neural Rendering, aún están en pruebas.
- **danielblnc en RX 7000:** el modo de mismo fotograma de su runtime cuesta mucho tiempo por fotograma en RDNA 3 por
  su propio diseño; usa lmxxf ahí.
- **Linux / Proton:** Neural Rendering no funciona ahí (ver abajo).

## Linux / Proton (Steam Deck, Linux de escritorio)

AMDNR funciona bajo Proton y Wine como una build de OptiScaler. **Neural Rendering no funciona en Linux (solo
Windows):** los dos runtimes de NR necesitan el HIP del controlador AMD de Windows, que Proton y Wine no ofrecen. Con
NR activado bajo Proton, NR no se ejecuta, y desde 0.3.5 la pestaña Neural y el reporte lo dicen (en lugar de "Idle").
Es lo esperado, no un cierre ni una instalación rota. El AMDNR Launcher es un programa de Windows que puede funcionar
bajo Proton (experimental, aún no lo hemos probado, ver abajo); la instalación a mano funciona sin él.

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
- **Títulos Vulkan** (juegos RTX Remix, id Tech 8), en Proton igual que en Windows: Ray Reconstruction y la generación
  de fotogramas propia de AMDNR responden "not supported" por diseño (el eliminador de ruido es D3D12, y los búferes
  de trazado de rayos se quedan en el dispositivo Vulkan); desde 0.3.5 las pestañas Ray Regeneration y Frame Gen lo
  dicen en lugar de pedirte que los actives en un juego que los deja en gris. La superresolución DLSS funciona a
  través del puente.
- Desde 0.3.5, `[Spoofing] Dxgi=true` bajo Proton (la ruta de actualización a FSR 4) ya no falla al arrancar.

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

1. Descarga `AMDNR-vX.X.X.zip` de la página de releases (para 0.3.5.6: `AMDNR-v0.3.5.6.zip`). Los zips del runtime de
   danielblnc (`v0.6.0-Runtime.zip` y los demás) solo los usa Neural Rendering, así que no los necesitas en Linux
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
- Una GPU AMD con AMD Software: Adrenalin Edition 26.9.1 o más reciente. El runtime neural usa HIP a través del controlador; no
  hace falta el SDK de HIP ni el modo desarrollador (salvo en RX 6000, abajo). Qué chips:
  - RX 9000 (RDNA 4): ambos runtimes.
  - RX 7000 (RDNA 3, escritorio y portátil): ambos runtimes - lmxxf mediante el backend RDNA 3 de AMDNR, más
    lento que en RDNA 4 (el nivel de tamaño de red y, desde 0.3.5.6, Fast mode están activados por defecto, ver
    arriba).
  - Strix Halo (Radeon 8060S / 8050S): lmxxf.
  - APU de consolas portátiles con 12+ unidades de cómputo (Z1 Extreme / Z2 / 780M, Z2 Extreme / 890M /
    880M): lmxxf, experimental y lento. Z1 (4 CU), 760M / 740M y 860M / 840M: no soportadas.
  - RX 6000 (RDNA 2, escritorio): el runtime 0.6.0 de danielblnc (`v0.6.0-Runtime.zip`), con el SDK de HIP de AMD
    (HIP 7.2) instalado, que pone el runtime HIP que necesita RX 6000 en su propia carpeta (AMDNR lo busca ahí); lmxxf
    aún no funciona en RX 6000 (previsto para 0.3.6). Steam Deck y APU RDNA 2: no soportadas (para Neural Rendering;
    para los upscalers bajo Proton, ver "Linux / Proton").

  La pestaña Neural dice qué puede ejecutar tu GPU (pasa el ratón por las entradas de runtime, o mira la
  línea GPU en Diagnostics).
- **AMDNR Anywhere** (preview): Windows, una tarjeta RX 9000 (RDNA 4) en esta release, y el AMDNR Launcher, que
  descarga el host de captura; el juego corre en ventana o en ventana sin bordes. Ver "AMDNR Anywhere".
- **Model E / Model L** (experimentales, opcionales): el AMDNR Launcher, que los descarga, y un juego D3D12; Model L
  además necesita una RX 9070 / RX 9070 XT y el runtime lmxxf (o Neural Rendering desactivado). Ver "Model E y Model L".
- Un juego Direct3D 12, Direct3D 11 o Vulkan. La ruta neural de AMD es D3D12; los títulos D3D11 y
  Vulkan la alcanzan a través del puente D3D12 de OptiScaler, lo que significa que el upscaler debe
  ser uno de los backends "w/Dx12" (`ffx_12`). Deja `Dx11Upscaler` / `VulkanUpscaler` en `auto` y
  esta build lo elige por ti cuando el renderizado neural está activo. Con Neural Rendering activado, la lista de Upscaling los nombra "... w/Dx12 - Neural".
- Unos 2 GB de VRAM libre a resoluciones de renderizado de clase 1080p.

## Qué hay en los archivos

**AMDNR-vX.X.X.zip**

| Archivo | Qué es |
|---|---|
| `OptiScaler.dll` | OptiScaler con el backend AMD de DLSS-NR (AMDNR 0.3.5.6). Renómbralo como dice la guía. |
| `OptiScaler.ini` | Ajustes. Neural Rendering está activado; el registro está encendido para que un reporte tenga algo que adjuntar. |
| `LmxxfNrRuntime.dll` | El runtime neural lmxxf (0.3.5.6: una comprobación, en la historia de la red, de dónde vino cada píxel, que acorta los rastros de ghosting, y un fotograma de reinicio que ya no destella con tanta fuerza; debe venir de la misma release que `OptiScaler.dll`. 0.3.5: comprueba cada módulo HIP del pak contra la lista de sumas de verificación del propio pak antes de usarlo, nombra ambos lados cuando ningún adaptador HIP coincide con la GPU del juego, mantiene los pasos de resolución dinámica sin reconstruir la red, y ya no lee las variables de entorno de lmxxf que cambian la imagen; los kernels de lmxxf, incluidos los de lmxxf 0.31, los kernels c32w de AMDNR, los tamaños de red pequeños y la máscara de personajes nativa). Se usa solo cuando se elige; lee `LmxxfNrRuntime.pak` junto a él, ver "El runtime lmxxf". |
| `LmxxfNrRuntime.pak` | Los pesos, módulos HIP y shaders del runtime lmxxf en un archivo cifrado (465 MB; 0.3.5: el conjunto de módulos para RX 7000 y para las consolas portátiles de la clase Z1 Extreme — Z1 Extreme, Z2, Radeon 780M — es alrededor de un 10% más rápido, con la misma imagen; RX 9000 recibe los módulos de lmxxf 0.37 / 0.39 de Kien (MIT) junto a su conjunto base sin cambios; los módulos de Z2 Extreme / 890M / 880M y Strix Halo no cambian). Solo lo lee el runtime lmxxf; es inofensivo mantenerlo con el runtime de danielblnc. |
| `OptiScaler\` | FSR, XeSS, el denoiser FidelityFX y el D3D12 Agility SDK que usa OptiScaler. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | El dlssg-to-fsr3 de Nukem9, sin modificar y renombrado: las llamadas de DLSS Frame Generation del juego servidas por la generación de fotogramas de FSR 3, también en Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, ver `Licenses/`. |
| `Licenses\`, `LICENSE` | Licencias de terceros, el aviso de AMDNR (`AMDNR_NOTICE.txt`) y la licencia GPL-3.0 de esta build. |
| `SHA256SUMS.txt` | Sumas de verificación de cada archivo de este zip, y de los archivos de los zips del runtime de danielblnc que figuran en él. |

**No incluido en este zip** (el AMDNR Launcher descarga cada uno por hash, solo cuando lo eliges): los zips del runtime
de danielblnc (abajo); el host de captura de AMDNR Anywhere (la build propia de AMDNR, de la release de AMDNR en
GitHub); el HelixSR de Model E y el d4r de Model L, de las propias releases de sus autores (ver "Model E y Model L"); y
OptiPatcher, de las releases del proyecto OptiScaler (GAME SETTINGS).

**Los zips del runtime de danielblnc** (DLSS-NR on AMD by Daniel Blanco, sin modificar, con su permiso; usa uno)

Cuál usar: `v0.6.0-Runtime.zip` (recomendado) de Alpha0.3.5.5R4 — en RX 9000 y RX 7000, y el único para RX 6000;
`v0.5.1-Runtime.zip` (Alpha0.3.5), `v0.5.0-Runtime.zip` (Alpha0.3.4.2), `v0.4.3-Runtime.zip` (Alpha0.3.4.2 y
Alpha0.3.4.1), `v0.4.1-Runtime.zip` y `v0.4.0-Runtime.zip` (Alpha0.3.4.1) siguen aceptados en RX 9000 y RX 7000. El
runtime lmxxf no necesita ningún zip de runtime en RX 7000 y RX 9000; las APU de consolas portátiles usan solo lmxxf.
El AMDNR Launcher ofrece para tu tarjeta 0.6.0 (recomendado, el primero de la lista), 0.5.1, 0.5.0, 0.4.3, 0.4.1 y
0.4.0, y lo elige por ti.

| Zip | Release | Runtime de danielblnc |
|---|---|---|
| `v0.6.0-Runtime.zip` | Alpha0.3.5.5R4 | 0.6.0, **recomendado en RX 9000 y RX 7000, y el que corresponde a RX 6000** (con el SDK de HIP de AMD instalado); los ajustes del runtime de danielblnc funcionan con él |
| `v0.5.1-Runtime.zip` | Alpha0.3.5 | 0.5.1, sigue aceptado; los ajustes del runtime de danielblnc funcionan con él |
| `v0.5.0-Runtime.zip` | Alpha0.3.4.2 | 0.5.0, sigue aceptado (puede congelar algunos juegos en algunas tarjetas: uno más nuevo es más seguro); los ajustes del runtime de danielblnc funcionan con él |
| `v0.4.3-Runtime.zip` | Alpha0.3.4.2 (y Alpha0.3.4.1) | 0.4.3, sigue aceptado; los ajustes del runtime de danielblnc funcionan con él |
| `v0.4.1-Runtime.zip` | Alpha0.3.4.1 (y Alpha0.3.4) | 0.4.1, sigue aceptado. Network style, Tone curve, Black lift y Game exposure aparecen en gris con él |
| `v0.4.0-Runtime.zip` | Alpha0.3.4.1 (y Alpha0.3.4) | 0.4.0, sigue aceptado; los ajustes del runtime de danielblnc funcionan con él |
| `v0.3.3-Runtime.zip` | [Alpha0.3.4](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/tag/Alpha0.3.4) | 0.3.3, retirado: ya no se recomienda. Sigue funcionando si ya lo tienes; los ajustes del runtime de danielblnc funcionan con él |
| `Runtime.zip` | Alpha0.3.4 | 0.3.1; los ajustes del runtime de danielblnc aparecen en gris con él |

**danielblnc 0.6.0 es el runtime de danielblnc recomendado** (el launcher lo pone el primero de la lista para tu
tarjeta). Desde 0.3.5.5R4, AMDNR ejecuta su runtime en su modo de mismo fotograma: bajo AMDNR, su modo asíncrono
completaba tres trabajos y después nada más, así que un `Async=1` en su `dlssnr_on_amd.ini` se ejecuta como mismo
fotograma y se avisa una vez (`[DlssNr] AmdDanielAsync=true` lo deja pasar). Desde 0.3.5.6, el runtime de danielblnc va
con Model E, no con Model L. Una build de danielblnc más nueva que 0.6.0 no la maneja esta release.

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
- **Neural runtime** (danielblnc / lmxxf, con la versión que se ejecuta, p. ej. `danielblnc 0.6.0` o `lmxxf 0.41`; desde 0.3.5.6 la entrada de lmxxf nombra el conjunto de kernels de lmxxf que se ejecuta, no la release de AMDNR) con una palabra de estado: running, restart the game to switch, not
  installed, not for this GPU o stopped. Debajo, el crédito del runtime activo y una línea de estado, p. ej.
  `Running - 1920x1080 at 100% - NR 62/s - model 62/s - 15.3 ms` (el último número es el coste de NR), y una fila **Live** cerrada con
  más detalle. Cuando
  algo requiere tu atención sigue una línea naranja, con un botón cuando hay arreglo (Retry lmxxf, Switch to
  danielblnc, Open Upscaling). En el estado por defecto no hay ninguna.
- **Preset**: Quality / Balanced / Performance ponen NR resolution al 100 / 85 / 70% y apagan Dynamic NR; nada
  más. En APU de consolas portátiles hay un cuarto botón, **Handheld** (ver "APU de consolas portátiles"). **NR
  style** (primero los tres aspectos — **Full Model Look**, el predeterminado desde 0.3.5.6, **NVIDIA look** y **AMDNR
  soft** — y después cuatro estilos con nombre; ver "Ajustes que conviene conocer"), y **Style slots** (Store / Apply /
  Clear).
- **Performance**: **Neural placement** (Before / After upscaling, nuevo en 0.3.5; ver "Ajustes que conviene
  conocer"), NR resolution (%) con su coste (del 25 al 200% desde 0.3.5.6), Neural passes, **Refine passes**
  (danielblnc, nuevo en 0.3.5.6), Full network, Fast mode, Dynamic NR resolution, Model interleave
  (Interleave preset y la línea de ritmo aparecen debajo mientras está activado).
- **Quality**: Residual strength, Residual limit, Temporal stability, Sharpening (CAS), y **More quality
  options** (Network history - una sola casilla para ambos runtimes -, Output smoothing, Stability mode,
  Residual temporal, Residual edge fade, Still-surface steadiness).
- **Image look**: Colour composition, Detail y Colour strength, y tres desplegables: **Model strength** (Tone y
  Structure intensity, Character structure, Edit detail / colour, Edge guard, Native character mask, y Network
  style, Tone curve y Black lift de danielblnc), **Exposure and highlights** (Auto-exposure, su tope de altas
  luces, Highlight colour guard, **Game exposure** Auto / On / Off) y **Appearance filter** (con su palabra off / on
  tras el nombre; su control **Anti-halo** se llamaba Halo protection antes de 0.3.5.6). Un "default" o "custom" tenue
  tras el nombre de un desplegable indica si cambiaste algo dentro.
- **Ray Regeneration**: ya no está en esta pestaña desde 0.3.5.3; sus controles y su estado están en su propia pestaña **Ray Regeneration** (justo después de Upscaling). Aquí solo la línea naranja bajo el runtime puede decir "Ray Regeneration is off in this title" o "Ray Regeneration could not start on this GPU" (pasa el ratón por encima para ver el motivo).
- **Screen-space GI** (sección nueva en 0.3.5.3): **AMDNR Screen-space GI**, desactivado por defecto, con un "by 3zwr1" tenue detrás; debajo, mientras está activado: GI quality (Low / Medium / High / Ultra; Auto elige uno para tu GPU hasta que pulses uno, y su etiqueta muestra el tiempo de GPU medido, o una estimación mientras GI no se ha ejecutado), Bounce light, Ambient occlusion, Radius, Object thickness, Camera FOV, **More GI options** (Bounce colour, Sky light, Multi-bounce, FOV axis, Colour encoding, Debug view) y Reset GI. En toda GPU AMD, con o sin runtime de NR; dentro de AMDNR Anywhere aparece en gris con su motivo.
- **La fila de herramientas**, cerrada al inicio: **Diagnostics** (Network output, Debug view, Edit shaper A/B, NR
  cost, las lecturas de ghosting y de autoajuste, la línea GPU, **Save report**; la vista de depuración de RR está en
  la pestaña Ray Regeneration desde 0.3.5) y **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy); **Experimental** solo aparece mientras tu ini tiene activado el Screen-space GI heredado y retirado (`[AmdRtgi] Enabled`), y solo con su casilla, con la etiqueta "retired".

Desde 0.3.5.6, un control que el runtime activo no puede ejecutar se oculta en ese runtime, con un recuento (p. ej.
Refine passes y Tone intensity en lmxxf; Edit detail, Edit colour, Edge guard, Output smoothing y Full network en
danielblnc); su clave del ini sigue funcionando. Un control que solo funciona en ciertas condiciones se queda, en gris
con el motivo (p. ej. Residual limit, Debug view, Neural passes); cambiar de runtime no mueve ninguna otra fila.

**Las otras pestañas:** Upscaling empieza con el upscaler, una línea de estado y Render resolution (los antiguos
Upscale Ratio Override y Output Scaling); en una tarjeta que no es NVIDIA ya no aparece "DLSS w/Dx12". Desde 0.3.5.6
la lista de upscalers también tiene **HelixSR (Model E) - restart required** (juegos D3D12) y **Model L (d4r) - restart
required** (en gris con su motivo donde no puede funcionar; ver "Model E y Model L"). **Ray
Regeneration** (nueva en 0.3.5) sigue a Upscaling mientras Ray Regeneration se ejecuta en el título: las líneas de
estado (por tarjeta y por API), la fila **Denoiser backend** (Automatic / AMD FSR Ray Regeneration / AMDNR Ray Denoiser (preview) / Off (the game's own): Automatic es el de AMD en RX 9000 y el AMDNR Ray Denoiser en las demás tarjetas, los tres primeros cambian con el juego en marcha, y Off le dice al juego que Ray Reconstruction no está soportado, así que conserva su propio denoiser, tras un reinicio), los controles, More Ray
Regeneration options, y su propio bloque Diagnostics con la vista de depuración de RR y la cifra de ruido (grano de
entrada y de salida, parpadeo con la cámara quieta); nunca se dibuja dentro de AMDNR Anywhere. Image contiene
Sharpness, Textures, Init Flags y el Magnifier. Frame Gen empieza con FG Input y FG Output y, desde 0.3.5, una línea
que nombra el paso que aún falta para que se genere algo. Interface tiene el overlay de FPS y Keybinds (un botón por
tecla). Advanced empieza con Active Quirks, luego Display (V-Sync), Compatibility y Logging. Dentro de AMDNR Anywhere
el menú muestra una pestaña **Anywhere** (la línea de captura y de red, los ajustes del host) en lugar de las
pestañas Frame Gen y Advanced. Los ajustes, las claves y lo que escribe Save Settings no cambian, salvo donde
`CHANGELOG.md` lo indica.

## Ajustes que conviene conocer

Abre la pestaña **Neural**. Los valores por defecto son la configuración probada más reciente, así
que el primer movimiento útil es cambiar una cosa a la vez.

- **NR style** (arriba en la pestaña Neural, `[DlssNr] AmdNrLook = auto | 0 | 1`, ambos runtimes) — los tres primeros
  elementos son aspectos completos. **Full Model Look** (`auto`, el predeterminado desde 0.3.5.6): la red del NVIDIA
  look (Style 0, los 71 bloques) con intensidades de modelo más fuertes — Structure intensity 2 en ambos runtimes, Edit
  detail 1.5 y Edit colour 1.25 en lmxxf —, sin residual limit ni edge guard, Temporal stability 0.5, mientras output
  smoothing, la historia de la red y las demás protecciones de estabilidad siguen activadas; pide Neural placement
  After upscaling mientras no hayas fijado tú el placement (ver Neural placement abajo). **NVIDIA look** (`0`, el
  predeterminado de 0.3.5.3 a 0.3.5.5R4): lo más parecido a la salida propia de NVIDIA, Before upscaling. **AMDNR
  soft** (`1`): el aspecto anterior de AMDNR, más suave y más estable. Un aspecto aporta sus valores solo mientras no
  hayas fijado tú esos controles, y bajo Model interleave las protecciones conservan sus valores anteriores. Elegir un
  aspecto en el desplegable también escribe su Neural placement (Full Model Look: After upscaling; los otros dos:
  Before) y Full network como valores tuyos. Los cuatro estilos con nombre que vienen después ajustan sus controles
  sobre el aspecto que esté en uso.
- **NR resolution** — la palanca principal de calidad/coste. Por debajo del 100% el modelo trabaja
  sobre una imagen más pequeña y solo su *corrección* se lleva de vuelta al fotograma a resolución
  completa, así que el fotograma conserva su propio detalle. Por encima del 100% el coste crece con
  el cuadrado (150% es 2.25x, 200% es 4x). Desde 0.3.5.6 el control va del 25% al 200% en ambos runtimes (antes se
  detenía en el 150%); la red de lmxxf sigue llegando como máximo a 1920x1080, y la fila lo dice cuando ese es el
  límite. El control se mueve en pasos del 5%: cada tamaño de NR nuevo puede
  retener VRAM hasta que reinicies el juego, así que reinícialo tras muchos cambios.
  El coste junto a él marca 1.00x al 100%; en lmxxf es el precio del nivel de tamaño de red en el que corre (su
  tooltip nombra el nivel). Los botones Preset lo ponen al 100 / 85 / 70%.
- **Neural placement** (Neural > Performance, `[DlssNr] AmdPlacement = pre | post`, nuevo en 0.3.5, ambos runtimes) —
  dónde se ejecuta el pase neural. `pre` (el valor por defecto de la propia clave, y lo que ejecutaba toda build
  anterior a 0.3.5.6) edita la imagen a resolución de render que el upscaler está a punto de leer. `post` edita en su
  lugar la imagen terminada del upscaler a resolución de pantalla: más nítido, porque el upscaler ya no vuelve a filtrar
  la edición, y más caro — en lmxxf la red corre al tamaño de la pantalla hasta su techo de 1920x1080, así que una
  pantalla 1080p paga el nivel superior con cualquier NR resolution, y una pantalla 1440p o 4K recibe una edición de
  1080 líneas vuelta a escalar (danielblnc no tiene ese techo, así que `post` le cuesta más) — algo menos tolerante en
  movimiento, y el HUD queda incluido si el juego lo compone antes del escalado. **Desde 0.3.5.6, mientras la línea
  esté vacía o en `auto`, el Full Model Look por defecto pide `post` en todas las tarjetas**; el control de coste, el
  historial de TDR de este juego y la protección TDR en vivo le ponen precio, y lo devuelven a `pre` durante la sesión
  cuando la tarjeta no puede pagarlo (la línea de estado de Neural y `amd_bridge.log` dicen por qué). Un `pre` que fijes
  tú (la fila, el preset Handheld o el launcher) mantiene el aspecto antes del escalado; la línea de estado lo dice, y
  volver a elegir Full Model Look en NR style escribe `post`. Se rechaza (vuelve a `pre`, una línea en
  `amd_bridge.log`) dentro de AMDNR Anywhere, en el modo de imagen final, y una vez que Ray Regeneration se ha
  ejecutado en el título. La fila NR resolution muestra ambos tamaños mientras `post` está en uso.
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
- **Refine passes** (Neural > Performance, `[DlssNr] AmdRefineMode = full | refine`, solo danielblnc, nuevo en
  0.3.5.6) — el modo de pases propio de AMDNR. *Full* (el predeterminado) ejecuta cada pase al tamaño completo.
  *Refine* ejecuta el pase 1 al tamaño completo y los pases 2 y 3 un tamaño de red más pequeño sobre la respuesta del
  pase anterior, y después lleva su edición de vuelta a la imagen de tamaño completo: todos los pases siguen
  ejecutándose, los extra cuestan menos. Sin efecto con 1 pase. El runtime de lmxxf construye un solo tamaño de red por
  sesión, así que ejecuta Refine como Full y la fila está oculta ahí.
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
- **Más detalle en las caras** (nuevo en 0.3.5.6, ambos runtimes, siempre activado) — mientras Character structure
  sigue a Structure intensity (la casilla por defecto), las caras y la piel reciben 1.5 veces la estructura de la
  escena, como máximo 4 (con el Full Model Look: 2.00 -> 3.00); sin pase extra ni coste extra. Un Character structure
  que fijes tú siempre gana, y el valor en gris del menú muestra el número elevado. `[DlssNr] AmdFaceDetail=false`
  (solo en el ini) lo desactiva.
- **Game exposure** (Image look > Exposure and highlights, `[DlssNr] AmdUseGameExposure = auto | 1 | 0`, ambos
  runtimes) — desde 0.3.5.6 sus opciones son **Auto / On / Off**. Auto usa el valor de brillo propio del juego cuando
  el juego publica uno; On siempre se lo pasa a la red; Off nunca lo hace, y la autoexposición propia del runtime toma
  el relevo — prueba Off si NR deja un juego demasiado oscuro o demasiado brillante. Actúa en el siguiente fotograma
  del modelo.
- **Anti-halo** (Image look > Appearance filter, `[AmdLook] AntiHalo`, 0..1, por defecto 0.901; se llamaba Halo
  protection antes de 0.3.5.6, misma clave) — limita la edición del filtro junto a los bordes fuertes para que no se
  forme ningún anillo claro u oscuro; más alto = bordes más seguros, detalle algo más suave junto a ellos. Necesita que
  el Effect mix del filtro esté por encima de 0.
- **La generación de fotogramas está apagada en un ini nuevo**, y activarla lleva cinco pasos: FG Input y FG Output
  en la pestaña Frame Gen (p. ej. "DLSSG via Streamline" en un juego con generación de fotogramas DLSS, y XeFG),
  **Save Settings**, un reinicio completo del juego, la generación de fotogramas **propia** del juego activada, y
  luego **Active** marcado bajo Frame Generation. Desde 0.3.5 la pestaña y el log nombran el paso que falta. La lista
  completa, con lo que hay que desactivar en el juego, está en el FAQ de abajo ("Generación de fotogramas: ¿no gana
  fps?").
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
- **FSR Ray Regeneration** — RX 9000 (RDNA 4); en RX 7000 (RDNA 3) como preview, activa por defecto, con el AMDNR Ray Denoiser como eliminador de ruido (ver abajo); solo en juegos que usan DLSS Ray Reconstruction (Cyberpunk 2077,
  Alan Wake 2), con el juego ejecutando DLSS (spoofing activado), trazado de rayos y Ray
  Reconstruction activados en sus propios ajustes. Neural Rendering se ejecuta entonces después,
  sobre su salida, lo que cuesta más: baja la NR resolution si caen los fotogramas. Desde 0.3.5 sus controles
  están en su propia pestaña **Ray Regeneration**, justo después de Upscaling, que se dibuja mientras Ray
  Regeneration se ejecuta en el título; desde 0.3.5.3 la pestaña Neural no tiene ninguna línea de Ray Regeneration, solo la línea naranja bajo el runtime cuando Ray Regeneration está apagado en un título o no pudo arrancar en esta GPU. La fila **Denoiser backend** de la pestaña (`[FSR-RR] RrBackend` = auto / amd / amdnr / off) ofrece Automatic (AMD FSR Ray Regeneration en RX 9000, el AMDNR Ray Denoiser (preview) en las demás tarjetas), AMD FSR Ray Regeneration, AMDNR Ray Denoiser (preview) y Off (the game's own); los tres primeros cambian con el juego en marcha, y Off le dice al juego que Ray Reconstruction no está soportado, para que conserve su propio denoiser (en el siguiente inicio del juego).
  En un título **Vulkan**, Ray Reconstruction es "not supported" por diseño (el eliminador de ruido es D3D12) y la
  pestaña lo dice. El **perfil path-traced** (menos grano en las caras con path tracing) es opcional desde 0.3.3.1:
  márcalo ahí para probarlo en Resident Evil Requiem o PRAGMATA. La misma pestaña tiene la intensidad de la bias
  mask y el **suavizado de piel** (experimental, para juegos que publican una guía SSS; desactivado por defecto, pero
  activado por defecto en Resident Evil Requiem desde 0.3.3.2); los controles de ajuste temporal están en *More Ray
  Regeneration options*, y la vista de depuración de RR y la cifra de ruido (grano de entrada y de salida, parpadeo
  con la cámara quieta) están en el bloque Diagnostics propio de la pestaña. En RX 7000 (RDNA 3) Ray
  Regeneration se ofrece como preview (0.3.5.1; 0.3.5 no lo ofrecía por defecto): el juego puede activar Ray
  Reconstruction y los ajustes de Ray Regeneration se muestran, y el AMDNR Ray Denoiser (preview) le quita el ruido, también en las APU RDNA 3 / 3.5 (AMD FSR Ray Regeneration está hecho para RDNA 4); la página de Ray Regeneration dice qué
  está en marcha. La casilla de la pestaña Upscaling **Preview: Ray Regeneration on this card (restart)** (con la
  etiqueta "preview - on by default") está marcada por defecto; desmárcala, o pon
  `[FSR-RR] FfxDenoiserAllowPreRdna4=false`, y el juego conserva su propio eliminador de ruido. RX 6000 y anteriores lo reciben solo como opción, con `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (pestaña Upscaling: **Offer Ray Regeneration on this GPU (restart)**, con la etiqueta "preview"), también con el AMDNR Ray Denoiser. **Enfoque después de RR** (0.3.4.1):
  cuando el juego no envía ningún valor de nitidez, AMDNR aplica un enfoque de 0.25 después de RR en Windows (0 en
  Linux / Proton); para desactivarlo: Image > Sharpness, marca Override, control deslizante a 0. Desde 0.3.4.2 ese número es su propia
  clave del ini, `[Sharpness] RrDefaultSharpness` (mismo valor por defecto 0.25): pon ahí 0.15, 0.10 o 0 sin tocar
  Override, y un valor que tu ini guardó en `[Sharpness] Sharpness` con Override desactivado se marca en el menú
  como pendiente.
- **AMDNR Screen-space GI by 3zwr1** (desactivado por defecto; Neural > Screen-space GI, o `[AmdGi] Enabled=true`) - la luz rebotada y la oclusión ambiental en espacio de pantalla propias de AMDNR, a partir de la profundidad del juego, antes de NR, del upscaler y de la interfaz; funciona con NR activado o desactivado; Low / Medium / High / Ultra (Auto por GPU), alrededor de 0.6 ms en High con un render de 1080p en una RX 9070 XT (medido). Es espacio de pantalla: falta la luz que viene de fuera de la pantalla. Ver `CHANGELOG.md`.
- **Model E / Model L** (pestaña Upscaling, nuevo en 0.3.5.6) — opciones experimentales de upscaler hechas por otros
  autores y descargadas por el AMDNR Launcher; Model L funciona con lmxxf o con NR desactivado. Ver "Model E y Model L".
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
archivo y su versión y dice qué hacer. Usa el runtime recomendado, con las tres DLL de pase del mismo zip:
`v0.6.0-Runtime.zip` de Alpha0.3.5.5R4 (176,366,023 bytes, con un SHA256 que empieza por `d2bb1702`; en RX 9000, RX
7000 y RX 6000); `v0.5.1-Runtime.zip` (Alpha0.3.5), `v0.5.0-Runtime.zip` (Alpha0.3.4.2; 149,550,553 bytes, SHA256 que
empieza por `7a49ab0e`), `v0.4.3-Runtime.zip` (Alpha0.3.4.2 y Alpha0.3.4.1; 116,484,918 bytes, SHA256 que empieza por
`07dd7774`), `v0.4.1-Runtime.zip` y `v0.4.0-Runtime.zip` (Alpha0.3.4.1) siguen aceptados (la
`dlssnr_amd_pass1.dll` de `v0.4.1-Runtime.zip` tiene 9,916,928 bytes y su SHA256 empieza por `823063eb`; en
`v0.4.0-Runtime.zip`: 10,027,008 bytes, `d62be3d8`). Builds
soportadas: 0.2.17, 0.3.0, 0.3.1, 0.3.2, 0.3.3, 0.4.0 y los zips de runtime nombrados arriba, hasta 0.6.0. No
instales el instalador propio de danielblnc ni su `dxgi.dll` / `version.dll` / `winhttp.dll` junto a AMDNR: AMDNR
ya ejecuta su runtime. **Una build de danielblnc más nueva que 0.6.0 no la maneja esta release:** la pestaña Neural
nombra el archivo y su versión y lo dice. Con 0.6.0, 0.5.1, 0.5.0, 0.4.3, 0.4.1 y 0.4.0 los ajustes exclusivos de
danielblnc (Network style, Tone curve, Black lift, Game exposure, Fast mode) funcionan; desde 0.3.5.5R4 un `Async=1` en
`dlssnr_on_amd.ini` se ejecuta como mismo fotograma (`[DlssNr] AmdDanielAsync=true` lo deja pasar; ver "Qué hay en los
archivos").

**danielblnc: ¿NR se detuvo tras unos cuantos "timeouts", o la pantalla decía que el controlador de vídeo había
reiniciado la GPU?** Corregido en 0.3.5.6 (probado en Silent Hill 2). Se encontró la causa: con los archivos de Model L
en la carpeta del juego, el runtime de danielblnc recibía la copia de HIP propia de Model L, y su espera nunca veía
terminar el trabajo. AMDNR ahora da el HIP del controlador a todo llamador que no sea Model L. Una espera que se rinde
se reporta como tal y ya no apaga Neural Rendering; no hubo ningún reinicio del controlador, y ese texto ha
desaparecido; y la nota "runtime failed" que lee el Doctor del launcher solo se escribe tras un fallo real. El runtime
de danielblnc va con Model E; usa lmxxf para Model L.

**¿Elegiste FSR 3 / FSR 4 y el juego ejecutó FSR 2.1.2?** Desde 0.3.5.6, una opción de upscaler que falla o no está
lista (Model E, Model L u otra) debería conservar tu propia elección de FSR (FSR 4 / FSR 3.1) en lugar de caer a FSR
2.1.2 (D3D12); FSR 2.1.2 solo se ejecuta cuando lo eliges, o cuando ninguna otra biblioteca FSR puede funcionar, y el
log lo dice. En algunos juegos D3D11 y Vulkan, un upscaler que falla aún puede volver a FSR 2.2. Si aún lo ves, envía
el zip del reporte.

**¿lmxxf no hace nada, o se detiene enseguida, en un PC con gráficos integrados?** Corregido en 0.3.3.2. En
un Ryzen de escritorio con los gráficos integrados activados, un portátil con APU AMD y una Radeon, o un PC
con dos GPU AMD, la GPU del juego a menudo no es el dispositivo HIP 0. lmxxf fallaba entonces en su primer
fotograma (`hipErrorInvalidHandle (400)`, y luego "session is poisoned" en `lmxxf_backend.log`) y se quedaba
apagado. Reemplaza tanto `OptiScaler.dll` (el archivo que renombraste, p. ej. `dxgi.dll`) como
`LmxxfNrRuntime.dll` por los de 0.3.3.2 o posteriores. Aún sin probar en un PC así: si lmxxf sigue deteniéndose, la
pestaña Neural ahora dice por qué; envía `lmxxf_backend.log` y `amd_bridge.log` (lista los dispositivos HIP).

**¿Un PC con gráficos integrados y una Radeon (un Ryzen de escritorio con su GPU integrada activada, o un portátil): NR
nunca arranca, y la pestaña Neural o la línea de GPU nombra la GPU integrada?** El pase neural de AMDNR se ejecuta en la
GPU con la que dibuja el juego. Si Windows inició el juego en la GPU integrada, NR no se ejecuta en tu Radeon en
absoluto. Asigna el juego a la GPU dedicada: Configuración de Windows > Sistema > Pantalla > Gráficos, añade el `.exe`
del juego, Opciones, Alto rendimiento; después reinicia el juego y mira la línea de GPU en Neural > Diagnostics, que
nombra el adaptador en el que se ejecuta NR (`OptiScaler.log` tiene una línea `AMD neural: NR runs on ...` cuando ese
adaptador no es la GPU principal). Visto en Starfield en un Ryzen de escritorio. Desde 0.3.5 la línea de la pestaña
Neural dice cuál de los tres casos es — aún sin runtime, NR ejecutándose **en la GPU integrada** (una APU que un
runtime acepta, como una Radeon 780M junto a una tarjeta Radeon: NR corre ahí, mucho más lento que en la tarjeta), o
un runtime en otro adaptador — y `amd_bridge.log` lista cada adaptador una vez.

**¿La línea de estado de lmxxf dice `c32w=off:nofile` en una RX 9070 / 9070 XT?** Una carpeta antigua
`DLSS5-AMD\native-game-tiled-assets` junto al `.exe` del juego (de una instalación anterior de lmxxf) se usa en
lugar de `LmxxfNrRuntime.pak`. No tiene los kernels c32w, así que lmxxf va a la velocidad de antes. Borra o
renombra la carpeta `DLSS5-AMD`: el pak contiene todo lo que lmxxf necesita. Un `LmxxfNrRuntime.pak` anterior a
0.3.3.2 da el mismo estado; sustitúyelo por el de esta versión.
`fk=fff-` en la misma línea significa lo mismo (un pak antiguo o una carpeta suelta): lmxxf sigue
funcionando, a la velocidad de antes.

**danielblnc: ¿el estilo de NR sigue cambiando cuando la NR resolution deja el 100%?** Sigue abierto de 0.3.4 a
0.3.5, y el valor por defecto no cambia. Al 100%, Residual strength 0.99 da el 99% de 1.00 (corregido en 0.3.3.2); fuera
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
si sigue ocurriendo, envía el zip del reporte.

**¿El menú se abría solo sobre el selector de runtime, los clics no hacían nada, o la tecla del menú lo ocultaba solo
mientras la mantenías pulsada (Assetto Corsa)?** Corregido en 0.3.4.2: la tecla del menú lo abre o cierra una sola
vez por pulsación física (un mensaje de tecla que llega tarde se ignora), los clics y las teclas del menú más cortos
que un fotograma se reproducen, el selector de runtime responde a `1` / `2` / `Enter` / `Esc` y a la X de su barra de
título, y cerrar el menú cuenta como "Decide later"; el selector ya no abre el menú por sí solo. Aún sin
confirmar por el jugador de Assetto Corsa: si sigue ocurriendo, envía el zip del reporte. Para recuperar la tecla del
menú y los clics de 0.3.4.1: añade `DiagInputHooksSkip=presslatch,clickreplay` bajo `[Hotfix]` en tu
`OptiScaler.ini` (sin clave nueva; el ini incluido solo describe la línea en un comentario).
0.3.5 añade dos cosas para Assetto Corsa: AMDNR se aparta de un `nvngx.dll` ajeno en la carpeta del juego y protege
la creación del dispositivo D3D11On12, y un juego DX11 que nunca presenta a través de D3D12 recibe una cola D3D12 de
arranque para el pase neural (`[DlssNr] AmdBootstrapQueue`, auto). Aún sin confirmar por un jugador de Assetto Corsa.

**¿Uncharted: Legacy of Thieves Collection se cerraba unos segundos después de arrancar en una RX 9000 con Neural
Rendering activado?** Corregido en 0.3.5: el juego ejecuta su trabajo en fibras pequeñas de 192 KiB, y la primera
inicialización de HIP (el driver compila sus kernels auxiliares dentro del juego) desbordaba esa pila en el primer
fotograma de NR. El primer uso de HIP del puente neural corre ahora en su propia pila grande (`[DlssNr] BigStackCall`,
auto). Si pusiste `[DlssNr] Enabled=false` ahí para 0.3.4.2, vuelve a activarlo. Aún sin confirmar por un jugador con
RX 9000: si sigue ocurriendo, envía el zip del reporte.

**¿Un juego con resolución dinámica (The Last of Us Part II) parpadea con Neural Rendering activado?** Corregido en
0.3.5: el juego cambiaba su tamaño de render cientos de veces por minuto, y cada paso reconstruía la red y reiniciaba
su historia. Un paso que se queda dentro de la asignación ahora se mantiene (sin asentamiento, sin calentamiento, sin
reinicio de la historia, ambos runtimes); un fotograma mayor o una caída real siguen reasignando. La línea de
estadísticas del reporte cuenta los pasos mantenidos. Aún sin confirmar en ese juego.

**Generación de fotogramas: ¿no gana fps, o "restart the game" para siempre?** En casi todos los reportes, la
generación de fotogramas simplemente no estaba activada — apagada, no rota. Lleva cinco pasos, y desde 0.3.5 la
pestaña Frame Gen y el log nombran el que falta:

1. Pestaña Frame Gen: elige **los dos**, FG Input y FG Output. Un juego DX12 con generación de fotogramas propia: su
   DLSS FG o FSR 3.1 FG es la entrada (juegos con FSR 3.1 FG: "FSR 3.1 FG", no "FSR 3.0 FG"); sin ningún FG en el
   juego: FG Input = OptiFG (Upscaler), HUD fix activado. DX11: solo OptiFG. Vulkan: sin salida FSR FG / XeFG (usa la
   del propio juego).
2. **Save Settings**.
3. **Cierra el juego por completo y vuelve a iniciarlo** — FG no puede activarse con el juego ya en marcha.
4. En las opciones gráficas del propio juego, activa su generación de fotogramas: DLSS Frame Generation (con DLSS
   como upscaler) para la entrada DLSSG, generación de fotogramas FSR (con FSR) para la entrada FSR 3.1 FG.
5. Abre el menú otra vez, pestaña Frame Gen, marca **Active** bajo Frame Generation. No se genera nada hasta que esa
   casilla está marcada (XeFG puede pedir un reinicio más).

Después, desactiva **en el juego**: la pantalla completa exclusiva (XeFG necesita ventana sin bordes), V-Sync y los
límites de fotogramas (o limita al doble de tus fps base), y la generación de fotogramas XeSS propia del juego si la
tiene (un solo generador de fotogramas por ventana; un juego que carga su propio XeSS FG recibe una nota en la
pestaña). Sin ganancia de fps = FG está apagada — el log dice
`... Enabled is off ...: no frames are generated. Frame generation off, not broken.`; la mitad de fps = un límite o
el V-Sync está reteniendo los fotogramas generados. Si aun así falla, pulsa **Save report** después del fallo y
publica el zip con el juego, el par que elegiste, qué opción de generación de fotogramas del juego estaba activada,
ventana sin bordes o pantalla completa, HDR activado o no, y lo que viste. El FAQ completo está fijado en el canal
de soporte de Discord.

**¿Otros mods (RED4ext o Cyber Engine Tweaks de Cyberpunk 2077) o ReShade junto a AMDNR?** Desde 0.3.5 el reporte y
el log nombran los cargadores proxy de los otros mods (`Mod loaders:` en `report.txt`; RED4ext es `winmm.dll`, Cyber
Engine Tweaks es `version.dll` a través de un cargador ASI) y avisan cuando AMDNR ocupa el nombre de un cargador
conocido de este juego sin encadenarlo. El AMDNR Launcher nunca toma ni mueve el archivo de un cargador: elige otro
nombre de proxy (Cyberpunk 2077: `dxgi.dll`), y su Doctor nombra cualquier cargador que un INSTALL anterior apartó
(`AMDNR_backup`). ReShade junto a AMDNR se detecta por el recurso de versión del módulo o por las exportaciones de
add-on de ReShade, nunca por un nombre de archivo, y se nombra en el reporte y en el log
(`[Game] ReShade detected: <module>`); no se carga, engancha ni bloquea nada, y que los dos compartan el dispositivo
D3D12 está diseñado, pero aún no construido.

**¿`No HIP adapter matches D3D12 LUID` en la pestaña Neural o en `amd_bridge.log`?** Desde 0.3.5 la línea nombra
ambos lados — el adaptador D3D12 del juego (nombre, LUID), cada dispositivo HIP (ordinal, nombre, gfx, LUID) — y la
causa probable con qué hacer: el juego corre en la GPU integrada o en otra tarjeta (Configuración de Windows >
Sistema > Pantalla > Gráficos: asigna el juego a la GPU dedicada), un runtime HIP sin identidad (un `amdhip64_7.dll`
suelto junto al juego: quítalo), ningún dispositivo HIP (instala el controlador Adrenalin propio de AMD), la misma
tarjeta bajo otra identidad, o un adaptador que no es AMD. Ambos runtimes escriben el mismo texto.

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

**¿No encuentras los ajustes de Ray Regeneration?** Desde 0.3.5 están en su propia pestaña **Ray Regeneration**,
justo después de Upscaling, que se dibuja mientras Ray Regeneration se ejecuta en el título (y se conserva el resto
de la partida una vez que se ha ejecutado); desde 0.3.5.3 la pestaña Neural no tiene ninguna línea de Ray Regeneration. Mientras no se ejecuta, la pestaña no se dibuja; si Ray Regeneration está apagado en este título o no pudo arrancar en esta GPU, lo dice la línea naranja bajo el runtime de la pestaña Neural. En RX 7000 y en las APU RDNA 3 / 3.5 Ray Regeneration se ofrece como preview, activa por defecto, y el AMDNR Ray Denoiser (preview) le quita el ruido (AMD FSR Ray Regeneration está hecho para RDNA 4); `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (o la casilla de la pestaña Upscaling **Preview: Ray Regeneration on this card (restart)**, desmarcada) apaga la preview para que el juego conserve su propio eliminador de ruido. En RX 6000 y anteriores no se ofrece por defecto: la casilla de la pestaña Upscaling **Offer Ray Regeneration on this GPU (restart)** (`[FSR-RR] FfxDenoiserAllowPreRdna4=true`) lo ofrece, con el AMDNR Ray Denoiser (preview). Para ponerlo en marcha, en los ajustes gráficos
del juego: elige **DLSS** como escalador (no FSR, no XeSS), activa el
**trazado de rayos** o el path tracing, y activa **Ray Reconstruction** (DLSS-RR); la pestaña Upscaling dirá entonces
"FSR Ray Regeneration". Qué ajuste cambiar para cada problema está en la guía de ajustes de Ray Regeneration
**RR-BEST-SETTINGS.md** (no está en el zip).

**¿Ray Regeneration se ve con grano o ruido?** Júzgalo primero con **Neural Rendering desactivado** (desmarca **Enable Neural Rendering** arriba en la pestaña Neural, pulsa Home mientras juegas, o pon
`[DlssNr] Enabled=false`): el pase neural se ejecuta después de Ray Regeneration, sobre
su salida, así que una captura hecha con NR activado no dice nada sobre el eliminador de ruido. Después, según el tipo
de grano — grano que repta en una escena quieta, puntos brillantes, grano en las caras, rastros detrás de personajes
en movimiento — los ajustes que probar están en esa misma guía, **RR-BEST-SETTINGS.md**. **Ningún valor
por defecto del eliminador de ruido ni del enfoque cambió en 0.3.4.2**: los números son los de 0.3.4.1. Lo que sí
cambió: el enfoque que AMDNR añade después de Ray Regeneration ya es su propia clave del ini,
`[Sharpness] RrDefaultSharpness` (mismo valor por defecto 0.25), así que 0.15, 0.10 o 0 es una edición del ini y no
una compilación nueva. Revisa tu ini
antes de perseguir el grano con el control de nitidez: un valor en `[Sharpness] Sharpness` no hace nada mientras
`OverrideSharpness` esté desactivado, y se aplica en el momento en que marcas **Override** en el menú; por eso Image > Sharpness ahora lo marca
("ini Sharpness 1.00 waits for Override"). Dos cosas que no
vamos a disimular: una parte del grano es el propio muestreo de rayos del juego — el eliminador de ruido de AMD no está
hecho para reparar ruido que llega correlacionado, y un juego que ofrece DLSS Ray Reconstruction apaga su propio
denoiser y nos entrega la señal cruda — y el grano que repta en una escena quieta tiene una causa estructural de
nuestro lado que ningún control elimina por completo. Ese es un problema conocido. 0.3.5 le pone un número: el bloque
Diagnostics de la pestaña Ray Regeneration muestra el grano de entrada y de salida y el parpadeo con la cámara quieta
(medidos mientras la pestaña está abierta), y la fila **Denoiser backend** de la pestaña puede ponerse en Off, lo que
le dice al juego que Ray Reconstruction no está soportado para que conserve su propio denoiser (tras un reinicio). Desde 0.3.5.3 la misma fila ofrece también el eliminador de ruido propio de AMDNR, el AMDNR Ray Denoiser (preview), que Automatic usa en todas las tarjetas menos en RX 9000.

**¿El Ray Reconstruction del juego está activado pero la pestaña Neural dice "Ray Regeneration is off in
this title"?** El juego no publica lo que FSR Ray Regeneration necesita: su plugin DLSS pasa matrices de cámara
vacías (Satisfactory), que el Ray Reconstruction de NVIDIA trata como opcionales y FSR Ray Regeneration necesita.
El escalado FSR corre en su lugar y NR toma su posición habitual antes del SR; la pestaña Upscaling dice lo
mismo. Desde 0.3.4 se queda apagado toda la sesión en un título Unreal con esta firma. Desactiva Ray
Reconstruction en el juego y restaura los ajustes de denoiser propios del motor.

**¿Un juego de Ubisoft Anvil (AC Black Flag Resynced, Shadows, Mirage) muestra "DX12 Error 0x80070057"?**
Esos juegos llevan su propia generación de fotogramas XeSS. Desde 0.3.5 la pestaña Frame Gen muestra ahí una nota de
consejo (deja desactivado el XeSS FG propio del juego, o dos generadores compartirán una misma ventana) y la salida
XeFG de AMDNR sigue funcionando; la ruta más sencilla es la opción XeSS FG del propio juego con la generación de
fotogramas de AMDNR desactivada. Si sigue ocurriendo, pon `[FrameGen] Enabled=false` y `[fakenvapi] ForceXeLL=false`
y repórtalo con el log.

**¿The Last of Us Part I se cierra al arrancar?** Es la propia inicialización de Streamline del
juego, un problema conocido de OptiScaler: renombra `sl.common.dll` en la carpeta del juego a
`sl.common.dll.bak` y elige **FSR 3.1** en los ajustes del juego en lugar de DLSS.

Notas completas de cada versión: `CHANGELOG.md` (en el zip y en el repositorio).

## Hoja de ruta

- **0.3.5.6** (esta build) — **Model E** (HelixSR de lonewolf0622) y **Model L** (d4r de countervolts) como opciones
  de upscaler experimentales y opcionales, descargadas de las propias releases de sus autores (Model L: RX 9070 / 9070
  XT, con lmxxf o con NR desactivado); el Full Model Look como estilo de NR por defecto, después del escalado donde la
  tarjeta puede pagarlo; más detalle en las caras; menos ghosting en lmxxf; Fast mode por defecto en RX 7000 y en las
  consolas portátiles; Game exposure (Auto / On / Off) y Anti-halo; NR resolution
  hasta el 200%; un menú que oculta lo que un runtime no puede ejecutar; Refine passes en danielblnc; la corrección de
  los falsos timeouts de danielblnc; FSR 3 / 4 conservado en lugar de FSR 2.1.2; el recuento de memoria de XeFG; AMDNR
  Launcher 0.3.5.25.
- **0.3.5.1 a 0.3.5.5R4** — danielblnc 0.6.0 (también en RX 6000, con el SDK de HIP de AMD); lmxxf 0.41 en RX 9000;
  el NVIDIA look; el AMDNR Ray Denoiser (preview) y AMDNR Screen-space GI (0.3.5.3); una protección contra
  congelaciones tras fallos de GPU contabilizados; correcciones de cierres a partir de tus reportes; el AMDNR Launcher
  rediseñado con ASK AI. Ver `CHANGELOG.md`.
- **0.3.5** — **AMDNR Anywhere** (preview, RX 9000, a través del launcher); la pestaña Ray Regeneration
  con líneas de estado por tarjeta y por API, la fila Denoiser backend en Off y la cifra de ruido; el pase neural
  después del escalado (`AmdPlacement`); la pestaña Frame Gen y el log dicen por qué no se genera nada; lmxxf 0.37
  de Kien (MIT) en RX 9000; el conjunto de módulos de 0.3.5 (alrededor de un 10% más rápido en RX 7000 y en las
  consolas portátiles de la clase Z1 Extreme, con la misma imagen); los pasos de resolución dinámica mantenidos
  sin reconstruir la red; la corrección del
  transporte en portátiles; la carpeta de runtime compartida (`AmdRuntimePath`); el rechazo del adaptador HIP que
  nombra ambos lados; los cargadores de otros mods y ReShade nombrados en el reporte; los procesos de anti-cheat y de
  reporte de cierres se dejan en paz; correcciones para Uncharted (RX 9000), Assetto Corsa, F1 25 y Kingdom Come:
  Deliverance II (Game Pass), Control Resonant con generación de fotogramas, Tainted Grail: The Fall of Avalon y Dead
  Space, GTA V Enhanced, Half-Life 2 RTX y otros títulos Vulkan, y los textos de Linux / Proton; AMDNR Launcher
  0.3.5.1.
- **0.3.4.2** — hotfix: el menú de Assetto Corsa (la tecla del menú lo abre o cierra una sola vez por
  pulsación, los clics más cortos que un fotograma se reproducen, el selector de runtime responde a las teclas y cerrar
  el menú cuenta como "Decide later"), el selector de runtime ya no abre el menú por sí solo, la sección Ray
  Regeneration está siempre en la pestaña Neural y dice por qué no se ejecuta, un layout de runtime de
  danielblnc más aceptado, correcciones del texto de
  Wine / Proton, añadidos al README (nombres de proxy, Uncharted, PC híbridos, `AmdLmxxfTierSnap` en RX 9000, las dos
  entradas del FAQ sobre Ray Regeneration); NR
  idéntico byte a byte a 0.3.4.1 salvo esa única fila de layout aceptado.
- **0.3.4.1** — hotfix: enfoque de Ray Regeneration cuando el juego no envía ningún valor de nitidez
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
- **0.3.6** — RX 6000 (RDNA 2) con kernels propios de AMDNR (el runtime 0.6.0 de danielblnc ya funciona ahí): tras
  una puerta de hardware, con el nivel 360 y Model interleave como preview en Navi 21; AMDNR Anywhere en RX 7000 cuando
  esté probado ahí. (El AMDNR Ray Denoiser, el eliminador de ruido propio de AMDNR detrás de la llamada de Ray
  Reconstruction, ya se distribuye como preview desde 0.3.5.3.)
- **0.4.0** — el nivel de red 1440p; Ray Regeneration en títulos Vulkan a través del puente; Neural Rendering entre
  adaptadores (el juego en una tarjeta, la red en la tarjeta AMD); Anywhere más allá de la preview (títulos sin
  upscaler propio, donde OptiScaler aporta el upscaler y el pase neural juntos).
- **Más adelante** — AMDNR en cualquier ventana (el escritorio).

---

## Créditos

Esta build es un trabajo de cableado sobre el trabajo de otras personas. Si te resulta útil, el
agradecimiento pertenece a upstream.

- **TheAutomatic** — DLSS 5 AMD project — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD by Daniel Blanco — https://github.com/danielblnc/DLSS-NR-on-AMD (los archivos `*Runtime.zip`, sin modificar)
- **lmxxf** (Kien) — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (el port de la red, los kernels y el runtime HIP, MIT)
- **TheAutomatic** — `LmxxfNrRuntime.cpp`, `LmxxfNrApi.h`, `LmxxfProductionOptions.h`: portions contributed to lmxxf by TheAutomatic (MIT)
- **kernels de lmxxf 0.31** en `LmxxfNrRuntime.pak` (the ViT projection (lmxxf031-vit-wide-deep), the C512 QKV and mix kernels (lmxxf031-c512-m32-mh, lmxxf031-c512-m32-deep) and one-wave-per-head attention (lmxxf031-c64-wave2)) — de lmxxf (Kien, MIT), compilados por AMDNR a partir de las fuentes y la receta de compilación de lmxxf; la parte de AMDNR es la carga, los pines SHA-256, el filtrado por GPU y las alternativas
- **kernels de lmxxf 0.37, 0.39 y 0.41** en `LmxxfNrRuntime.pak` (los módulos lmxxf037-*, lmxxf039-* y lmxxf041-* para RX 9000) y su código de lanzamiento en `LmxxfNrRuntime.dll` — lmxxf 0.37 / 0.39 / 0.41 by Kien (MIT), distribuidos tal como lmxxf los compiló; la parte de AMDNR es la carga como un único grupo fijado, los pines SHA-256, los valores por defecto por GPU, las opciones para desactivarlos y las alternativas
- **kernels c32w** (0.3.3.2) — kernels RDNA 4 de una wave propios de AMDNR para la red de lmxxf, Copyright (c) 2026 3zwr1 (AMDNR); ideas de la documentación pública de WMMA de RDNA 4 de AMD (GPUOpen, ROCm matrix instruction calculator)
- **El backend RDNA 3 de AMDNR** (0.3.3; las builds para portátiles gfx1103 / gfx1150 en 0.3.4), la política de niveles de tamaño de red y los tamaños de red pequeños (0.3.4) — Copyright (c) 2026 3zwr1 (AMDNR)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Dagherbou / OptiScaler_DLSSNR** — https://github.com/Dagherbou/OptiScaler_DLSSNR
- **wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass** — https://github.com/wilsjo2/OptiScaler-DLSSNR-PreSR-Multipass
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, sin modificar)
- **HelixSR** — lonewolf0622 — https://github.com/lonewolf0622/HelixSR (Model E, 0.3.5.6: descargado por el AMDNR Launcher de su propia release y usado bajo su propia licencia; AMDNR no lo distribuye; construye su red en tu PC a partir del archivo DLSS de NVIDIA, que el launcher descarga del propio GitHub de NVIDIA o toma de tu juego)
- **d4r** — countervolts — https://github.com/countervolts/d4r (Model L, 0.3.5.6: el runtime DLSS de NVIDIA bajo el EULA de NVIDIA, ejecutado a través de **ZLUDA** (vosen, https://github.com/vosen/ZLUDA) y HIP; descargado por el AMDNR Launcher de la propia release de d4r, bajo la propia licencia de d4r; AMDNR no lo distribuye; su autor no lo ha probado en Windows)
- **OptiPatcher** — de las releases del proyecto OptiScaler, https://github.com/optiscaler/OptiPatcher (lo descarga GAME SETTINGS del AMDNR Launcher cuando lo pides; AMDNR no lo distribuye)
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0)** — el host de captura de ventana dentro del que se ejecuta AMDNR Anywhere, en la build propia de AMDNR publicada en las releases de AMDNR en GitHub (`Magpie.exe` recompilado con los cambios de AMDNR; la oferta de código fuente, un único diff respecto a la etiqueta del fork y las opciones de compilación a su lado, en `Anywhere-host-source/`; ningún archivo de NVIDIA) — https://github.com/Blinue/Magpie (el fork: https://github.com/SAOG0721/Magpie)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (matemática de la composición de color, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), la base del desbloqueo multifotograma de XeFG integrado y de su ritmo
- **Zach Hembree (DarkHelmet)** — FSR Ray Regeneration para OptiScaler, el origen de la ruta de Ray Regeneration de AMDNR, continuado por **burak113**, de cuya rama AMDNR lo portó (rama de OptiScaler ffx-denoise-experimental, GPL-3.0)
- **Screen-space GI** (el efecto heredado; retirado del menú en 0.3.4, `[AmdRtgi] Enabled` en el ini) — un efecto que AMDNR heredó del linaje OptiScaler-AMD-PreSR; el mérito es de sus autores originales. Necesita la carpeta `experimental_lighting` del paquete de danielblnc, que AMDNR no distribuye.
- **AMDNR Screen-space GI by 3zwr1** - obra propia de AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), escrita a partir de investigaciones publicadas (que se citan en `CHANGELOG.md` y `Licenses/AMDNR_NOTICE.txt`).
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## AMDNR Launcher

**AMDNR Launcher** (nuevo en 0.3.4) es un programa de Windows 10 / 11 que también puede funcionar bajo Proton en
Linux (experimental, aún no lo hemos probado: ver "Linux / Proton"). Instala y actualiza AMDNR juego por juego:
encuentra tus juegos (Steam, Epic, la app de Xbox, Ubisoft Connect, la app de EA, GOG, Rockstar, Battle.net y Amazon
Games), elige el nombre de la DLL, descarga la build y el runtime de danielblnc que elijas (y, cuando los eliges, Model
E y Model L de las propias releases de sus autores), comprueba cada instalación con su Doctor, ejecuta AMDNR Anywhere en los juegos sin upscaler propio (PLAY ANYWHERE) y se actualiza a sí mismo.
Descarga `AMDNR-Launcher.exe`, el AMDNR Launcher de la release más reciente, en la página de releases:
<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>

**Novedades del Launcher 0.3.5.25** (en la release Alpha0.3.5.6; primero se actualiza a sí mismo, después tus juegos):

- **SR model** en GAME SETTINGS de un juego: Auto / Model E / Model L (ver "Model E y Model L"). La primera vez que
  eliges un modelo se muestra su aviso (ACCEPT / Cancel; Cancel es la opción por defecto); después el launcher descarga
  sus archivos de la propia release del autor, verificados por hash. Model L va con el runtime lmxxf (o con Neural
  Rendering desactivado); con el runtime de danielblnc el launcher lo rechaza y dice por qué. Auto vuelve a quitar los
  archivos de Model L.
- Filas **Game exposure** (Auto / On / Off) y **Anti-halo** en GAME SETTINGS, que solo se escriben en una build de
  AMDNR que las lee.
- **Ajustes de Anywhere** en la página ANYWHERE del launcher: runtime de NR, pases, tamaño neural, FSR (Auto / FSR 4 /
  FSR 3 / Off — Off conserva NR), generación de fotogramas (experimental), movimiento para el escalado, nitidez y SR
  model; se aplican en el siguiente PLAY ANYWHERE.
- Aspecto **Most FPS**: ejecuta la red un tamaño más pequeño (lmxxf), con un tamaño neural menor; no se salta ningún
  fotograma neural (Refine passes en danielblnc cuando se ejecuta más de un pase).
- **Public Chat** en Community Lab: los mensajes son públicos y los revisa una IA, y pueden traducirse automáticamente
  al idioma de tu launcher; el equipo puede eliminar mensajes.
- El runtime que eliges para un juego se conserva para ese juego.

**Novedades del Launcher 0.3.5.1** (en la release Alpha0.3.5; primero se actualiza a sí mismo, después tus juegos):

- **PLAY ANYWHERE** en un juego sin upscaler propio (ver "AMDNR Anywhere"): el launcher descarga el host de captura
  de la release de AMDNR en GitHub, inicia el juego y ejecuta Neural Rendering sobre su ventana, con un resumen de solo
  lectura de los ajustes del host junto al botón (los ajustes en sí están en la pestaña Anywhere del menú). RX 9000
  en esta release. Los juegos en los que el mod no puede cargarse (32 bits, DirectX 9 / OpenGL sin upscaler) se
  rechazan en INSTALL con una línea clara y se les ofrece Anywhere.
- **UPDATE ALL** y actualización al arrancar; espejos de paquetes; RETRY / OPEN DOWNLOAD / IMPORT PACKAGE cuando
  falla una descarga.
- **COLLECT LOGS** activa el manejador de cierres inesperados durante una ejecución y adjunta `amdnr_crash.log`, el
  volcado más reciente y el `dlssnr_on_amd.ini` de danielblnc.
- **PLAY** inicia los juegos de Xbox / Microsoft Store por su id de app.
- **Un solo PLAY** con selector de ruta (el upscaler propio del juego o AMDNR Anywhere, recordado por juego) y la
  API gráfica (DX9 / 10 / 11 / 12 / Vulkan / OpenGL) en la página de cada juego.
- **Doce idiomas**: el turco, el coreano y el húngaro se suman a los nueve de abajo (ahora trece: también el uzbeko).
- **Los juegos de Rockstar se inician a través de su tienda** (Steam, Epic o el Rockstar Games Launcher), nunca por
  su exe.
- **AMDNR Anywhere**: el juego corre con una prioridad de GPU por debajo de la normal para que el host y Neural
  Rendering vayan primero; el icono de la bandeja del host de captura queda oculto.
- **Resident Evil 2 / 3 / 4 (2023) / Village**: un aviso de configuración (necesitan el plugin de upscaler de
  PureDark con REFramework). Más juegos sin DLSS, XeSS ni FSR 2+ se marcan como no compatibles, con el motivo,
  antes de cualquier descarga.
- **FSR 4 (INT8)** como opción experimental que se activa a mano en la página del juego en las consolas portátiles
  RDNA 3.
- **Los cargadores de otros mods nunca se toman ni se mueven** (RED4ext, Cyber Engine Tweaks y similares): el
  launcher elige otro nombre de proxy y su Doctor nombra cualquier cargador que un INSTALL anterior apartó.
- **GTA V Enhanced**: la página del juego lleva la línea de ruta (el FSR 3.1 elegido en el juego es la entrada;
  Neural Rendering se ejecuta antes) y una nota sobre `settings.xml`.

**Novedades del Launcher 0.3.4.1: lo pediste, lo hicimos** (a partir de los primeros comentarios en Discord):

- Nueve idiomas (doce desde 0.3.5.1, ahora trece): inglés, árabe, chino (simplificado), francés, español, portugués,
  italiano, ruso y polaco, más turco, coreano, húngaro y uzbeko. El launcher sigue el idioma de tu Windows (si no es uno de estos, inglés);
  elige otro en LANGUAGE (IDIOMA) o en
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

El `nvngx_dlssnr.dll` de NVIDIA no está en estos archivos, ni tampoco ningún otro archivo de NVIDIA. El paquete d4r de
Model L (el runtime DLSS de NVIDIA bajo el EULA de NVIDIA) y el archivo DLSS de NVIDIA a partir del cual construye
Model E solo los descarga el AMDNR Launcher cuando los eliges, de la propia release de d4r y del propio GitHub de
NVIDIA (o de tu juego); AMDNR no los aloja ni los distribuye, y Model E y Model L son obra de sus autores, no de AMDNR.
AMDNR no está afiliado a AMD ni a NVIDIA ni cuenta con su respaldo. Nada de esto está respaldado, afiliado ni
soportado por NVIDIA, AMD ni ningún editor de juegos. Maneja directamente una función no documentada.
Úsalo bajo tu propio riesgo.


## Runtime update - 2026-10-22

AMDNR 0.3.5.7 Public View - lmxxf 0.4.4 integration

Kien/lmxxf (MIT), integrated with AMDNR settings and own host lifecycle.
Includes Model A/B, C32 W16h, C512 W16, paired byte edges, variable Small
geometry, corrected adaptive Small token stride and uploads before producer waits.

New in this complete local package:
- Separate A/B networks retain their weights while switching at unchanged
  geometry, controls and tier policy. At most two networks belong to one bridge.
- Shared D3D input/history/output buffers and image codecs remain in place.
- Temporal history and adaptive anchors reset on every model change.
- Geometry/policy/control changes release the cache and rebuild correctly.
- If allocating the second network reports out-of-memory, release the previous
  network through the established drained teardown before loading the new model.
  Other errors propagate; GPU completion failures are never bypassed.

Qualification: 132 temporal model switches, final output byte-identical to a
fresh model, VRAM stable about2059MB from frame16 through512 in both cycle runs.
Two-pass model-switch tests and live scale tests accompany this delivery.
The cache uses additional VRAM to retain the second model; it is bounded, not
a claim that long-session VRAM leaks or every game issue have been eliminated.
First loading each model still takes time. Revisited-model timings are recorded
in the local qualification evidence; no steady-state game FPS claim is made.

Includes unchanged signed launcher0.3.5.35 and separate unchanged Daniel0.8.0
runtime and mochi0.0.7 component downloads, with schema1 server/GitHub manifests.
Mochi RDNA3 remains deferred. NVIDIA DLLs/weights are not included.

This is AMDNR's clean-room integration, not every upstream implementation copied
verbatim. Excluded vit-stream/VBE modules remain excluded. AMDNR's own temporal
pipeline remains. Model A retains its configured resolution ceiling.
No upload or live manifest activation was performed.
