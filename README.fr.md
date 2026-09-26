# AMDNR — DLSS 5 Neural Rendering sur AMD (build OptiScaler) — v0.3.3.2

[English](README.md) | [中文](README.zh-CN.md) | [Português](README.pt-BR.md) | [Español](README.es.md) | [العربية](README.ar.md) | **Français** | [Italiano](README.it.md) | [Русский](README.ru.md) | [Polski](README.pl.md)

> **Nous avons besoin de votre soutien.** Rejoignez le serveur Discord — <https://discord.gg/AMDNR> — pour
> l'aide, les rapports de bugs et les builds de test ; chaque rapport accompagné d'un log améliore le build suivant.

DLSS 5 Neural Rendering qui tourne sur les GPU AMD, intégré à OptiScaler pour fonctionner dans n'importe quel
jeu Direct3D 12 dans lequel OptiScaler s'injecte déjà. En plus de la passe neuronale : model interleave pour un
gros gain de framerate, composition résiduelle, frame generation XeSS débloquée jusqu'à 6X (jusqu'à 10X
en option dans les jeux D3D12), et FSR Ray Regeneration pour les jeux qui utilisent DLSS Ray Reconstruction.

**Discord : <https://discord.gg/AMDNR>** — support, rapports de bugs (`#bug-report`), builds
de test.

**Soutenir le projet : <https://ko-fi.com/3zinr>**

> **Le runtime danielblnc est l'œuvre de Daniel Blanco.** Le runtime neuronal AMD contenu dans `Runtime.zip`
> (`dlssnr_amd_pass1..3.dll`) est **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> AMDNR le distribue sans modification, avec son autorisation ; ce n'est pas le travail d'AMDNR. Merci de soutenir son projet.
> Les crédits complets de toutes les autres personnes se trouvent à la fin de cette page.

---

## AMDNR - Guide d'installation d'OptiScaler

L'installation est assez simple.

### 1. Télécharger les fichiers

Téléchargez ces deux fichiers depuis GitHub (<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>) :

* `AMDNR-vX.X.X.zip`
* `Runtime.zip`

### 2. Extraire les deux fichiers

Extrayez le contenu des deux fichiers `.zip`.

### 3. Tout copier dans le dossier du jeu

Copiez d'abord tous les fichiers de `AMDNR-vX.X.X` dans le dossier racine du jeu — le même dossier que
celui où se trouve le `.exe` du jeu.

Faites ensuite de même avec tous les fichiers de `Runtime`.

### 4. Renommer OptiScaler.dll

Dans le dossier du jeu, trouvez :

`OptiScaler.dll`

Renommez-le en :

`dxgi.dll`

`dxgi.dll` est l'option recommandée.

Si le jeu ne se lance pas ou si le mod ne se charge pas, essayez plutôt de renommer `OptiScaler.dll` en
l'un de ces noms :

* `d3d12.dll`
* `winmm.dll`
* `version.dll`
* `dbghelp.dll`

Testez un seul nom à la fois. Ne créez pas plusieurs copies de `OptiScaler.dll`.

> **Resident Evil Requiem (et sa démo) a besoin de REFramework.** C'est une exigence connue, pas un bug d'AMDNR : OptiScaler s'appuie dessus pour
> passer l'anti-tamper de Capcom ([wiki OptiScaler](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-9-Requiem)). Sans lui, le jeu plante 15-60 s
> après le lancement (« An unhandled exception occurred »). Placez `dinput8.dll`, tiré de `REFramework.zip` dans la dernière nightly
> (<https://github.com/praydog/REFramework-nightly/releases>), à côté de `dxgi.dll`, et changez la touche du menu de REFramework (p. ex. pour Suppr / Delete) : elle aussi est sur Insert.
> Après une mise à jour du jeu, attendez-vous à des plantages jusqu'à ce que REFramework soit mis à jour. PRAGMATA, Monster Hunter Wilds et Onimusha en ont probablement besoin aussi (non confirmé).

### 5. Lancer le jeu

La touche `HOME` active ou désactive Neural Rendering en cours de partie (avec les deux runtimes ; une petite notification affiche
On / Off). Réassignez-la à côté de la case Enable dans l'onglet Neural ou dans Interface > Keybinds.

C'est tout.

Lancez le jeu normalement et appuyez sur :

`INSERT`

Cela ouvre le menu OptiScaler / AMDNR, où vous pouvez configurer le mod comme bon vous semble.

### Si ça ne fonctionne pas

Si le jeu ne se lance toujours pas, quel que soit le nom essayé ci-dessus, merci de le signaler dans le salon
`#bug-report` sur Discord.

Quand vous signalez le problème, envoyez aussi tous les fichiers `.log` qui ont pu être générés dans le
dossier racine du jeu.

Ces logs sont très importants et nous aideront à identifier le problème beaucoup plus vite.

> Le `.exe` ne se trouve généralement pas là où pointe le raccourci. Les jeux Unreal le rangent dans
> `<Game>\Binaries\Win64\`.

---

### Le runtime lmxxf (0.3.0, optionnel)

Un second runtime neuronal (sous licence MIT, par lmxxf) peut exécuter la passe à la place de celui de
danielblnc. Il tourne sur RDNA 4, et sur RDNA 3 (RX 7000, Strix Halo) via le backend RDNA 3 d'AMDNR - plus lentement sur ces puces :
commencez avec une NR resolution de 67 %. Il a besoin de deux choses à côté du jeu :

1. `LmxxfNrRuntime.dll` - dans cette archive, à côté de `OptiScaler.dll` (il est copié avec le reste).
2. `LmxxfNrRuntime.pak` (416 Mo, inclus dans le zip AMDNR) à côté de `LmxxfNrRuntime.dll` - les fichiers de
   poids, les modules HIP et le HLSL de lmxxf dans un seul fichier chiffré et authentifié. Le runtime l'ouvre en
   mémoire ; rien n'est extrait sur le disque.

Au premier lancement où un runtime installé est détecté sans qu'aucun choix n'ait encore été fait, le menu demande lequel
utiliser (`[DlssNr] NrBackend = daniel | lmxxf` dans l'ini enregistre ce choix ; Neural > Neural runtime
permet de le changer, avec effet au prochain démarrage du jeu). La retouche de lmxxf est appliquée avec une image de retard, transportée
par les vecteurs de mouvement, de sorte que l'image n'attend jamais le réseau (environ 17 ms en 1080p
sur une RX 9070 XT). Son log est `lmxxf_backend.log`, à côté du jeu.

**Compatibilité (lmxxf).** Le runtime ne voit que ce que voit DLSS, donc ce qui varie d'un titre à l'autre tient
en une courte liste : format de couleur et HDR, vecteurs de mouvement et leur échelle, profondeur et son sens,
le masque réactif, la texture d'exposition, le flag Reset, et l'emplacement de la passe (avant Super
Resolution, ou après Ray Reconstruction). Testé jusqu'ici :

| Titre | API / emplacement | Remarques |
|---|---|---|
| Silent Hill 2 | D3D12, avant SR | titre de référence ; l'allocation de couleur avec padding d'Unreal est prise en charge |
| Forza Horizon 6 | D3D12, avant SR | |
| Stray | D3D11 via le pont D3D12, avant SR | |
| GTA V Enhanced | D3D12, avant SR, HDR, masque réactif à un seul canal | corrigé en 0.3.0 : le masque était lu comme « tout réactif » et la retouche ne s'appliquait jamais |
| Tout titre avec Ray Reconstruction | D3D12, après RR (réécrite dans la sortie) | pris en charge depuis 0.3.0 ; pas encore confirmé en jeu |

Si un titre ne montre aucun effet : `lmxxf_backend.log` contient une ligne `lmxxf inputs:` (formats, tailles,
échelle de mouvement, sens de la profondeur, masque, exposition) et une ligne `lmxxf stats @N:` toutes les 600 images
(exposition, luminosité en entrée, la retouche du modèle, la retouche transportée, keep, moyenne réactive, longueur
des vecteurs et fraction rejetée). Joignez le log à un rapport ; ces deux lignes disent généralement pourquoi.

Sous lmxxf, le bloc Neural runtime propose **Network history** (l'entrée temporelle propre au modèle),
et Image look propose le groupe **lmxxf edit** : la mise en forme de la retouche (Edit detail,
Edit colour, Edge guard : gain sur la partie fine de la retouche du modèle, sa couleur par rapport à son
changement de luminosité, et un fondu de la retouche au niveau des bords de profondeur) et **Output smoothing**
(la passe côté sortie du projet d'origine, nécessite Network history). Neural passes, Residual strength/limit,
la netteté (sharpening), Debug view 1 et le filtre Appearance s'appliquent avec les deux runtimes.

**Full network** (Neural > Performance, `[DlssNr] LmxxfFullNetwork`, lmxxf uniquement) exécute les 71 blocs du
réseau au lieu de sauter les blocs 42, 43 et 46 : légèrement plus fidèle, environ 0.5 ms plus lent en 1080p
(16.6 -> 17.1 ms sur une RX 9070 XT). Désactivé par défaut.

## Configuration requise

- Un GPU AMD avec un pilote à jour. Le runtime neuronal utilise HIP via le pilote ; ni le SDK HIP
  ni le mode développeur ne sont nécessaires. Puces concernées : les RX 9000 (RDNA 4) font tourner les deux runtimes ;
  les RX 7000 (RDNA 3, de bureau et portables) aussi - lmxxf via le backend RDNA 3 d'AMDNR, plus lentement
  que sur RDNA 4 ; Strix Halo (8060S / 8050S) fait tourner lmxxf ; les APU des consoles portables (Z1 Extreme / 780M,
  Z2 Extreme / 890M) et RDNA 2 (RX 6000, Steam Deck) ne sont pris en charge par aucun des deux.
  L'onglet Neural indique ce que votre GPU peut faire tourner.
- Un jeu Direct3D 12, Direct3D 11 ou Vulkan. Le chemin neuronal AMD lui-même est en D3D12 ; les titres D3D11 et
  Vulkan y accèdent via le pont D3D12 d'OptiScaler, ce qui signifie que l'upscaler doit être
  l'un des backends « w/Dx12 » (`ffx_12`). Laissez `Dx11Upscaler` / `VulkanUpscaler` sur `auto`
  et ce build le choisit pour vous quand le rendu neuronal est activé.
- Environ 2 Go de VRAM libre à des résolutions de rendu de l'ordre du 1080p.

## Contenu des deux archives

**AMDNR-vX.X.X.zip**

| Fichier | Description |
|---|---|
| `OptiScaler.dll` | OptiScaler avec le backend AMD DLSS-NR. Renommez-le comme indiqué dans le guide. |
| `OptiScaler.ini` | Paramètres. Neural Rendering est activé ; les logs sont activés pour qu'un rapport de bug ait quelque chose à joindre. |
| `LmxxfNrRuntime.dll` | Le runtime neuronal lmxxf (kernels lmxxf 0.29). Utilisé uniquement s'il est choisi ; lit `LmxxfNrRuntime.pak` placé à côté, voir « Le runtime lmxxf ». |
| `LmxxfNrRuntime.pak` | Les poids, les modules HIP et les shaders du runtime lmxxf dans un seul fichier chiffré (416 Mo). Seul le runtime lmxxf le lit ; on peut le garder sans risque avec le runtime danielblnc. |
| `OptiScaler\` | FSR, XeSS, le denoiser FidelityFX et le D3D12 Agility SDK utilisés par OptiScaler. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | dlssg-to-fsr3 de Nukem9, non modifié et renommé : les appels DLSS Frame Generation du jeu sont traités par la frame generation FSR 3, y compris sous Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, voir `Licenses/`. |
| `Licenses\`, `LICENSE` | Licences tierces, les mentions d'AMDNR (`AMDNR_NOTICE.txt`) et la licence GPL-3.0 de ce build. |
| `SHA256SUMS.txt` | Sommes de contrôle de chaque fichier livré, pour les deux archives. |

**Runtime.zip**

| Fichier | Description |
|---|---|
| `dlssnr_amd_pass1..3.dll` | Le runtime neuronal AMD, la v0.3.1 de danielblnc, non modifiée. Trois copies pour que le multi-passe en ait une par passe. |
| `dlssnr_on_amd_weights.bin` | Les poids du réseau chargés par le runtime. |

## Réglages à connaître

Ouvrez l'onglet **Neural**. Les valeurs par défaut correspondent à la dernière configuration testée, donc le premier
réflexe utile est de changer une seule chose à la fois.

- **NR resolution** — le principal levier qualité/coût. En dessous de 100 %, le modèle travaille sur une image
  plus petite et seule sa *correction* est ramenée sur l'image en pleine résolution, de sorte que
  l'image conserve ses propres détails. Au-dessus de 100 %, le coût augmente au carré (150 % donne 2.25x). Le curseur
  avance par pas de 5 % : chaque nouvelle taille NR peut garder de la VRAM jusqu'au redémarrage du jeu, donc redémarrez
  le jeu après de nombreux changements.
- **Residual strength** — la part de la retouche du modèle qui est appliquée ; au-dessus de 1, elle est amplifiée. C'est
  le réglage qui change le plus l'image.
- **Residual limit** — un plafond qui limite jusqu'où un pixel peut bouger. Plaques ou taches à l'image : **baissez-le**.
- **Model interleave** — exécute le modèle une image sur deux pour un gros gain de framerate. Les
  images sautées sont remplies par l'**Interleave preset** ; *Edit accumulation* (preset 10, les deux
  runtimes) est le preset par défaut : chaque image est le rendu de cette image-là plus la correction
  portée par le modèle, si bien qu'aucune image précédente n'est conservée. *Guided fill v2* (preset 6,
  danielblnc) et *Classic carry* (lmxxf) sont les remplissages plus anciens. Le pacing des deux types
  d'image est automatique. Adaptive interleave est désactivé dans cette build.
- **Neural passes** — 2 et 3 empilent le modèle, avec des gains décroissants. Sous lmxxf, l'historique
  du réseau reste celui de sa première passe ; les passes supplémentaires ne font que de l'affinage spatial.
  danielblnc exécute 1 passe sur les titres Vulkan (une note sous le curseur l'indique).
- **Colour composition** (Neural > Image look, les deux runtimes) — *Classic* (par défaut) est l'image
  que vous aviez avant. *RenoDX (experimental)* exécute la composition des couleurs de RenoDX après le modèle, comme
  le fait le chemin NVIDIA : Composition detail et colour, un **Highlight guard** bilatéral (2x par défaut)
  qui borne la réponse du modèle par rapport à l'original, et des réglages optionnels peau / environnement.
  Sur une image display-referred (SDR), il revient à Classic, avec une note dans le menu. Les styles NR
  et les presets n'y touchent pas.
- **La frame generation est désactivée dans un ini neuf.** Onglet Frame Gen : choisissez le FG Input (p. ex. « DLSSG via
  Streamline » dans un jeu avec la frame generation DLSS) et le FG Output (XeFG), puis cochez **Active** dans
  la section Frame Generation (XeFG) et cliquez sur Save Settings. Un ini 0.1.0 où elle était activée n'est pas
  repris lorsque vous installez l'ini de la 0.2.0.
- **Frame generation multi-images XeFG** — le 3X à 6X est intégré et activé par défaut (`XeFG\UnlockMFG`),
  pour la copie d'OptiScaler comme pour celle du jeu. **Supprimez `XeFGUnlock.asi`** de `OptiScaler\plugins`
  si vous l'avez encore : deux copies du même patch font planter le jeu.
  **Jusqu'à 10X, sur activation manuelle** (jeux D3D12 uniquement) : réglez *XeFG ceiling (restart)* sous FG Output dans
  l'onglet Frame Gen (4X, 6X par défaut, 8X ou 10X ; `[XeFG] MaxInterpolatedFrames`), redémarrez, puis choisissez le
  multiplicateur dans la liste MFG. Au-delà de 6X, il faut le provider XeFG propre à OptiScaler avec Extra pacing activé ;
  la copie de XeSS 3 propre au jeu reste limitée à 6X. Le 10X nécessite un écran 360 Hz ou plus et une limite de FPS
  réglée sur la fréquence de rafraîchissement / 10 ; la latence est élevée, et le provider réserve environ 128 Mio de VRAM
  en plus en 4K. Le 7X-10X n'est pas encore confirmé en jeu : testeurs, merci d'envoyer `OptiScaler.log`.
- **FSR Ray Regeneration** — par défaut, RDNA 4 (RX 9000) uniquement ; et seulement dans les jeux qui utilisent DLSS Ray Reconstruction (Cyberpunk 2077,
  Alan Wake 2), le jeu devant tourner en DLSS (spoofing activé), avec le ray tracing et Ray Reconstruction
  activés dans ses propres paramètres. Neural Rendering s'exécute alors après lui, sur sa sortie, ce qui coûte
  davantage : baissez la NR resolution si le framerate chute. Ses réglages (Neural > Quality > Ray
  Regeneration) n'apparaissent que lorsque le jeu utilise Ray Reconstruction. L'option **path-traced profile**
  (moins de grain sur les visages en path tracing) doit être activée manuellement depuis la 0.3.3.1 : cochez-la à cet endroit pour
  l'essayer dans Resident Evil Requiem ou PRAGMATA. Au même endroit se trouvent l'intensité du bias mask, une vue
  de debug RR et l'option **skin smoothing** (lissage de la peau ; expérimental, pour les jeux qui exposent un guide SSS ;
  désactivé par défaut, mais activé par défaut dans Resident Evil Requiem depuis la 0.3.3.2).

## En cas de problème

`OptiScaler.log` apparaît dans le dossier du jeu. Joignez-le dans `#bug-report`, en précisant le jeu et
le GPU. Le backend AMD écrit aussi `amd_presr.log` et `amd_bridge.log`, qui sont les plus utiles
quand c'est précisément la passe neuronale qui se comporte mal. Le log de la session précédente est conservé sous le nom
`OptiScaler.previous.<exe>.log` ; après un plantage, joignez-le aussi (le nouveau log indique alors « no clean exit
recorded »).

**NR frames 0/s, et l'onglet Neural ou `amd_presr.log` indique que la DLL de passe est un build que cet AMDNR ne
pilote pas ?** Vos `dlssnr_amd_pass1..3.dll` sont un build de danielblnc que cet AMDNR ne connaît pas (un ensemble 0.2.16
a été vu en circulation), ou l'une des trois est manquante. Depuis 0.3.3.2, l'onglet Neural nomme le fichier et sa
version, et indique la marche à suivre. Utilisez `v0.4.0-Runtime.zip` (le plus récent) ou `Runtime.zip` (0.3.1) de cette release,
en prenant les trois DLL de passe dans le même zip : la `dlssnr_amd_pass1.dll` de `v0.4.0-Runtime.zip` fait 10,027,008
octets, avec un SHA256 qui commence par `d62be3d8`. Builds pris en charge : 0.2.17, 0.3.0, 0.3.1, 0.3.2, 0.3.3, 0.4.0,
ainsi que 0.4.x avant même leur sortie. N'installez pas le setup de danielblnc, ni ses
`dxgi.dll` / `version.dll` / `winhttp.dll`, à côté d'AMDNR : AMDNR fait déjà tourner son runtime.

**lmxxf ne fait rien, ou s'arrête aussitôt, sur un PC avec une puce graphique intégrée ?** Corrigé en 0.3.3.2. Sur un
Ryzen de bureau avec sa puce graphique intégrée activée, un portable avec un APU AMD et une Radeon, ou un PC avec deux
GPU AMD, le GPU du jeu n'est souvent pas le périphérique HIP 0. lmxxf échouait alors dès sa première image
(`hipErrorInvalidHandle (400)`, puis « session is poisoned » dans `lmxxf_backend.log`) et restait désactivé. Remplacez
à la fois `OptiScaler.dll` (le fichier que vous avez renommé, p. ex. `dxgi.dll`) et `LmxxfNrRuntime.dll` par les
fichiers de la 0.3.3.2. Pas encore testé sur un tel PC : si lmxxf s'arrête encore, l'onglet Neural indique désormais
pourquoi ; envoyez `lmxxf_backend.log` et `amd_bridge.log` (ce dernier liste les périphériques HIP).

**La ligne d'état de lmxxf affiche `c32w=off:nofile` sur une RX 9070 / 9070 XT ?** Un ancien dossier
`DLSS5-AMD\native-game-tiled-assets` à côté du `.exe` du jeu (reste d'une ancienne installation de lmxxf) est
utilisé à la place de `LmxxfNrRuntime.pak`. Il ne contient pas les kernels c32w, donc lmxxf tourne à l'ancienne
vitesse. Supprimez ou renommez le dossier `DLSS5-AMD` : le pak contient tout ce dont lmxxf a besoin. Un
`LmxxfNrRuntime.pak` antérieur à la 0.3.3.2 donne le même état ; remplacez-le par celui de cette version.

**danielblnc : le style NR change encore quand la NR resolution quitte 100 % ?** Connu, non corrigé dans la
0.3.3.2 (un correctif est prévu pour la 0.3.4). La 0.3.3.2 corrige uniquement le saut à 100 % de NR resolution :
là, Residual strength 0.99 donne désormais 99 % de 1.00, et les styles NR ont le rendu qu'indique leur intensité.
En dehors de 100 % (y compris les paliers de Dynamic NR et les presets Balanced / Performance), strength, limit
et edge fade agissent toujours sur le résultat entier, donc le rendu peut changer. `[DlssNr] AmdEditShaper=true`
les applique plutôt à la retouche du modèle elle-même, mais il est désactivé par défaut : lors d'un test, il a
délavé les hautes lumières (Forza Horizon 6, Classic, NR à 115 %). lmxxf n'est pas concerné.

**Un jeu Vulkan (Indiana Jones and the Great Circle) s'arrête au démarrage avec « Could not create the Vulkan
device (VK_ERROR_EXTENSION_NOT_PRESENT) » ?** Corrigé en 0.3.2 : le chemin neuronal NVIDIA hérité demandait au
pilote AMD deux extensions de périphérique propres à NVIDIA. Les titres Vulkan accèdent à la passe neuronale via le
pont D3D12 d'OptiScaler (voir Configuration requise).

**lmxxf figeait un jeu Vulkan à la première image NR ?** Corrigé en 0.3.3 ; attendez-vous à une seule saccade
d'environ 1 s au démarrage de NR. Si jamais une session Vulkan s'arrête avant la première réponse de lmxxf, le
démarrage suivant utilise le runtime de danielblnc et l'onglet Neural indique pourquoi ; cliquez sur **Retry lmxxf**
à cet endroit (cela supprime `lmxxf_vk_launch.pending` à côté de `OptiScaler.dll`) pour réessayer lmxxf.

**danielblnc se figeait plusieurs secondes, puis arrêtait NR, sur un jeu Vulkan (Indiana Jones) avec 2-3 Neural
passes ?** Corrigé en 0.3.3 : sur les titres Vulkan, il exécute 1 passe, et son attente de 80 ms après soumission a
disparu. La première image NR d'une session provoque encore une pause d'environ 5 s ; une note sous le choix du runtime
explique les lignes de log correspondantes. Testeurs : `[DlssNr] AmdVkLateCopyWait=true` (expérimental, désactivé par
défaut, pas encore testé en jeu) devrait supprimer cette pause ; envoyez `OptiScaler.log`, `amd_presr.log` et
`dlssnr_on_amd.log`.

**La consommation de RAM de lmxxf grimpait tant que NR tournait ?** Corrigé en 0.3.3 (c'était environ 45 Go par heure
à 60 FPS NR). Ce qui subsiste : danielblnc garde de la VRAM pour chaque nouvelle taille NR au-delà d'environ 1 MP (la 0.3.3.2
arrondit ses tailles par pas de 64 px en dehors de 100 %, il n'y en a donc que quelques-unes) ; avec danielblnc, redémarrez le jeu
après de nombreux changements. Depuis la 0.3.3.2, lmxxf ne garde plus environ 97 Mo à chaque changement de NR resolution ou de
mode DLSS : il crée ses buffers de réseau une seule fois par taille de réseau et les réutilise (il reste un petit reliquat
d'environ 10-25 Mo de VRAM par changement).

**Un jeu Streamline échoue au démarrage avec l'erreur slInit 0x18 (vu avec NBA 2K27 sur AMD) ?** La 0.3.3 élimine
l'une des façons dont les hooks d'OptiScaler sur les plugins Streamline pouvaient la provoquer, mais rien ne
confirme que ce soit la cause dans NBA 2K27. `OptiScaler.log` enregistre désormais des lignes `slInit returned ...` et
`[SLINIT]` : envoyez le log avec le rapport.

**Ray Reconstruction est activé dans le jeu mais l'onglet Neural indique « Ray Regeneration is off in this title » ?**
Le jeu n'expose pas ce dont FSR Ray Regeneration a besoin (Satisfactory : pas de matrices de caméra). L'upscaling
FSR tourne à sa place et NR reprend sa position normale avant le SR ; rien dans l'ini ne change cela.

**Un jeu Ubisoft sous Anvil (AC Black Flag Resynced, Shadows, Mirage) affiche « DX12 Error 0x80070057 » ?**
Ces jeux embarquent leur propre XeSS Frame Generation. Ce build la leur laisse (la sortie XeFG d'OptiScaler se retire
dans ce cas et l'onglet Frame Gen l'indique) ; utilisez l'option XeSS FG du jeu. Si le problème persiste, réglez
`[FrameGen] Enabled=false` et `[fakenvapi] ForceXeLL=false` et faites un rapport avec le log.

**The Last of Us Part I plante au démarrage ?** C'est l'initialisation de Streamline propre au jeu, un problème
connu d'OptiScaler : renommez `sl.common.dll` dans le dossier du jeu en `sl.common.dll.bak` et choisissez
**FSR 3.1** dans les paramètres du jeu à la place de DLSS.

Notes complètes pour chaque version : `CHANGELOG.md` (dans le zip et dans le dépôt).

## Feuille de route

- **0.3.3.x** (ce build) — lmxxf sur RDNA 3 (RX 7000 ; backend propre à AMDNR) ; composition des couleurs
  RenoDX (expérimentale, en option) sur les deux runtimes ; lmxxf : option Full network, fuite de RAM
  corrigée, titres Vulkan corrigés (upload différé des poids dans le pont Vulkan), kernels 0.29 (identiques au bit
  près, plus rapides) ; danielblnc sur les titres Vulkan : 1 Neural pass, messages plus clairs, une attente de copie
  tardive en option ; XeFG jusqu'à 10X (en option, D3D12) ; démarrage de Streamline renforcé et diagnostics ; profil
  path-traced et lissage de la peau pour FSR Ray Regeneration ; robustesse UE5.
- **0.3.2** — les rapports de la 0.3.1 : les titres Vulkan démarrent et tournent avec lmxxf, couleurs de lmxxf
  alignées sur celles de danielblnc (auto-exposition), la liste déroulante du runtime, statut et réglages de Ray
  Reconstruction ; dlssg-to-fsr3 de Nukem9 dans le zip pour la frame generation sous Vulkan.
- **0.3.1** — correctifs issus des premiers rapports sur la 0.3.0 (lmxxf seul ne se lançait jamais, le NR
  resté sans effet dans Where Winds Meet, le plantage lors d'un changement de qualité DLSS, GTA V Legacy) et presets
  de style NR avec trois emplacements personnalisés.
- **0.3.0** — le runtime neuronal HIP **lmxxf** (RDNA 4) comme runtime sélectionnable à côté de celui de
  danielblnc, livré sous forme de `LmxxfNrRuntime.dll` + `LmxxfNrRuntime.pak` : historique du réseau,
  de vraies Neural passes, la mise en forme de la retouche, le placement après Ray Regeneration, des diagnostics
  par titre et l'auto-réparation. Un grand merci à TheAutomatic, dont le travail sur le projet DLSS 5 AMD
  sert de base à cette intégration.
- **0.4.0** — l'AMDNR Launcher (installation en un clic des runtimes et du pak, mises à jour) et la prise
  en charge des titres sans upscaler propre (type Stray), où OptiScaler fournit à la fois l'upscaler et la
  passe neuronale.

---

## Crédits

Ce build est un travail d'assemblage qui repose sur le travail d'autres personnes. S'il vous est utile, les remerciements
reviennent aux projets d'origine (upstream).

- **TheAutomatic** — DLSS 5 AMD project — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD — https://github.com/danielblnc/DLSS-NR-on-AMD (`Runtime.zip`, non modifié)
- **lmxxf** — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (le runtime HIP, MIT)
- **c32w kernels** (0.3.3.2) — les kernels RDNA 4 à une seule wave propres à AMDNR pour le réseau de lmxxf, Copyright (c) 2026 3zwr1 (AMDNR) ; idées tirées de la documentation publique d'AMD sur WMMA pour RDNA 4 (GPUOpen, ROCm matrix instruction calculator)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Dagherbou / OptiScaler_DLSSNR** — https://github.com/Dagherbou/OptiScaler_DLSSNR
- **wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass** — https://github.com/wilsjo2/OptiScaler-DLSSNR-PreSR-Multipass
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, non modifié)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (calculs de la composition des couleurs, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), la base du déblocage multi-images XeFG intégré et de son pacing
- **burak113** — le préprocesseur FSR Ray Regeneration (branche OptiScaler ffx-denoise-experimental, GPL-3.0)
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## Copyright / Licence

AMDNR est Copyright (c) 2026 3zwr1 (AMDNR). C'est un fork d'OptiScaler, distribué sous la licence GPL-3.0
figurant dans `LICENSE`.

Le travail propre à AMDNR est soumis à une condition supplémentaire au titre de la section 7(b) de la GPL-3.0 (voir
`Licenses/AMDNR_NOTICE.txt`) : toute copie, tout fork ou toute œuvre dérivée qui l'utilise doit conserver ses mentions
et créditer **AMDNR by 3zwr1** (<https://github.com/3zwr1/AMD-NR---OptiScaler>).

**Copyright du menu AMDNR.** Le menu AMDNR — sa disposition, son design, ses textes et le code ajouté par AMDNR pour ce menu — est Copyright (c) 2026 3zwr1 (AMDNR). Il fait partie de ce fork sous GPL-3.0, avec les conditions supplémentaires suivantes (GPL-3.0 section 7) : (b) quiconque en réutilise une partie, quelle qu'elle soit, doit conserver cette ligne de copyright et créditer AMDNR by 3zwr1 de façon visible, dans le menu et dans le README ; (c) vous ne pouvez pas le présenter, ni en présenter une copie modifiée, comme votre propre travail ; les versions modifiées doivent être clairement signalées comme modifiées ; (e) aucun droit n'est accordé sur le nom ni sur le logo AMDNR ; les autres projets ne peuvent pas les utiliser.

Le travail upstream crédité ci-dessus reste la propriété de ses auteurs, sous leurs propres licences ; AMDNR ne
revendique aucun copyright dessus.

Le code source sera publié avec AMDNR 0.5.0.

## Mentions légales

Ce build est distribué sous la licence GPL-3.0 figurant dans `LICENSE` ; les licences des bibliothèques tierces se
trouvent dans `Licenses\`. Le runtime neuronal AMD et ses poids sont redistribués sous la paternité de leurs auteurs
d'origine, telle que créditée ci-dessus, uniquement par commodité, sans aucune revendication de propriété et sans
aucune garantie.

Le `nvngx_dlssnr.dll` de NVIDIA ne fait pas partie de ces archives. Rien de tout cela n'est approuvé ni soutenu par
NVIDIA, AMD ou un quelconque éditeur de jeux, ni affilié à eux. Ce build pilote directement une fonctionnalité non
documentée. Utilisez-le à vos propres risques.
