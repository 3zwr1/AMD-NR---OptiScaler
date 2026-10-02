# AMDNR — DLSS 5 Neural Rendering sur AMD (build OptiScaler) — v0.3.5

[English](README.md) | [中文](README.zh-CN.md) | [Português](README.pt-BR.md) | [Español](README.es.md) | [العربية](README.ar.md) | **Français** | [Italiano](README.it.md) | [Русский](README.ru.md) | [Polski](README.pl.md)

> **Nous avons besoin de votre soutien.** Rejoignez le serveur Discord — <https://discord.gg/AMDNR> — pour
> l'aide, les rapports de bugs et les builds de test ; chaque rapport accompagné d'un log améliore le build suivant.

DLSS 5 Neural Rendering qui tourne sur les GPU AMD, intégré à OptiScaler pour fonctionner dans n'importe quel
jeu Direct3D 12 dans lequel OptiScaler s'injecte déjà. En plus de la passe neuronale : model interleave pour un
gros gain de framerate, composition résiduelle, frame generation XeSS débloquée jusqu'à 6X (jusqu'à 10X
en option dans les jeux D3D12), et FSR Ray Regeneration pour les jeux qui utilisent DLSS Ray Reconstruction.
Depuis la 0.3.5, **AMDNR Anywhere** (preview) apporte Neural Rendering aux jeux qui n'ont aucun upscaler propre, via
l'AMDNR Launcher, sans rien écrire dans le dossier du jeu (voir « AMDNR Anywhere »).

**Discord : <https://discord.gg/AMDNR>** — support, rapports de bugs (`#bug-report`), builds
de test.

**Soutenir le projet : <https://ko-fi.com/3zinr>**

> **Le runtime danielblnc est l'œuvre de Daniel Blanco.** Le runtime neuronal AMD contenu dans les fichiers `*Runtime.zip`
> (`dlssnr_amd_pass1..3.dll`) est **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> AMDNR le distribue sans modification, avec son autorisation ; ce n'est pas le travail d'AMDNR. Merci de soutenir son projet.
> Les crédits complets de toutes les autres personnes se trouvent à la fin de cette page.

> **Nouveautés de la 0.3.5 :** **AMDNR Anywhere** (preview) : Neural Rendering pour les jeux qui n'ont ni DLSS, ni
> XeSS, ni FSR 2 propres - un seul bouton **PLAY ANYWHERE** dans l'AMDNR Launcher, rien n'est écrit dans le dossier
> du jeu ; RX 9000 (RDNA 4) dans cette release (voir « AMDNR Anywhere »). **Ray Regeneration a son propre onglet**,
> juste après Upscaling, et ses lignes d'état disent, par carte et par API, ce qui tourne et pourquoi pas ; sur RX 7000, elle est,
> depuis la 0.3.5.1, de nouveau proposée en preview, activée par défaut (le débruiteur d'AMD n'a pas de fournisseur pour
> RDNA 3, donc l'image est mise à l'échelle sans débruiteur ; la case de l'onglet Upscaling ou
> `[FSR-RR] FfxDenoiserAllowPreRdna4=false` la coupe et le jeu garde son propre débruiteur). **La
> passe neuronale peut tourner après l'upscaling** (`[DlssNr] AmdPlacement=post`, Neural > Performance > Placement ;
> la valeur par défaut `pre` est inchangée). **L'onglet Frame Gen dit pourquoi rien n'est généré** et nomme les cinq
> étapes (voir l'entrée de la FAQ sur la frame generation). **Plus rapide sur RX 7000 et sur les consoles portables
> de la classe Z1 Extreme :** environ 10 % de temps réseau en moins, image identique au bit près (le jeu de modules
> de la 0.3.5 dans `LmxxfNrRuntime.pak`). **lmxxf 0.37 de Kien (MIT) est activé par défaut sur RX 9000 :** environ
> 20 % de temps réseau en moins sur une RX 9070 XT (expérimental sur les RX 9060 / 9060 XT ;
> `[DlssNr] AmdLmxxfL37=false` le désactive). Sur les consoles portables RDNA 3, **FSR 4 (INT8)** est disponible en
> option expérimentale, et les Style slots personnalisés conservent désormais tout le rendu. Les jeux à résolution
> dynamique ne
> reconstruisent plus le réseau à chaque pas, la retouche transportée ne disparaît plus quand vous bougez sur une
> console portable avec Model interleave, Uncharted: Legacy of Thieves ne plante plus sur RX 9000, et bien d'autres
> correctifs. **Les trois fichiers changent : remplacez `OptiScaler.dll`, `LmxxfNrRuntime.dll` et
> `LmxxfNrRuntime.pak` ensemble ; utilisateurs du launcher : il se met à jour pour vous.** Détails : `CHANGELOG.md`.

> **Nouveautés de la 0.3.4.2 (hotfix) :** le menu dans Assetto Corsa : la touche du menu l'ouvre ou le ferme une seule
> fois par appui, les clics plus courts qu'une image ne sont plus perdus, et le sélecteur de runtime répond à `1` / `2` /
> `Enter` / `Esc`, a une croix dans sa barre de titre et fermer le menu vaut « Decide later ». Le sélecteur de runtime
> n'ouvre plus le menu tout seul (une notification à la place), la section **Ray Regeneration** de l'onglet Neural ne
> se cache plus — elle est toujours là et une ligne grisée dit pourquoi elle ne tourne pas — et le texte Wine / Proton
> dit que Ray Regeneration y est un problème connu. AMDNR **accepte aussi une disposition (layout) de runtime
> danielblnc de plus**, donc un build danielblnc plus récent pourra tourner ici sans aucune mise à jour d'AMDNR. **Neural Rendering est identique octet pour octet à la 0.3.4.1, à cette seule ligne de layout
> accepté près** (la passe neuronale, les deux
> runtimes et le pak sont inchangés) : depuis la 0.3.4.1 ou la 0.3.4, remplacez seulement `OptiScaler.dll` ;
> utilisateurs du launcher : il se met à jour pour vous. Détails : `CHANGELOG.md`.

> **Nouveautés de la 0.3.4.1 (hotfix) :** Ray Regeneration est moins doux sous Windows (un sharpening de 0.25 quand
> le jeu n'en envoie aucun ; pour le désactiver : Image > Sharpness, cochez Override, curseur à 0) ; plus de faux
> pop-up « Upscaler failed to run! » dans Control Resonant ; sous Linux / Proton, le menu fonctionne (confirmé par
> un joueur, y compris avec la frame generation activée) et Ray Regeneration n'y ajoute aucun sharpening par défaut
> (voir « Linux / Proton »). **AMDNR Launcher 0.3.4.1**, conçu à partir de vos retours sur Discord : neuf langues,
> recherche, favoris, masquage, renommage, CHOOSE GAME .EXE, PLAY, un UNINSTALL complet et plus encore (voir « AMDNR
> Launcher »). Neural Rendering est inchangé depuis la 0.3.4 (même runtime et même pak) : si vous venez de la 0.3.4,
> remplacez uniquement `OptiScaler.dll` ; utilisateurs du launcher : il fait la mise à jour pour vous. Détails :
> `CHANGELOG.md`.

> **Nouveautés de la 0.3.4 :** un nouveau menu (l'onglet Neural refait, le même style dans tous les onglets et un
> bouton **Save report** qui zippe vos logs pour un rapport de bug) ; lmxxf est plus rapide sur RX 7000 (1440p FSR
> Quality : 73.3 -> 52.2 ms par exécution du réseau sur une RX 7800 XT, temps réseau mesuré hors jeu) et sur RX 9070 / 9070 XT (kernels
> de lmxxf 0.31) ; lmxxf tourne sur les APU des consoles portables (expérimental ; l'essai d'un testeur, en jeu : environ 29 fps dans Shadow of the Tomb Raider sur une
> ROG Ally) ; un **Fast mode** optionnel pour lmxxf ; **AMDNR Screen GI**, la GI en espace écran d'AMDNR (preview,
> désactivée par défaut) ; et de nombreux correctifs. Remplacez `OptiScaler.dll`, `LmxxfNrRuntime.dll` et `LmxxfNrRuntime.pak` ensemble. Détails :
> `CHANGELOG.md`.

---

## AMDNR - Guide d'installation d'OptiScaler

L'installation est assez simple. **Sous Windows, l'AMDNR Launcher fait tout cela pour vous** (voir « AMDNR
Launcher » plus bas). À la main :

### 1. Télécharger les fichiers

Téléchargez ces fichiers depuis la dernière release sur GitHub (<https://github.com/3zwr1/AMD-NR---OptiScaler/releases> ;
la 0.3.5 correspond au tag Alpha0.3.5) :

* `AMDNR-vX.X.X.zip` (pour la 0.3.5 : `AMDNR-v0.3.5.zip`), avec le runtime lmxxf complet.
* Pour le runtime danielblnc, un zip de runtime : sur **RX 9000 comme sur RX 7000**, `v0.5.0-Runtime.zip`
  (recommandé) de la release Alpha0.3.4.2 ; `v0.4.3-Runtime.zip` (Alpha0.3.4.2 et Alpha0.3.4.1),
  `v0.4.1-Runtime.zip` et `v0.4.0-Runtime.zip` (Alpha0.3.4.1) restent acceptés. Le runtime lmxxf
  se trouve dans `AMDNR-vX.X.X.zip` et n'a besoin d'aucun zip de runtime sur RX 7000 et RX 9000 ; les APU des
  consoles portables n'utilisent que lmxxf. L'AMDNR Launcher choisit le bon zip pour votre GPU. Voir « Contenu des
  archives ».

### 2. Extraire les deux fichiers

Extrayez le contenu des deux fichiers `.zip`.

### 3. Tout copier dans le dossier du jeu

Copiez d'abord tous les fichiers de `AMDNR-vX.X.X` dans le dossier racine du jeu — le même dossier que
celui où se trouve le `.exe` du jeu.

Faites ensuite de même avec tous les fichiers du zip de runtime (p. ex. `v0.5.0-Runtime` sur RX 9000 comme sur RX 7000).

> **Mise à jour depuis un AMDNR plus ancien ?** Recopiez tout en écrasant les fichiers. **En 0.3.5, trois fichiers
> ont changé ensemble :** `OptiScaler.dll` (remplacez le fichier que vous avez renommé, p. ex. `dxgi.dll`, par le
> nouveau renommé de la même façon), `LmxxfNrRuntime.dll` et `LmxxfNrRuntime.pak` (440 Mo). Ne les mélangez pas avec
> d'anciennes copies. Vous pouvez garder votre propre `OptiScaler.ini` : les nouveaux réglages prennent leur valeur
> par défaut. Les fichiers de votre zip de runtime danielblnc restent tels quels. L'AMDNR Launcher le fait pour
> vous : UPDATE ALL, ou REPAIR / UPDATE sur un jeu qui affiche « Update available » (le launcher se met d'abord à
> jour lui-même).

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
* `winhttp.dll`
* `wininet.dll`

Testez un seul nom à la fois. Ne créez pas plusieurs copies de `OptiScaler.dll`. Ce sont les noms sous lesquels le mod
se charge (plus `OptiScaler.asi` avec un chargeur ASI) ; `d3d11.dll` n'en fait pas partie.

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
Le **COLLECT LOGS** de l'AMDNR Launcher écrit le même zip pour n'importe quel jeu et, depuis la 0.3.5.1, y ajoute le
log du plantage et le dump le plus récent après un plantage.

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
(16.6 -> 17.1 ms sur une RX 9070 XT, mesuré en 0.3.3). Désactivé par défaut.

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
- **RX 9000 :** `[DlssNr] AmdLmxxfTierSnap=true` (désactivé par défaut là) déplace la taille NR de lmxxf vers une taille
  de réseau à chaque résolution de rendu : certaines tailles descendent d'un palier (1440p FSR Quality, 1707x960 -> la
  taille 900 : réseau 14.08 -> 9.96 ms par passage sur une RX 9070 XT, mesuré hors jeu, image un peu plus douce),
  d'autres grandissent dans leur palier (80 % d'un rendu 1080p -> 1600x900 : même coût, un peu plus de détails). Non
  défini, les RX 9000 gardent les tailles de la 0.3.3.2.
- Commencez avec une NR resolution de 70 % ou moins (le palier 720 avec un rendu 1080p ; le preset Performance
  est à 70 %). Le coût affiché à côté de NR resolution correspond au palier sur lequel tourne le réseau ; son
  infobulle nomme le palier.
- **0.3.5 : environ 10 % de temps réseau en moins sur RX 7000**, image identique au bit près : le jeu de modules de
  la 0.3.5 dans `LmxxfNrRuntime.pak` (mesuré et vérifié par hash par un testeur sur une RX 7800 XT ; le tableau
  ci-dessus est celui de la 0.3.4).
- **0.3.5 sur RX 9000 : lmxxf 0.37 de Kien (MIT) est activé par défaut** - environ 20 % de temps réseau en moins
  sur une RX 9070 XT à la taille 1080 (14.0 -> environ 11.3 ms, mesuré hors jeu ; l'image n'est pas identique au
  bit près à celle de la 0.3.4.2). Sur les RX 9060 / 9060 XT, il est aussi activé, avec les kernels c32w et FastK, à
  titre expérimental (pas encore essayé sur cette carte). Pour les désactiver : `[DlssNr] AmdLmxxfL37=false`,
  `AmdLmxxfC32w=false`, `AmdLmxxfFastK=false`.

### APU des consoles portables (expérimental, nouveau en 0.3.4)

lmxxf tourne sur les APU des consoles portables dotés d'au moins 12 unités de calcul, via le backend RDNA 3
d'AMDNR par 3zwr1 : **Z1 Extreme, Z2 et Radeon 780M** (gfx1103), **Z2 Extreme, Radeon 890M et 880M** (gfx1150).
C'est expérimental et lent. La ligne Neural runtime affiche « experimental » après le
crédit RDNA 3.
Premiers résultats d'un testeur (ROG Ally, Z1 Extreme) : la sonde de lmxxf hors jeu, 54.7 ms par exécution du réseau
à la taille 360p, 110.9 ms en 576p ; en jeu, l'essai d'un testeur (Shadow of the Tomb Raider, 1280x720 avec XeSS, preset Handheld),
62 ms par exécution du réseau en moyenne en 360p avec le modèle une image sur 4, environ 29 fps avec NR activé.
**La 0.3.5 retire environ 10 % à ces chiffres sur la classe Z1 Extreme** (Z1 Extreme, Z2, Radeon 780M : environ 50
ms en 360p, environ 105 ms en 576p, image identique au bit près, vérifiée par hash par un testeur) ; les modules Z2
Extreme / 890M / 880M sont inchangés.

- **Non pris en charge :** Z1 et Radeon 740M (4 unités de calcul), Radeon 760M (8), Radeon 860M / 840M. Le
  runtime de danielblnc ne tourne pas sur les APU des consoles portables. Les RX 6000 (RDNA 2) sont prévues pour
  la 0.3.6 ; le Steam Deck et les autres APU RDNA 2 ne sont pas pris en charge.
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
- **FSR 4 :** FSR 4 (INT8) est disponible en option expérimentale sur les consoles portables RDNA 3 (onglet Upscaling) ; non validé par AMD.
  La case **FSR 4 (INT8) - Experimental on this GPU (restart)** écrit `[FSR] Fsr4ForceModel=2` ; il ne s'active jamais tout seul
  (avec `Dx12Upscaler=auto` l'upscaler est XeSS). Environ 1,5-3 ms par image sur une Z1 Extreme (estimation) ; avec NR,
  FSR 4 ou la taille 576, pas les deux.
- **Shadow of the Tomb Raider** (et les jeux qui créent leur device D3D12 deux fois) ne plante plus au démarrage de
  l'upscaler (corrigé en 0.3.4).
- **Pilote :** utilisez le pilote Adrenalin d'AMD. lmxxf a besoin de HIP (`amdhip64_7.dll`), que certains pilotes
  de fabricants de consoles portables omettent ; `amd_bridge.log` indique alors que HIP n'est pas disponible.
- **Bouger avec Model interleave (corrigé en 0.3.5) :** la retouche transportée disparaissait dès que vous bougiez
  (« l'effet disparaît quand on bouge ») : sous Model interleave, le réseau tourne sur des images de durées
  inégales, et la protection du transport supposait des images égales. Elle lit désormais les vecteurs de l'image
  précédente à la durée de cette image (les deux runtimes).
- **Utilisez ensemble les fichiers de la 0.3.5 :** la 0.3.5 change les trois fichiers (`OptiScaler.dll`,
  `LmxxfNrRuntime.dll` et `LmxxfNrRuntime.pak`), remplacez-les donc ensemble ; le runtime refuse une console
  portable quand `OptiScaler.dll` est antérieur à la 0.3.4 (« this handheld needs OptiScaler.dll 0.3.4 or newer »).
- **Testeurs équipés d'une console portable :** demandez sur Discord le kit de test pour consoles portables
  (`handheld-test.zip`). Son `run_probe.bat` mesure le réseau sur votre appareil et écrit `handheld_result.txt`
  (votre nom d'utilisateur Windows y est masqué).

## AMDNR Anywhere (preview, nouveau en 0.3.5)

**Ce que c'est.** Neural Rendering pour les jeux qui n'ont ni DLSS, ni XeSS, ni FSR 2 propres - et rien n'est écrit
dans le dossier du jeu. AMDNR tourne à l'intérieur d'un hôte de capture de fenêtre : l'hôte capture la fenêtre du
jeu, la met à l'échelle de votre écran avec FSR 3, et Neural Rendering tourne sur l'image capturée ; notre menu
s'affiche dans l'hôte (votre touche de menu, `INSERT` par défaut) avec son propre onglet **Anywhere**. L'hôte est
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR** :
l'AMDNR Launcher le télécharge depuis la release de son auteur (467 Mo, une seule fois).

**Comment l'utiliser.** Dans l'AMDNR Launcher, un jeu sans upscaler affiche **PLAY ANYWHERE** au lieu d'INSTALL.
Appuyez dessus : le launcher récupère l'hôte (la première fois), lance le jeu, et l'hôte capture sa fenêtre. Lancez
le jeu **en fenêtré ou en fenêtré sans bordure**, pas en plein écran exclusif, et appuyez sur votre touche de menu
pour ouvrir le menu AMDNR dans l'hôte. Un jeu doté de son propre upscaler garde la voie INSTALL normale : Anywhere
est pour les jeux qui n'en ont aucun.

**Les réglages de l'hôte se trouvent dans le menu**, dans Host settings de l'onglet Anywhere, pas dans le launcher :
la taille de la fenêtre du jeu (720p / 900p / 1080p - un conseil sur ce qu'il faut régler dans le jeu ; l'hôte
capture la fenêtre que le jeu ouvre, quelle qu'elle soit), la liste des étapes (V1 : une passe FSR 3 vers l'écran ;
V2 : FSR 3 à 1x, puis une passe de remplissage), le palier NR (Auto / 720 / 900 / 1080), le VRR, le frame pacing et
la **fréquence d'images de l'hôte** (Default = la fréquence de rafraîchissement de votre écran, 60 au plus ; Auto =
la fréquence que le réseau a tenue lors de la dernière session de jeu ; de 30 à 120 ; Display refresh = aucune
limite). Ils s'appliquent au **prochain** PLAY ANYWHERE, et le launcher affiche un résumé en lecture seule à côté du
bouton. Dans l'`OptiScaler.ini` propre à l'hôte, ce sont `[DlssNr] AnywhereWindow`, `AnywhereEffect`,
`AnywhereNrTier`, `AnywhereVrr`, `AnywherePacing` et `AnywhereHostFps`. Limitez aussi le jeu, avec son propre
limiteur, entre 60 et 90 fps : l'hôte ne peut afficher que les images que le jeu a dessinées, et chaque image de
l'hôte exécute le réseau une fois.

**Ce que l'hôte ne peut pas donner au réseau.** Une fenêtre capturée n'a ni profondeur, ni vecteurs de mouvement, ni
jitter, ni exposition propres ; l'hôte estime le mouvement. L'onglet Anywhere nomme donc ce qui est estimé ; les
lignes de l'onglet Neural qui ne peuvent pas agir là sont masquées ou refusées avec une raison (Ray Regeneration,
Screen GI, et Model interleave - il dépenserait la retouche transportée sur un mouvement estimé) ; et une ligne
d'état sur la page Anywhere indique quand le réseau dépasse le budget d'image de l'hôte et ce qu'il faut baisser.
**Une fenêtre de jeu de 1920x1080 ou plus petite est le cas exact au pixel près :** une fenêtre plus grande est
d'abord ramenée au plafond du réseau puis remise à l'échelle, et la page le dit, avec la part des pixels de l'écran
que le réseau a vue. La ligne NR resolution affiche la taille réelle du réseau dans l'hôte.

**État : preview.** RX 9000 (RDNA 4) uniquement dans cette release ; les RX 7000 suivront une fois testées. Un léger
scintillement ou des saccades dans les mouvements rapides peuvent subsister (baissez le palier NR à 720, limitez le
jeu à 60-90 fps, laissez Model interleave désactivé - l'hôte le refuse). L'hôte de capture est récupéré depuis la
release GitHub de son auteur, pas depuis la nôtre. Rapports : le zip **Save report** du menu dans l'hôte (son titre
nomme le jeu mis à l'échelle), ou le COLLECT LOGS du launcher.

## Linux / Proton (Steam Deck, Linux de bureau)

AMDNR tourne sous Proton et Wine en tant que build OptiScaler. **Neural Rendering ne tourne pas sous Linux (Windows
uniquement) :** les deux runtimes NR ont besoin du HIP du pilote AMD pour Windows, que Proton et Wine ne
fournissent pas. Si NR est activé sous Proton, NR ne tourne pas, et depuis la 0.3.5 l'onglet Neural et le rapport
le disent (au lieu de « Idle »). C'est normal : ce n'est ni un plantage ni une installation défectueuse.
L'AMDNR Launcher est un programme Windows qui peut tourner sous Proton
(expérimental, pas encore testé par nos soins, voir plus bas) ; l'installation à la main fonctionne sans lui.

**Ce qui fonctionne :** les upscalers FSR (FSR 3.1, et FSR 4 sur les GPU et pilotes qui le prennent en charge), le
menu (`INSERT`) et **Save report**. Un joueur a confirmé sous Steam Proton (RX 9070 XT, vkd3d-proton, Resident Evil
Requiem) que le jeu démarre, que le menu s'ouvre et prend la souris, et que Save report fonctionne, y compris avec la
frame generation activée.

**Ray Regeneration et frame generation :**
- Problème connu : Ray Regeneration peut afficher des taches roses / magenta sous Proton ; le sharpening par défaut y est désormais désactivé, mais si vous les voyez encore, utilisez FSR seul (FSR 4 sur RX 9000) et envoyez un Save report.
  Sous Proton, AMDNR n'ajoute aucun sharpening après Ray Regeneration quand le jeu n'en envoie aucun (sous Windows,
  il ajoute 0.25) : Image > Sharpness affiche « RR default 0 (off on Proton) », et Override règle toujours votre
  propre valeur.
- La **frame generation** s'active désormais sans planter, mais les compteurs de FPS comptent aussi les images
  générées : avec une limite à 60 FPS ou un V-Sync à 60 Hz, cela fait 30 images réelles, ce qui ressemble à du
  30 FPS. Laissez-la désactivée sous Proton pour l'instant (`[FrameGen] FGOutput=nofg`), ou ne l'utilisez que si le
  jeu atteint environ 60 FPS sans elle, sur un écran de plus de 60 Hz.
- **Titres Vulkan** (jeux RTX Remix, id Tech 8), sous Proton comme sous Windows : Ray Reconstruction et la frame
  generation propre à AMDNR reçoivent la réponse « not supported » par conception (le débruiteur est en D3D12, et
  les buffers de ray tracing restent sur le périphérique Vulkan) ; depuis la 0.3.5, les onglets Ray Regeneration et
  Frame Gen le disent au lieu de vous demander de les activer dans un jeu qui les grise. La super résolution DLSS
  fonctionne via le pont.
- Depuis la 0.3.5, `[Spoofing] Dxgi=true` sous Proton (la voie de mise à niveau vers FSR 4) ne plante plus au
  démarrage.

**Configuration requise :** un Proton ou un Wine récent (testé : Proton 11, c'est-à-dire Wine 11), avec le jeu sous
vkd3d-proton (D3D12) ou DXVK (D3D11), ce qui est le réglage par défaut de Proton. Les versions plus anciennes n'ont
pas été testées.

**L'AMDNR Launcher sous Linux (expérimental, pas encore testé par nos soins).** Le launcher est le même programme
Windows, `AMDNR-Launcher.exe` (autonome : aucun .NET ni autre runtime à installer). Sous Wine / Proton, il détecte
Wine et affiche un avertissement indiquant la marche à suivre. Il cherche aussi votre bibliothèque Steam Linux via le
lecteur `Z:` de Wine (`~/.steam/steam` et `~/.local/share/Steam`, ainsi que les dossiers de bibliothèque listés dans
`libraryfolders.vdf`). Nous ne l'avons pas encore testé nous-mêmes : si vous l'essayez, dites-nous sur Discord si
cela fonctionne. Pour l'essayer :

1. Dans Steam, ajoutez `AMDNR-Launcher.exe` comme jeu non-Steam (**Jeux > Ajouter un jeu non-Steam à ma
   bibliothèque**, Games > Add a Non-Steam Game to My Library).
2. Dans ses **Propriétés > Compatibilité** (Properties > Compatibility), forcez une version de Proton (Proton
   Experimental), puis lancez-le depuis Steam.
3. Si le jeu n'est pas dans **LIBRARY** (BIBLIOTHÈQUE), appuyez sur **ADD** (AJOUTER) et choisissez le dossier du
   jeu (vos dossiers Linux se trouvent sur le lecteur `Z:`) ; si le launcher prend le mauvais `.exe`, utilisez
   **CHOOSE GAME .EXE** (CHOISIR LE .EXE DU JEU).
4. Sélectionnez le jeu et appuyez sur **INSTALL** (INSTALLER).
5. Dans **Propriétés > Général > Options de lancement** du jeu (Properties > General > Launch Options), saisissez
   `WINEDLLOVERRIDES="dxgi=n,b" %command%` (si le launcher a utilisé un autre nom de DLL pour ce jeu, mettez ce nom
   à la place de `dxgi`), puis poursuivez avec les étapes 5 et 6 de l'installation à la main ci-dessous (lancez le
   jeu depuis Steam ; le bouton **PLAY** du launcher est désactivé sous Wine / Proton).

**Installation à la main** (sans le launcher) :

1. Téléchargez `AMDNR-vX.X.X.zip` depuis la page des releases (pour la 0.3.5 : `AMDNR-v0.3.5.zip`). Les zips de
   runtime danielblnc (`v0.5.0-Runtime.zip` et les autres) ne servent qu'à Neural Rendering ; vous n'en avez donc pas
   besoin sous Linux (en copier un ne pose aucun problème).
2. Extrayez le zip et copiez tout dans le dossier du jeu, à côté du `.exe` du jeu.
3. Renommez `OptiScaler.dll` en `dxgi.dll`.
4. Dans Steam, ouvrez **Propriétés > Général > Options de lancement** du jeu (Properties > General > Launch Options)
   et saisissez :

   ```
   WINEDLLOVERRIDES="dxgi=n,b" %command%
   ```

   Cela indique à Wine de charger le `dxgi.dll` du dossier du jeu au lieu du sien ; sans cela, AMDNR ne se charge
   pas. Si vous avez utilisé un autre nom (par exemple `winmm.dll` ou `version.dll`), mettez ce nom à la place de
   `dxgi`, p. ex. `WINEDLLOVERRIDES="winmm=n,b" %command%`. Lutris, Heroic et Bottles : ajoutez le même override
   (`dxgi` = `native,builtin`) dans les réglages d'overrides de DLL ou d'environnement du runner.
5. Dans `OptiScaler.ini`, réglez `[FrameGen] FGOutput=nofg` (frame generation désactivée, voir plus haut).
6. Lancez le jeu et appuyez sur `INSERT` pour ouvrir le menu. Configurez l'upscaler à cet endroit.

**Le menu.** En 0.3.4, avec la frame generation activée, le menu pouvait s'ouvrir sans prendre la souris ni le
clavier, ou ne pas s'ouvrir du tout. La 0.3.4.1 attache le menu à la fenêtre du jeu ; un joueur a confirmé sous
Proton que le menu s'ouvre et prend la souris, y compris avec la frame generation activée. Si cela se produit encore
sur votre configuration, AMDNR affiche l'avertissement « Menu window lost ». Réglez alors `[FrameGen] FGOutput=nofg`
dans `OptiScaler.ini` ; si le menu ne répond toujours pas, réglez aussi `[Menu] OverlayMenu=false` (le menu
classique, qui ne dépend pas de la fenêtre d'overlay).

**HDR.** AMDNR n'active pas le HDR sous Proton. Le HDR dépend de votre configuration Proton et de votre bureau : un
build de Proton qui prend en charge le HDR et une session capable d'afficher du HDR (par exemple gamescope, ou un
bureau Wayland avec le HDR activé). Si le HDR fonctionne dans le jeu sans AMDNR, il continue de fonctionner avec
AMDNR ; si l'option HDR du jeu est grisée, la solution se trouve dans votre configuration Proton ou de bureau.

**Signaler un problème sous Linux :** utilisez le bouton **Save report** du menu (gardez `[Log] LogToFile=true`, la
valeur par défaut, pour que le rapport contienne le log de cette session) ; le rapport indique si le jeu tournait sous
Wine/Proton, vkd3d-proton ou DXVK. Merci d'indiquer aussi votre distribution, votre GPU, votre version de Mesa et
votre version de Proton.

## Configuration requise

- Windows 10 ou 11 (64 bits) pour Neural Rendering. L'AMDNR Launcher est un programme Windows qui peut tourner sous
  Proton (expérimental, pas encore testé par nos soins). Sous Linux / Proton, AMDNR tourne en tant que build
  OptiScaler sans NR (voir « Linux / Proton »).
- Un GPU AMD avec AMD Software: Adrenalin Edition 26.9.1 ou plus récent. Le runtime neuronal utilise HIP via le pilote ; ni le SDK HIP
  ni le mode développeur ne sont nécessaires. Puces concernées :
  - RX 9000 (RDNA 4) : les deux runtimes.
  - RX 7000 (RDNA 3, de bureau et portables) : les deux runtimes - lmxxf via le backend RDNA 3 d'AMDNR, plus
    lentement que sur RDNA 4 (le palier de taille du réseau est activé par défaut, voir plus haut).
  - Strix Halo (Radeon 8060S / 8050S) : lmxxf.
  - APU des consoles portables avec 12 unités de calcul ou plus (Z1 Extreme / Z2 / 780M, Z2 Extreme / 890M /
    880M) : lmxxf, expérimental et lent. Z1 (4 CU), 760M / 740M et 860M / 840M : non pris en charge.
  - RX 6000 (RDNA 2) : pas encore pris en charge, prévu pour la 0.3.6. Steam Deck et APU RDNA 2 : non pris en
    charge (pour Neural Rendering ; pour les upscalers sous Proton, voir « Linux / Proton »).

  L'onglet Neural indique ce que votre GPU peut faire tourner (survolez les entrées de runtime, ou voyez la
  ligne GPU dans Diagnostics).
- **AMDNR Anywhere** (preview) : Windows, une carte RX 9000 (RDNA 4) dans cette release, et l'AMDNR Launcher, qui
  récupère l'hôte de capture ; le jeu tourne en fenêtré ou en fenêtré sans bordure. Voir « AMDNR Anywhere ».
- Un jeu Direct3D 12, Direct3D 11 ou Vulkan. Le chemin neuronal AMD lui-même est en D3D12 ; les titres D3D11 et
  Vulkan y accèdent via le pont D3D12 d'OptiScaler, ce qui signifie que l'upscaler doit être
  l'un des backends « w/Dx12 » (`ffx_12`). Laissez `Dx11Upscaler` / `VulkanUpscaler` sur `auto`
  et ce build le choisit pour vous quand le rendu neuronal est activé. Avec Neural Rendering activé, la liste Upscaling les nomme « ... w/Dx12 - Neural ».
- Environ 2 Go de VRAM libre à des résolutions de rendu de l'ordre du 1080p.

## Contenu des archives

**AMDNR-vX.X.X.zip**

| Fichier | Description |
|---|---|
| `OptiScaler.dll` | OptiScaler avec le backend AMD DLSS-NR (AMDNR 0.3.5). Renommez-le comme indiqué dans le guide. |
| `OptiScaler.ini` | Paramètres. Neural Rendering est activé ; les logs sont activés pour qu'un rapport de bug ait quelque chose à joindre. |
| `LmxxfNrRuntime.dll` | Le runtime neuronal lmxxf (0.3.5 : il vérifie chaque module HIP du pak par rapport à la liste d'empreintes du pak lui-même avant usage, nomme les deux côtés quand aucun adaptateur HIP ne correspond au GPU du jeu, conserve les pas de résolution dynamique sans reconstruire le réseau, et ne lit plus les variables d'environnement de lmxxf qui changent l'image ; les kernels de lmxxf, y compris ceux de lmxxf 0.31, les kernels c32w d'AMDNR, les petites tailles de réseau et le masque de personnages natif). Utilisé uniquement s'il est choisi ; lit `LmxxfNrRuntime.pak` placé à côté, voir « Le runtime lmxxf ». |
| `LmxxfNrRuntime.pak` | Les poids, les modules HIP et les shaders du runtime lmxxf dans un seul fichier chiffré (465 Mo ; 0.3.5 : le jeu de modules pour RX 7000 et pour les consoles portables de la classe Z1 Extreme - Z1 Extreme, Z2, Radeon 780M - est environ 10 % plus rapide, même image ; les RX 9000 reçoivent les modules de lmxxf 0.37 / 0.39 de Kien (MIT) à côté de leur jeu de base inchangé ; les modules Z2 Extreme / 890M / 880M et Strix Halo sont inchangés). Seul le runtime lmxxf le lit ; on peut le garder sans risque avec le runtime danielblnc. |
| `OptiScaler\` | FSR, XeSS, le denoiser FidelityFX et le D3D12 Agility SDK utilisés par OptiScaler. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | dlssg-to-fsr3 de Nukem9, non modifié et renommé : les appels DLSS Frame Generation du jeu sont traités par la frame generation FSR 3, y compris sous Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, voir `Licenses/`. |
| `Licenses\`, `LICENSE` | Licences tierces, les mentions d'AMDNR (`AMDNR_NOTICE.txt`) et la licence GPL-3.0 de ce build. |
| `SHA256SUMS.txt` | Sommes de contrôle de chaque fichier de ce zip, et des fichiers des zips de runtime danielblnc qu'il liste. |

**Les zips de runtime danielblnc** (DLSS-NR on AMD by Daniel Blanco, non modifié, avec son autorisation ; utilisez-en un)

Lequel choisir : sur **RX 9000 comme sur RX 7000**, `v0.5.0-Runtime.zip` (recommandé) de la release Alpha0.3.4.2 ;
`v0.4.3-Runtime.zip` (Alpha0.3.4.2 et Alpha0.3.4.1), `v0.4.1-Runtime.zip` et `v0.4.0-Runtime.zip` (Alpha0.3.4.1)
restent acceptés. Le runtime lmxxf n'a besoin d'aucun zip de runtime sur RX 7000 et RX 9000 ; les APU des consoles
portables n'utilisent que lmxxf. L'AMDNR Launcher propose 0.5.0 (recommandé), 0.4.3, 0.4.1 et 0.4.0, et le choisit
pour vous.

| Zip | Release | Runtime danielblnc |
|---|---|---|
| `v0.5.0-Runtime.zip` | Alpha0.3.4.2 | 0.5.0, **recommandé sur RX 9000 et RX 7000** ; les réglages du runtime danielblnc fonctionnent avec lui, et depuis la 0.3.5 votre propre clé `Async` dans son `dlssnr_on_amd.ini` lui parvient |
| `v0.4.3-Runtime.zip` | Alpha0.3.4.2 (et Alpha0.3.4.1) | 0.4.3, toujours accepté ; les réglages du runtime danielblnc fonctionnent avec lui |
| `v0.4.1-Runtime.zip` | Alpha0.3.4.1 (et Alpha0.3.4) | 0.4.1, toujours accepté. Network style, Tone curve, Black lift et Game exposure sont grisés avec lui |
| `v0.4.0-Runtime.zip` | Alpha0.3.4.1 (et Alpha0.3.4) | 0.4.0, toujours accepté ; les réglages du runtime danielblnc fonctionnent avec lui |
| `v0.3.3-Runtime.zip` | [Alpha0.3.4](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/tag/Alpha0.3.4) | 0.3.3, retiré : plus recommandé. Il fonctionne toujours si vous l'avez déjà ; les réglages du runtime danielblnc fonctionnent avec lui |
| `Runtime.zip` | Alpha0.3.4 | 0.3.1 ; les réglages du runtime danielblnc sont grisés avec lui |

**danielblnc 0.5.0 est le runtime danielblnc recommandé depuis la 0.3.5** (il tourne aussi sur la 0.3.4.2). La 0.3.5
transmet aussi votre propre clé `Async` (ou l'ancienne `Inline`) de `dlssnr_on_amd.ini` jusqu'au runtime au lieu
d'imposer le mode même image ; sans aucune des deux clés, une installation par défaut est inchangée. Un build
danielblnc plus récent que la 0.5.0 n'est pas piloté par cette release.

Chacun contient :

| Fichier | Description |
|---|---|
| `dlssnr_amd_pass1..3.dll` | Le runtime neuronal AMD, non modifié. Trois copies pour que le multi-passe en ait une par passe. |
| `dlssnr_on_amd_weights.bin` | Les poids du réseau chargés par le runtime. |
| `danielblnc_ATTRIBUTION.txt` | Le crédit de Daniel Blanco et les conditions sous lesquelles AMDNR distribue son runtime. |

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
- **Performance** : **Placement** (avant / après l'upscaling, nouveau en 0.3.5 ; voir « Réglages à connaître »), NR
  resolution (%) avec son coût, Neural passes, Full network, Fast mode, Dynamic NR resolution, Model interleave
  (Interleave preset et la ligne de pacing apparaissent dessous tant qu'il est activé).
- **Quality** : Residual strength, Residual limit, Temporal stability, Sharpening (CAS), et **More quality
  options** (Network history - une seule case pour les deux runtimes -, Output smoothing, Stability mode,
  Residual temporal, Residual edge fade, Still-surface steadiness).
- **Image look** : Colour composition, Detail et Colour strength, et trois sections repliables : **Model
  strength** (Tone et Structure intensity, Character structure, Edit detail / colour, Edge guard, Native
  character mask, et Network style, Tone curve et Black lift de danielblnc), **Exposure and highlights**
  (Auto-exposure, son plafond des hautes lumières, Highlight colour guard, Game exposure) et **Appearance filter** (avec son mot off / on après le nom). Un « default » ou « custom » discret après le nom d'une
  section repliable indique si vous y avez changé quelque chose.
- **Ray Regeneration** : depuis la 0.3.5, un renvoi. Tant que Ray Regeneration tourne dans le titre, ses réglages se
  trouvent dans son propre onglet **Ray Regeneration** (juste après Upscaling) et cette ligne propose un bouton
  **Open Ray Regeneration** ; tant qu'il ne tourne pas, la ligne dit pourquoi (le jeu n'a pas activé Ray
  Reconstruction, le pilote a refusé le débruiteur sur cette carte, Ray Regeneration a renoncé à ce titre et
  pourquoi, ou quand il a tourné pour la dernière fois).
- **La ligne d'outils**, fermée au démarrage : **Diagnostics** (Network output, Debug view, Edit shaper A/B, NR
  cost, les indicateurs de ghosting et d'auto-réglage, la ligne GPU, **Save report** ; la vue de debug RR est dans
  l'onglet Ray Regeneration depuis la 0.3.5), **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight
  proxy) et **Experimental** (AMDNR Screen-space GI, en preview).

Un réglage que le runtime actif n'a pas est grisé avec une courte étiquette (p. ex. « not in lmxxf yet ») ou
masqué avec un décompte (« 3 danielblnc-only options hidden ») ; changer de runtime ne déplace aucune autre ligne.

**Les autres onglets :** Upscaling commence par l'upscaler, une ligne d'état et Render resolution (les anciens
Upscale Ratio Override et Output Scaling) ; sur une carte qui n'est pas NVIDIA, « DLSS w/Dx12 » n'est plus proposé.
**Ray Regeneration** (nouveau en 0.3.5) suit Upscaling tant que Ray Regeneration tourne dans le titre : les lignes
d'état (par carte et par API), la ligne **Denoiser backend** (Automatic / Off - Off indique au jeu que Ray
Reconstruction n'est pas pris en charge, il garde donc son propre débruiteur ; après un redémarrage), les réglages,
More Ray Regeneration options, et son propre bloc Diagnostics avec la vue de debug RR et le chiffre de bruit (grain
en entrée et en sortie, scintillement caméra immobile) ; il n'est jamais affiché dans AMDNR Anywhere. Image contient
Sharpness, Textures, Init Flags et le Magnifier. Frame Gen commence par FG Input et FG Output et, depuis la 0.3.5,
une ligne qui nomme l'étape qui manque encore avant que quoi que ce soit soit généré. Interface contient l'overlay
FPS et Keybinds (un bouton par touche). Advanced commence par Active Quirks, puis Display (V-Sync), Compatibility et
Logging. Dans AMDNR Anywhere, le menu affiche un onglet **Anywhere** (la ligne de capture et de réseau, les réglages
de l'hôte) à la place des onglets Frame Gen et Advanced. Les réglages, les clés et ce qu'écrit Save Settings ne
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
- **Placement** (Neural > Performance, `[DlssNr] AmdPlacement = pre | post`, nouveau en 0.3.5, les deux runtimes) —
  l'endroit où tourne la passe neuronale. `pre` (la valeur par défaut, et ce que faisaient tous les builds
  précédents) retouche l'image en résolution de rendu que l'upscaler s'apprête à lire. `post` retouche à la place
  l'image finie de l'upscaler, en résolution d'affichage : plus net, car l'upscaler ne refiltre plus la retouche, et
  plus coûteux - le réseau tourne à la taille d'affichage jusqu'à son plafond de 1920x1080, donc un écran 1080p paie
  le palier le plus haut quelle que soit la NR resolution, et un écran 1440p ou 4K reçoit une retouche de 1080
  lignes remise à l'échelle - un peu moins tolérant en mouvement, et le HUD est inclus si le jeu le compose avant
  l'upscaling. Refusé (retour à `pre`, une ligne dans `amd_bridge.log`) dans AMDNR Anywhere, en mode image finale,
  et dès que Ray Regeneration a tourné dans le titre. La ligne NR resolution affiche les deux tailles tant que
  `post` tourne.
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
- **La frame generation est désactivée dans un ini neuf**, et il faut cinq étapes pour l'activer : FG Input et FG
  Output dans l'onglet Frame Gen (p. ex. « DLSSG via Streamline » dans un jeu avec la frame generation DLSS, et
  XeFG), **Save Settings**, un redémarrage complet du jeu, la frame generation **propre** au jeu activée, puis
  **Active** coché sous Frame Generation. Depuis la 0.3.5, l'onglet et le log nomment l'étape qui manque. La liste
  complète, avec ce qu'il faut désactiver dans le jeu, se trouve dans la FAQ plus bas (« Frame generation : aucun
  gain de fps ? »).
- **Frame generation multi-images XeFG** — le 3X à 6X est intégré et activé par défaut (`XeFG\UnlockMFG`),
  pour la copie d'OptiScaler comme pour celle du jeu. **Supprimez `XeFGUnlock.asi`** de `OptiScaler\plugins`
  si vous l'avez encore : deux copies du même patch font planter le jeu.
  **Jusqu'à 10X, sur activation manuelle** (jeux D3D12 uniquement) : réglez *XeFG ceiling (restart)* sous FG Output dans
  l'onglet Frame Gen (4X, 6X par défaut, 8X ou 10X ; `[XeFG] MaxInterpolatedFrames`), redémarrez, puis choisissez le
  multiplicateur dans la liste MFG. Au-delà de 6X, il faut le provider XeFG propre à OptiScaler avec Extra pacing activé ;
  la copie de XeSS 3 propre au jeu reste limitée à 6X. Le 10X nécessite un écran 360 Hz ou plus et une limite de FPS
  réglée sur la fréquence de rafraîchissement / 10 ; la latence est élevée, et le provider réserve environ 128 Mio de VRAM
  en plus en 4K. Le 7X-10X n'est pas encore confirmé en jeu : testeurs, merci d'envoyer `OptiScaler.log`.
- **FSR Ray Regeneration** — RX 9000 (RDNA 4) ; sur RX 7000 (RDNA 3), en preview, activée par défaut, sans débruiteur (voir plus bas) ; et seulement dans les jeux qui utilisent DLSS Ray Reconstruction (Cyberpunk 2077,
  Alan Wake 2), le jeu devant tourner en DLSS (spoofing activé), avec le ray tracing et Ray Reconstruction
  activés dans ses propres paramètres. Neural Rendering s'exécute alors après lui, sur sa sortie, ce qui coûte
  davantage : baissez la NR resolution si le framerate chute. Depuis la 0.3.5, ses réglages se trouvent dans son
  propre onglet **Ray Regeneration**, juste après Upscaling, affiché tant que Ray Regeneration tourne dans le
  titre ; l'onglet Neural y renvoie et, tant que Ray Regeneration ne tourne pas, garde la ligne grisée qui dit
  pourquoi (le jeu n'a pas activé Ray Reconstruction, le pilote a refusé le débruiteur sur cette carte, Ray
  Regeneration a renoncé à ce titre et pourquoi, ou depuis combien de temps il a tourné pour la dernière fois). La
  ligne **Denoiser backend** de l'onglet (`[FSR-RR] RrBackend = auto | off`) peut indiquer au jeu que Ray
  Reconstruction n'est pas pris en charge, pour qu'il garde son propre débruiteur (au prochain démarrage du jeu).
  Sur un titre **Vulkan**, Ray Reconstruction est « not supported » par conception (le débruiteur est en D3D12) et
  l'onglet le dit. L'option **path-traced profile** (moins de grain sur les visages en path tracing) doit être
  activée manuellement depuis la 0.3.3.1 : cochez-la à cet endroit pour l'essayer dans Resident Evil Requiem ou
  PRAGMATA. Le même onglet contient l'intensité du bias mask et l'option **skin smoothing** (lissage de la peau ;
  expérimental, pour les jeux qui exposent un guide SSS ; désactivé par défaut, mais activé par défaut dans Resident
  Evil Requiem depuis la 0.3.3.2) ; les réglages temporels sont sous *More Ray Regeneration options*, et la vue de
  debug RR et le chiffre de bruit (grain en entrée et en sortie, scintillement caméra immobile) sont dans le bloc
  Diagnostics propre à l'onglet. Sur RX 7000 (RDNA 3), Ray Regeneration
  est proposée en preview (0.3.5.1 ; la 0.3.5 ne la proposait pas par défaut) : le jeu peut activer Ray Reconstruction
  et les réglages de Ray Regeneration s'affichent, mais le débruiteur d'AMD n'a pas de fournisseur pour RDNA 3, donc le
  débruiteur refuse de démarrer et l'image Ray Reconstruction est mise à l'échelle sans débruiteur, ce qui peut
  paraître plus bruité que le débruiteur du jeu ; la page Ray Regeneration dit ce qui tourne. La case de l'onglet
  Upscaling **Preview: Ray Regeneration on this card (restart)** (marquée « preview - on by default ») est cochée par
  défaut ; décochez-la, ou mettez `[FSR-RR] FfxDenoiserAllowPreRdna4=false`, et le jeu garde son propre débruiteur.
  Un débruiteur propre à AMDNR pour les RX 7000 est prévu. Les RX 6000
  et plus anciennes ne l'ont qu'avec `[FSR-RR] FfxDenoiserAllowPreRdna4=true` (onglet Upscaling :
  **Offer FSR Ray Regeneration on this GPU (restart)**). **Sharpening après RR**
  (0.3.4.1) : quand le jeu n'envoie aucune valeur de sharpness, AMDNR applique un sharpening de 0.25 après RR sous
  Windows (0 sous Linux / Proton) ; pour le désactiver : Image > Sharpness, cochez Override, curseur à 0. Depuis la 0.3.4.2 ce nombre est sa propre
  clé ini, `[Sharpness] RrDefaultSharpness` (même valeur par défaut 0.25) : mettez-y 0.15, 0.10 ou 0 sans toucher à
  Override, et une valeur que votre ini a gardée sous `[Sharpness] Sharpness` alors qu'Override est désactivé est
  signalée dans le menu comme en attente.
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
pilote pas ?** Vos `dlssnr_amd_pass1..3.dll` sont un build de danielblnc que cet AMDNR ne connaît pas (un ensemble
0.2.16 a été vu en circulation), ou l'une des trois est manquante. Depuis 0.3.3.2, l'onglet Neural nomme le fichier
et sa version, et indique la marche à suivre. Utilisez le runtime recommandé, en prenant les trois DLL de passe dans
le même zip : sur **RX 9000 comme sur RX 7000**, `v0.5.0-Runtime.zip` de la release Alpha0.3.4.2 (149,550,553
octets, avec un SHA256 qui commence par `7a49ab0e`) ; `v0.4.3-Runtime.zip` (Alpha0.3.4.2 et Alpha0.3.4.1 ;
116,484,918 octets, SHA256 qui commence par `07dd7774`), `v0.4.1-Runtime.zip` et `v0.4.0-Runtime.zip` (Alpha0.3.4.1)
restent acceptés (la `dlssnr_amd_pass1.dll` de `v0.4.1-Runtime.zip` fait 9,916,928 octets, avec un SHA256 qui
commence par `823063eb` ; dans `v0.4.0-Runtime.zip` : 10,027,008 octets, `d62be3d8`). Builds pris en charge :
0.2.17, 0.3.0, 0.3.1, 0.3.2, 0.3.3, 0.4.0 et les zips de runtime nommés ci-dessus, jusqu'à la 0.5.0. N'installez pas
le setup de danielblnc, ni ses `dxgi.dll` / `version.dll` / `winhttp.dll`, à côté d'AMDNR : AMDNR fait déjà tourner
son runtime. **Un build danielblnc plus récent que la 0.5.0 n'est pas piloté par cette release :** l'onglet Neural
nomme le fichier et sa version et le dit. Avec 0.5.0, 0.4.3, 0.4.1 et 0.4.0, les réglages propres à danielblnc
(Network style, Tone curve, Black lift, Game exposure, Fast mode) fonctionnent, et depuis la 0.3.5 votre propre clé
`Async` dans `dlssnr_on_amd.ini` parvient au runtime (voir « Contenu des archives »).

**lmxxf ne fait rien, ou s'arrête aussitôt, sur un PC avec une puce graphique intégrée ?** Corrigé en 0.3.3.2. Sur un
Ryzen de bureau avec sa puce graphique intégrée activée, un portable avec un APU AMD et une Radeon, ou un PC avec deux
GPU AMD, le GPU du jeu n'est souvent pas le périphérique HIP 0. lmxxf échouait alors dès sa première image
(`hipErrorInvalidHandle (400)`, puis « session is poisoned » dans `lmxxf_backend.log`) et restait désactivé. Remplacez
à la fois `OptiScaler.dll` (le fichier que vous avez renommé, p. ex. `dxgi.dll`) et `LmxxfNrRuntime.dll` par les
fichiers de la 0.3.3.2 ou d'une version plus récente. Pas encore testé sur un tel PC : si lmxxf s'arrête encore, l'onglet Neural indique désormais
pourquoi ; envoyez `lmxxf_backend.log` et `amd_bridge.log` (ce dernier liste les périphériques HIP).

**Un PC avec une puce graphique intégrée et une Radeon (un Ryzen de bureau avec sa puce intégrée activée, ou un
portable) : NR ne démarre jamais, et l'onglet Neural ou la ligne GPU nomme la puce intégrée ?** La passe neuronale
d'AMDNR s'exécute sur le GPU avec lequel le jeu dessine. Si Windows a lancé le jeu sur la puce intégrée, NR ne
s'exécute pas du tout sur votre Radeon. Affectez le jeu au GPU dédié : Paramètres Windows > Système > Écran >
Graphiques, ajoutez le `.exe` du jeu, Options, Hautes performances ; puis relancez le jeu et vérifiez la ligne GPU dans
Neural > Diagnostics, qui nomme l'adaptateur sur lequel NR s'exécute (`OptiScaler.log` contient une ligne
`AMD neural: NR runs on ...` quand cet adaptateur n'est pas le GPU principal). Vu dans Starfield sur un Ryzen de bureau.
Depuis la 0.3.5, la ligne de l'onglet Neural dit dans lequel des trois cas vous êtes - pas encore de runtime, NR qui
tourne **sur la puce graphique intégrée** (un APU qu'un runtime accepte, comme une Radeon 780M à côté d'une carte
Radeon : NR tourne là, bien plus lentement que sur la carte), ou un runtime sur un autre adaptateur - et
`amd_bridge.log` liste chaque adaptateur une fois.

**La ligne d'état de lmxxf affiche `c32w=off:nofile` sur une RX 9070 / 9070 XT ?** Un ancien dossier
`DLSS5-AMD\native-game-tiled-assets` à côté du `.exe` du jeu (reste d'une ancienne installation de lmxxf) est
utilisé à la place de `LmxxfNrRuntime.pak`. Il ne contient pas les kernels c32w, donc lmxxf tourne à l'ancienne
vitesse. Supprimez ou renommez le dossier `DLSS5-AMD` : le pak contient tout ce dont lmxxf a besoin. Un
`LmxxfNrRuntime.pak` antérieur à la 0.3.3.2 donne le même état ; remplacez-le par celui de cette version.
`fk=fff-` sur la même ligne signifie la même chose (un ancien pak ou un dossier isolé) : lmxxf tourne
toujours, à l'ancienne vitesse.

**danielblnc : le style NR change encore quand la NR resolution quitte 100 % ?** Toujours ouvert de la 0.3.4 à la
0.3.5, et le réglage par défaut ne change pas. À 100 %, Residual strength 0.99 donne 99 % de 1.00 (corrigé en 0.3.3.2) ; en
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

**Le menu s'ouvrait tout seul sur le sélecteur de runtime, les clics ne faisaient rien, ou la touche du menu ne le
cachait que tant qu'elle restait enfoncée (Assetto Corsa) ?** Corrigé en 0.3.4.2 : la touche du menu l'ouvre ou le
ferme une seule fois par appui physique (un message de touche qui arrive en retard est ignoré), les clics et les
touches du menu plus courts qu'une image sont rejoués, le sélecteur de runtime répond à `1` / `2` / `Enter` / `Esc` et
à la croix de sa barre de titre, et fermer le menu vaut « Decide later » ; le sélecteur n'ouvre plus le menu tout
seul. Pas encore confirmé par le joueur d'Assetto Corsa : si cela se produit encore, envoyez le zip du
rapport. Pour retrouver la touche du menu et les clics de la 0.3.4.1 : ajoutez vous-même
`DiagInputHooksSkip=presslatch,clickreplay` sous `[Hotfix]` dans votre `OptiScaler.ini` (pas de nouvelle clé ; l'ini
livré ne décrit la ligne qu'en commentaire).
La 0.3.5 ajoute deux choses pour Assetto Corsa : AMDNR s'efface devant un `nvngx.dll` étranger dans le dossier du
jeu et protège la création du device D3D11On12, et un jeu DX11 qui ne présente jamais via D3D12 reçoit une file
D3D12 d'amorçage pour la passe neuronale (`[DlssNr] AmdBootstrapQueue`, auto). Pas encore confirmé par un joueur
d'Assetto Corsa.

**Uncharted: Legacy of Thieves Collection plantait quelques secondes après le lancement sur RX 9000 avec Neural
Rendering activé ?** Corrigé en 0.3.5 : le jeu exécute son travail sur de petites fibres de 192 Kio, et la première
initialisation de HIP (le pilote compile ses kernels auxiliaires dans le jeu) faisait déborder cette pile à la
première image NR. La première utilisation de HIP par le pont neuronal tourne désormais sur sa propre grande pile
(`[DlssNr] BigStackCall`, auto). Si vous aviez mis `[DlssNr] Enabled=false` dans ce jeu pour la 0.3.4.2,
réactivez-le. Pas encore confirmé par un joueur sur RX 9000 : si cela se produit encore, envoyez le zip du rapport.

**Un jeu à résolution dynamique (The Last of Us Part II) scintille avec Neural Rendering activé ?** Corrigé en
0.3.5 : le jeu changeait sa taille de rendu des centaines de fois par minute, et chaque pas reconstruisait le réseau
et réinitialisait son historique. Un pas qui reste dans l'allocation est désormais conservé (ni stabilisation, ni
préchauffage, ni réinitialisation de l'historique, les deux runtimes) ; une image plus grande ou une vraie baisse
réalloue toujours. La ligne de statistiques du rapport compte les pas conservés. Pas encore confirmé dans ce jeu.

**Frame generation : aucun gain de fps, ou « restart the game » sans fin ?** Dans presque tous les rapports, la
frame generation n'était tout simplement pas activée - désactivée, pas cassée. Il faut cinq étapes, et depuis la
0.3.5 l'onglet Frame Gen et le log nomment celle qui manque :

1. Onglet Frame Gen : choisissez **à la fois** FG Input et FG Output. Un jeu DX12 avec sa propre frame generation :
   son DLSS FG ou son FSR 3.1 FG est l'entrée (jeux avec FSR 3.1 FG : « FSR 3.1 FG », pas « FSR 3.0 FG ») ; aucune
   FG dans le jeu : FG Input = OptiFG (Upscaler), HUD fix activé. DX11 : OptiFG uniquement. Vulkan : pas de sortie
   FSR FG / XeFG (utilisez celle du jeu).
2. **Save Settings**.
3. **Fermez complètement le jeu et relancez-le** - la FG ne peut pas s'activer en cours de partie.
4. Dans les options graphiques du jeu lui-même, **activez** sa frame generation : DLSS Frame Generation (avec DLSS
   comme upscaler) pour l'entrée DLSSG, la frame generation FSR (avec FSR) pour l'entrée FSR 3.1 FG.
5. Rouvrez le menu, onglet Frame Gen, cochez **Active** sous Frame Generation. Rien n'est généré tant que cette case
   n'est pas cochée (XeFG peut demander un redémarrage de plus).

Ensuite, **désactivez** dans le jeu : le plein écran exclusif (XeFG a besoin du fenêtré sans bordure), le V-Sync et
les limites de FPS (ou limitez au double de vos fps de base), et la frame generation XeSS propre au jeu s'il en a
une (un seul générateur d'images par fenêtre ; un jeu qui charge sa propre XeSS FG reçoit une note dans l'onglet).
Aucun gain de fps = la FG est désactivée - le log dit
`... Enabled is off ...: no frames are generated. Frame generation off, not broken.` ; la moitié des fps = une
limite ou le V-Sync retient les images générées. Si cela échoue encore, appuyez sur **Save report** après l'échec et
publiez le zip avec le jeu, la paire choisie, l'option de frame generation du jeu qui était activée, fenêtré sans
bordure ou plein écran, HDR activé ou non, et ce que vous avez vu. La FAQ complète est épinglée dans le salon de
support du Discord.

**D'autres mods (RED4ext, Cyber Engine Tweaks pour Cyberpunk 2077) ou ReShade à côté d'AMDNR ?** Depuis la 0.3.5, le
rapport et le log nomment les chargeurs proxy des autres mods (`Mod loaders:` dans `report.txt` ; RED4ext est
`winmm.dll`, Cyber Engine Tweaks est `version.dll` via un chargeur ASI) et avertissent quand AMDNR occupe le nom
d'un chargeur connu de ce jeu sans le chaîner. L'AMDNR Launcher ne prend ni ne déplace jamais le fichier d'un
chargeur : il choisit un autre nom de proxy (Cyberpunk 2077 : `dxgi.dll`), et son Doctor nomme tout chargeur qu'un
INSTALL précédent a mis de côté (`AMDNR_backup`). ReShade à côté d'AMDNR est détecté par la ressource de version du
module ou par les exports d'add-on de ReShade, jamais par un nom de fichier, et nommé dans le rapport et le log
(`[Game] ReShade detected: <module>`) ; rien n'est chargé, hooké ni bloqué, et le partage du device D3D12 entre les
deux est conçu, pas encore construit.

**`No HIP adapter matches D3D12 LUID` dans l'onglet Neural ou dans `amd_bridge.log` ?** Depuis la 0.3.5, la ligne
nomme les deux côtés - l'adaptateur D3D12 du jeu (nom, LUID), chaque périphérique HIP (ordinal, nom, gfx, LUID) - et
la cause probable avec la marche à suivre : le jeu tourne sur la puce graphique intégrée ou sur une autre carte
(Paramètres Windows > Système > Écran > Graphiques : affectez le jeu au GPU dédié), un runtime HIP sans identité (un
`amdhip64_7.dll` isolé à côté du jeu : supprimez-le), aucun périphérique HIP (installez le pilote Adrenalin d'AMD),
la même carte sous une autre identité, ou un adaptateur qui n'est pas AMD. Les deux runtimes écrivent le même texte.

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

**Vous ne trouvez pas les réglages de Ray Regeneration ?** Depuis la 0.3.5, ils se trouvent dans leur propre onglet
**Ray Regeneration**, juste après Upscaling, affiché tant que Ray Regeneration tourne dans le titre (et conservé
pour la session une fois qu'il a tourné) ; la ligne **Ray Regeneration** de l'onglet Neural propose alors un bouton
**Open Ray Regeneration**. Tant qu'il ne tourne pas, l'onglet n'est pas affiché et cette ligne de l'onglet Neural
dit pourquoi : le jeu n'a pas activé Ray Reconstruction, Ray Regeneration a renoncé à ce titre et pourquoi, ou il y
a combien de secondes il a tourné pour la dernière fois. Sur une RX 7000, une ligne grisée supplémentaire ajoute
qu'il y est proposé en preview : le débruiteur d'AMD n'a pas de fournisseur pour RDNA 3, donc dès que le jeu active
Ray Reconstruction, son image est mise à l'échelle sans débruiteur, et `[FSR-RR] FfxDenoiserAllowPreRdna4=false` (ou
la case de l'onglet Upscaling **Preview: Ray Regeneration on this card (restart)**, décochée) coupe la preview pour
que le jeu garde son propre débruiteur. Sur
RX 6000 et plus ancien, elle dit qu'il n'est pas proposé sur ce GPU, qu'AMD publie le débruiteur pour RDNA 4, et que
`[FSR-RR] FfxDenoiserAllowPreRdna4=true` le propose quand même. Pour le faire tourner, dans
les paramètres graphiques du jeu : choisissez **DLSS** comme upscaler (pas FSR, pas XeSS), activez le **ray tracing**
ou le path tracing, et activez **Ray Reconstruction** (DLSS-RR) ; l'onglet Upscaling indique alors « FSR Ray
Regeneration ». Quel réglage changer pour quel problème : le guide de réglages de Ray Regeneration **RR-BEST-SETTINGS.md**
(pas dans le zip).

**Ray Regeneration paraît granuleux ou bruité ?** Jugez-le d'abord avec **Neural Rendering désactivé** (décochez **Enable Neural Rendering** en haut de l'onglet Neural, appuyez sur Home en jeu, ou mettez
`[DlssNr] Enabled=false`) : la passe neuronale tourne après Ray Regeneration, sur sa
sortie, donc une capture prise avec NR activé ne dit rien du débruiteur. Ensuite, selon le type de grain — grain qui
rampe dans une scène immobile, points brillants, grain sur les visages, traînées derrière les personnages en mouvement
— les réglages à essayer sont dans ce même guide, **RR-BEST-SETTINGS.md**. **Aucune valeur par défaut du
débruiteur ni du sharpening n'a changé en 0.3.4.2** : les nombres sont ceux de la 0.3.4.1. Ce qui a changé : le
sharpening qu'AMDNR ajoute après Ray Regeneration est désormais sa propre clé ini,
`[Sharpness] RrDefaultSharpness` (même valeur par défaut 0.25), donc 0.15, 0.10 ou 0 est une modification du ini et
non un nouveau build. Vérifiez votre ini avant de
chasser le grain avec le curseur de netteté : une valeur sous `[Sharpness] Sharpness` ne fait rien tant que
`OverrideSharpness` est désactivé, et elle s'applique à l'instant où vous cochez **Override** dans le menu - c'est pourquoi Image > Sharpness la
signale maintenant (« ini Sharpness 1.00 waits for Override »). Deux choses
que nous ne cacherons pas : une partie du grain vient de l'échantillonnage de rayons du jeu lui-même — le débruiteur
d'AMD n'est pas fait pour réparer un bruit qui arrive corrélé, et un jeu qui propose DLSS Ray Reconstruction éteint son
propre débruiteur et nous livre le signal brut — et le grain qui rampe dans une scène immobile a chez nous une cause
structurelle qu'aucun curseur ne supprime complètement. Celui-là est un problème connu. La 0.3.5 lui donne un chiffre :
le bloc Diagnostics de l'onglet Ray Regeneration affiche le grain en entrée et en sortie et le scintillement caméra
immobile (mesurés tant que l'onglet est ouvert), et la ligne **Denoiser backend** de l'onglet peut être réglée sur
Off, ce qui indique au jeu que Ray Reconstruction n'est pas pris en charge, pour qu'il garde son propre débruiteur
(après un redémarrage). Le débruiteur propre à AMDNR est un travail de la 0.3.6.

**Ray Reconstruction est activé dans le jeu mais l'onglet Neural indique « Ray Regeneration is off in this title » ?**
Le jeu n'expose pas ce dont FSR Ray Regeneration a besoin : son plugin DLSS transmet des matrices de caméra vides
(Satisfactory), que le Ray Reconstruction de NVIDIA traite comme optionnelles et dont FSR Ray Regeneration a
besoin. L'upscaling FSR tourne à sa place et NR reprend sa position normale avant le SR ; l'onglet Upscaling
l'indique aussi. Depuis la 0.3.4, il reste désactivé pour toute la session dans un titre Unreal présentant cette
signature. Désactivez Ray Reconstruction dans le jeu et rétablissez les réglages de denoiser du moteur.

**Un jeu Ubisoft sous Anvil (AC Black Flag Resynced, Shadows, Mirage) affiche « DX12 Error 0x80070057 » ?**
Ces jeux embarquent leur propre XeSS Frame Generation. Depuis la 0.3.5, l'onglet Frame Gen y affiche une note de
conseil (laissez la XeSS FG du jeu désactivée, sinon deux générateurs se partagent une même fenêtre) et la sortie
XeFG d'AMDNR tourne quand même ; la voie la plus simple est l'option XeSS FG du jeu avec la frame generation d'AMDNR
désactivée. Si le problème persiste, réglez `[FrameGen] Enabled=false` et `[fakenvapi] ForceXeLL=false` et faites un
rapport avec le log.

**The Last of Us Part I plante au démarrage ?** C'est l'initialisation de Streamline propre au jeu, un problème
connu d'OptiScaler : renommez `sl.common.dll` dans le dossier du jeu en `sl.common.dll.bak` et choisissez
**FSR 3.1** dans les paramètres du jeu à la place de DLSS.

Notes complètes pour chaque version : `CHANGELOG.md` (dans le zip et dans le dépôt).

## Feuille de route

- **0.3.5** (ce build) — **AMDNR Anywhere** (preview, RX 9000, via le launcher) ; l'onglet Ray Regeneration avec des
  lignes d'état par carte et par API, la ligne Denoiser backend sur Off et le chiffre de bruit ; la passe neuronale
  après l'upscaling (`AmdPlacement`) ; l'onglet Frame Gen et le log disent pourquoi rien n'est généré ; lmxxf 0.37
  de Kien (MIT) sur RX 9000 ; le jeu de modules de la 0.3.5 (environ 10 % plus rapide sur RX 7000 et sur les
  consoles portables de la classe Z1 Extreme, même image) ; les pas de résolution dynamique conservés sans
  reconstruction du réseau ; le correctif du transport
  sur consoles portables ; le dossier de runtime partagé (`AmdRuntimePath`) ; le refus d'adaptateur HIP qui nomme
  les deux côtés ; les chargeurs des autres mods et ReShade nommés dans le rapport ; les processus anti-triche et de
  rapport de plantage laissés tranquilles ; correctifs pour Uncharted (RX 9000), Assetto Corsa, F1 25 et Kingdom
  Come: Deliverance II (Game Pass), Control Resonant avec la frame generation, Tainted Grail: The Fall of Avalon et
  Dead Space, GTA V Enhanced, Half-Life 2 RTX et d'autres titres Vulkan, et les textes Linux / Proton ; AMDNR
  Launcher 0.3.5.1.
- **0.3.4.2** — hotfix : le menu d'Assetto Corsa (la touche du menu l'ouvre ou le ferme une seule fois par
  appui, les clics plus courts qu'une image sont rejoués, le sélecteur de runtime répond aux touches et fermer le menu
  vaut « Decide later »), le sélecteur de runtime n'ouvre plus le menu tout seul, la section Ray Regeneration est
  toujours dans l'onglet Neural et dit pourquoi elle ne tourne pas, un layout de runtime danielblnc de plus accepté,
  corrections du texte Wine / Proton,
  ajouts au README (noms de proxy, Uncharted, PC hybrides, `AmdLmxxfTierSnap` sur RX 9000, les deux entrées de FAQ sur
  Ray Regeneration) ; NR identique octet pour
  octet à la 0.3.4.1, à cette seule ligne de layout accepté près.
- **0.3.4.1** — hotfix : le sharpening de Ray Regeneration quand le jeu n'en envoie aucun (Windows ;
  aucun par défaut sous Linux / Proton), le faux pop-up « Upscaler failed to run! » de Control Resonant, le menu
  sous Linux / Proton attaché à la fenêtre du jeu, Save report qui nomme vkd3d-proton / DXVK ; AMDNR Launcher
  0.3.4.1 (neuf langues, recherche, favoris, masquage, renommage, CHOOSE GAME .EXE, PLAY, un UNINSTALL complet) ;
  NR inchangé.
- **0.3.4** — le nouveau menu (l'onglet Neural refait, le même style dans tous les onglets, Save
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
- **0.3.6** — RX 6000 (RDNA 2) : les kernels propres à AMDNR derrière une vérification matérielle, avec le palier
  360 et Model interleave comme preview sur Navi 21 ; la preview du débruiteur AMDNR (ARD), un débruiteur propre à
  AMDNR derrière l'appel Ray Reconstruction pour les cartes que le débruiteur d'AMD refuse ; AMDNR Anywhere sur
  RX 7000 une fois testé là.
- **0.4.0** — le palier de réseau 1440p ; Ray Regeneration sur les titres Vulkan via le pont ; Neural Rendering
  entre adaptateurs (le jeu sur une carte, le réseau sur la carte AMD) ; Anywhere au-delà de la preview (titres sans
  upscaler propre, où OptiScaler fournit à la fois l'upscaler et la passe neuronale).
- **Plus tard** — AMDNR sur n'importe quelle fenêtre (le bureau).

---

## Crédits

Ce build est un travail d'assemblage qui repose sur le travail d'autres personnes. S'il vous est utile, les remerciements
reviennent aux projets d'origine (upstream).

- **TheAutomatic** — DLSS 5 AMD project — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD by Daniel Blanco — https://github.com/danielblnc/DLSS-NR-on-AMD (les fichiers `*Runtime.zip`, non modifiés)
- **lmxxf** (Kien) — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (le portage du réseau, les kernels et le runtime HIP, MIT)
- **TheAutomatic** — `LmxxfNrRuntime.cpp`, `LmxxfNrApi.h`, `LmxxfProductionOptions.h` : portions contributed to lmxxf by TheAutomatic (MIT)
- **kernels de lmxxf 0.31** dans `LmxxfNrRuntime.pak` (the ViT projection (lmxxf031-vit-wide-deep), the C512 QKV and mix kernels (lmxxf031-c512-m32-mh, lmxxf031-c512-m32-deep) and one-wave-per-head attention (lmxxf031-c64-wave2)) — ceux de lmxxf (Kien, MIT), compilés par AMDNR à partir des sources et de la recette de compilation de lmxxf ; la part d'AMDNR est le chargement, les empreintes SHA-256, le filtrage par GPU et les solutions de repli
- **kernels de lmxxf 0.37 et 0.39** dans `LmxxfNrRuntime.pak` (les modules lmxxf037-* et lmxxf039-* pour RX 9000) et leur code de lancement dans `LmxxfNrRuntime.dll` — lmxxf 0.37 / 0.39 by Kien (MIT), livrés tels que lmxxf les a compilés ; la part d'AMDNR est le chargement comme un seul groupe épinglé, les empreintes SHA-256, les réglages par défaut par GPU, les interrupteurs de désactivation et les solutions de repli
- **c32w kernels** (0.3.3.2) — les kernels RDNA 4 à une seule wave propres à AMDNR pour le réseau de lmxxf, Copyright (c) 2026 3zwr1 (AMDNR) ; idées tirées de la documentation publique d'AMD sur WMMA pour RDNA 4 (GPUOpen, ROCm matrix instruction calculator)
- **Le backend RDNA 3 d'AMDNR** (0.3.3 ; les builds pour consoles portables gfx1103 / gfx1150 en 0.3.4), la politique de paliers de taille du réseau et les petites tailles de réseau (0.3.4) — Copyright (c) 2026 3zwr1 (AMDNR)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Dagherbou / OptiScaler_DLSSNR** — https://github.com/Dagherbou/OptiScaler_DLSSNR
- **wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass** — https://github.com/wilsjo2/OptiScaler-DLSSNR-PreSR-Multipass
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, non modifié)
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR** — l'hôte de capture de fenêtre dans lequel tourne AMDNR Anywhere — https://github.com/Blinue/Magpie (le fork : https://github.com/SAOG0721/Magpie)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (calculs de la composition des couleurs, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), la base du déblocage multi-images XeFG intégré et de son pacing
- **Zach Hembree (DarkHelmet)** — FSR Ray Regeneration pour OptiScaler, à l'origine du chemin Ray Regeneration d'AMDNR, poursuivi par **burak113**, dont AMDNR a porté la branche (branche OptiScaler ffx-denoise-experimental, GPL-3.0)
- **Screen-space GI** (l'effet hérité ; retiré du menu en 0.3.4, `[AmdRtgi] Enabled` dans l'ini) — un effet qu'AMDNR a hérité de la lignée OptiScaler-AMD-PreSR ; le mérite en revient à ses auteurs d'origine. Il nécessite le dossier `experimental_lighting` du paquet danielblnc, qu'AMDNR ne distribue pas.
- **AMDNR Screen GI** (preview 0.3.4) — le travail propre d'AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), écrit à partir d'articles publiés (Therrien, Levesque et Gilet 2023 ; Jimenez et al. 2016 ; Schied et al. 2017 ; et les autres cités dans `CHANGELOG.md` et `Licenses/AMDNR_NOTICE.txt`)
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## AMDNR Launcher

**AMDNR Launcher** (nouveau dans la 0.3.4) est un programme Windows 10 / 11 qui peut aussi tourner sous Linux avec
Proton (expérimental, pas encore testé par nos soins : voir « Linux / Proton »). Il installe et met à jour AMDNR
jeu par jeu : il trouve vos jeux (Steam, Epic, l'application Xbox, Ubisoft Connect, l'application EA, GOG, Rockstar,
Battle.net et Amazon Games), choisit le nom de la DLL, télécharge le build et le runtime danielblnc de votre choix,
vérifie chaque installation avec son Doctor,
lance AMDNR Anywhere pour les jeux sans upscaler propre (PLAY ANYWHERE), et se met à jour tout seul.
Téléchargez `AMDNR-Launcher.exe`, l'AMDNR
Launcher de la release la plus récente, depuis la page des releases :
<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>

**Nouveau dans le Launcher 0.3.5.1** (sur la release Alpha0.3.5 ; il se met d'abord à jour lui-même, puis vos
jeux) :

- **PLAY ANYWHERE** sur un jeu sans upscaler propre (voir « AMDNR Anywhere ») : le launcher récupère l'hôte de
  capture depuis la release de son auteur, lance le jeu et fait tourner Neural Rendering sur sa fenêtre, avec un
  résumé en lecture seule des réglages de l'hôte à côté du bouton (les réglages eux-mêmes sont dans l'onglet
  Anywhere du menu). RX 9000 dans cette release. Les jeux dans lesquels le mod ne peut pas se charger (32 bits,
  DirectX 9 / OpenGL sans upscaler) sont refusés à l'INSTALL avec une ligne claire, et Anywhere leur est proposé.
- **UPDATE ALL** et la mise à jour au démarrage ; des miroirs de paquets ; RETRY / OPEN DOWNLOAD / IMPORT PACKAGE
  quand un téléchargement échoue.
- **COLLECT LOGS** active le gestionnaire de plantages pour un lancement et joint `amdnr_crash.log`, le dump le plus
  récent et le `dlssnr_on_amd.ini` de danielblnc.
- **PLAY** lance les jeux Xbox / Microsoft Store par leur identifiant d'application.
- **Un seul PLAY** avec un sélecteur de route (l'upscaler du jeu ou AMDNR Anywhere, mémorisé par jeu) et l'API
  graphique (DX9 / 10 / 11 / 12 / Vulkan / OpenGL) sur chaque page de jeu.
- **Douze langues** : le turc, le coréen et le hongrois rejoignent les neuf ci-dessous.
- **Les jeux Rockstar démarrent via leur boutique** (Steam, Epic ou le Rockstar Games Launcher), jamais par leur exe.
- **AMDNR Anywhere** : le jeu tourne avec une priorité GPU inférieure à la normale pour que l'hôte et Neural
  Rendering passent en premier ; l'icône de l'hôte de capture dans la zone de notification est masquée.
- **Resident Evil 2 / 3 / 4 (2023) / Village** : un avis de configuration (ils ont besoin du plugin d'upscaler de
  PureDark avec REFramework). D'autres jeux sans DLSS, XeSS ni FSR 2+ sont marqués non pris en charge, avec la
  raison, avant tout téléchargement.
- **FSR 4 (INT8)** en option expérimentale sur la page du jeu des consoles portables RDNA 3.
- **Les chargeurs des autres mods ne sont jamais pris ni déplacés** (RED4ext, Cyber Engine Tweaks et consorts) : le
  launcher choisit un autre nom de proxy et son Doctor nomme tout chargeur qu'un INSTALL précédent a mis de côté.
- **GTA V Enhanced** : la page du jeu porte la ligne de route (le FSR 3.1 choisi dans le jeu est l'entrée ; Neural
  Rendering tourne avant lui) et une note sur `settings.xml`.

**Nouveau dans le Launcher 0.3.4.1 : vous l'avez demandé, nous l'avons fait** (d'après les premiers retours sur
Discord) :

- Neuf langues (douze depuis la 0.3.5.1) : anglais, arabe, chinois (simplifié), français, espagnol, portugais,
  italien, russe et polonais, plus le turc, le coréen et le hongrois. Le
  launcher suit la langue de votre Windows (l'anglais si elle n'en fait pas partie) ; choisissez-en une autre via
  LANGUAGE (LANGUE) ou dans SETTINGS (PARAMÈTRES), où ce choix est aussi proposé au premier démarrage. Les constats
  du Doctor, les messages d'installation et le rapport COLLECT LOGS restent en anglais pour que le support puisse
  les lire.
- Recherche dans la bibliothèque ; favoris (une étoile, et les jeux étoilés en premier) ; masquer des jeux (HIDDEN
  les réaffiche) ; renommer un jeu.
- CHOOSE GAME .EXE : choisissez vous-même l'exe du jeu quand le launcher a pris le mauvais ou n'en a trouvé aucun.
  Cyberpunk 2077 et The Witcher 3 (REDengine) sont désormais trouvés dans le bon dossier sans cela.
- PLAY et OPEN FOLDER sur la page de chaque jeu ; STORES active ou désactive des boutiques entières ; les dossiers
  ajoutés à la main restent dans la liste, même quand leur disque est débranché, jusqu'à ce que vous les retiriez.
- UNINSTALL demande d'abord confirmation et retire tout ce que le mod a placé, y compris ce qu'il a écrit pendant que
  le jeu tournait (logs, caches, crash dumps, rapports inachevés) ; il garde les zips Save report terminés et une DLL
  portant le nom du proxy qui n'est plus un OptiScaler (celle du jeu lui-même).
- COLLECT LOGS sur n'importe quel jeu, installé ou non, avec un rapport de scan.
- Les consoles portables et les APU (ROG Ally Z1 Extreme et autres APU Ryzen) sont reconnus, avec une note indiquant
  qu'AMDNR y est encore en test.
- Linux (expérimental, pas encore testé par nos soins) : le même exe Windows peut tourner sous Proton, y affiche un
  avertissement indiquant la marche à suivre et cherche aussi les jeux dans votre bibliothèque Steam Linux ; étapes
  dans « Linux / Proton ». Dites-nous sur Discord si cela fonctionne.

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
