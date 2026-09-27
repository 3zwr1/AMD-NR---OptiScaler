# Manual test matrix — AMD NR Launcher 0.3.4

Automated tests cover everything except the one thing that matters most:
whether the mod actually loads inside a game. That needs a human.

Run all of these before any public release (`RELEASING.md` step 4).

## Prerequisites

- An AMD GPU. Test on an RX 7000 card; if one is available, repeat cases 1, 2, 6 and 31 on an RX 9000.
  On both, AMDNR can also run on its built-in lmxxf runtime.
- The manifest under test at an https URL. A release build reads the compiled-in
  `https://raw.githubusercontent.com/3zwr1/AMD-NR---OptiScaler/main/Launcher/manifest.json`; to test
  another one, start `AMDNR-Launcher.exe --manifest <url>`. The caption then shows a red
  **TEST MANIFEST** label, and the address is used for that run only — it is never saved. Cases that
  need an edited manifest (9, 26, 27) host the edited copy on a branch or gist and use `--manifest`.
- Everything the launcher keeps lives in `%LOCALAPPDATA%\AMDNR` (settings, records, games list, cached
  packages, manifest cache, crash log). A first run again is `settings.json`, `manifest-cache.json` and
  the `assets` folder deleted, with the launcher closed. **Never delete `records`, or the whole folder,
  while a test install is still in a game:** the records are the only thing that knows which files in a
  game folder are the launcher's and where the game's own files went in `AMDNR_backup`. Without them the
  install cannot be uninstalled, and the game's own files stay stranded in the backup. To start from an
  empty folder, UNINSTALL every test install first.
- At least one DX12 game and one DX11 game installed through Steam. Cases 21–24 use SILENT HILL 2,
  Stray and Marvel's Spider-Man Remastered; cases 28–29 use Forza Horizon 6 and Call of Duty Modern
  Warfare 3 from the Xbox app (on this PC both are in `E:\XboxGames`).

## First run and build choice

| # | Case | Steps | Pass |
|---|------|-------|------|
| 1 | AMDNR with the DLSSNR AMD files | Delete `settings.json`, `manifest-cache.json` and `assets` from `%LOCALAPPDATA%\AMDNR` (keep `records`), start the launcher | The chooser appears over the art: both builds with author, summary and homepage link; **AMDNR** and **Yes** pre-selected; AMDNR's runtime note is shown. CONTINUE downloads the AMDNR package and the DLSSNR AMD files (about 110 MB) once, then lists games |
| 2 | AMDNR without the files | As 1, choose **No** | No amber warning (AMDNR does not require them). The DLSSNR AMD files are not downloaded. An install places no `dlssnr_amd_pass*.dll` and the Doctor raises no runtime errors |
| 3 | OptiScaler AMD pre-SR 1.9.1-alpha with the files | As 1, pick **OptiScaler AMD pre-SR** by TheAutomatic, **Yes** | The 1.9.1-alpha zip downloads from `github.com/TheAutomatic/dlss-5-amd-project/releases/…`, not from 3zwr1. An install places `OptiScaler.dll` under the proxy name, `LmxxfNrRuntime.dll`, the `lmxxf-modules\` folder and the pass DLLs, and none of `Setup.bat`, `Setup.ps1`, `Uninstall_OptiScaler_NR.*`, `SHA256SUMS.txt` or the root `README*.md`. The game's `OptiScaler.ini` says `Enabled=true` and `NrBackend=daniel` under `[DlssNr]` (the packaged file says `Enabled=false` and `NrBackend=lmxxf`), and Neural Rendering runs in game |
| 4 | OptiScaler AMD pre-SR 1.9.1-alpha without the files | As 3, choose **No** | Before CONTINUE an amber line reads "Neural Rendering will not run without them." After an install, the game's `OptiScaler.ini` is the packaged one (`Enabled=false`, `NrBackend=lmxxf`) and the Doctor warns that the DLSSNR AMD files are not installed |
| 5 | Change build later | Settings → pick the other build → CONTINUE | The panel says the change affects future installs only. Installed games keep their build until INSTALL is used on them again. ABOUT shows the credits, links and the "Not affiliated with or endorsed by AMD or NVIDIA" line, and under the launcher's copyright the notice `Source published to be read, not reused — all rights reserved. See LICENSE.txt.` |
| 37 | Discord mark | Any page, the first-run chooser included | Discord's mark sits in the title bar, left of the window buttons, with the tooltip `Join the AMD NR Discord — discord.gg/AMDNR`. One click opens `https://discord.gg/AMDNR` in the default browser and the window does not move (the click is not taken as a drag). ABOUT's `Discord` link opens the same address |
| 38 | CONTINUE always in view | Open the chooser at the window's default size (1240×760) and at its minimum (980×580) | CONTINUE — and CANCEL in Settings — sit in a footer under the page, fully visible without scrolling at both sizes; only the text above them scrolls |
| 39 | Games before downloads | Delete `assets` from `%LOCALAPPDATA%\AMDNR` (keep `records` and `settings.json`), start the launcher | The library fills within seconds and the status line then reads `Downloading AMDNR …` with the progress bar; the games are listed the whole time the download runs, INSTALL stays greyed until it finishes, and the final status is `N games found.` A start with everything cached shows no download at all |

## Runtime by GPU

The manifest lists two DLSSNR AMD runtimes by danielblnc: `runtime-033` (`v0.3.3-Runtime.zip`, about
110 MB) first, for every generation, and `runtime-040` (`v0.4.0-Runtime.zip`, about 113 MB) second, for
RX 9000 only. The launcher installs the first one whose list names the detected GPU
(`RELEASING.md`, *The two runtimes*).

| # | Case | Steps | Pass |
|---|------|-------|------|
| 30 | RX 7000 gets 0.3.3 | Case 1 on an RX 7000 | The DLSSNR AMD files downloaded are 0.3.3: the download cache under `%LOCALAPPDATA%\AMDNR\assets` gains a `runtime-033` folder and neither `runtime-041` nor `runtime-040`. An install places `dlssnr_amd_pass1.dll`, `pass2`, `pass3` and `dlssnr_on_amd_weights.bin`; the Doctor shows neither "could not be verified" nor a hash mismatch, and Neural Rendering runs in game |
| 31 | RX 9000 gets 0.4.1 | Case 1 on an RX 9000 | The files downloaded are 0.4.1 (`runtime-041` folder), not 0.3.3 or 0.4.0: `runtime-041` is listed first for `rdna4`. With a `--manifest` copy whose `runtimes` list `runtime-033` first, the same card gets 0.3.3 and an RX 7000 is unchanged either way |
| 41 | Choosing the DLSSNR AMD files | First run or SETTINGS on an RX 9000 | Under the question, one radio per runtime the build lists for the card, in the manifest's order — `0.4.1 · RECOMMENDED`, `0.3.3`, `0.4.0` — then `NO`; the size line follows the selected one. CONTINUE with `0.3.3` downloads `runtime-033` (no `runtime-041` folder appears), an INSTALL places 0.3.3's pass DLLs, the record says runtime 0.3.3, and the Doctor does not offer 0.4.1 for that game; REPAIR / UPDATE keeps 0.3.3. Back on `0.4.1 · RECOMMENDED`, `settings.json` has no `RuntimeId` (the user follows the manifest again). On an RX 7000 the radios are `0.3.3 · RECOMMENDED` and `NO` only |
| 40 | A newer runtime for the card | Install on an RX 9000 with a `--manifest` copy that lists `runtime-033` first for `rdna4`, then restart on the real manifest (0.4.1 first) | The game shows `Update available` and the stage reads `DLSSNR AMD files 0.4.1 are now the runtime for this card; this game has 0.3.3. Use REPAIR / UPDATE to install them.`; REPAIR / UPDATE replaces the three pass DLLs and the weights, the record then says runtime 0.4.1, and the Doctor is clean. The same game on an RX 7000, a game installed without the files, and a TheAutomatic install show no such line |
| 32 | Every current runtime verifies | Rescan a game with either runtime installed | No "could not be verified" and no hash-mismatch finding: the manifest's eight `passSha256` values cover every danielblnc runtime the mod accepts, published or still to come |

## Install, repair, uninstall

| # | Case | Steps | Pass |
|---|------|-------|------|
| 6 | Fresh install, DX12 | Install into a DX12 game | Game shows as `Installed`; the stage shows the next steps with the chosen DLL name. In game, INSERT opens the AMDNR menu and Neural Rendering is active |
| 7 | Fresh install, DX11 | Same, for a DX11 game | As above |
| 8 | Wrong proxy recovery | Install, force a name the game rejects, launch, fail, click `TRY NEXT PROXY`, relaunch | The second name loads; only the proxy file changed on disk |
| 33 | The DLL name is picked before INSTALL | Select a game not yet installed; on the stage's row of six names click one other than the marked one | The reason line reads `INSTALL will use <picked>. The launcher would have picked <name> — <reason>.`; INSTALL places the mod under the picked name; RESCAN keeps the pick. A name a file this launcher did not put there holds is greyed, and its tooltip says to move that file aside yourself. A game whose exe imports one of the six names (`re9.exe` → `dxgi.dll`) has that name marked first with `<exe> loads <name> itself when it starts`; the games in the manifest's `proxyOverrides` get their listed name |
| 34 | Picking a name on an installed game | Select an installed game and click another name | The DLL moves to that name and nothing else in the folder changes; the status line names the new DLL. Clicking a name another file holds changes nothing and the stage snaps back with the installer's own words |
| 35 | The round never dead-ends | Press `TRY NEXT PROXY` until every free name has been tried, then once more | The last press says every name has been tried once and starts again from the first name the launcher would pick; a name another file holds (ReShade's `dxgi.dll`) is never offered. Close and reopen the launcher between presses: the names already tried are remembered (`TriedProxies` in the record), and `REPAIR / UPDATE` keeps them too |
| 36 | Earlier install, another name picked | On a row reading `Earlier install found`, pick a DLL name other than the one in the note, then INSTALL | Before pressing, the note reads `Found an earlier AMDNR / OptiScaler install (<old>). INSTALL moves it into AMDNR_backup and installs as <picked>.`; afterwards the mod loads through the picked name and the old DLL sits in `AMDNR_backup`. Picking the old name again makes the note say INSTALL keeps that DLL name |
| 9 | Update | `--manifest` with a copy whose `amdnr` package is a newer version | Row shows `Update available`; `Repair / Update` installs it |
| 10 | Uninstall | Uninstall a game that had a pre-existing `ReShade`-style DLL | The folder matches its pre-install state; the moved-aside DLL is back |
| 11 | Existing ini preserved | Edit `OptiScaler.ini`, then `Repair` | Your edits survive |
| 12 | Ini warning | Set `LogToFile=false`, rescan | Doctor warns; `RESET INI` restores it and leaves a `.bak-` file. The window keeps painting while it runs (it works off the UI thread like INSTALL), and closing the launcher mid-reset asks first |
| 13 | Anti-cheat | Select a game with `EasyAntiCheat` | An amber line stays on the stage. INSTALL asks first, with **No** as the default and the "can get your account banned in online play" wording; No cancels, Yes installs |
| 14 | Xbox Game Pass | Install into `XboxGames\<game>\Content\` | Allowed and works |
| 15 | WindowsApps | Select a `WindowsApps` game | Marked `Not supported` with the reason shown; Install is unavailable |
| 16 | Program Files | Install into a game under `C:\Program Files` without elevation | Either it works, or the launcher offers `Restart as administrator` — never a raw permission exception |
| 17 | Game running | Start the game, then try to install | Blocked, naming the running process |
| 18 | Offline | Disconnect the network after packages are cached | Launcher opens, shows an offline message, and can still install |
| 19 | Collect logs | Click `Collect logs` on an installed game | A zip lands on the desktop; the clipboard summary's first line is the build stamp |
| 20 | Messy folder | Install into a folder full of `.bak`, `.dll2`, and capture directories | Doctor stays healthy; nothing unrelated is touched. The launcher's backups are under `AMDNR_backup` |
| 21 | Earlier hand-made install | A game where OptiScaler was placed by hand (SILENT HILL 2's `dxgi.dll`, Spider-Man's `dbghelp.dll`), no launcher record | Status `Earlier install found`; the stage names the DLL. INSTALL keeps that DLL name and moves the old DLL into `AMDNR_backup`. Uninstall says the earlier OptiScaler build was kept, switched off, and names the path; the game then starts with no OptiScaler |

## Game detection

| # | Case | Steps | Pass |
|---|------|-------|------|
| 22 | SILENT HILL 2 | Select it | Folder is `SHProto\Binaries\Win64`, exe `SHProto-Win64-Shipping.exe` — not the root `SHProto.exe` stub, never `Engine\…` |
| 23 | Stray | Select it | Folder is `Hk_project\Binaries\Win64`, exe `Stray-Win64-Shipping.exe` |
| 24 | Spider-Man Remastered | Select it | Folder is the game root, exe `Spider-Man.exe` (not `crs-handler.exe`, `crs-video.exe` or the leftover setup exe); proxy `dbghelp.dll` from the manifest override |
| 25 | Steam tools | Scan | Steamworks Common Redistributables (228980) and other Steam tools are not listed as games. A row with no resolved exe folder shows no DLL name |
| 28 | Forza Horizon 6 (Xbox app) | Scan | Listed as **Forza Horizon 6** with store **Xbox**, found on E: without adding anything by hand. Folder is `E:\XboxGames\Forza Horizon 6\Content`, exe `forzahorizon6.exe` (not `gamelaunchhelper.exe` or the leftover `dlssnr_on_amd_setup.exe`) |
| 42 | Other launchers | Scan on a PC with games from Ubisoft Connect, the EA app, GOG Galaxy, the Rockstar Games Launcher, Battle.net or Amazon Games | Each game is listed once with its store's name in the rail (`Ubisoft Connect`, `EA app`, `GOG`, `Rockstar`, `Battle.net`, `Amazon Games`) and its install folder; the launcher itself, Social Club, Origin / EA Desktop and Battle.net's own app are never listed; a game whose folder is gone is not listed. On this PC the Rockstar Games Launcher's GTA V appears once, as **Rockstar** at `E:\GTAV No Mods` (the launcher's key and the `Grand Theft Auto V Legacy` entry name the same folder); the stale Add-or-remove-programs entry pointing at `C:\Program Files\Rockstar Games\Games\Grand Theft Auto V`, a folder that no longer exists, is not listed |
| 43 | Capcom RE Engine without REFramework | Select an RE Engine game (`re_chunk_000.pak` beside the exe — RE2/3/4/7, Village, Requiem, DMC5, Monster Hunter, SF6, Dragon's Dogma 2) that has no REFramework | The stage shows an amber line before INSTALL: the game closes itself 15–60 s after launch with mods loaded unless REFramework is installed, with the nightly releases link; the Doctor lists the same as a warning (`reframework.missing`), not an error, and INSTALL is not blocked. With REFramework's `dinput8.dll` beside the exe (23 MB, or with its `reframework\` folder / logs) the line is gone. On this PC RE Requiem has REFramework and shows nothing |
| 29 | Call of Duty MW3 (Xbox app) | Scan, select it | Listed as **Call of Duty Modern Warfare 3** (no ®), exe `cod23-cod.exe`, not `bootstrapper.exe`. Status **Not supported**; the stage says Ricochet bans accounts for injected DLLs and AMDNR will not install here. INSTALL is unavailable and nothing asks for consent |
| 30 | A cover for every Steam game | Scan, wait a few seconds, look down the rail | Every Steam row has box art, none shows only a letter on a coloured plate. A game never opened in Steam's library gets its cover from the CDN a moment after the scan (the rail updates on its own; the scan does not wait). Offline, such a game shows its exe's icon letterboxed on the plate instead. `%LOCALAPPDATA%\AMDNR\art\steam\<appid>\` holds what was fetched; a `.missing` file marks a picture the CDN does not have. A game Steam's own cache already has pictures for gets no `art\steam\<appid>` folder at all: the CDN is asked only for what that cache lacks |
| 31 | Xbox covers | Select Forza Horizon 6 | Rail tile is the game's square logo, whole on the plate (not cropped); the stage shows its splash screen as the key art |
| 32 | A game with no art anywhere | Add a folder by hand (any exe with an icon) | Rail tile is the exe's icon, letterboxed; the stage and the window background show the same icon blurred and enlarged, not an empty dark panel. Nothing in the scan waits on any of this and no picture failure ever produces an error |

## Launcher self-update

| # | Case | Steps | Pass |
|---|------|-------|------|
| 26 | Update from a non-ASCII folder | Copy `AMDNR-Launcher.exe` into `C:\ألعاب ÇÃO 游戏\`. Build a second launcher with a higher `<Version>` (for example 0.4.1) and put it at any https URL. Start the first one with `--manifest` pointing at a copy whose `launcher` block names that version, URL and sha256 | It downloads, restarts as the new version from the same folder, and `AMDNR-Launcher.exe.old` is gone after the next start. Nothing else in the folder changes |
| 27 | Update that cannot swap | Same, with the exe under `C:\Program Files\…` and no elevation | A plain message that names the manual download URL; the old launcher still works |
