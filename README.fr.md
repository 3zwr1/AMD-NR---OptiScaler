# AMDNR — DLSS 5 Neural Rendering sur AMD (build OptiScaler) — v0.3.4

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

> **Nouveautés de la 0.3.4 :** un nouveau menu (l'onglet Neural refait, le même style dans tous les onglets et un
> bouton **Save report** qui zippe vos logs pour un rapport de bug) ; lmxxf est plus rapide sur RX 7000 (1440p FSR
> Quality : 73.3 -> 52.2 ms par exécution du réseau sur une RX 7800 XT, temps réseau mesuré hors jeu) et sur RX 9070 / 9070 XT (kernels
> de lmxxf 0.31) ; lmxxf tourne sur les APU des consoles portables (expérimental ; l'essai d'un testeur, en jeu : environ 29 fps dans Shadow of the Tomb Raider sur une
> ROG Ally) ; un **Fast mode** optionnel pour lmxxf ; **AMDNR Screen GI**, la GI en espace écran d'AMDNR (preview,
> désactivée par défaut) ; et de nombreux correctifs. Remplacez `OptiScaler.dll`, `LmxxfNrRuntime.dll` et `LmxxfNrRuntime.pak` ensemble. Détails :
> `CHANGELOG.md`.

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

> **Mise à jour depuis un AMDNR plus ancien ?** Recopiez tout en écrasant les fichiers. En 0.3.4, trois fichiers ont
> changé ensemble : `OptiScaler.dll` (remplacez le fichier que vous avez renommé, p. ex. `dxgi.dll`, par le nouveau
> renommé de la même façon), `LmxxfNrRuntime.dll` et `LmxxfNrRuntime.pak` (440 Mo, nouveau dans cette
> version). Ne les mélangez pas avec d'anciennes copies. Vous pouvez garder votre propre `OptiScaler.ini` : les
> nouveaux réglages prennent leur valeur par défaut.

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

**Le plus simple : Save report.** Si le menu s'ouvre, cliquez sur **Save report** (la dernière ligne de Neural > Diagnostics, ou la première d'Advanced > Logging). Cela écrit
un zip, `AMDNR-report-<exe du jeu>-<date>.zip`, dans le dossier du jeu (sur le Bureau si le dossier du jeu est en
lecture seule, sinon dans `%TEMP%`), avec `report.txt`, les logs et les fichiers ini, et le menu indique où il se
trouve. Votre nom d'utilisateur Windows et le nom du PC sont remplacés par des marqueurs ; un nom présent dans un
chemin du jeu hors de `C:\Users\` ne l'est pas. Joignez le zip dans `#bug-report`.

> Le `.exe` ne se trouve généralement pas là où pointe le raccourci. Les jeux Unreal le rangent dans
> `<Game>\Binaries\Win64\`.

---

### Le runtime lmxxf (0.3.0, optionnel)

Un second runtime neuronal (sous licence MIT, par lmxxf) peut exécuter la passe à la place de celui de
danielblnc. Il tourne nativement sur RDNA 4 ; sur RDNA 3 (RX 7000, Strix Halo), il passe par le backend RDNA 3
d'AMDNR par 3zwr1 - plus lentement, voir « RX 7000 » plus bas : commencez avec une NR resolution de 70 % ou moins.
Les APU des consoles portables le font aussi tourner, à titre expérimental (voir « APU des consoles portables »
plus bas). Il a besoin de deux choses à côté du jeu :

1. `LmxxfNrRuntime.dll` - dans cette archive, à côté de `OptiScaler.dll` (il est copié avec le reste).
2. `LmxxfNrRuntime.pak` (440 Mo, inclus dans le zip AMDNR) à côté de `LmxxfNrRuntime.dll` - les
   fichiers de poids, les modules HIP et le HLSL de lmxxf dans un seul fichier chiffré et authentifié. Le runtime
   l'ouvre en mémoire ; rien n'est extrait sur le disque.

Au premier lancement où un runtime installé est détecté sans qu'aucun choix n'ait encore été fait, le menu demande lequel
utiliser (`[DlssNr] NrBackend = daniel | lmxxf` dans l'ini enregistre ce choix ; Neural > Neural runtime
permet de le changer, avec effet au prochain démarrage du jeu). La retouche de lmxxf est appliquée avec une image de retard, transportée
par les vecteurs de mouvement, de sorte que l'image n'attend jamais le réseau (environ 14.1 ms de temps
réseau en 1080p sur une RX 9070 XT). Son log est `lmxxf_backend.log`, à côté du jeu.

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

Les deux runtimes partagent un seul onglet Neural (voir « Le menu » plus bas). Les réglages que le runtime actif
n'a pas sont grisés avec une courte étiquette, ou masqués avec un décompte. Propres à lmxxf : **Full network**,
**Output smoothing** (Quality > More quality options, nécessite Network history), **Edit detail**, **Edit
colour** et **Edge guard** (Image look > Model strength : gain sur la partie fine de la retouche du modèle, sa
couleur par rapport à son changement de luminosité, et un fondu de la retouche au niveau des bords de
profondeur) et le plafond des hautes lumières de l'auto-exposition. Nouveau sur lmxxf en 0.3.4 : Network output,
Encoding, Residual edge fade, Game exposure, Fast mode, l'affichage du pacing de l'interleave et le masque
de personnages natif du modèle avec Structure intensity et Character structure (chaque
changement reconstruit le réseau : une saccade d'environ 1 s).

**Full network** (Neural > Performance, `[DlssNr] LmxxfFullNetwork`, lmxxf uniquement) exécute les 71 blocs du
réseau au lieu de sauter les blocs 42, 43 et 46 : légèrement plus fidèle, environ 0.5 ms plus lent en 1080p
(16.6 -> 17.1 ms sur une RX 9070 XT). Désactivé par défaut.

**Fast mode** (Neural > Performance, `[DlssNr] AmdLmxxfFastMode`, lmxxf, optionnel, désactivé par défaut) fait
tourner le réseau un palier de taille plus bas (1080 -> 900, 900 -> 720) : environ 29 % de temps réseau en moins en
1080p (RX 9070 XT, mesuré hors jeu), avec des détails fins un peu plus doux. Les builds danielblnc qui ont leur propre
Fast mode y ont aussi une ligne Fast mode (`[DlssNr] AmdDanielFastMode`) ; les runtimes des zips de runtime de cette
release ne l'ont pas, donc la ligne est masquée.

### RX 7000 (RDNA 3) : plus rapide grâce au palier de taille du réseau (nouveau en 0.3.4)

Le réseau de lmxxf tourne à quelques tailles fixes (paliers) : 720 (1280x720), 900 (1600x900) et 1080
(1920x1080), plus 576 et 360 (nouveaux, utilisés sur les consoles portables). Un palier coûte la même chose,
quelle que soit la part qu'en remplit l'image. Sur RDNA 3 (RX 7000, Radeon 8060S / 8050S et les APU des
consoles portables), la taille NR de lmxxf se cale désormais par défaut sur un palier : elle descend au palier
inférieur quand elle en est plus proche (moins cher), sinon elle grandit jusqu'à remplir son propre palier (même
coût, un peu plus de détail), sans jamais dépasser la taille de l'image elle-même.

Temps réseau par exécution sur une RX 7800 XT (mesuré par un testeur avec la sonde de lmxxf ; réseau seul,
moyenne de 30 exécutions ; le temps du palier 900 a été mesuré en 1600x900) :

| Réglage du jeu | 0.3.3.2 | 0.3.4 sur RX 7000 |
|---|---|---|
| 1440p, FSR Quality (rendu 1706x960), NR 100 % | palier 1080 : 73.3 ms | palier 900 : 52.2 ms |
| Rendu 1080p, NR 85 % | palier 1080 : 73.2 ms | palier 900 : 52.2 ms |
| Rendu 1080p, NR 70 % | palier 900 : 52.2 ms | palier 720 : 34.4 ms |
| Rendu 1080p, NR 80 % | palier 900 : 52.2 ms | palier 900, rempli : 52.2 ms (plus de détail) |
| Rendu 1080p, NR 100 % | palier 1080 : 73.2 ms | inchangé |

- En jeu, le gain par image affichée est plus faible : avec Model interleave, le réseau tourne une image sur
  deux, et le jeu a son propre coût. Pas encore mesuré en jeu.
- Le réseau voit une image un peu plus petite (en 1440p Quality, environ 6 % de pixels en moins par côté), donc
  les détails fins peuvent être un peu plus doux. `[DlssNr] AmdLmxxfTierSnap=false` rétablit les tailles de la
  0.3.3.2. Les RX 9000 gardent les tailles de la 0.3.3.2 sauf si vous le réglez sur `true`.
- Commencez avec une NR resolution de 70 % ou moins (le palier 720 avec un rendu 1080p ; le preset Performance
  est à 70 %). Le coût affiché à côté de NR resolution correspond au palier sur lequel tourne le réseau ; son
  infobulle nomme le palier.

### APU des consoles portables (expérimental, nouveau en 0.3.4)

lmxxf tourne sur les APU des consoles portables dotés d'au moins 12 unités de calcul, via le backend RDNA 3
d'AMDNR par 3zwr1 : **Z1 Extreme, Z2 et Radeon 780M** (gfx1103), **Z2 Extreme, Radeon 890M et 880M** (gfx1150).
C'est expérimental et lent. La ligne Neural runtime affiche « experimental » après le
crédit RDNA 3.
Premiers résultats d'un testeur (ROG Ally, Z1 Extreme) : la sonde de lmxxf hors jeu, 54.7 ms par exécution du réseau
à la taille 360p, 110.9 ms en 576p ; en jeu, l'essai d'un testeur (Shadow of the Tomb Raider, 1280x720 avec XeSS, preset Handheld),
62 ms par exécution du réseau en moyenne en 360p avec le modèle une image sur 4, environ 29 fps avec NR activé.

- **Non pris en charge :** Z1 et Radeon 740M (4 unités de calcul), Radeon 760M (8), Radeon 860M / 840M. Le
  runtime de danielblnc ne tourne pas sur les APU des consoles portables. Les RX 6000 (RDNA 2) sont prévues pour
  la 0.4.0 ; le Steam Deck et les autres APU RDNA 2 ne sont pas pris en charge.
- **Ce qu'il fait tout seul** (uniquement tant que votre ini n'a pas de valeur propre) : le réseau tourne à sa
  plus petite taille, 360p (640x360), et le modèle tourne une image sur quatre (Model interleave ; non
  enregistré). Neural passes reste à 1.
- **La vitesse, honnêtement :** À
  titre de comparaison : une RX 7800 XT (60 unités de calcul) a besoin de 34.4 ms par exécution du réseau à la
  taille 720 ; ces puces en ont de 12 à 16 et tournent à des fréquences plus basses. Attendez-vous à une forte
  baisse de framerate même en 360p avec le modèle une image sur quatre, à un peu de ghosting dû à l'interleave
  long, et à un rendu plus doux que sur un GPU de bureau. Le coût NR à la fin de la ligne d'état de l'onglet
  Neural (et dans Diagnostics) affiche le vrai chiffre sur votre appareil.
- **Réglages :**
  - Plus net mais plus lent : `[DlssNr] AmdLmxxfTierCap=576` (la taille de réseau 1024x576).
  - Avec un rendu 720p ou 800p, une NR resolution de 100 % alimente déjà la taille 360p, donc une NR resolution
    plus basse ne coûte pas moins.
  - Model interleave sur Off est enregistré comme `[DlssNr] AmdInterleave=1` (également désactivé), pour que la
    valeur par défaut des consoles portables ne revienne pas au démarrage suivant. Pour le désactiver à la main,
    écrivez 1, pas 0.
  - Preset > **Handheld** règle NR resolution à 100 %, Dynamic NR désactivé, le modèle une image sur 4, 1 Neural pass et Full network désactivé. Le bouton n'apparaît que sur ces APU ; Quality, Balanced et Performance gardent aussi ici la taille de réseau 360p (le menu l'indique).
- **FSR 4 :** sur ces puces (et sur les Radeon 780M / 760M / 740M en général), FSR 4 INT8 ne s'active plus tout seul :
  avec `Dx12Upscaler=auto` l'upscaler est XeSS, et FSR 3.1 reste FSR 3.1. `[FSR] Fsr4ForceModel=2` le force toujours
  (expérimental).
- **Shadow of the Tomb Raider** (et les jeux qui créent leur device D3D12 deux fois) ne plante plus au démarrage de
  l'upscaler (corrigé en 0.3.4).
- **Pilote :** utilisez le pilote Adrenalin d'AMD. lmxxf a besoin de HIP (`amdhip64_7.dll`), que certains pilotes
  de fabricants de consoles portables omettent ; `amd_bridge.log` indique alors que HIP n'est pas disponible.
- **Utilisez ensemble les trois fichiers de la 0.3.4 :** seul le pak de la 0.3.4 contient les modules pour
  consoles portables, et le `LmxxfNrRuntime.dll` de la 0.3.4 refuse une console portable quand `OptiScaler.dll`
  est plus ancien (« this handheld needs OptiScaler.dll 0.3.4 or newer »).
- **Testeurs équipés d'une console portable :** demandez sur Discord le kit de test pour consoles portables
  (`handheld-test.zip`). Son `run_probe.bat` mesure le réseau sur votre appareil et écrit `handheld_result.txt`
  (votre nom d'utilisateur Windows y est masqué).

## Configuration requise

- Un GPU AMD avec un pilote à jour. Le runtime neuronal utilise HIP via le pilote ; ni le SDK HIP
  ni le mode développeur ne sont nécessaires. Puces concernées :
  - RX 9000 (RDNA 4) : les deux runtimes.
  - RX 7000 (RDNA 3, de bureau et portables) : les deux runtimes - lmxxf via le backend RDNA 3 d'AMDNR, plus
    lentement que sur RDNA 4 (le palier de taille du réseau est activé par défaut, voir plus haut).
  - Strix Halo (Radeon 8060S / 8050S) : lmxxf.
  - APU des consoles portables avec 12 unités de calcul ou plus (Z1 Extreme / Z2 / 780M, Z2 Extreme / 890M /
    880M) : lmxxf, expérimental et lent. Z1 (4 CU), 760M / 740M et 860M / 840M : non pris en charge.
  - RX 6000 (RDNA 2) : pas encore pris en charge, prévu pour la 0.4.0. Steam Deck et APU RDNA 2 : non pris en
    charge.

  L'onglet Neural indique ce que votre GPU peut faire tourner (survolez les entrées de runtime, ou voyez la
  ligne GPU dans Diagnostics).
- Un jeu Direct3D 12, Direct3D 11 ou Vulkan. Le chemin neuronal AMD lui-même est en D3D12 ; les titres D3D11 et
  Vulkan y accèdent via le pont D3D12 d'OptiScaler, ce qui signifie que l'upscaler doit être
  l'un des backends « w/Dx12 » (`ffx_12`). Laissez `Dx11Upscaler` / `VulkanUpscaler` sur `auto`
  et ce build le choisit pour vous quand le rendu neuronal est activé. Avec Neural Rendering activé, la liste Upscaling les nomme « ... w/Dx12 - Neural ».
- Environ 2 Go de VRAM libre à des résolutions de rendu de l'ordre du 1080p.

## Contenu des deux archives

**AMDNR-vX.X.X.zip**

| Fichier | Description |
|---|---|
| `OptiScaler.dll` | OptiScaler avec le backend AMD DLSS-NR (AMDNR 0.3.4). Renommez-le comme indiqué dans le guide. |
| `OptiScaler.ini` | Paramètres. Neural Rendering est activé ; les logs sont activés pour qu'un rapport de bug ait quelque chose à joindre. |
| `LmxxfNrRuntime.dll` | Le runtime neuronal lmxxf (0.3.4 : les kernels de lmxxf, y compris ceux de lmxxf 0.31, les kernels c32w d'AMDNR, les petites tailles de réseau et le masque de personnages natif). Utilisé uniquement s'il est choisi ; lit `LmxxfNrRuntime.pak` placé à côté, voir « Le runtime lmxxf ». |
| `LmxxfNrRuntime.pak` | Les poids, les modules HIP et les shaders du runtime lmxxf dans un seul fichier chiffré (440 Mo ; nouveau en 0.3.4 : les modules pour consoles portables et les kernels de lmxxf 0.31). Seul le runtime lmxxf le lit ; on peut le garder sans risque avec le runtime danielblnc. |
| `OptiScaler\` | FSR, XeSS, le denoiser FidelityFX et le D3D12 Agility SDK utilisés par OptiScaler. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | dlssg-to-fsr3 de Nukem9, non modifié et renommé : les appels DLSS Frame Generation du jeu sont traités par la frame generation FSR 3, y compris sous Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, voir `Licenses/`. |
| `Licenses\`, `LICENSE` | Licences tierces, les mentions d'AMDNR (`AMDNR_NOTICE.txt`) et la licence GPL-3.0 de ce build. |
| `SHA256SUMS.txt` | Sommes de contrôle de chaque fichier livré, pour les deux archives. |

**Runtime.zip**

| Fichier | Description |
|---|---|
| `dlssnr_amd_pass1..3.dll` | Le runtime neuronal AMD, la v0.3.1 de danielblnc, non modifiée. Trois copies pour que le multi-passe en ait une par passe. |
| `dlssnr_on_amd_weights.bin` | Les poids du réseau chargés par le runtime. |

## Le menu (nouveau en 0.3.4)

Appuyez sur `INSERT`. Tous les onglets ont le même style : des onglets en texte, une ligne d'en-tête avec
Discord et GitHub (qui ouvre cette page), une ligne de crédits (le nom de Daniel Blanco ouvre sa page GitHub), la ligne **Components** (combien des sept
composants d'OptiScaler sont actifs ; cliquez pour voir la liste), et un pied de fenêtre avec Menu Scale, Save Settings et Close. L'aide s'ouvre quand vous
survolez le libellé d'un réglage.

**L'onglet Neural, de haut en bas :**

- **Enable Neural Rendering** et sa touche (le bouton, p. ex. `Home` : cliquez dessus, puis appuyez sur une
  autre touche pour la réassigner).
- **Neural runtime** (danielblnc / lmxxf, avec la version exacte de vos fichiers, p. ex. `lmxxf 0.3.4`) avec un mot d'état : running, restart the game to switch, not
  installed, not for this GPU ou stopped. En dessous, le crédit du runtime actif et une ligne d'état, p. ex.
  `Running - 1920x1080 at 100% - NR 62/s - model 62/s - 15.3 ms` (le dernier chiffre est le coût NR), et une ligne **Live** fermée avec plus
  de détails. Quand
  quelque chose demande votre attention, une ligne orange suit, avec un bouton quand il existe une solution
  (Retry lmxxf, Switch to danielblnc, Open Upscaling). Dans l'état par défaut, il n'y en a aucune.
- **Preset** : Quality / Balanced / Performance règlent NR resolution sur 100 / 85 / 70 % et désactivent
  Dynamic NR ; rien d'autre. Sur les APU des consoles portables, un quatrième bouton, **Handheld** (voir « APU des
  consoles portables »). **NR style**, et **Style slots** (Store / Apply / Clear).
- **Performance** : NR resolution (%) avec son coût, Neural passes, Full network, Fast mode, Dynamic NR resolution, Model
  interleave (Interleave preset et la ligne de pacing apparaissent dessous tant qu'il est activé).
- **Quality** : Residual strength, Residual limit, Temporal stability, Sharpening (CAS), et **More quality
  options** (Network history - une seule case pour les deux runtimes -, Output smoothing, Stability mode,
  Residual temporal, Residual edge fade, Still-surface steadiness).
- **Image look** : Colour composition, Detail et Colour strength, et trois sections repliables : **Model
  strength** (Tone et Structure intensity, Character structure, Edit detail / colour, Edge guard, Native
  character mask, et Network style, Tone curve et Black lift de danielblnc), **Exposure and highlights**
  (Auto-exposure, son plafond des hautes lumières, Highlight colour guard, Game exposure) et **Appearance filter** (avec son mot off / on après le nom). Un « default » ou « custom » discret après le nom d'une
  section repliable indique si vous y avez changé quelque chose.
- **Ray Regeneration** : sa propre section, affichée seulement quand le jeu utilise FSR Ray Regeneration.
- **La ligne d'outils**, fermée au démarrage : **Diagnostics** (Network output, Debug view, la vue de debug RR,
  Edit shaper A/B, NR cost, les indicateurs de ghosting et d'auto-réglage, la ligne GPU, **Save report**), **Runtime options**
  (Encoding, Every-frame NR, NR slots, Highlight proxy) et **Experimental** (AMDNR Screen-space GI, en preview).

Un réglage que le runtime actif n'a pas est grisé avec une courte étiquette (p. ex. « not in lmxxf yet ») ou
masqué avec un décompte (« 3 danielblnc-only options hidden ») ; changer de runtime ne déplace aucune autre ligne.

**Les autres onglets :** Upscaling commence par l'upscaler, une ligne d'état et Render resolution (les anciens
Upscale Ratio Override et Output Scaling) ; sur une carte qui n'est pas NVIDIA, « DLSS w/Dx12 » n'est plus
proposé. Image contient Sharpness, Textures, Init Flags et le Magnifier. Frame Gen commence par FG Input et FG
Output. Interface contient l'overlay FPS et Keybinds (un bouton par touche). Advanced commence par Active Quirks, puis Display (V-Sync), Compatibility et Logging. Les réglages, les clés et ce qu'écrit Save Settings ne
changent pas, sauf là où `CHANGELOG.md` l'indique.

## Réglages à connaître

Ouvrez l'onglet **Neural**. Les valeurs par défaut correspondent à la dernière configuration testée, donc le premier
réflexe utile est de changer une seule chose à la fois.

- **NR resolution** — le principal levier qualité/coût. En dessous de 100 %, le modèle travaille sur une image
  plus petite et seule sa *correction* est ramenée sur l'image en pleine résolution, de sorte que
  l'image conserve ses propres détails. Au-dessus de 100 %, le coût augmente au carré (150 % donne 2.25x). Le curseur
  avance par pas de 5 % : chaque nouvelle taille NR peut garder de la VRAM jusqu'au redémarrage du jeu, donc redémarrez
  le jeu après de nombreux changements.
  Le coût affiché à côté indique 1.00x à 100 % ; sous lmxxf, c'est le prix du palier de taille du réseau sur
  lequel il tourne (son infobulle nomme le palier). Les boutons Preset le règlent sur 100 / 85 / 70 %.
- **Residual strength** — la part de la retouche du modèle qui est appliquée ; au-dessus de 1, elle est amplifiée. C'est
  le réglage qui change le plus l'image.
- **Residual limit** — un plafond qui limite jusqu'où un pixel peut bouger. Plaques ou taches à l'image : **baissez-le**.
- **Model interleave** — exécute le modèle une image sur deux pour un gros gain de framerate. Les
  images sautées sont remplies par l'**Interleave preset** ; *Edit accumulation* (preset 10, les deux
  runtimes) est le preset par défaut : chaque image est le rendu de cette image-là plus la correction
  portée par le modèle, si bien qu'aucune image précédente n'est conservée. *Guided fill v2* (preset 6,
  danielblnc) et *Classic carry* (lmxxf) sont les remplissages plus anciens. Le pacing des deux types
  d'image est automatique sous danielblnc et désactivé sous lmxxf (`[DlssNr] AmdInterleavePacing` entre 0 et 1
  cadence les deux, au prix de quelques FPS) ; une ligne atténuée sous le preset affiche la mesure. Adaptive
  interleave est désactivé dans cette build.
- **Neural passes** — 2 et 3 empilent le modèle, avec des gains décroissants. Sous lmxxf, l'historique
  du réseau reste celui de sa première passe ; les passes supplémentaires ne font que de l'affinage spatial.
  danielblnc exécute 1 passe sur les titres Vulkan (une note sous le curseur l'indique).
- **Colour composition** (Neural > Image look, les deux runtimes) — *Classic* (par défaut) est l'image
  que vous aviez avant. *RenoDX (experimental)* exécute la composition des couleurs de RenoDX après le modèle, comme
  le fait le chemin NVIDIA : Composition detail et colour, un **Highlight guard** bilatéral (2x par défaut)
  qui borne la réponse du modèle par rapport à l'original, et des réglages optionnels peau / environnement.
  Sur une image display-referred (SDR), avec Network output ou avec Encoding sRGB / Gamma 2.2, il revient à
  Classic sur les deux runtimes ; la note du menu propose alors un bouton qui désactive ce qui bloque. Les
  styles NR et les presets n'y touchent pas.
- **Native character mask** (Image look > Model strength, `[DlssNr] AutoMask`, activé par défaut) — le
  traitement propre au modèle pour les visages et la peau. Le décocher agit désormais sur les deux runtimes
  (sous lmxxf, cela reconstruit le réseau : une saccade d'environ 1 s) ; sous lmxxf, Structure
  intensity et Character structure agissent désormais aussi.
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
- **FSR Ray Regeneration** — par défaut, RDNA 4 (RX 9000) uniquement ; et seulement dans les jeux qui utilisent DLSS Ray Reconstruction (Cyberpunk 2077,
  Alan Wake 2), le jeu devant tourner en DLSS (spoofing activé), avec le ray tracing et Ray Reconstruction
  activés dans ses propres paramètres. Neural Rendering s'exécute alors après lui, sur sa sortie, ce qui coûte
  davantage : baissez la NR resolution si le framerate chute. Ses réglages ont leur propre section,
  **Neural > Ray Regeneration**, affichée seulement quand le jeu utilise Ray Reconstruction. L'option
  **path-traced profile** (moins de grain sur les visages en path tracing) doit être activée manuellement depuis
  la 0.3.3.1 : cochez-la à cet endroit pour l'essayer dans Resident Evil Requiem ou PRAGMATA. La même section
  contient l'intensité du bias mask et l'option **skin smoothing** (lissage de la peau ; expérimental, pour les
  jeux qui exposent un guide SSS ; désactivé par défaut, mais activé par défaut dans Resident Evil Requiem depuis
  la 0.3.3.2) ; les réglages temporels sont sous *More Ray Regeneration options*, et la vue de debug RR est dans
  Diagnostics. Sur RX 7000 (RDNA 3), FSR Ray Regeneration est de nouveau proposé par défaut (la 0.3.3.2 ne le proposait
  que sur RDNA 4). AMD ne le fournit que pour RDNA 4 : si le pilote le refuse, le jeu reçoit FSR sans le
  débruiteur. `[FSR-RR] FfxDenoiserAllowPreRdna4=false` le réserve à RDNA 4 ; les RX 6000 et plus anciennes ne l'ont
  qu'avec `true` (onglet Upscaling : **Offer FSR Ray Regeneration on this GPU (restart)**).
- **AMDNR Screen GI** (preview, nouveau en 0.3.4, désactivé par défaut ; Neural > Experimental, ou `[AmdGi] Enabled=true`) — la lumière rebondie et l'occlusion ambiante en espace écran propres à AMDNR, à partir de la profondeur du jeu, avant NR et l'upscaler ; fonctionne avec NR activé ou non ; environ 1 ms en High pour un rendu 1080p sur une RX 9070 XT (mesuré hors jeu). C'est de l'espace écran : la lumière venant de hors de l'écran manque. Voir `CHANGELOG.md`.
- **Save report** (Neural > Diagnostics, ou Advanced > Logging) — un zip avec tous les logs et les fichiers ini pour un rapport ; voir « Si
  ça ne fonctionne pas » plus haut.

## En cas de problème

`OptiScaler.log` apparaît dans le dossier du jeu. Joignez-le dans `#bug-report`, en précisant le jeu et
le GPU ; **Save report** (Neural > Diagnostics, ou Advanced > Logging) le zippe avec tout le reste. Le backend AMD écrit aussi
`amd_presr.log` et `amd_bridge.log`, qui sont les plus utiles quand c'est précisément la passe neuronale qui se
comporte mal. Les logs des trois dernières sessions sont conservés sous `OptiScaler.previous.<exe>.log` (le plus
récent), `OptiScaler.previous-1.<exe>.log` et `OptiScaler.previous-2.<exe>.log` (`[Log] KeepPreviousLogs` ; 1 n'en
garde qu'un, comme avant). Après un plantage, joignez-les aussi : le nouveau log indique alors « no clean exit
recorded » (depuis la 0.3.4, plus après une fermeture normale).

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
`fk=fff-` sur la même ligne signifie la même chose (un ancien pak ou un dossier isolé) : lmxxf tourne
toujours, à l'ancienne vitesse.

**danielblnc : le style NR change encore quand la NR resolution quitte 100 % ?** Toujours ouvert en 0.3.4, et le
réglage par défaut ne change pas. À 100 %, Residual strength 0.99 donne 99 % de 1.00 (corrigé en 0.3.3.2) ; en
dehors de 100 % (y compris les paliers de Dynamic NR et les presets Balanced / Performance), strength, limit et
edge fade agissent toujours sur le résultat entier, donc le rendu peut changer. La 0.3.4 ajoute un A/B pour
trouver le bon correctif : Neural > Diagnostics > **Edit shaper (A/B, not saved)** avec Literal, F1 et F2, plus
Only below 100% et Carry cap (danielblnc uniquement ; Save Settings ne l'enregistre pas ; les clés de l'ini sont
`[DlssNr] AmdEditShaper`, `AmdEditShaperLimit`, `AmdEditShaperScope` et `AmdEditShaperCarryCap`). Si l'un d'eux
donne à 85 % le même rendu qu'à 100 % dans votre jeu, dites-le-nous sur Discord avec des captures. lmxxf n'est
pas concerné.

**Le menu s'ouvrait et se fermait deux fois par appui, ou le clavier et la souris ne répondaient plus sur tout le
bureau quand le menu était ouvert (Assetto Corsa) ?** Corrigé en 0.3.4 : un second appui sur la touche du menu ou
de NR dans les 400 ms est ignoré (`[Hotfix] MenuToggleDebounceMs`, 0 = l'ancien comportement), et quand le menu
est ouvert, le hook clavier ou souris bas niveau du jeu est ignoré mais la touche parvient toujours à Windows
(`[Hotfix] MenuLowLevelHookPassThrough=false` = l'ancien comportement). Pas encore confirmé dans Assetto Corsa :
si cela se produit encore, envoyez le zip du rapport.

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

**Ray Reconstruction est activé dans le jeu mais l'onglet Neural indique « Ray Regeneration is off in this title » ?**
Le jeu n'expose pas ce dont FSR Ray Regeneration a besoin : son plugin DLSS transmet des matrices de caméra vides
(Satisfactory), que le Ray Reconstruction de NVIDIA traite comme optionnelles et dont FSR Ray Regeneration a
besoin. L'upscaling FSR tourne à sa place et NR reprend sa position normale avant le SR ; l'onglet Upscaling
l'indique aussi. Depuis la 0.3.4, il reste désactivé pour toute la session dans un titre Unreal présentant cette
signature. Désactivez Ray Reconstruction dans le jeu et rétablissez les réglages de denoiser du moteur.

**Un jeu Ubisoft sous Anvil (AC Black Flag Resynced, Shadows, Mirage) affiche « DX12 Error 0x80070057 » ?**
Ces jeux embarquent leur propre XeSS Frame Generation. Ce build la leur laisse (la sortie XeFG d'OptiScaler se retire
dans ce cas et l'onglet Frame Gen l'indique) ; utilisez l'option XeSS FG du jeu. Si le problème persiste, réglez
`[FrameGen] Enabled=false` et `[fakenvapi] ForceXeLL=false` et faites un rapport avec le log.

**The Last of Us Part I plante au démarrage ?** C'est l'initialisation de Streamline propre au jeu, un problème
connu d'OptiScaler : renommez `sl.common.dll` dans le dossier du jeu en `sl.common.dll.bak` et choisissez
**FSR 3.1** dans les paramètres du jeu à la place de DLSS.

Notes complètes pour chaque version : `CHANGELOG.md` (dans le zip et dans le dépôt).

## Feuille de route

- **0.3.4** (ce build) — le nouveau menu (l'onglet Neural refait, le même style dans tous les onglets, Save
  report) ; lmxxf plus rapide sur RX 7000 (le palier de taille du réseau par défaut) et sur RX 9070 /
  9070 XT (kernels de lmxxf 0.31) ; lmxxf sur les APU des consoles portables (expérimental ;
  nouvelles tailles de réseau 360p et 576p) ; lmxxf gagne Network output, Encoding, Residual edge fade, le masque de personnages natif et un Fast mode optionnel ; AMDNR Screen GI (preview) ; les réglages du runtime danielblnc (Network style, Tone
  curve, Black lift, Game exposure) et une protection de la couleur des hautes lumières ; réglages et diagnostics
  de Ray Regeneration ; correctifs de l'entrée du menu dans Assetto Corsa, de Shadow of the Tomb Raider, de Marvel's Midnight Suns et de
  The Last of Us Part II, de la fermeture propre et des logs.
- **0.3.3.x** — lmxxf sur RDNA 3 (RX 7000 ; backend propre à AMDNR) ; composition des couleurs
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
- **0.4.0** — les RX 6000
  (RDNA 2), et la prise en charge des titres sans upscaler propre (type Stray), où OptiScaler fournit à la fois
  l'upscaler et la passe neuronale.

---

## Crédits

Ce build est un travail d'assemblage qui repose sur le travail d'autres personnes. S'il vous est utile, les remerciements
reviennent aux projets d'origine (upstream).

- **TheAutomatic** — DLSS 5 AMD project — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD by Daniel Blanco — https://github.com/danielblnc/DLSS-NR-on-AMD (`Runtime.zip`, non modifié)
- **lmxxf** (Kien) — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (le portage du réseau, les kernels et le runtime HIP, MIT)
- **TheAutomatic** — `LmxxfNrRuntime.cpp`, `LmxxfNrApi.h`, `LmxxfProductionOptions.h` : portions contributed to lmxxf by TheAutomatic (MIT)
- **kernels de lmxxf 0.31** dans `LmxxfNrRuntime.pak` (the ViT projection (lmxxf031-vit-wide-deep), the C512 QKV and mix kernels (lmxxf031-c512-m32-mh, lmxxf031-c512-m32-deep) and one-wave-per-head attention (lmxxf031-c64-wave2)) — ceux de lmxxf (Kien, MIT), compilés par AMDNR à partir des sources et de la recette de compilation de lmxxf ; la part d'AMDNR est le chargement, les empreintes SHA-256, le filtrage par GPU et les solutions de repli
- **c32w kernels** (0.3.3.2) — les kernels RDNA 4 à une seule wave propres à AMDNR pour le réseau de lmxxf, Copyright (c) 2026 3zwr1 (AMDNR) ; idées tirées de la documentation publique d'AMD sur WMMA pour RDNA 4 (GPUOpen, ROCm matrix instruction calculator)
- **Le backend RDNA 3 d'AMDNR** (0.3.3 ; les builds pour consoles portables gfx1103 / gfx1150 en 0.3.4), la politique de paliers de taille du réseau et les petites tailles de réseau (0.3.4) — Copyright (c) 2026 3zwr1 (AMDNR)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Dagherbou / OptiScaler_DLSSNR** — https://github.com/Dagherbou/OptiScaler_DLSSNR
- **wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass** — https://github.com/wilsjo2/OptiScaler-DLSSNR-PreSR-Multipass
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, non modifié)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (calculs de la composition des couleurs, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), la base du déblocage multi-images XeFG intégré et de son pacing
- **burak113** — le préprocesseur FSR Ray Regeneration (branche OptiScaler ffx-denoise-experimental, GPL-3.0)
- **Screen-space GI** (l'effet hérité ; retiré du menu en 0.3.4, `[AmdRtgi] Enabled` dans l'ini) — un effet qu'AMDNR a hérité de la lignée OptiScaler-AMD-PreSR ; le mérite en revient à ses auteurs d'origine. Il nécessite le dossier `experimental_lighting` du paquet danielblnc, qu'AMDNR ne distribue pas.
- **AMDNR Screen GI** (preview 0.3.4) — le travail propre d'AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), écrit à partir d'articles publiés (Therrien, Levesque et Gilet 2023 ; Jimenez et al. 2016 ; Schied et al. 2017 ; et les autres cités dans `CHANGELOG.md` et `Licenses/AMDNR_NOTICE.txt`)
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## AMDNR Launcher

**AMDNR Launcher** (nouveau dans la 0.3.4) installe et met à jour AMDNR jeu par jeu. Téléchargez
`AMDNR-Launcher.exe` depuis la release Alpha0.3.4 : <https://github.com/3zwr1/AMD-NR---OptiScaler/releases/download/Alpha0.3.4/AMDNR-Launcher.exe>

Son code source se trouve dans `Launcher/OpenSource/` du dépôt GitHub de ce projet, avec sa propre licence,
`Launcher/OpenSource/LICENSE.txt`. Il n'est **pas** couvert par la licence GPL-3.0 (`LICENSE`) de ce dépôt : il est
en source disponible (source-available), tous droits réservés, Copyright (c) 2026 3zwr1 (AMDNR). Le manifeste du
launcher est `Launcher/manifest.json`. Voir aussi la section 7 de `Licenses/AMDNR_NOTICE.txt`.

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
