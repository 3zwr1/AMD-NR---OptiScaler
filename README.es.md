# AMDNR — DLSS 5 Neural Rendering en AMD (build de OptiScaler) — v0.3.1

[English](README.md) | [中文](README.zh-CN.md) | [Português](README.pt-BR.md) | **Español**

> **Necesitamos tu apoyo.** Únete al servidor de Discord — <https://discord.gg/QzbzxfKYyh> — para
> ayuda, reportes de errores y builds de prueba; cada reporte con un log hace mejor la siguiente build.

DLSS 5 Neural Rendering funcionando en GPUs AMD, integrado en OptiScaler para que funcione en
cualquier juego Direct3D 12 que OptiScaler ya engancha. Sobre el pase neural: model interleave para
una gran ganancia de fotogramas, composición residual, generación de fotogramas XeSS desbloqueada
hasta 6X, y FSR Ray Regeneration para los juegos que usan DLSS Ray Reconstruction.

**Discord: <https://discord.gg/QzbzxfKYyh>** — soporte, reportes de errores (`#bug-report`), builds
de prueba. Si necesitas el `nv` de NV para algo, está disponible allí; no viene en
estos archivos y la ruta AMD no lo necesita.

**Apoya el proyecto: <https://ko-fi.com/3zinr>**

---

## AMDNR - Guía de instalación de OptiScaler

La instalación es bastante sencilla.

### 1. Descarga los archivos

Descarga estos dos archivos desde GitHub:

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
> `<Juego>\Binaries\Win64\`.

---

### El runtime lmxxf (0.3.0, opcional)

Un segundo runtime neural (licencia MIT, de lmxxf) puede ejecutar el pase en lugar del de
danielblnc. Solo RDNA 4. Necesita dos cosas junto al juego:

1. `LmxxfNrRuntime.dll` - en este archivo, junto a `OptiScaler.dll` (se copia con el resto).
2. `LmxxfNrRuntime.pak` (382 MB, incluido en el zip de AMDNR) junto a `LmxxfNrRuntime.dll` - los
   pesos, módulos HIP y HLSL de lmxxf en un solo archivo cifrado y autenticado. El runtime lo abre
   en memoria; nada se desempaqueta en disco. La distribución antigua en carpeta sigue funcionando

En el primer inicio que encuentra un runtime instalado y ninguna elección hecha, el menú pregunta
cuál usar (`[DlssNr] NrBackend = daniel | lmxxf` en el ini lo registra; Neural > Neural runtime lo
cambia, en el siguiente inicio del juego). La edición de lmxxf se aplica un fotograma después,
transportada por los vectores de movimiento, así que el fotograma nunca espera a la red (unos 30 ms
a 1080p, 15 ms a 720p en una RX 9070 XT). Su log es `lmxxf_backend.log` junto al juego.

**Compatibilidad (lmxxf).** El runtime solo ve lo que ve DLSS, así que lo que varía por título es
una lista corta: formato de color y HDR, vectores de movimiento y su escala, profundidad y su
dirección, la máscara reactiva, la textura de exposición, la bandera Reset, y dónde se sitúa el
pase (antes de Super Resolution, o después de Ray Reconstruction). Probado hasta ahora:

| Título | API / posición | Notas |
|---|---|---|
| Silent Hill 2 | D3D12, antes de SR | título de referencia; manejada la asignación de color con relleno de Unreal |
| Forza Horizon 6 | D3D12, antes de SR | |
| Stray | D3D11 a través del puente D3D12, antes de SR | |
| GTA V Enhanced | D3D12, antes de SR, HDR, máscara reactiva de un canal | corregido en esta build: la máscara se leía como "todo reactivo" y la edición nunca llegaba |
| Cualquier título con Ray Reconstruction | D3D12, después de RR (escrita de vuelta en la salida) | soportado desde esta build; aún no confirmado en un juego |

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
Claves del ini: `AmdLmxxfHistory`, `AmdLmxxfEditDetail`, `AmdLmxxfEditSaturation`,
`AmdLmxxfEdgeGuard`, `AmdLmxxfOutputSmooth` en `[DlssNr]`.

## Requisitos

- Una GPU AMD con un controlador actual. El runtime neural usa HIP a través del controlador; no
  hace falta el SDK de HIP ni el modo desarrollador. Qué chips: RX 9000 (RDNA 4) ejecuta ambos
  runtimes; RX 7000 (RDNA 3, escritorio y portátil) solo el de danielblnc; las APU de consolas
  portátiles (Z1 Extreme / 780M, Z2 Extreme / 890M) y RDNA 2 (RX 6000, Steam Deck) aún no están
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
| `LmxxfNrRuntime.dll` | El runtime neural lmxxf (0.3.0). Se usa solo cuando se elige; lee `LmxxfNrRuntime.pak` junto a él, ver "El runtime lmxxf". |
| `LmxxfNrRuntime.pak` | Los pesos, módulos HIP y shaders del runtime lmxxf en un archivo cifrado (382 MB). Solo lo lee el runtime lmxxf; es inofensivo mantenerlo con el runtime de danielblnc. |
| `OptiScaler\` | FSR, XeSS, el denoiser FidelityFX y el D3D12 Agility SDK que usa OptiScaler. |
| `Licenses\`, `LICENSE` | Licencias de terceros y la licencia GPL-3.0 de esta build. |
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
  el cuadrado (150% es 2,25x).
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
- **La generación de fotogramas está apagada en un ini nuevo.** Pestaña Frame Gen: elige el FG
  Input (p. ej. "DLSSG via Streamline" en un juego con generación de fotogramas DLSS) y el FG
  Output (XeFG), luego marca **Active** en la sección Frame Generation (XeFG) y pulsa Save
  Settings. Un ini de 0.1.0 que la tenía activada no se conserva al instalar el ini de 0.2.0.
- **Generación multifotograma XeFG** — 3X a 6X viene integrada y activada por defecto
  (`XeFG\UnlockMFG`), para la copia de OptiScaler y la del propio juego. **Borra `XeFGUnlock.asi`**
  de `OptiScaler\plugins` si aún lo tienes: dos copias del mismo parche hacen que el juego se cierre.
- **FSR Ray Regeneration** — solo en juegos que usan DLSS Ray Reconstruction (Cyberpunk 2077,
  Alan Wake 2), con el juego ejecutando DLSS (spoofing activado), trazado de rayos y Ray
  Reconstruction activados en sus propios ajustes. Neural Rendering se ejecuta entonces después,
  sobre su salida, lo que cuesta más: baja la NR resolution si caen los fotogramas.

## Si algo sale mal

`OptiScaler.log` aparece en la carpeta del juego. Adjúntalo en `#bug-report`, y di qué juego y qué
GPU. El backend AMD también escribe `amd_presr.log` y `amd_bridge.log`, que son los útiles cuando
lo que falla es específicamente el pase neural.

**¿NR frames 0/s, el desplegable del runtime muestra `pass1?` y `amd_presr.log` dice que la DLL del pase
es una build que este OptiScaler no maneja?** Tus `dlssnr_amd_pass1..3.dll` no son la 0.3.1 de danielblnc
(se ha visto circular un juego 0.2.16). Usa el `Runtime.zip` de esta release: `dlssnr_amd_pass1.dll` tiene
7.304.192 bytes y su SHA256 empieza por `b108d640`. Builds soportadas: 0.2.17, 0.3.0, 0.3.1.

**¿The Last of Us Part I se cierra al arrancar?** Es la propia inicialización de Streamline del
juego, un problema conocido de OptiScaler: renombra `sl.common.dll` en la carpeta del juego a
`sl.common.dll.bak` y elige **FSR 3.1** en los ajustes del juego en lugar de DLSS.

Notas completas de esta versión: `RELEASE-NOTES.md` en el repositorio.

## Hoja de ruta

- **0.3.1** (esta build) — correcciones de los primeros reportes de 0.3.0 (lmxxf solo nunca
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

- **DLSS-NR on AMD** — *danielblnc* — <https://github.com/danielblnc/DLSS-NR-on-AMD>
  El runtime neural AMD y los pesos en `Runtime.zip` son su release v0.3.1, redistribuida sin
  modificar. Todo lo que la red realmente calcula es suyo.
- **DLSS 5 AMD project** — *TheAutomatic* — <https://github.com/TheAutomatic/dlss-5-amd-project>
  Base y referencia para DLSS 5 Neural Rendering en hardware AMD; la integración del runtime lmxxf
  distribuida en 0.3.0 sigue su trabajo. Muchas gracias.
- **Matheus / dlss-5-amd** — <https://github.com/MatheusGViana/dlss-5-amd-project>
  El puente AMD pre-SR del que desciende este árbol.
- **lmxxf / dlss5-on-amd-9070xt-porting** — <https://github.com/lmxxf/dlss5-on-amd-9070xt-porting>
  Runtime de renderizado neural HIP de código abierto (MIT); el modo temporal con umbral de
  diferencia de aquí sigue su `native_output_smooth`.
- **OptiScaler** — *Overclockers* y colaboradores — <https://github.com/Overclockers/OptiScaler-Releases>
  El framework en el que esto se integra: el hooking, la fontanería de FSR/XeSS/generación de
  fotogramas, el menú y la compatibilidad con juegos que hace que algo de esto sea alcanzable.

Linaje del código: OptiScaler → Dagherbou / OptiScaler_DLSSNR → wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass
→ Matheus / dlss-5-amd → esta build. El desbloqueo y el ritmo de XeFG están portados del XeFGUnlock
de Coldwood1026 (GPL-3.0); el manejo de gama `CubeScale` es de *hhkbble*.

## Legal

Esta build se distribuye bajo la licencia GPL-3.0 en `LICENSE`; las licencias de las bibliotecas de
terceros están en `Licenses\`. El runtime neural AMD y sus pesos se redistribuyen bajo su autoría
original tal como se acredita arriba, solo por comodidad, sin reclamar propiedad y sin ofrecer
garantía.

El `nvngx_dlssnr.dll` de NVIDIA no está en estos archivos. Nada de esto está respaldado, afiliado ni
soportado por NVIDIA, AMD ni ningún editor de juegos. Maneja directamente una función no documentada.
Úsalo bajo tu propio riesgo.
