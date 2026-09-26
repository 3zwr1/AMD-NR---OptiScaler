# AMDNR — DLSS 5 Neural Rendering na AMD (build OptiScaler) — v0.3.3.2

[English](README.md) | [中文](README.zh-CN.md) | [Português](README.pt-BR.md) | [Español](README.es.md) | [العربية](README.ar.md) | [Français](README.fr.md) | [Italiano](README.it.md) | [Русский](README.ru.md) | **Polski**

> **Potrzebujemy Twojego wsparcia.** Dołącz do serwera Discord — <https://discord.gg/AMDNR> — znajdziesz tam
> pomoc, zgłosisz błędy i pobierzesz testowe buildy; każde zgłoszenie z logiem sprawia, że kolejny build jest lepszy.

DLSS 5 Neural Rendering uruchamiany na kartach graficznych AMD i wbudowany w OptiScaler, dzięki czemu działa
w każdej grze Direct3D 12, do której OptiScaler już się podpina. Oprócz samego przebiegu neuronowego: model
interleave dający duży wzrost liczby klatek, kompozycja rezydualna, generowanie klatek XeSS odblokowane do 6X
(do 10X opcjonalnie w grach D3D12) oraz FSR Ray Regeneration dla gier korzystających z DLSS Ray Reconstruction.

**Discord: <https://discord.gg/AMDNR>** — wsparcie, zgłoszenia błędów (`#bug-report`), testowe
buildy.

**Wesprzyj projekt: <https://ko-fi.com/3zinr>**

> **Autorem runtime'u danielblnc jest Daniel Blanco.** Neuronowy runtime AMD z `Runtime.zip`
> (`dlssnr_amd_pass1..3.dll`) to **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> AMDNR dołącza go bez modyfikacji, za jego zgodą; to nie jest dzieło AMDNR. Prosimy, wesprzyj jego projekt.
> Pełne podziękowania dla wszystkich pozostałych znajdziesz na końcu tej strony.

---

## AMDNR - Instrukcja instalacji OptiScaler

Instalacja jest dość prosta.

### 1. Pobierz pliki

Pobierz te dwa pliki ze strony GitHub (<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>):

* `AMDNR-vX.X.X.zip`
* `Runtime.zip`

### 2. Wypakuj oba pliki

Wypakuj zawartość obu plików `.zip`.

### 3. Skopiuj wszystko do folderu gry

Najpierw skopiuj wszystkie pliki z `AMDNR-vX.X.X` do głównego folderu gry — tego samego folderu, w którym
znajduje się plik `.exe` gry.

Następnie zrób to samo ze wszystkimi plikami z `Runtime`.

### 4. Zmień nazwę OptiScaler.dll

W folderze gry znajdź:

`OptiScaler.dll`

Zmień jego nazwę na:

`dxgi.dll`

`dxgi.dll` to zalecana opcja.

Jeśli gra się nie uruchamia albo mod się nie ładuje, spróbuj zamiast tego zmienić nazwę `OptiScaler.dll` na
jedną z tych:

* `d3d12.dll`
* `winmm.dll`
* `version.dll`
* `dbghelp.dll`

Sprawdzaj po jednej nazwie naraz. Nie twórz kilku kopii `OptiScaler.dll`.

> **Resident Evil Requiem (i jego demo) wymaga REFramework.** To znane wymaganie, a nie błąd AMDNR: OptiScaler potrzebuje go,
> żeby obejść zabezpieczenie anti-tamper firmy Capcom ([OptiScaler wiki](https://github.com/optiscaler/OptiScaler/wiki/Resident-Evil-9-Requiem)). Bez niego gra crashuje 15-60 s
> po uruchomieniu ("An unhandled exception occurred"). Umieść `dinput8.dll` z `REFramework.zip` z najnowszego buildu nightly
> (<https://github.com/praydog/REFramework-nightly/releases>) obok `dxgi.dll` i zmień klawisz menu REFramework (np. na Delete): REFramework również używa klawisza Insert.
> Po aktualizacji gry spodziewaj się crashy, dopóki REFramework nie zostanie zaktualizowany. PRAGMATA, Monster Hunter Wilds i Onimusha prawdopodobnie też go wymagają (niepotwierdzone).

### 5. Uruchom grę

`HOME` włącza i wyłącza Neural Rendering w trakcie gry (w obu runtime'ach; mały komunikat pokazuje
On / Off). Klawisz można zmienić obok pola wyboru Enable w zakładce Neural albo w Interface > Keybinds.

To wszystko.

Uruchom grę normalnie i naciśnij:

`INSERT`

Otworzy to menu OptiScaler / AMDNR, w którym możesz skonfigurować moda tak, jak chcesz.

### Jeśli to nie działa

Jeśli gra nadal nie uruchamia się z żadną z powyższych nazw, prosimy zgłosić to na kanale
`#bug-report` na serwerze Discord.

Zgłaszając problem, wrzuć też wszystkie pliki `.log`, które mogły zostać wygenerowane w głównym folderze
gry.

Te logi są bardzo ważne i pomogą nam znacznie szybciej zidentyfikować problem.

> Plik `.exe` zwykle nie znajduje się tam, gdzie wskazuje skrót. Gry na silniku Unreal trzymają go w
> `<Game>\Binaries\Win64\`.

---

### Runtime lmxxf (0.3.0, opcjonalny)

Drugi runtime neuronowy (na licencji MIT, autorstwa lmxxf) może obsługiwać przebieg zamiast
runtime'u danielblnc. Działa na RDNA 4 oraz na RDNA 3 (RX 7000, Strix Halo) przez backend RDNA 3 od AMDNR - tam wolniej:
zacznij od NR resolution 67%. Potrzebuje dwóch rzeczy obok gry:

1. `LmxxfNrRuntime.dll` - w tym archiwum, obok `OptiScaler.dll` (kopiuje się go razem z resztą).
2. `LmxxfNrRuntime.pak` (416 MB, dołączony do zipa AMDNR) obok `LmxxfNrRuntime.dll` - pliki wag
   lmxxf, moduły HIP i HLSL w jednym zaszyfrowanym, uwierzytelnionym pliku. Runtime otwiera go w
   pamięci; nic nie jest wypakowywane na dysk.

Przy pierwszym uruchomieniu, podczas którego zostanie wykryty zainstalowany runtime, a wybór nie został jeszcze
dokonany, menu zapyta, którego użyć (wybór zapisuje wpis `[DlssNr] NrBackend = daniel | lmxxf` w ini; zmienisz go
w Neural > Neural runtime, ze skutkiem od następnego uruchomienia gry). Edycja lmxxf jest nakładana z opóźnieniem
o jedną klatkę, przenoszona przez wektory ruchu, więc klatka nigdy nie czeka na sieć (około 17 ms w 1080p
na RX 9070 XT). Jego log to `lmxxf_backend.log` obok gry.

**Kompatybilność (lmxxf).** Runtime widzi tylko to, co widzi DLSS, więc lista rzeczy, które różnią się między
tytułami, jest krótka: format koloru i HDR, wektory ruchu i ich skala, głębia i jej kierunek, maska reaktywna,
tekstura ekspozycji, flaga Reset oraz miejsce, w którym znajduje się przebieg (przed Super Resolution albo po
Ray Reconstruction). Dotychczas przetestowane:

| Tytuł | API / umiejscowienie | Uwagi |
|---|---|---|
| Silent Hill 2 | D3D12, przed SR | tytuł referencyjny; obsłużona alokacja koloru z paddingiem stosowana przez Unreal |
| Forza Horizon 6 | D3D12, przed SR | |
| Stray | D3D11 przez most D3D12, przed SR | |
| GTA V Enhanced | D3D12, przed SR, HDR, jednokanałowa maska reaktywna | naprawione w 0.3.0: maska była odczytywana jako "wszystko reaktywne" i edycja nigdy nie trafiała do obrazu |
| Każdy tytuł z Ray Reconstruction | D3D12, po RR (zapisywane z powrotem do wyjścia) | obsługiwane od 0.3.0; jeszcze niepotwierdzone w grze |

Jeśli w danym tytule nie widać efektu: `lmxxf_backend.log` zawiera linię `lmxxf inputs:` (formaty, rozmiary,
skala ruchu, kierunek głębi, maska, ekspozycja) oraz linię `lmxxf stats @N:` co 600 klatek
(ekspozycja, podawana jasność, edycja modelu, przenoszona edycja, keep, średnia reaktywność, długość
wektorów i odsetek odrzuconych). Dołącz log do zgłoszenia; z tych dwóch linii zwykle widać, dlaczego.

W trybie lmxxf blok Neural runtime ma **Network history** (własne wejście czasowe modelu),
a Image look ma grupę **lmxxf edit**: kształtowanie edycji (Edit detail,
Edit colour, Edge guard: wzmocnienie drobnej części edycji modelu, jej kolor względem zmiany
jasności oraz wygaszanie edycji na krawędziach głębi) oraz **Output smoothing**
(przebieg po stronie wyjścia z upstreamu, wymaga Network history). Neural passes, Residual strength/limit,
wyostrzanie, Debug view 1 i filtr Appearance działają w obu runtime'ach.

**Full network** (Neural > Performance, `[DlssNr] LmxxfFullNetwork`, tylko lmxxf) uruchamia wszystkie 71 bloków
sieci zamiast pomijać 42, 43 i 46: nieco wierniejszy wynik, około 0.5 ms wolniej w 1080p
(16.6 -> 17.1 ms na RX 9070 XT). Domyślnie wyłączone.

## Wymagania

- Karta graficzna AMD z aktualnym sterownikiem. Runtime neuronowy korzysta z HIP przez sterownik; nie potrzeba
  ani HIP SDK, ani trybu dewelopera. Zgodność układów: RX 9000 (RDNA 4) obsługuje oba runtime'y;
  RX 7000 (RDNA 3, desktopowe i mobilne) również oba - lmxxf przez backend RDNA 3 od AMDNR, wolniej
  niż na RDNA 4; Strix Halo (8060S / 8050S) obsługuje lmxxf; APU w handheldach (Z1 Extreme / 780M, Z2 Extreme /
  890M) i RDNA 2 (RX 6000, Steam Deck) nie są obsługiwane przez żaden z nich.
  Zakładka Neural pokazuje, co może uruchomić Twoje GPU.
- Gra Direct3D 12, Direct3D 11 lub Vulkan. Sama ścieżka neuronowa AMD działa na D3D12; tytuły D3D11 i
  Vulkan docierają do niej przez most D3D12 w OptiScaler, co oznacza, że upscaler musi być
  jednym z backendów "w/Dx12" (`ffx_12`). Zostaw `Dx11Upscaler` / `VulkanUpscaler` na `auto`,
  a ten build wybierze go za Ciebie, gdy neural rendering jest włączony.
- Około 2 GB wolnego VRAM przy rozdzielczościach renderowania klasy 1080p.

## Co jest w obu archiwach

**AMDNR-vX.X.X.zip**

| Plik | Co to jest |
|---|---|
| `OptiScaler.dll` | OptiScaler z backendem DLSS-NR dla AMD. Zmień jego nazwę zgodnie z instrukcją. |
| `OptiScaler.ini` | Ustawienia. Neural Rendering jest włączony; logowanie jest włączone, żeby do zgłoszenia błędu było co dołączyć. |
| `LmxxfNrRuntime.dll` | Runtime neuronowy lmxxf (kernele lmxxf 0.29). Używany tylko po wybraniu; odczytuje `LmxxfNrRuntime.pak` leżący obok, patrz "Runtime lmxxf". |
| `LmxxfNrRuntime.pak` | Wagi, moduły HIP i shadery runtime'u lmxxf w jednym zaszyfrowanym pliku (416 MB). Czyta go tylko runtime lmxxf; można go bez szkody zostawić, korzystając z runtime'u danielblnc. |
| `OptiScaler\` | FSR, XeSS, denoiser FidelityFX i D3D12 Agility SDK, z których korzysta OptiScaler. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | dlssg-to-fsr3 autorstwa Nukem9, bez modyfikacji, ze zmienioną nazwą: wywołania DLSS Frame Generation z gry obsługiwane przez generowanie klatek FSR 3, także na API Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, patrz `Licenses/`. |
| `Licenses\`, `LICENSE` | Licencje stron trzecich, nota AMDNR (`AMDNR_NOTICE.txt`) i licencja GPL-3.0 tego buildu. |
| `SHA256SUMS.txt` | Sumy kontrolne każdego dostarczanego pliku, z obu archiwów. |

**Runtime.zip**

| Plik | Co to jest |
|---|---|
| `dlssnr_amd_pass1..3.dll` | Runtime neuronowy AMD, v0.3.1 od danielblnc, bez modyfikacji. Trzy kopie, żeby multi-pass miał po jednej na każdy przebieg. |
| `dlssnr_on_amd_weights.bin` | Wagi sieci, które ładuje runtime. |

## Ustawienia, które warto znać

Otwórz zakładkę **Neural**. Wartości domyślne to najnowsza przetestowana konfiguracja, więc na początek
najlepiej zmieniać jedną rzecz naraz.

- **NR resolution** — główny regulator jakości i kosztu. Poniżej 100% model pracuje na mniejszym
  obrazie i tylko jego *korekta* jest przenoszona z powrotem na klatkę w pełnej rozdzielczości, więc
  klatka zachowuje własne detale. Powyżej 100% koszt rośnie kwadratowo (150% to 2.25x). Suwak
  zmienia się co 5%: każdy nowy rozmiar NR może zajmować VRAM aż do restartu gry, więc zrestartuj grę
  po wielu zmianach.
- **Residual strength** — jaka część edycji modelu jest nakładana; powyżej 1 ją wzmacnia. To ustawienie
  najbardziej zmienia obraz.
- **Residual limit** — górny limit tego, jak bardzo może się zmienić pojedynczy piksel. Widzisz plamy na obrazie? **Obniż** go.
- **Model interleave** — uruchamia model co drugą klatkę, dając duży wzrost liczby klatek. Pominięte
  klatki wypełnia **Interleave preset**; *Guided fill v2* jest domyślny i to nad nim trwają obecnie
  prace. Frame pacing obu typów klatek jest automatyczny, a **Adaptive interleave** (domyślnie
  włączony) uruchamia model na każdej klatce, gdy obraz jest w ruchu, więc pomijanie klatek - i związane z nim
  artefakty - zdarza się tylko wtedy, gdy obraz stoi w miejscu.
- **Neural passes** — 2 i 3 nakładają model wielokrotnie, z malejącymi korzyściami. W trybie lmxxf
  historia sieci pozostaje przy pierwszym przebiegu; dodatkowe przebiegi to wyłącznie dopracowanie
  przestrzenne. danielblnc wykonuje 1 przebieg w tytułach Vulkan (informuje o tym notka pod suwakiem).
- **Colour composition** (Neural > Image look, oba runtime'y) — *Classic* (domyślny) to obraz taki
  jak dotąd. *RenoDX (experimental)* uruchamia kompozycję koloru RenoDX po modelu, tak jak robi to
  ścieżka NVIDIA: Composition detail i colour, dwustronny **Highlight guard** (domyślnie 2x), który
  ogranicza odpowiedź modelu względem oryginału, oraz opcjonalne ustawienia skóry / otoczenia.
  Na klatce display-referred (SDR) wraca do Classic, z notką w menu. Style NR i presety go
  nie ruszają.
- **Generowanie klatek jest wyłączone w nowym ini.** Zakładka Frame Gen: wybierz FG Input (np. "DLSSG via
  Streamline" w grze z generowaniem klatek DLSS) i FG Output (XeFG), potem zaznacz **Active** w
  sekcji Frame Generation (XeFG) i kliknij Save Settings. Jeśli w ini z wersji 0.1.0 było ono włączone,
  to ustawienie nie zostanie przeniesione po zainstalowaniu ini z 0.2.0.
- **Generowanie wielu klatek XeFG** — od 3X do 6X jest wbudowane i domyślnie włączone (`XeFG\UnlockMFG`),
  zarówno dla kopii z OptiScaler, jak i dla własnej kopii gry. **Usuń `XeFGUnlock.asi`** z `OptiScaler\plugins`,
  jeśli nadal go masz: dwie kopie tej samej łatki crashują grę.
  **Do 10X opcjonalnie** (tylko gry D3D12): ustaw *XeFG ceiling (restart)* w sekcji FG Output w
  zakładce Frame Gen (4X, 6X domyślnie, 8X lub 10X; `[XeFG] MaxInterpolatedFrames`), zrestartuj grę, potem
  wybierz mnożnik z listy MFG. Powyżej 6X potrzebny jest provider XeFG wbudowany w OptiScaler, z włączonym Extra pacing;
  kopia XeSS 3 dostarczana z grą pozostaje ograniczona do 6X. 10X wymaga monitora 360 Hz+ i limitu klatek
  ustawionego na odświeżanie / 10; opóźnienie jest wysokie, a provider rezerwuje około 128 MiB więcej VRAM w 4K.
  7X-10X nie jest jeszcze potwierdzone w żadnej grze: testerzy, prosimy o przesłanie `OptiScaler.log`.
- **FSR Ray Regeneration** — domyślnie tylko na RDNA 4 (RX 9000); tylko w grach korzystających z DLSS Ray Reconstruction (Cyberpunk 2077,
  Alan Wake 2), gdy gra działa na DLSS (spoofing włączony), a ray tracing i Ray Reconstruction są
  włączone w jej własnych ustawieniach. Neural Rendering działa wtedy po nim, na jego wyjściu, co kosztuje
  więcej: obniż NR resolution, jeśli spadnie liczba klatek. Jego ustawienia (Neural > Quality > Ray
  Regeneration) pojawiają się tylko wtedy, gdy gra używa Ray Reconstruction. **Profil path-traced**
  (mniej ziarna na twarzach przy path tracingu) jest opcjonalny od 0.3.3.1: zaznacz go tam, jeśli chcesz go
  wypróbować w Resident Evil Requiem lub PRAGMATA. W tym samym miejscu znajdziesz siłę bias mask, widok debug RR i
  **wygładzanie skóry** (eksperymentalne, dla gier udostępniających SSS guide; domyślnie wyłączone, ale
  w Resident Evil Requiem domyślnie włączone od 0.3.3.2).

## Jeśli coś pójdzie nie tak

`OptiScaler.log` pojawia się w folderze gry. Dołącz go na kanale `#bug-report` i napisz, jaka to gra i
jakie masz GPU. Backend AMD zapisuje też `amd_presr.log` i `amd_bridge.log`, które przydają się wtedy,
gdy nieprawidłowo działa konkretnie przebieg neuronowy. Log z poprzedniej sesji jest zachowywany jako
`OptiScaler.previous.<exe>.log`; po crashu dołącz także ten plik (nowy log mówi wtedy "no clean exit
recorded").

**NR frames 0/s, a zakładka Neural lub `amd_presr.log` mówi, że DLL przebiegu to build, którego ten AMDNR
nie obsługuje?** Twoje `dlssnr_amd_pass1..3.dll` to build danielblnc, którego ten AMDNR nie zna (w obiegu
widziano zestaw 0.2.16), albo brakuje jednego z trzech plików. Od 0.3.3.2 zakładka Neural podaje nazwę
pliku i jego wersję oraz mówi, co zrobić. Użyj `v0.4.0-Runtime.zip` (najnowszy) albo `Runtime.zip` (0.3.1)
z tego wydania, biorąc wszystkie trzy DLL przebiegów z tego samego zipa: `dlssnr_amd_pass1.dll` w `v0.4.0-Runtime.zip`
ma 10,027,008 bajtów, a jego SHA256 zaczyna się od `d62be3d8`. Obsługiwane buildy: 0.2.17, 0.3.0, 0.3.1,
0.3.2, 0.3.3, 0.4.0 oraz 0.4.1 / 0.4.2 jeszcze przed ich wydaniem. Nie instaluj własnego instalatora
danielblnc ani jego `dxgi.dll` / `version.dll` / `winhttp.dll` obok AMDNR: AMDNR już uruchamia jego runtime.

**lmxxf nic nie robi albo od razu się zatrzymuje na PC ze zintegrowaną grafiką?** Naprawione w 0.3.3.2. Na
komputerze stacjonarnym z procesorem Ryzen i włączoną zintegrowaną grafiką, na laptopie z APU AMD i kartą Radeon
albo na PC z dwoma GPU AMD często zdarza się, że GPU, na którym działa gra, nie jest urządzeniem HIP 0. lmxxf zawodził wtedy już na pierwszej klatce
(`hipErrorInvalidHandle (400)`, potem "session is poisoned" w `lmxxf_backend.log`) i pozostawał wyłączony.
Podmień oba pliki: `OptiScaler.dll` (plik po zmianie nazwy, np. `dxgi.dll`) oraz `LmxxfNrRuntime.dll` na pliki
z 0.3.3.2. Jeszcze nieprzetestowane na takim PC: jeśli lmxxf nadal się zatrzymuje, zakładka Neural mówi teraz,
dlaczego; wyślij `lmxxf_backend.log` i `amd_bridge.log` (zawiera listę urządzeń HIP).

**Gra na API Vulkan (Indiana Jones and the Great Circle) zatrzymuje się przy starcie z komunikatem "Could not
create the Vulkan device (VK_ERROR_EXTENSION_NOT_PRESENT)"?** Naprawione w 0.3.2: odziedziczona ścieżka
neuronowa NVIDIA prosiła sterownik AMD o dwa rozszerzenia urządzenia dostępne tylko na NVIDIA. Tytuły Vulkan
docierają do przebiegu neuronowego przez most D3D12 w OptiScaler (patrz Wymagania).

**lmxxf zamrażał grę na API Vulkan na pierwszej klatce NR?** Naprawione w 0.3.3; spodziewaj się jednego
przycięcia trwającego około 1 s, gdy NR startuje. Jeśli sesja Vulkan kiedykolwiek zakończy się przed pierwszą
odpowiedzią lmxxf, przy następnym starcie uruchomi się runtime danielblnc, a zakładka Neural wyjaśni, dlaczego;
naciśnij tam **Retry lmxxf** (usuwa `lmxxf_vk_launch.pending` obok `OptiScaler.dll`), żeby ponownie wypróbować
lmxxf.

**danielblnc zawieszał się na kilka sekund, a potem wyłączał NR w grze na API Vulkan (Indiana Jones) przy
2-3 Neural passes?** Naprawione w 0.3.3: w tytułach Vulkan wykonuje 1 przebieg, a jego 80-milisekundowe
oczekiwanie po wysłaniu pracy (post-submit wait) zostało usunięte. Pierwsza klatka NR w sesji nadal zatrzymuje
grę na około 5 s; notka pod wyborem runtime'u wyjaśnia związane z tym linie w logu. Testerzy:
`[DlssNr] AmdVkLateCopyWait=true` (eksperymentalne, domyślnie wyłączone, jeszcze nietestowane w grze) powinno
usunąć tę pauzę; wyślijcie `OptiScaler.log`, `amd_presr.log` i `dlssnr_on_amd.log`.

**Zużycie RAM przez lmxxf rosło przez cały czas działania NR?** Naprawione w 0.3.3 (było to około 45 GB na
godzinę przy 60 fps NR). Co pozostało: danielblnc nie zwalnia VRAM zajętego przez każdy nowy rozmiar NR powyżej
około 1 MP (0.3.3.2 zaokrągla jego rozmiary do 64 px przy wartościach innych niż 100%, więc jest ich tylko kilka);
z danielblnc zrestartuj grę po wielu zmianach. Od 0.3.3.2 lmxxf nie zatrzymuje już około 97 MB przy każdej zmianie
NR resolution lub trybu DLSS: tworzy bufory sieci raz dla każdego rozmiaru sieci i używa ich ponownie (przy każdej
zmianie zostaje tylko niewielka reszta, około 10-25 MB VRAM).

**Gra ze Streamline nie startuje z błędem slInit 0x18 (widziane w NBA 2K27 na AMD)?** 0.3.3 zamyka jedną
z dróg, którymi mogły to powodować hooki wtyczki Streamline w OptiScaler, ale nie jest potwierdzone, że to
właśnie przyczyna w NBA 2K27. `OptiScaler.log` zapisuje teraz linie `slInit returned ...` i `[SLINIT]`:
wyślij log razem ze zgłoszeniem.

**Ray Reconstruction w grze jest włączone, ale zakładka Neural mówi "Ray Regeneration is off in this title"?**
Gra nie udostępnia tego, czego potrzebuje FSR Ray Regeneration (Satisfactory: brak macierzy kamery). W zamian
działa upscaling FSR, a NR zajmuje swoje normalne miejsce przed SR; żadne ustawienie w ini tego
nie zmienia.

**Gra na silniku Ubisoft Anvil (AC Black Flag Resynced, Shadows, Mirage) pokazuje "DX12 Error 0x80070057"?**
Te gry mają własne XeSS Frame Generation. Ten build zostawia je samym grom (wyjście XeFG z OptiScaler
ustępuje tam miejsca, a zakładka Frame Gen o tym informuje); użyj opcji XeSS FG w samej grze. Jeśli błąd
nadal występuje, ustaw `[FrameGen] Enabled=false` i `[fakenvapi] ForceXeLL=false` i zgłoś problem, dołączając log.

**The Last of Us Part I crashuje przy starcie?** Przyczyną jest inicjalizacja Streamline w samej grze, znany
problem OptiScaler: zmień nazwę `sl.common.dll` w folderze gry na `sl.common.dll.bak` i wybierz
**FSR 3.1** w ustawieniach gry zamiast DLSS.

Pełne informacje o każdej wersji: `CHANGELOG.md` (w zipie i w repozytorium).

## Plan rozwoju

- **0.3.3** (ten build) — lmxxf na RDNA 3 (RX 7000; własny backend AMDNR); kompozycja koloru RenoDX
  (eksperymentalna, opcjonalna) w obu runtime'ach; lmxxf: opcja Full network, naprawiony wyciek RAM,
  naprawione tytuły Vulkan (leniwe wgrywanie wag wewnątrz mostu Vulkan), kernele 0.29 (bit-exact,
  szybsze); danielblnc w tytułach Vulkan: 1 Neural pass, czytelniejsze komunikaty, opcjonalne late copy
  wait; XeFG do 10X (opcjonalnie, D3D12); uodpornienie startu Streamline i diagnostyka; profil path-traced
  FSR Ray Regeneration i wygładzanie skóry; większa odporność w UE5.
- **0.3.2** — odpowiedź na zgłoszenia z 0.3.1: tytuły Vulkan startują i działają z lmxxf, kolory lmxxf
  dopasowane do danielblnc (auto-ekspozycja), lista wyboru runtime'u, status i strojenie Ray Reconstruction;
  dlssg-to-fsr3 od Nukem9 w zipie do generowania klatek na API Vulkan.
- **0.3.1** — poprawki po pierwszych zgłoszeniach do 0.3.0 (sam lmxxf nigdy się nie uruchamiał, NR w Where Winds
  Meet po cichu nie działał, crash przy zmianie jakości DLSS, GTA V Legacy) oraz presety stylów NR
  z trzema własnymi slotami.
- **0.3.0** — runtime neuronowy HIP **lmxxf** (RDNA 4) jako runtime do wyboru obok runtime'u danielblnc,
  dostarczany jako `LmxxfNrRuntime.dll` + `LmxxfNrRuntime.pak`: network history, prawdziwe Neural passes,
  kształtowanie edycji, umiejscowienie po Ray Regeneration, diagnostyka dla poszczególnych tytułów
  i samonaprawianie. Wielkie podziękowania dla TheAutomatic, na którego pracy nad DLSS 5 AMD project
  opiera się ta integracja.
- **0.4.0** — AMDNR Launcher (instalacja runtime'ów i pliku pak jednym kliknięciem, aktualizacje) oraz
  obsługa tytułów bez własnego upscalera (klasy Stray), w których OptiScaler dostarcza
  jednocześnie upscaler i przebieg neuronowy.

---

## Podziękowania

Ten build to połączenie w całość pracy innych ludzi. Jeśli uważasz go za przydatny,
podziękowania należą się autorom projektów źródłowych (upstream).

- **TheAutomatic** — DLSS 5 AMD project — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD — https://github.com/danielblnc/DLSS-NR-on-AMD (`Runtime.zip`, bez modyfikacji)
- **lmxxf** — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (runtime HIP, MIT)
- **c32w kernels** (0.3.3.2) — własne jednofalowe (one-wave) kernele AMDNR dla RDNA 4, przeznaczone dla sieci lmxxf, Copyright (c) 2026 3zwr1 (AMDNR); pomysły z publicznej dokumentacji AMD dotyczącej RDNA 4 WMMA (GPUOpen, ROCm matrix instruction calculator)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, bez modyfikacji)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (matematyka kompozycji koloru, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), podstawa wbudowanego odblokowania generowania wielu klatek XeFG i jego frame pacingu
- **burak113** — preprocesor FSR Ray Regeneration (gałąź OptiScaler ffx-denoise-experimental, GPL-3.0)
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## Prawa autorskie / Licencja

AMDNR: Copyright (c) 2026 3zwr1 (AMDNR). Jest to fork OptiScaler, rozpowszechniany na licencji GPL-3.0
zawartej w `LICENSE`.

Własna praca AMDNR podlega dodatkowemu warunkowi na mocy sekcji 7(b) GPL-3.0 (patrz
`Licenses/AMDNR_NOTICE.txt`): każda kopia, fork lub utwór zależny, który z niej korzysta, musi zachować
jej noty prawne i wskazywać autorstwo: **AMDNR by 3zwr1** (<https://github.com/3zwr1/AMD-NR---OptiScaler>).

**Prawa autorskie do menu AMDNR.** Menu AMDNR — jego układ, projekt graficzny, teksty oraz kod, który AMDNR do niego dodał — to Copyright (c) 2026 3zwr1 (AMDNR). Jest ono częścią tego forka na licencji GPL-3.0, z następującymi dodatkowymi warunkami (GPL-3.0 section 7): (b) każdy, kto ponownie wykorzystuje jakąkolwiek jego część, musi zachować tę linię o prawach autorskich i w widoczny sposób wskazać autorstwo AMDNR by 3zwr1, w menu i w README; (c) nie wolno przedstawiać go ani jego zmodyfikowanej kopii jako własnej pracy; zmodyfikowane wersje muszą być wyraźnie oznaczone jako zmienione; (e) nie udziela się żadnych praw do nazwy ani logo AMDNR; inne projekty nie mogą ich używać.

Wymienione wyżej prace z projektów źródłowych pozostają przy ich autorach i podlegają ich własnym licencjom;
AMDNR nie rości sobie do nich żadnych praw autorskich.

Kod źródłowy zostanie opublikowany wraz z AMDNR 0.5.0.

## Informacje prawne

Ten build jest rozpowszechniany na licencji GPL-3.0 zawartej w `LICENSE`; licencje bibliotek stron trzecich
znajdują się w `Licenses\`. Neuronowy runtime AMD i jego wagi są redystrybuowane z zachowaniem oryginalnego
autorstwa, zgodnie z podziękowaniami powyżej, wyłącznie dla wygody, bez roszczeń do własności i bez żadnej
gwarancji.

`nvngx_dlssnr.dll` od NVIDIA nie znajduje się w tych archiwach. Nic z tego nie jest popierane przez NVIDIA,
AMD ani żadnego wydawcę gier, nie jest z nimi powiązane ani przez nich wspierane. Ten build bezpośrednio steruje
nieudokumentowaną funkcją. Używasz go na własną odpowiedzialność.
