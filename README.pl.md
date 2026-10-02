# AMDNR — DLSS 5 Neural Rendering na AMD (build OptiScaler) — v0.3.5

[English](README.md) | [中文](README.zh-CN.md) | [Português](README.pt-BR.md) | [Español](README.es.md) | [العربية](README.ar.md) | [Français](README.fr.md) | [Italiano](README.it.md) | [Русский](README.ru.md) | **Polski**

> **Potrzebujemy Twojego wsparcia.** Dołącz do serwera Discord — <https://discord.gg/AMDNR> — znajdziesz tam
> pomoc, zgłosisz błędy i pobierzesz testowe buildy; każde zgłoszenie z logiem sprawia, że kolejny build jest lepszy.

DLSS 5 Neural Rendering uruchamiany na kartach graficznych AMD i wbudowany w OptiScaler, dzięki czemu działa
w każdej grze Direct3D 12, do której OptiScaler już się podpina. Oprócz samego przebiegu neuronowego: model
interleave dający duży wzrost liczby klatek, kompozycja rezydualna, generowanie klatek XeSS odblokowane do 6X
(do 10X opcjonalnie w grach D3D12) oraz FSR Ray Regeneration dla gier korzystających z DLSS Ray Reconstruction. Od
0.3.5 **AMDNR Anywhere** (preview) przenosi Neural Rendering do gier, które nie mają własnego upscalera, przez AMDNR
Launcher, bez zapisywania czegokolwiek w folderze gry (patrz "AMDNR Anywhere").

**Discord: <https://discord.gg/AMDNR>** — wsparcie, zgłoszenia błędów (`#bug-report`), testowe
buildy.

**Wesprzyj projekt: <https://ko-fi.com/3zinr>**

> **Autorem runtime'u danielblnc jest Daniel Blanco.** Neuronowy runtime AMD z plików `*Runtime.zip`
> (`dlssnr_amd_pass1..3.dll`) to **DLSS-NR on AMD by Daniel Blanco (danielblnc)** -
> <https://github.com/danielblnc/DLSS-NR-on-AMD>. Copyright (c) 2026 Daniel Blanco, all rights reserved.
> AMDNR dołącza go bez modyfikacji, za jego zgodą; to nie jest dzieło AMDNR. Prosimy, wesprzyj jego projekt.
> Pełne podziękowania dla wszystkich pozostałych znajdziesz na końcu tej strony.

> **Nowości w 0.3.5:** **AMDNR Anywhere** (preview): Neural Rendering dla gier, które nie mają własnego DLSS, XeSS ani
> FSR 2 - jeden przycisk **PLAY ANYWHERE** w AMDNR Launcherze, nic nie jest zapisywane w folderze gry; w tym wydaniu
> RX 9000 (RDNA 4) (patrz "AMDNR Anywhere"). **Ray Regeneration ma własną zakładkę**, zaraz po Upscaling, a jej linie
> stanu mówią, osobno dla każdej karty i każdego API, co działa, a jeśli nie działa, to dlaczego; na RX 7000 od
> 0.3.5.1 jest znów oferowane jako preview, domyślnie włączone (denoiser AMD nie ma providera dla RDNA 3, więc obraz jest
> skalowany bez odszumiania; pole w zakładce Upscaling albo `[FSR-RR] FfxDenoiserAllowPreRdna4=false` je wyłącza, a gra
> zostaje przy swoim denoiserze). **Przebieg neuronowy
> może działać po upscalingu** (`[DlssNr] AmdPlacement=post`, Neural > Performance > Placement; domyślne `pre` się nie
> zmienia). **Zakładka Frame Gen mówi, dlaczego nic nie jest generowane**, i wymienia pięć kroków (patrz FAQ
> o generowaniu klatek). **Szybciej na RX 7000 i handheldach klasy Z1 Extreme:** około 10 procent mniej czasu sieci,
> ten sam obraz bit w bit (zestaw modułów 0.3.5 w `LmxxfNrRuntime.pak`). **lmxxf 0.37 autorstwa Kiena (MIT) jest
> domyślnie włączony na RX 9000:** około 20 procent mniej czasu sieci na RX 9070 XT (eksperymentalnie na RX 9060 /
> 9060 XT; `[DlssNr] AmdLmxxfL37=false` go wyłącza). Na handheldach RDNA 3 **FSR 4 (INT8)** jest eksperymentalną
> opcją do włączenia na własne życzenie, a własne sloty stylów (Style slots) zachowują teraz cały wygląd. Gry z dynamiczną
> rozdzielczością nie przebudowują już sieci przy każdym kroku, przenoszona edycja nie znika już, gdy poruszasz się na
> handheldzie z Model interleave, Uncharted: Legacy of Thieves nie wywala się już na RX 9000, do tego wiele innych
> poprawek. **Zmieniają się wszystkie trzy pliki: podmień `OptiScaler.dll`, `LmxxfNrRuntime.dll` i
> `LmxxfNrRuntime.pak` razem; użytkownicy launchera: aktualizuje się sam.** Szczegóły: `CHANGELOG.md`.

> **Nowości w 0.3.4.2 (hotfix):** menu w Assetto Corsa: klawisz menu otwiera lub zamyka je tylko raz na naciśnięcie,
> kliknięcia krótsze niż jedna klatka nie giną, a okno wyboru runtime'u reaguje na `1` / `2` / `Enter` / `Esc`, ma X na
> pasku tytułu, a zamknięcie menu liczy się jako "Decide later". Okno wyboru runtime'u nie otwiera już menu samo z
> siebie (zamiast tego jedno powiadomienie), sekcja **Ray Regeneration** w zakładce Neural już się nie ukrywa — jest
> tam zawsze, a przygaszona linijka mówi, dlaczego nie działa — a tekst o Wine / Proton mówi, że Ray Regeneration jest
> tam znanym problemem. AMDNR **przyjmuje też o jeden układ (layout) runtime'u
> danielblnc więcej**, więc nowszy build danielblnc będzie mógł działać tutaj bez żadnej aktualizacji AMDNR.
> **Neural Rendering jest identyczny bajt w bajt z 0.3.4.1 poza tym jednym wierszem przyjętego układu** (przebieg
> neuronowy, oba runtime'y i pak bez
> zmian): przechodząc z 0.3.4.1 lub 0.3.4, podmień tylko `OptiScaler.dll`; użytkownicy launchera: aktualizuje się
> sam. Szczegóły: `CHANGELOG.md`.

> **Nowości w 0.3.4.1 (hotfix):** Ray Regeneration daje mniej miękki obraz na Windows (wyostrzanie 0.25, gdy gra nie
> przekazuje własnego; aby je wyłączyć: Image > Sharpness, zaznacz Override, suwak na 0); koniec z fałszywym
> komunikatem "Upscaler failed to run!" w Control Resonant; na Linux / Proton menu działa (potwierdził to gracz, także
> z włączonym generowaniem klatek), a Ray Regeneration nie dodaje tam domyślnego wyostrzania (patrz "Linux / Proton").
> **AMDNR Launcher 0.3.4.1**, zbudowany na podstawie Waszych opinii z Discorda: dziewięć języków, wyszukiwanie,
> ulubione, ukrywanie, zmiana nazwy, CHOOSE GAME .EXE, PLAY, pełne UNINSTALL i więcej (patrz "AMDNR Launcher").
> Neural Rendering nie zmienił się od 0.3.4 (ten sam runtime i pak): przy przejściu z 0.3.4 podmień tylko
> `OptiScaler.dll`; jeśli używasz launchera, zaktualizuje on AMDNR za Ciebie. Szczegóły: `CHANGELOG.md`.

> **Nowości w 0.3.4:** nowe menu (przebudowana zakładka Neural, ten sam wygląd wszystkich zakładek i przycisk
> **Save report**, który pakuje logi do zipa na potrzeby zgłoszenia); lmxxf jest szybszy na RX 7000 (1440p FSR
> Quality: 73.3 -> 52.2 ms na jedno przejście sieci na RX 7800 XT, czas sieci zmierzony poza grą) oraz na RX 9070 / 9070 XT (kernele
> lmxxf 0.31); lmxxf działa na APU w handheldach (eksperymentalnie; test jednego testera, w grze: około 29 fps w Shadow of the Tomb Raider na ROG
> Ally); opcjonalny **Fast mode** dla lmxxf; **AMDNR Screen GI**, własne GI w przestrzeni ekranu od AMDNR (preview,
> domyślnie wyłączone); do tego wiele poprawek. Podmień
> `OptiScaler.dll`, `LmxxfNrRuntime.dll` i `LmxxfNrRuntime.pak` razem. Szczegóły: `CHANGELOG.md`.

---

## AMDNR - Instrukcja instalacji OptiScaler

Instalacja jest dość prosta. **Na Windows AMDNR Launcher zrobi to wszystko za Ciebie** (patrz "AMDNR Launcher"
niżej). Ręcznie:

### 1. Pobierz pliki

Pobierz te pliki z najnowszego wydania na GitHubie (<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>;
0.3.5 to tag Alpha0.3.5):

* `AMDNR-vX.X.X.zip` (dla 0.3.5: `AMDNR-v0.3.5.zip`), z kompletnym runtime'em lmxxf.
* Dla runtime'u danielblnc jeden zip z runtime'em: na **RX 9000 i na RX 7000** `v0.5.0-Runtime.zip`
  (zalecany) z wydania Alpha0.3.4.2; `v0.4.3-Runtime.zip` (Alpha0.3.4.2 i Alpha0.3.4.1), `v0.4.1-Runtime.zip` i
  `v0.4.0-Runtime.zip` (Alpha0.3.4.1) są nadal akceptowane. Runtime
  lmxxf jest w `AMDNR-vX.X.X.zip` i na RX 7000 oraz RX 9000 nie potrzebuje żadnego zipa z runtime'em; APU
  w handheldach używają tylko lmxxf. AMDNR Launcher sam wybiera zip właściwy dla Twojego GPU.
  Patrz "Co jest w archiwach".

### 2. Wypakuj oba pliki

Wypakuj zawartość obu plików `.zip`.

### 3. Skopiuj wszystko do folderu gry

Najpierw skopiuj wszystkie pliki z `AMDNR-vX.X.X` do głównego folderu gry — tego samego folderu, w którym
znajduje się plik `.exe` gry.

Następnie zrób to samo ze wszystkimi plikami z zipa z runtime'em (np. `v0.5.0-Runtime` na RX 9000 i na RX 7000).

> **Aktualizujesz ze starszego AMDNR?** Skopiuj wszystko ponownie, nadpisując pliki. **W 0.3.5 zmieniły się razem
> trzy pliki:** `OptiScaler.dll` (zastąp plik, któremu zmieniłeś nazwę, np. `dxgi.dll`, nowym plikiem o tej samej
> nazwie), `LmxxfNrRuntime.dll` i `LmxxfNrRuntime.pak` (440 MB). Nie mieszaj ich ze starszymi kopiami. Możesz
> zachować swój `OptiScaler.ini`: nowe ustawienia używają wartości domyślnych. Pliki z Twojego zipa z runtime'em
> danielblnc zostają bez zmian. AMDNR Launcher zrobi to za Ciebie: UPDATE ALL albo REPAIR / UPDATE przy grze, która
> pokazuje "Update available" (launcher najpierw aktualizuje sam siebie).

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
* `winhttp.dll`
* `wininet.dll`

Sprawdzaj po jednej nazwie naraz. Nie twórz kilku kopii `OptiScaler.dll`. To są nazwy, pod którymi mod się ładuje
(plus `OptiScaler.asi` z loaderem ASI); `d3d11.dll` do nich nie należy.

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

**Najprościej: Save report.** Jeśli menu się otwiera, kliknij **Save report** (ostatni wiersz Neural > Diagnostics albo pierwszy wiersz Advanced > Logging). Zapisuje on
jeden zip, `AMDNR-report-<exe gry>-<data>.zip`, w folderze gry (na Pulpicie, jeśli folder gry jest tylko do odczytu,
w przeciwnym razie w `%TEMP%`), z `report.txt`, logami i plikami ini, a menu pokazuje, gdzie trafił. Twoja nazwa
użytkownika Windows i nazwa komputera są zastępowane symbolami zastępczymi; nazwa w ścieżce gry poza `C:\Users\`
już nie. Dołącz zip na kanale `#bug-report`. **COLLECT LOGS** w AMDNR Launcherze zapisuje ten sam zip dla dowolnej
gry, a od 0.3.5.1 dodaje po crashu log awarii i najnowszy zrzut awarii.

> Plik `.exe` zwykle nie znajduje się tam, gdzie wskazuje skrót. Gry na silniku Unreal trzymają go w
> `<Game>\Binaries\Win64\`.

---

### Runtime lmxxf (0.3.0, opcjonalny)

Drugi runtime neuronowy (na licencji MIT, autorstwa lmxxf) może obsługiwać przebieg zamiast
runtime'u danielblnc. Na RDNA 4 działa natywnie; na RDNA 3 (RX 7000, Strix Halo) przez backend RDNA 3 od AMDNR
autorstwa 3zwr1 - tam wolniej, patrz "RX 7000" niżej: zacznij od NR resolution 70% lub mniej. Działa też na APU
w handheldach, eksperymentalnie (patrz "APU w handheldach" niżej). Potrzebuje dwóch rzeczy obok gry:

1. `LmxxfNrRuntime.dll` - w tym archiwum, obok `OptiScaler.dll` (kopiuje się go razem z resztą).
2. `LmxxfNrRuntime.pak` (440 MB, dołączony do zipa AMDNR) obok `LmxxfNrRuntime.dll` - pliki wag
   lmxxf, moduły HIP i HLSL w jednym zaszyfrowanym, uwierzytelnionym pliku. Runtime otwiera go w
   pamięci; nic nie jest wypakowywane na dysk.

Przy pierwszym uruchomieniu, podczas którego zostanie wykryty zainstalowany runtime, a wybór nie został jeszcze
dokonany, menu zapyta, którego użyć (wybór zapisuje wpis `[DlssNr] NrBackend = daniel | lmxxf` w ini; zmienisz go
w Neural > Neural runtime, ze skutkiem od następnego uruchomienia gry). Edycja lmxxf jest nakładana z opóźnieniem
o jedną klatkę, przenoszona przez wektory ruchu, więc klatka nigdy nie czeka na sieć (około 14.1 ms
czasu sieci w 1080p na RX 9070 XT). Jego log to `lmxxf_backend.log` obok gry.

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

Oba runtime'y dzielą jedną zakładkę Neural (patrz "Menu" niżej). Ustawienia, których aktywny runtime nie ma, są
wyszarzone z krótką etykietą albo ukryte z licznikiem. Tylko lmxxf: **Full network**, **Output smoothing**
(Quality > More quality options, wymaga Network history), **Edit detail**, **Edit colour** i **Edge guard** (Image
look > Model strength: wzmocnienie drobnej części edycji modelu, jej kolor względem zmiany jasności oraz
wygaszanie edycji na krawędziach głębi) oraz limit jasnych partii w auto-ekspozycji. Nowe w lmxxf w 0.3.4:
Network output, Encoding, Residual edge fade, Game exposure, Fast mode, odczyt pacingu interleave oraz
natywna maska postaci modelu razem z Structure intensity i Character structure (każda zmiana
przebudowuje sieć: przycięcie trwające około 1 s).

**Full network** (Neural > Performance, `[DlssNr] LmxxfFullNetwork`, tylko lmxxf) uruchamia wszystkie 71 bloków
sieci zamiast pomijać 42, 43 i 46: nieco wierniejszy wynik, około 0.5 ms wolniej w 1080p
(16.6 -> 17.1 ms na RX 9070 XT, zmierzone w 0.3.3). Domyślnie wyłączone.

**Fast mode** (Neural > Performance, `[DlssNr] AmdLmxxfFastMode`, lmxxf, opcjonalny, domyślnie wyłączony) uruchamia
sieć o jeden poziom rozmiaru niżej (1080 -> 900, 900 -> 720): około 29% mniej czasu sieci w 1080p (RX 9070 XT,
zmierzone poza grą), a drobne detale są nieco miększe. Buildy danielblnc, które mają własny Fast mode, też mają tam
wiersz Fast mode (`[DlssNr] AmdDanielFastMode`); runtime'y z paczek runtime tego wydania go nie mają, więc wiersz
jest ukryty.

### RX 7000 (RDNA 3): szybciej dzięki poziomom rozmiaru sieci (nowość w 0.3.4)

Sieć lmxxf działa w kilku stałych rozmiarach (poziomach): 720 (1280x720), 900 (1600x900) i 1080 (1920x1080), do
tego 576 i 360 (nowe, używane w handheldach). Poziom kosztuje tyle samo niezależnie od tego, jaką jego część
wypełnia obraz. Na RDNA 3 (RX 7000, Radeon 8060S / 8050S i APU w handheldach) rozmiar NR w lmxxf jest teraz
domyślnie dopasowywany do poziomu: schodzi do następnego mniejszego poziomu, gdy jest bliżej niego (taniej), w
przeciwnym razie rośnie, aż wypełni własny poziom (ten sam koszt, trochę więcej detali), nigdy powyżej rozmiaru
samej klatki.

Czas sieci na jedno przejście na RX 7800 XT (zmierzony przez testera sondą lmxxf; tylko sieć, średnia z 30
przejść; czas poziomu 900 zmierzono w 1600x900):

| Ustawienie gry | 0.3.3.2 | 0.3.4 na RX 7000 |
|---|---|---|
| 1440p, FSR Quality (render 1706x960), NR 100% | poziom 1080: 73.3 ms | poziom 900: 52.2 ms |
| Render 1080p, NR 85% | poziom 1080: 73.2 ms | poziom 900: 52.2 ms |
| Render 1080p, NR 70% | poziom 900: 52.2 ms | poziom 720: 34.4 ms |
| Render 1080p, NR 80% | poziom 900: 52.2 ms | poziom 900, wypełniony: 52.2 ms (więcej detali) |
| Render 1080p, NR 100% | poziom 1080: 73.2 ms | bez zmian |

- W grze zysk na wyświetlaną klatkę jest mniejszy: z Model interleave sieć działa co drugą klatkę, a gra ma
  własny koszt. Jeszcze niezmierzone w grze.
- Sieć widzi nieco mniejszy obraz (w 1440p Quality około 6% mniej pikseli na bok), więc drobne detale mogą być
  trochę bardziej miękkie. `[DlssNr] AmdLmxxfTierSnap=false` przywraca rozmiary z 0.3.3.2. RX 9000 zachowuje
  rozmiary z 0.3.3.2, chyba że ustawisz `true`.
- **RX 9000:** `[DlssNr] AmdLmxxfTierSnap=true` (tam domyślnie wyłączone) przenosi rozmiar NR lmxxf na rozmiar sieci
  przy każdej rozdzielczości renderu: niektóre rozmiary schodzą o poziom niżej (1440p FSR Quality, 1707x960 -> rozmiar
  900: sieć 14.08 -> 9.96 ms na przebieg na RX 9070 XT, zmierzone poza grą, obraz trochę bardziej miękki), inne rosną w
  obrębie swojego poziomu (80% renderu 1080p -> 1600x900: ten sam koszt, trochę więcej detali). Bez ustawienia RX 9000
  zachowuje rozmiary z 0.3.3.2.
- Zacznij od NR resolution 70% lub mniej (poziom 720 przy renderze 1080p; preset Performance to 70%). Koszt obok
  NR resolution jest liczony według poziomu, na którym działa sieć; jego podpowiedź podaje nazwę poziomu.
- **0.3.5: około 10 procent mniej czasu sieci na RX 7000**, ten sam obraz bit w bit: zestaw modułów 0.3.5 w
  `LmxxfNrRuntime.pak` (zmierzone i sprawdzone sumami kontrolnymi przez testera na RX 7800 XT; tabela wyżej dotyczy
  0.3.4).
- **0.3.5 na RX 9000: lmxxf 0.37 autorstwa Kiena (MIT) jest domyślnie włączony** — około 20 procent mniej czasu
  sieci na RX 9070 XT przy rozmiarze 1080 (14.0 -> około 11.3 ms, zmierzone poza grą; obraz nie jest identyczny bit
  w bit z obrazem 0.3.4.2). Na RX 9060 / 9060 XT też jest włączony, razem z kernelami c32w i FastK, jako eksperyment
  (jeszcze nieuruchomiony na tej karcie). Wyłączniki: `[DlssNr] AmdLmxxfL37=false`, `AmdLmxxfC32w=false`,
  `AmdLmxxfFastK=false`.

### APU w handheldach (eksperymentalnie, nowość w 0.3.4)

lmxxf działa na APU w handheldach z co najmniej 12 jednostkami obliczeniowymi, przez backend RDNA 3 od AMDNR
autorstwa 3zwr1: **Z1 Extreme, Z2 i Radeon 780M** (gfx1103), **Z2 Extreme, Radeon 890M i 880M** (gfx1150). To rozwiązanie eksperymentalne i wolne. Wiersz Neural runtime pokazuje
"experimental" po podziękowaniu za backend RDNA 3.
Pierwsze wyniki testera (ROG Ally, Z1 Extreme): sonda lmxxf poza grą, 54.7 ms na przejście sieci w rozmiarze 360p,
110.9 ms w 576p; w grze, test jednego testera (Shadow of the Tomb Raider, 1280x720 z XeSS, preset Handheld) średnio 62 ms na przejście
sieci w 360p przy modelu co 4. klatkę, około 29 fps z włączonym NR. **0.3.5 odejmuje od tych liczb około 10 procent
na układach klasy Z1 Extreme** (Z1 Extreme, Z2, Radeon 780M: około 50 ms w 360p, około 105 ms w 576p, ten sam obraz
bit w bit, sprawdzone sumami kontrolnymi przez testera); moduły dla Z2 Extreme / 890M / 880M są bez zmian.

- **Nieobsługiwane:** Z1 i Radeon 740M (4 jednostki obliczeniowe), Radeon 760M (8), Radeon 860M / 840M. Runtime
  danielblnc nie działa na APU w handheldach. RX 6000 (RDNA 2) jest planowane na 0.3.6; Steam Deck i inne APU
  RDNA 2 nie są obsługiwane.
- **Co robi samo** (tylko dopóki Twój ini nie ma własnej wartości): sieć działa w najmniejszym rozmiarze, 360p
  (640x360), a model działa co czwartą klatkę (Model interleave; niezapisywane). Neural passes zostaje na 1.
- **Szybkość, uczciwie:** Dla
  porównania: RX 7800 XT (60 jednostek obliczeniowych) potrzebuje 34.4 ms na przejście sieci w rozmiarze 720; te
  układy mają ich od 12 do 16 i pracują z niższymi zegarami. Spodziewaj się dużego spadku liczby klatek nawet w
  360p z modelem co czwartą klatkę, trochę ghostingu przez długi interleave i bardziej miękkiego obrazu niż na
  desktopowym GPU. Koszt NR na końcu linii stanu zakładki Neural (i w Diagnostics) pokazuje prawdziwą wartość na
  Twoim urządzeniu.
- **Ustawienia:**
  - Ostrzej, ale wolniej: `[DlssNr] AmdLmxxfTierCap=576` (rozmiar sieci 1024x576).
  - Przy renderze 720p lub 800p NR resolution 100% już podaje rozmiar 360p, więc niższa NR resolution nie
    obniża kosztu.
  - Model interleave ustawione na Off jest zapisywane jako `[DlssNr] AmdInterleave=1` (też wyłączone), żeby
    domyślna wartość handhelda nie wracała przy następnym starcie. Aby wyłączyć ręcznie, wpisz 1, nie 0.
  - Preset > **Handheld** ustawia NR resolution na 100%, wyłączone Dynamic NR, model co 4. klatkę, 1 Neural pass i wyłączony Full network. Przycisk pojawia się tylko na tych APU; Quality, Balanced i Performance też zostają tu przy rozmiarze sieci 360p (menu to mówi).
- **FSR 4:** FSR 4 (INT8) jest dostępne jako eksperymentalna opcja na handheldach RDNA 3 (zakładka Upscaling); niezweryfikowane przez AMD.
  Pole **FSR 4 (INT8) - Experimental on this GPU (restart)** zapisuje `[FSR] Fsr4ForceModel=2`; nigdy nie włącza się samo
  (przy `Dx12Upscaler=auto` upscalerem jest XeSS). Około 1,5-3 ms na klatkę na Z1 Extreme (szacunek); z NR
  FSR 4 albo rozmiar 576, nie oba naraz.
- **Shadow of the Tomb Raider** (i gry, które tworzą urządzenie D3D12 dwa razy) nie wysypuje się już przy starcie
  upscalera (poprawione w 0.3.4).
- **Sterownik:** używaj sterownika Adrenalin od AMD. lmxxf potrzebuje HIP (`amdhip64_7.dll`), którego niektóre
  sterowniki producentów handheldów nie zawierają; `amd_bridge.log` mówi wtedy, że HIP jest niedostępny.
- **Ruch z Model interleave (naprawione w 0.3.5):** przenoszona edycja znikała, gdy tylko zaczynałeś się poruszać
  ("efekt znika podczas ruchu"): pod Model interleave sieć działa na klatkach o nierównej długości, a zabezpieczenie
  przenoszenia zakładało równe klatki. Teraz odczytuje wektory poprzedniej klatki według czasu trwania bieżącej
  klatki (oba runtime'y).
- **Używaj razem plików z 0.3.5:** 0.3.5 zmienia wszystkie trzy pliki (`OptiScaler.dll`, `LmxxfNrRuntime.dll` i
  `LmxxfNrRuntime.pak`), więc podmień je razem; runtime odrzuca handheld, gdy `OptiScaler.dll` jest starszy niż 0.3.4
  ("this handheld needs OptiScaler.dll 0.3.4 or newer").
- **Testerzy z handheldem:** poproś na Discordzie o zestaw testowy dla handheldów (`handheld-test.zip`). Jego
  `run_probe.bat` mierzy sieć na Twoim urządzeniu i zapisuje `handheld_result.txt` (Twoja nazwa użytkownika
  Windows jest maskowana).

## AMDNR Anywhere (preview, nowość w 0.3.5)

**Co to jest.** Neural Rendering dla gier, które nie mają własnego DLSS, XeSS ani FSR 2 - i nic nie jest zapisywane
w folderze gry. AMDNR działa wewnątrz hosta przechwytującego okno: host przechwytuje okno gry, skaluje je do
rozdzielczości Twojego ekranu przez FSR 3, a Neural Rendering działa na przechwyconym obrazie; nasze menu jest
rysowane wewnątrz hosta (Twój klawisz menu, domyślnie `INSERT`) z własną zakładką **Anywhere**. Hostem jest
**Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR**:
AMDNR Launcher pobiera go z wydania jego autora (467 MB, jednorazowo).

**Jak tego używać.** W AMDNR Launcherze gra bez upscalera pokazuje **PLAY ANYWHERE** zamiast INSTALL. Naciśnij go:
launcher pobiera hosta (za pierwszym razem), uruchamia grę, a host przechwytuje jej okno. Uruchom grę **w oknie albo
w oknie bez ramki**, nie w trybie pełnoekranowym na wyłączność, i naciśnij swój klawisz menu, żeby otworzyć menu AMDNR
wewnątrz hosta. Gra z własnym upscalerem zostaje przy zwykłej ścieżce INSTALL: Anywhere jest dla gier, które go nie
mają.

**Ustawienia hosta są w menu**, w Host settings zakładki Anywhere, nie w launcherze: rozmiar okna gry (720p / 900p /
1080p - wskazówka, co ustawić w grze; host przechwytuje każde okno, które otworzy gra), lista etapów (V1: jeden
przebieg FSR 3 do ekranu; V2: FSR 3 w 1x, potem przebieg wypełniający), poziom NR (Auto / 720 / 900 / 1080), VRR,
frame pacing oraz **liczba klatek hosta** (Default = częstotliwość odświeżania Twojego ekranu, najwyżej 60; Auto =
tempo, które sieć utrzymała w ostatniej sesji; od 30 do 120; Display refresh = bez limitu). Obowiązują od
**następnego** PLAY ANYWHERE, a launcher pokazuje obok przycisku ich podsumowanie tylko do odczytu. We własnym
`OptiScaler.ini` hosta są to `[DlssNr] AnywhereWindow`, `AnywhereEffect`, `AnywhereNrTier`, `AnywhereVrr`,
`AnywherePacing` i `AnywhereHostFps`. Ogranicz też samą grę, jej własnym limiterem, do 60-90 fps: host może pokazać
tylko klatki, które gra narysowała, a każda klatka hosta uruchamia sieć jeden raz.

**Czego host nie może dać sieci.** Przechwycone okno nie ma własnej głębi, wektorów ruchu, jittera ani ekspozycji;
host szacuje ruch. Dlatego zakładka Anywhere podaje, co jest szacowane; wiersze zakładki Neural, które nie mogą tam
działać, są ukryte albo odrzucone z podaniem powodu (Ray Regeneration, Screen GI i Model interleave - wydałby
przenoszoną edycję na szacowany ruch); a linia stanu na stronie Anywhere mówi, kiedy sieć przekracza budżet klatki
hosta i co obniżyć. **Okno gry 1920x1080 lub mniejsze to przypadek co do piksela:** większe okno jest najpierw
zmniejszane do górnej granicy sieci, a potem skalowane z powrotem w górę, i strona to mówi, podając, jaką część
pikseli ekranu widziała sieć. Wiersz NR resolution pokazuje prawdziwy rozmiar sieci wewnątrz hosta.

**Status: preview.** W tym wydaniu tylko RX 9000 (RDNA 4); RX 7000 dołączy, gdy zostanie tam przetestowane. Lekkie
migotanie lub przycinanie (judder) przy szybkim ruchu może pozostać (obniż poziom NR do 720, ogranicz grę do 60-90
fps, zostaw Model interleave wyłączone - host go odrzuca). Host przechwytujący jest pobierany z wydania jego autora na
GitHubie, nie z naszego. Zgłoszenia: zip z **Save report** z menu wewnątrz hosta (jego tytuł podaje skalowaną grę)
albo COLLECT LOGS w launcherze.

## Linux / Proton (Steam Deck, Linux na komputerze)

AMDNR działa pod Protonem i Wine jako build OptiScaler. **Neural Rendering nie działa na Linuksie (tylko Windows):**
oba runtime'y NR potrzebują HIP ze sterownika AMD dla Windows, którego Proton i Wine nie zapewniają. Gdy NR jest
włączony pod Protonem, NR się nie uruchamia, a od 0.3.5 zakładka Neural i raport mówią to wprost (zamiast "Idle"). To
oczekiwane zachowanie, a nie crash ani zepsuta instalacja. AMDNR Launcher to program dla Windows, który może działać pod Protonem (eksperymentalnie, jeszcze tego
nie testowaliśmy, patrz niżej); instalacja ręczna działa bez niego.

**Co działa:** upscalery FSR (FSR 3.1 oraz FSR 4 na GPU i sterownikach, które je obsługują), menu (`INSERT`) i
**Save report**. Gracz potwierdził na Steam Proton (RX 9070 XT, vkd3d-proton, Resident Evil Requiem), że gra się
uruchamia, menu się otwiera i przejmuje mysz, a Save report działa, także z włączonym generowaniem klatek.

**Ray Regeneration i generowanie klatek:**
- Znany problem: Ray Regeneration może pokazywać różowe / purpurowe plamy pod Protonem; domyślne wyostrzanie jest tam teraz wyłączone, ale jeśli plamy nadal się pojawiają, użyj zwykłego FSR (FSR 4 na RX 9000) i wyślij Save report.
  Pod Protonem AMDNR nie dodaje wyostrzania po Ray Regeneration, gdy gra nie przekazuje własnego (na Windows dodaje
  0.25): Image > Sharpness pokazuje "RR default 0 (off on Proton)", a Override nadal ustawia Twoją własną wartość.
- **Generowanie klatek** włącza się teraz bez crasha, ale liczniki fps liczą też wygenerowane klatki: przy limicie
  60 fps albo V-Sync 60 Hz to 30 prawdziwych klatek, co wygląda jak 30. Na razie zostaw je wyłączone pod Protonem
  (`[FrameGen] FGOutput=nofg`) albo używaj go tylko wtedy, gdy gra osiąga bez niego około 60 fps, na ekranie
  szybszym niż 60 Hz.
- **Tytuły Vulkan** (gry RTX Remix, id Tech 8), pod Protonem tak samo jak na Windows: Ray Reconstruction i własne
  generowanie klatek AMDNR dostają z założenia odpowiedź "not supported" (denoiser działa na D3D12, a bufory ray
  tracingu zostają na urządzeniu Vulkan); od 0.3.5 zakładki Ray Regeneration i Frame Gen mówią to wprost, zamiast
  prosić o ich włączenie w grze, która je wyszarza. Super resolution DLSS działa przez most.
- Od 0.3.5 `[Spoofing] Dxgi=true` pod Protonem (ścieżka aktualizacji do FSR 4) nie wywala się już przy starcie.

**Wymagania:** aktualny Proton lub Wine (przetestowane: Proton 11, czyli Wine 11), z grą na vkd3d-proton (D3D12)
albo DXVK (D3D11), co jest domyślne w Protonie. Starsze wersje nie są testowane.

**AMDNR Launcher na Linuksie (eksperymentalnie, jeszcze tego nie testowaliśmy).** Launcher to ten sam program dla
Windows, `AMDNR-Launcher.exe` (samowystarczalny: nie trzeba instalować .NET ani innego runtime'u). Pod Wine / Proton
wykrywa Wine i pokazuje komunikat z informacją, co zrobić. Szuka też Twojej linuksowej biblioteki Steam przez dysk
`Z:` w Wine (`~/.steam/steam` i `~/.local/share/Steam` oraz foldery bibliotek wymienione w `libraryfolders.vdf`).
Sami jeszcze tego nie testowaliśmy: jeśli spróbujesz, daj nam znać na Discordzie, czy działa. Jak spróbować:

1. W Steam dodaj `AMDNR-Launcher.exe` jako grę spoza Steam (**Games > Add a Non-Steam Game to My Library**).
2. W **Properties > Compatibility** launchera wymuś wersję Protona (Proton Experimental), a potem uruchom go przez
   Steam.
3. Jeśli gry nie ma w **LIBRARY** (BIBLIOTEKA), naciśnij **ADD** (DODAJ) i wybierz folder gry (Twoje foldery
   z Linuksa są na dysku `Z:`); jeśli launcher wybierze zły plik `.exe`, użyj **CHOOSE GAME .EXE**
   (WYBIERZ PLIK .EXE GRY).
4. Wybierz grę i naciśnij **INSTALL** (ZAINSTALUJ).
5. W **Properties > General > Launch Options** gry (w polskim Steam: **Właściwości > Ogólne > Opcje uruchamiania**)
   wpisz `WINEDLLOVERRIDES="dxgi=n,b" %command%` (jeśli launcher użył dla tej gry innej nazwy DLL, wpisz ją
   w miejsce `dxgi`), a potem przejdź do kroków 5 i 6 instalacji ręcznej niżej (grę uruchamiaj przez Steam;
   przycisk **PLAY** w launcherze jest pod Wine / Proton wyłączony).

**Instalacja ręczna** (bez launchera):

1. Pobierz `AMDNR-vX.X.X.zip` ze strony wydań (dla 0.3.5: `AMDNR-v0.3.5.zip`). Zipy z runtime'em danielblnc
   (`v0.5.0-Runtime.zip` i pozostałe) są używane tylko przez Neural Rendering, więc na Linuksie nie są potrzebne
   (skopiowanie jednego z nich nie szkodzi).
2. Wypakuj zip i skopiuj wszystko do folderu gry, obok pliku `.exe` gry.
3. Zmień nazwę `OptiScaler.dll` na `dxgi.dll`.
4. W Steam otwórz **Properties > General > Launch Options** gry (w polskim Steam: **Właściwości > Ogólne > Opcje
   uruchamiania**) i wpisz:

   ```
   WINEDLLOVERRIDES="dxgi=n,b" %command%
   ```

   To każe Wine załadować `dxgi.dll` z folderu gry zamiast własnego; bez tego AMDNR się nie załaduje.
   Jeśli użyłeś innej nazwy (na przykład `winmm.dll` lub `version.dll`), wpisz ją w miejsce `dxgi`, np.
   `WINEDLLOVERRIDES="winmm=n,b" %command%`. Lutris, Heroic i Bottles: dodaj to samo nadpisanie (`dxgi` =
   `native,builtin`) w ustawieniach DLL overrides lub zmiennych środowiskowych runnera.
5. W `OptiScaler.ini` ustaw `[FrameGen] FGOutput=nofg` (generowanie klatek wyłączone, patrz wyżej).
6. Uruchom grę i naciśnij `INSERT`, żeby otworzyć menu. Tam skonfiguruj upscaler.

**Menu.** W 0.3.4, z włączonym generowaniem klatek, menu mogło się otworzyć, nie przejmując myszy ani klawiatury,
albo w ogóle się nie otworzyć. 0.3.4.1 dołącza menu do okna gry; gracz potwierdził pod Protonem, że menu się
otwiera i przejmuje mysz, także z włączonym generowaniem klatek. Jeśli u Ciebie nadal się to zdarza, AMDNR pokazuje
ostrzeżenie "Menu window lost". Wtedy w `OptiScaler.ini` ustaw `[FrameGen] FGOutput=nofg`; jeśli menu nadal nie
reaguje, ustaw też `[Menu] OverlayMenu=false` (klasyczne menu, które nie zależy od okna nakładki).

**HDR.** AMDNR nie włącza HDR pod Protonem. HDR zależy od Twojej konfiguracji Protona i pulpitu: potrzebny jest
build Protona z obsługą HDR i sesja, która potrafi wyświetlać HDR (na przykład gamescope albo pulpit Wayland
z włączonym HDR). Jeśli HDR działa w grze bez AMDNR, działa też z AMDNR; jeśli opcja HDR w grze jest wyszarzona,
rozwiązania trzeba szukać w konfiguracji Protona lub pulpitu.

**Zgłaszanie problemu na Linuksie:** użyj przycisku **Save report** w menu (zostaw `[Log] LogToFile=true`, wartość
domyślną, żeby raport zawierał log z tej sesji); raport pokazuje, czy gra działała pod Wine/Proton, na vkd3d-proton
czy na DXVK. Prosimy, dopisz swoją dystrybucję, GPU, wersję Mesa i wersję Protona.

## Wymagania

- Windows 10 lub 11 (64-bit) dla Neural Rendering. AMDNR Launcher to program dla Windows, który może działać pod
  Protonem (eksperymentalnie, jeszcze tego nie testowaliśmy). Pod Linux / Proton AMDNR działa jako build OptiScaler
  bez NR (patrz "Linux / Proton").
- Karta graficzna AMD ze sterownikiem AMD Software: Adrenalin Edition 26.9.1 lub nowszym. Runtime neuronowy korzysta z HIP przez sterownik; nie potrzeba
  ani HIP SDK, ani trybu dewelopera. Zgodność układów:
  - RX 9000 (RDNA 4): oba runtime'y.
  - RX 7000 (RDNA 3, desktopowe i mobilne): oba runtime'y - lmxxf przez backend RDNA 3 od AMDNR, wolniej niż na
    RDNA 4 (poziomy rozmiaru sieci są domyślnie włączone, patrz wyżej).
  - Strix Halo (Radeon 8060S / 8050S): lmxxf.
  - APU w handheldach z 12+ jednostkami obliczeniowymi (Z1 Extreme / Z2 / 780M, Z2 Extreme / 890M / 880M):
    lmxxf, eksperymentalnie i wolno. Z1 (4 CU), 760M / 740M i 860M / 840M: nieobsługiwane.
  - RX 6000 (RDNA 2): jeszcze nieobsługiwane, planowane na 0.3.6. Steam Deck i APU RDNA 2: nieobsługiwane (w
    przypadku Neural Rendering; upscalery pod Protonem opisuje sekcja "Linux / Proton").

  Zakładka Neural pokazuje, co może uruchomić Twoje GPU (najedź na pozycje runtime'ów albo zobacz linię GPU w
  Diagnostics).
- **AMDNR Anywhere** (preview): Windows, w tym wydaniu karta RX 9000 (RDNA 4), oraz AMDNR Launcher, który pobiera
  host przechwytujący; gra działa w oknie albo w oknie bez ramki. Patrz "AMDNR Anywhere".
- Gra Direct3D 12, Direct3D 11 lub Vulkan. Sama ścieżka neuronowa AMD działa na D3D12; tytuły D3D11 i
  Vulkan docierają do niej przez most D3D12 w OptiScaler, co oznacza, że upscaler musi być
  jednym z backendów "w/Dx12" (`ffx_12`). Zostaw `Dx11Upscaler` / `VulkanUpscaler` na `auto`,
  a ten build wybierze go za Ciebie, gdy neural rendering jest włączony. Przy włączonym Neural Rendering lista Upscaling nazywa je "... w/Dx12 - Neural".
- Około 2 GB wolnego VRAM przy rozdzielczościach renderowania klasy 1080p.

## Co jest w archiwach

**AMDNR-vX.X.X.zip**

| Plik | Co to jest |
|---|---|
| `OptiScaler.dll` | OptiScaler z backendem DLSS-NR dla AMD (AMDNR 0.3.5). Zmień jego nazwę zgodnie z instrukcją. |
| `OptiScaler.ini` | Ustawienia. Neural Rendering jest włączony; logowanie jest włączone, żeby do zgłoszenia błędu było co dołączyć. |
| `LmxxfNrRuntime.dll` | Runtime neuronowy lmxxf (0.3.5: przed użyciem sprawdza każdy moduł HIP w paku względem własnej listy skrótów paka, podaje obie strony, gdy żaden adapter HIP nie pasuje do GPU gry, utrzymuje kroki dynamicznej rozdzielczości bez przebudowy sieci i nie odczytuje już zmiennych środowiskowych lmxxf, które zmieniają obraz; kernele lmxxf, w tym z lmxxf 0.31, kernele c32w od AMDNR, małe rozmiary sieci i natywna maska postaci). Używany tylko po wybraniu; odczytuje `LmxxfNrRuntime.pak` leżący obok, patrz "Runtime lmxxf". |
| `LmxxfNrRuntime.pak` | Wagi, moduły HIP i shadery runtime'u lmxxf w jednym zaszyfrowanym pliku (465 MB; 0.3.5: zestaw modułów dla RX 7000 i handheldów klasy Z1 Extreme - Z1 Extreme, Z2, Radeon 780M - jest około 10 procent szybszy, ten sam obraz; RX 9000 dostaje moduły lmxxf 0.37 / 0.39 autorstwa Kiena (MIT) obok swojego niezmienionego zestawu bazowego; moduły Z2 Extreme / 890M / 880M i Strix Halo są bez zmian). Czyta go tylko runtime lmxxf; można go bez szkody zostawić, korzystając z runtime'u danielblnc. |
| `OptiScaler\` | FSR, XeSS, denoiser FidelityFX i D3D12 Agility SDK, z których korzysta OptiScaler. |
| `OptiScaler/amdnr_dlssg_fsr3.dll` | dlssg-to-fsr3 autorstwa Nukem9, bez modyfikacji, ze zmienioną nazwą: wywołania DLSS Frame Generation z gry obsługiwane przez generowanie klatek FSR 3, także na API Vulkan (`FGNvngxReplacement=Nukems`). GPLv3, patrz `Licenses/`. |
| `Licenses\`, `LICENSE` | Licencje stron trzecich, nota AMDNR (`AMDNR_NOTICE.txt`) i licencja GPL-3.0 tego buildu. |
| `SHA256SUMS.txt` | Sumy kontrolne każdego pliku w tym zipie oraz tych plików z zipów z runtime'em danielblnc, które wymienia. |

**Zipy z runtime'em danielblnc** (DLSS-NR on AMD by Daniel Blanco, bez modyfikacji, za jego zgodą; użyj jednego)

Który wybrać: na **RX 9000 i na RX 7000** `v0.5.0-Runtime.zip` (zalecany) z wydania Alpha0.3.4.2;
`v0.4.3-Runtime.zip` (Alpha0.3.4.2 i Alpha0.3.4.1), `v0.4.1-Runtime.zip` i `v0.4.0-Runtime.zip` (Alpha0.3.4.1) są
nadal akceptowane.
Runtime lmxxf nie potrzebuje żadnego zipa z runtime'em na RX 7000 i RX 9000; APU w handheldach używają tylko lmxxf.
AMDNR Launcher oferuje 0.5.0 (zalecany), 0.4.3, 0.4.1 i 0.4.0 i wybiera go za Ciebie.

| Zip | Wydanie | Runtime danielblnc |
|---|---|---|
| `v0.5.0-Runtime.zip` | Alpha0.3.4.2 | 0.5.0, **zalecany na RX 9000 i RX 7000**; ustawienia runtime'u danielblnc działają z nim, a od 0.3.5 dociera do niego Twój własny klucz `Async` w jego `dlssnr_on_amd.ini` |
| `v0.4.3-Runtime.zip` | Alpha0.3.4.2 (i Alpha0.3.4.1) | 0.4.3, nadal akceptowany; ustawienia runtime'u danielblnc działają z nim |
| `v0.4.1-Runtime.zip` | Alpha0.3.4.1 (i Alpha0.3.4) | 0.4.1, nadal akceptowany. Network style, Tone curve, Black lift i Game exposure są z nim wyszarzone |
| `v0.4.0-Runtime.zip` | Alpha0.3.4.1 (i Alpha0.3.4) | 0.4.0, nadal akceptowany; ustawienia runtime'u danielblnc działają z nim |
| `v0.3.3-Runtime.zip` | [Alpha0.3.4](https://github.com/3zwr1/AMD-NR---OptiScaler/releases/tag/Alpha0.3.4) | 0.3.3, wycofany: już nie zalecany. Nadal działa, jeśli już go masz; ustawienia runtime'u danielblnc działają z nim |
| `Runtime.zip` | Alpha0.3.4 | 0.3.1; ustawienia runtime'u danielblnc są z nim wyszarzone |

**danielblnc 0.5.0 jest zalecanym runtime'em danielblnc od 0.3.5** (działa też na 0.3.4.2). 0.3.5 przekazuje mu
też Twój własny klucz `Async` (albo starszy `Inline`) z `dlssnr_on_amd.ini`, zamiast wymuszać tryb tej samej klatki
(same-frame); bez żadnego z tych kluczy domyślna instalacja się nie zmienia. Build danielblnc nowszy niż 0.5.0 nie
jest obsługiwany przez to wydanie.

Każdy z nich zawiera:

| Plik | Co to jest |
|---|---|
| `dlssnr_amd_pass1..3.dll` | Runtime neuronowy AMD, bez modyfikacji. Trzy kopie, żeby multi-pass miał po jednej na każdy przebieg. |
| `dlssnr_on_amd_weights.bin` | Wagi sieci, które ładuje runtime. |
| `danielblnc_ATTRIBUTION.txt` | Informacja o autorstwie Daniela Blanco i warunki, na których AMDNR dołącza jego runtime. |

## Menu (nowość w 0.3.4)

Naciśnij `INSERT`. Wszystkie zakładki wyglądają tak samo: tekstowe zakładki, wiersz nagłówka z Discord i
GitHub (otwiera tę stronę), wiersz podziękowań (nazwisko Daniela Blanco otwiera jego stronę GitHub), linia **Components** (ile z siedmiu
komponentów OptiScaler jest aktywnych; kliknij, aby zobaczyć listę) oraz stopka z
Menu Scale, Save Settings i Close. Pomoc otwiera się po najechaniu na etykietę ustawienia.

**Zakładka Neural, od góry do dołu:**

- **Enable Neural Rendering** i jego klawisz (przycisk, np. `Home`: kliknij go, a potem naciśnij inny klawisz,
  żeby go zmienić).
- **Neural runtime** (danielblnc / lmxxf, z dokładną wersją Twoich plików, np. `lmxxf 0.3.4`) z jednym słowem stanu: running, restart the game to switch, not
  installed, not for this GPU albo stopped. Pod spodem podziękowanie dla aktywnego runtime'u i jedna linia stanu,
  np. `Running - 1920x1080 at 100% - NR 62/s - model 62/s - 15.3 ms` (ostatnia liczba to koszt NR) oraz zwinięty wiersz **Live** z
  dodatkowymi szczegółami. Gdy coś
  wymaga Twojej uwagi, pojawia się pomarańczowa linia, z przyciskiem, gdy jest rozwiązanie (Retry lmxxf, Switch to
  danielblnc, Open Upscaling). W stanie domyślnym nie ma żadnej.
- **Preset**: Quality / Balanced / Performance ustawiają NR resolution na 100 / 85 / 70% i wyłączają Dynamic NR;
  nic więcej. Na APU w handheldach jest czwarty przycisk, **Handheld** (patrz "APU w handheldach"). **NR style** oraz
  **Style slots** (Store / Apply / Clear).
- **Performance**: **Placement** (przed / po upscalingu, nowość w 0.3.5; patrz "Ustawienia, które warto znać"), NR
  resolution (%) z kosztem, Neural passes, Full network, Fast mode, Dynamic NR resolution, Model interleave
  (Interleave preset i linia pacingu pojawiają się pod nim, gdy jest włączony).
- **Quality**: Residual strength, Residual limit, Temporal stability, Sharpening (CAS) oraz **More quality
  options** (Network history - jedno pole dla obu runtime'ów -, Output smoothing, Stability mode, Residual
  temporal, Residual edge fade, Still-surface steadiness).
- **Image look**: Colour composition, Detail i Colour strength oraz trzy zwijane sekcje: **Model strength** (Tone
  i Structure intensity, Character structure, Edit detail / colour, Edge guard, Native character mask oraz Network
  style, Tone curve i Black lift od danielblnc), **Exposure and highlights** (Auto-exposure, jego limit jasnych
  partii, Highlight colour guard, Game exposure) i **Appearance filter** (ze słowem off / on po nazwie). Przygaszone "default" lub "custom" po nazwie zwijanej sekcji
  mówi, czy coś w niej zmieniono.
- **Ray Regeneration**: od 0.3.5 odsyłacz. Gdy Ray Regeneration działa w danym tytule, jego ustawienia są na
  własnej zakładce **Ray Regeneration** (zaraz po Upscaling), a ta linia oferuje przycisk **Open Ray Regeneration**;
  gdy nie działa, linia mówi dlaczego (gra nie włączyła Ray Reconstruction, sterownik odrzucił denoiser na tej karcie,
  Ray Regeneration zrezygnowało z tej gry i dlaczego, albo kiedy działało ostatni raz).
- **Wiersz narzędzi**, zamknięty na starcie: **Diagnostics** (Network output, Debug view, Edit shaper A/B, NR cost,
  odczyty ghostingu i samostrojenia, linia GPU, **Save report**; widok debug RR jest od 0.3.5 na zakładce Ray
  Regeneration), **Runtime options** (Encoding, Every-frame NR, NR slots, Highlight proxy) i **Experimental** (AMDNR
  Screen-space GI, w wersji preview).

Ustawienie, którego aktywny runtime nie ma, jest wyszarzone z krótką etykietą (np. "not in lmxxf yet") albo ukryte
z licznikiem ("3 danielblnc-only options hidden"); zmiana runtime'u nie przesuwa żadnego innego wiersza.

**Pozostałe zakładki:** Upscaling zaczyna się od upscalera, jednej linii stanu i Render resolution (dawne Upscale
Ratio Override i Output Scaling); na karcie innej niż NVIDIA "DLSS w/Dx12" nie jest już wyświetlane. **Ray
Regeneration** (nowość w 0.3.5) następuje po Upscaling, gdy Ray Regeneration działa w danym tytule: linie stanu (dla
każdej karty i każdego API), wiersz **Denoiser backend** (Automatic / Off - Off mówi grze, że Ray Reconstruction nie
jest obsługiwane, więc gra zachowuje własny denoiser; po restarcie), ustawienia, More Ray Regeneration options oraz
własny blok Diagnostics z widokiem debug RR i liczbą szumu (ziarno na wejściu i wyjściu, migotanie przy nieruchomej
kamerze); ta zakładka nigdy nie jest rysowana wewnątrz AMDNR Anywhere. Image zawiera Sharpness, Textures, Init Flags
i Magnifier. Frame Gen zaczyna się od FG Input i FG Output oraz, od 0.3.5, jednej linii, która podaje krok wciąż
brakujący, zanim cokolwiek zostanie wygenerowane. Interface ma nakładkę FPS i Keybinds (jeden przycisk na klawisz).
Advanced zaczyna się od Active Quirks, potem Display (V-Sync), Compatibility i Logging. Wewnątrz AMDNR Anywhere menu
pokazuje zakładkę **Anywhere** (linia przechwytywania i sieci, ustawienia hosta) zamiast zakładek Frame Gen
i Advanced. Ustawienia, klucze i to, co zapisuje Save Settings, się nie zmieniają, z wyjątkiem miejsc opisanych w
`CHANGELOG.md`.

## Ustawienia, które warto znać

Otwórz zakładkę **Neural**. Wartości domyślne to najnowsza przetestowana konfiguracja, więc na początek
najlepiej zmieniać jedną rzecz naraz.

- **NR resolution** — główny regulator jakości i kosztu. Poniżej 100% model pracuje na mniejszym
  obrazie i tylko jego *korekta* jest przenoszona z powrotem na klatkę w pełnej rozdzielczości, więc
  klatka zachowuje własne detale. Powyżej 100% koszt rośnie kwadratowo (150% to 2.25x). Suwak
  zmienia się co 5%: każdy nowy rozmiar NR może zajmować VRAM aż do restartu gry, więc zrestartuj grę
  po wielu zmianach.
  Koszt obok pokazuje 1.00x przy 100%; w lmxxf to cena poziomu rozmiaru sieci, na którym działa (jego podpowiedź
  podaje poziom). Przyciski Preset ustawiają go na 100 / 85 / 70%.
- **Placement** (Neural > Performance, `[DlssNr] AmdPlacement = pre | post`, nowość w 0.3.5, oba runtime'y) — gdzie
  działa przebieg neuronowy. `pre` (domyślne i to, co robił każdy wcześniejszy build) edytuje obraz w rozdzielczości
  renderowania, który upscaler zaraz odczyta. `post` edytuje zamiast tego gotowy obraz upscalera w rozdzielczości
  wyświetlania: ostrzej, bo upscaler nie filtruje już ponownie edycji, i drożej - sieć działa w rozmiarze wyświetlania
  aż do swojej górnej granicy 1920x1080, więc ekran 1080p płaci za najwyższy poziom przy każdej NR resolution, a ekran
  1440p lub 4K dostaje edycję w 1080 liniach przeniesioną z powrotem w górę - trochę mniej wyrozumiałe w ruchu, a HUD
  też jest edytowany, jeśli gra składa go przed upscalingiem. Odrzucane (powrót do `pre`, jedna linia
  w `amd_bridge.log`) wewnątrz AMDNR Anywhere, w trybie final image oraz gdy Ray Regeneration już działało w danym
  tytule. Wiersz NR resolution pokazuje oba rozmiary, gdy działa `post`.
- **Residual strength** — jaka część edycji modelu jest nakładana; powyżej 1 ją wzmacnia. To ustawienie
  najbardziej zmienia obraz.
- **Residual limit** — górny limit tego, jak bardzo może się zmienić pojedynczy piksel. Widzisz plamy na obrazie? **Obniż** go.
- **Model interleave** — uruchamia model co drugą klatkę, dając duży wzrost liczby klatek. Pominięte
  klatki wypełnia **Interleave preset**; domyślny jest *Edit accumulation* (preset 10, oba runtime'y):
  każda klatka to jej własny obraz plus korekta niesiona przez model, więc żaden wcześniejszy obraz nie
  jest przetrzymywany. *Guided fill v2* (preset 6, danielblnc) i *Classic carry* (lmxxf) to starsze
  wypełnienia. Frame pacing obu typów klatek jest automatyczny w danielblnc i wyłączony w lmxxf
  (`[DlssNr] AmdInterleavePacing` od 0 do 1 wyrównuje oba, kosztem kilku klatek); przygaszona linia pod presetem
  pokazuje pomiar. Adaptive interleave jest w tej wersji wyłączony.
- **Neural passes** — 2 i 3 nakładają model wielokrotnie, z malejącymi korzyściami. W trybie lmxxf
  historia sieci pozostaje przy pierwszym przebiegu; dodatkowe przebiegi to wyłącznie dopracowanie
  przestrzenne. danielblnc wykonuje 1 przebieg w tytułach Vulkan (informuje o tym notka pod suwakiem).
- **Colour composition** (Neural > Image look, oba runtime'y) — *Classic* (domyślny) to obraz taki
  jak dotąd. *RenoDX (experimental)* uruchamia kompozycję koloru RenoDX po modelu, tak jak robi to
  ścieżka NVIDIA: Composition detail i colour, dwustronny **Highlight guard** (domyślnie 2x), który
  ogranicza odpowiedź modelu względem oryginału, oraz opcjonalne ustawienia skóry / otoczenia.
  Na klatce display-referred (SDR), z Network output lub z Encoding sRGB / Gamma 2.2 wraca do Classic w obu
  runtime'ach; notka w menu oferuje wtedy przycisk, który wyłącza przeszkodę. Style NR i presety go nie ruszają.
- **Native character mask** (Image look > Model strength, `[DlssNr] AutoMask`, domyślnie włączone) — własne
  traktowanie twarzy i skóry przez model. Odznaczenie działa teraz w obu runtime'ach (w lmxxf przebudowuje sieć:
  przycięcie trwające około 1 s); w lmxxf działają teraz także Structure intensity i Character
  structure.
- **Generowanie klatek jest wyłączone w nowym ini** i wymaga pięciu kroków: FG Input i FG Output w zakładce Frame
  Gen (np. "DLSSG via Streamline" w grze z generowaniem klatek DLSS oraz XeFG), **Save Settings**, pełny restart gry,
  włączenie **własnego** generowania klatek w grze, a potem zaznaczenie **Active** w sekcji Frame Generation. Od 0.3.5
  zakładka i log podają krok, którego brakuje. Pełna lista, z tym, co wyłączyć w grze, jest w FAQ niżej
  ("Generowanie klatek: brak wzrostu fps?").
- **Generowanie wielu klatek XeFG** — od 3X do 6X jest wbudowane i domyślnie włączone (`XeFG\UnlockMFG`),
  zarówno dla kopii z OptiScaler, jak i dla własnej kopii gry. **Usuń `XeFGUnlock.asi`** z `OptiScaler\plugins`,
  jeśli nadal go masz: dwie kopie tej samej łatki crashują grę.
  **Do 10X opcjonalnie** (tylko gry D3D12): ustaw *XeFG ceiling (restart)* w sekcji FG Output w
  zakładce Frame Gen (4X, 6X domyślnie, 8X lub 10X; `[XeFG] MaxInterpolatedFrames`), zrestartuj grę, potem
  wybierz mnożnik z listy MFG. Powyżej 6X potrzebny jest provider XeFG wbudowany w OptiScaler, z włączonym Extra pacing;
  kopia XeSS 3 dostarczana z grą pozostaje ograniczona do 6X. 10X wymaga monitora 360 Hz+ i limitu klatek
  ustawionego na odświeżanie / 10; opóźnienie jest wysokie, a provider rezerwuje około 128 MiB więcej VRAM w 4K.
  7X-10X nie jest jeszcze potwierdzone w żadnej grze: testerzy, prosimy o przesłanie `OptiScaler.log`.
- **FSR Ray Regeneration** — na RX 9000 (RDNA 4); na RX 7000 (RDNA 3) jako preview, domyślnie włączone, bez denoisera (patrz niżej); tylko w grach korzystających z DLSS Ray Reconstruction (Cyberpunk 2077,
  Alan Wake 2), gdy gra działa na DLSS (spoofing włączony), a ray tracing i Ray Reconstruction są
  włączone w jej własnych ustawieniach. Neural Rendering działa wtedy po nim, na jego wyjściu, co kosztuje
  więcej: obniż NR resolution, jeśli spadnie liczba klatek. Od 0.3.5 jego ustawienia są na własnej zakładce **Ray
  Regeneration**, zaraz po Upscaling, rysowanej, gdy Ray Regeneration działa w danym tytule; zakładka Neural na nią
  wskazuje i, gdy Ray Regeneration nie działa, zachowuje przygaszoną linijkę, która mówi dlaczego (gra nie włączyła
  Ray Reconstruction, sterownik odrzucił denoiser na tej karcie, Ray Regeneration zrezygnowało z tej gry i dlaczego,
  albo kiedy działało ostatni raz). Wiersz **Denoiser backend** tej zakładki (`[FSR-RR] RrBackend = auto | off`) może
  powiedzieć grze, że Ray Reconstruction nie jest obsługiwane, więc gra zachowuje własny denoiser (od następnego
  uruchomienia gry). W tytule **Vulkan** Ray Reconstruction jest z założenia "not supported" (denoiser działa na
  D3D12), a zakładka to mówi. **Profil path-traced** (mniej ziarna na twarzach przy path tracingu) jest opcjonalny od
  0.3.3.1: zaznacz go tam, jeśli chcesz go wypróbować w Resident Evil Requiem lub PRAGMATA. Ta sama zakładka ma siłę
  bias mask i **wygładzanie skóry** (eksperymentalne, dla gier udostępniających SSS guide; domyślnie wyłączone, ale
  w Resident Evil Requiem domyślnie włączone od 0.3.3.2); suwaki strojenia czasowego są w *More Ray Regeneration
  options*, a widok debug RR i liczba szumu (ziarno na wejściu i wyjściu, migotanie przy nieruchomej kamerze) są we
  własnym bloku Diagnostics zakładki. Na RX 7000 (RDNA 3) Ray Regeneration jest
  oferowane jako preview (0.3.5.1; 0.3.5 nie oferowało go domyślnie): gra może włączyć Ray Reconstruction i ustawienia
  Ray Regeneration się pokazują, ale denoiser AMD nie ma providera dla RDNA 3, więc denoiser odmawia startu, a obraz
  Ray Reconstruction jest skalowany bez odszumiania, co może wyglądać na bardziej zaszumione niż własny denoiser gry;
  strona Ray Regeneration mówi, co działa. Pole w zakładce Upscaling **Preview: Ray Regeneration on this card (restart)**
  (z dopiskiem "preview - on by default") jest domyślnie zaznaczone; odznacz je albo ustaw
  `[FSR-RR] FfxDenoiserAllowPreRdna4=false`, a gra zostanie przy swoim denoiserze. Własny denoiser AMDNR dla RX 7000 jest
  planowany. RX 6000 i starsze dostają je tylko z `[FSR-RR] FfxDenoiserAllowPreRdna4=true`
  (zakładka Upscaling: **Offer FSR Ray Regeneration on this GPU (restart)**). **Wyostrzanie po RR** (0.3.4.1): gdy
  gra nie przekazuje ostrości, AMDNR wyostrza obraz o 0.25 po RR na Windows (0 na Linux / Proton); aby to wyłączyć:
  Image > Sharpness, zaznacz Override, suwak na 0. Od 0.3.4.2 ta liczba ma własny klucz w ini,
  `[Sharpness] RrDefaultSharpness` (ta sama wartość domyślna 0.25): wpisz tam 0.15, 0.10 albo 0 bez ruszania
  Override, a wartość, którą twój ini zachował pod `[Sharpness] Sharpness` przy wyłączonym Override, jest w menu
  oznaczona jako oczekująca.
- **AMDNR Screen GI** (preview, nowość w 0.3.4, domyślnie wyłączone; Neural > Experimental albo `[AmdGi] Enabled=true`) — własne światło odbite i okluzja otoczenia w przestrzeni ekranu od AMDNR, z głębi gry, przed NR i upscalerem; działa z włączonym i wyłączonym NR; około 1 ms na High przy renderze 1080p na RX 9070 XT (zmierzone poza grą). To przestrzeń ekranu: brakuje światła spoza ekranu. Patrz `CHANGELOG.md`.
- **Save report** (Neural > Diagnostics albo Advanced > Logging) — jeden zip ze wszystkimi logami i plikami ini do zgłoszenia; patrz "Jeśli to
  nie działa" wyżej.

## Jeśli coś pójdzie nie tak

`OptiScaler.log` pojawia się w folderze gry. Dołącz go na kanale `#bug-report` i napisz, jaka to gra i
jakie masz GPU; **Save report** (Neural > Diagnostics albo Advanced > Logging) pakuje go razem z resztą. Backend AMD zapisuje też
`amd_presr.log` i `amd_bridge.log`, które przydają się wtedy, gdy nieprawidłowo działa konkretnie przebieg
neuronowy. Logi z trzech ostatnich sesji są zachowywane jako `OptiScaler.previous.<exe>.log` (najnowszy),
`OptiScaler.previous-1.<exe>.log` i `OptiScaler.previous-2.<exe>.log` (`[Log] KeepPreviousLogs`; 1 zachowuje tylko
jeden, jak dawniej). Po crashu dołącz także je: nowy log mówi wtedy "no clean exit recorded" (od 0.3.4 już nie po
normalnym wyjściu z gry).

**NR frames 0/s, a zakładka Neural lub `amd_presr.log` mówi, że DLL przebiegu to build, którego ten AMDNR
nie obsługuje?** Twoje `dlssnr_amd_pass1..3.dll` to build danielblnc, którego ten AMDNR nie zna (w obiegu
widziano zestaw 0.2.16), albo brakuje jednego z trzech plików. Od 0.3.3.2 zakładka Neural podaje nazwę
pliku i jego wersję oraz mówi, co zrobić. Użyj zalecanego runtime'u, biorąc wszystkie trzy DLL przebiegów
z tego samego zipa: na **RX 9000 i na RX 7000** `v0.5.0-Runtime.zip` z wydania Alpha0.3.4.2
(149,550,553 bajtów, SHA256 zaczyna się od `7a49ab0e`); `v0.4.3-Runtime.zip` (Alpha0.3.4.2 i Alpha0.3.4.1;
116,484,918 bajtów, SHA256 zaczyna się od `07dd7774`), `v0.4.1-Runtime.zip` i `v0.4.0-Runtime.zip` (Alpha0.3.4.1) są
nadal akceptowane (`dlssnr_amd_pass1.dll` w `v0.4.1-Runtime.zip` ma 9,916,928 bajtów, a jego SHA256
zaczyna się od `823063eb`; w `v0.4.0-Runtime.zip`: 10,027,008 bajtów, `d62be3d8`). Obsługiwane buildy: 0.2.17, 0.3.0,
0.3.1, 0.3.2, 0.3.3, 0.4.0 i zipy runtime'u wymienione powyżej, aż do 0.5.0. Nie instaluj własnego instalatora
danielblnc ani jego `dxgi.dll` / `version.dll` / `winhttp.dll` obok AMDNR: AMDNR już uruchamia jego runtime. **Build
danielblnc nowszy niż 0.5.0 nie jest obsługiwany przez to wydanie:** zakładka Neural podaje plik i jego wersję i to
mówi. Z 0.5.0, 0.4.3, 0.4.1 i 0.4.0 działają ustawienia dostępne tylko dla danielblnc (Network style, Tone curve,
Black lift, Game exposure, Fast mode), a od 0.3.5 Twój własny klucz `Async` w `dlssnr_on_amd.ini` dociera do
runtime'u (patrz "Co jest w archiwach").

**lmxxf nic nie robi albo od razu się zatrzymuje na PC ze zintegrowaną grafiką?** Naprawione w 0.3.3.2. Na
komputerze stacjonarnym z procesorem Ryzen i włączoną zintegrowaną grafiką, na laptopie z APU AMD i kartą Radeon
albo na PC z dwoma GPU AMD często zdarza się, że GPU, na którym działa gra, nie jest urządzeniem HIP 0. lmxxf zawodził wtedy już na pierwszej klatce
(`hipErrorInvalidHandle (400)`, potem "session is poisoned" w `lmxxf_backend.log`) i pozostawał wyłączony.
Podmień oba pliki: `OptiScaler.dll` (plik po zmianie nazwy, np. `dxgi.dll`) oraz `LmxxfNrRuntime.dll` na pliki
z 0.3.3.2 lub nowszego. Jeszcze nieprzetestowane na takim PC: jeśli lmxxf nadal się zatrzymuje, zakładka Neural mówi teraz,
dlaczego; wyślij `lmxxf_backend.log` i `amd_bridge.log` (zawiera listę urządzeń HIP).

**PC ze zintegrowaną grafiką i kartą Radeon (komputer stacjonarny z Ryzenem i włączonym iGPU albo laptop): NR nigdy nie
startuje, a zakładka Neural lub linia GPU wymienia zintegrowaną grafikę?** Przebieg neuronowy AMDNR działa na tym GPU,
na którym gra rysuje. Jeśli Windows uruchomił grę na zintegrowanej grafice, NR w ogóle nie działa na Twoim Radeonie.
Przypisz grę do karty dedykowanej: Ustawienia Windows > System > Ekran > Grafika, dodaj `.exe` gry, Opcje, Wysoka
wydajność; potem uruchom grę ponownie i sprawdź linię GPU w Neural > Diagnostics, która podaje adapter, na którym działa
NR (`OptiScaler.log` ma linię `AMD neural: NR runs on ...`, gdy ten adapter nie jest głównym GPU). Zaobserwowane w
Starfield na komputerze stacjonarnym z Ryzenem. Od 0.3.5 linia w zakładce Neural mówi, który z trzech przypadków
zachodzi - jeszcze brak runtime'u, NR działa **na zintegrowanym GPU** (APU, które runtime akceptuje, np. Radeon 780M
obok karty Radeon: NR działa tam, dużo wolniej niż na karcie) albo runtime na innym adapterze - a `amd_bridge.log`
jednorazowo wypisuje wszystkie adaptery.

**Linia stanu lmxxf pokazuje `c32w=off:nofile` na RX 9070 / 9070 XT?** Stary folder
`DLSS5-AMD\native-game-tiled-assets` obok pliku `.exe` gry (pozostałość po wcześniejszej instalacji lmxxf) jest
używany zamiast `LmxxfNrRuntime.pak`. Nie ma w nim kerneli c32w, więc lmxxf działa z dawną szybkością. Usuń
folder `DLSS5-AMD` albo zmień jego nazwę: pak zawiera wszystko, czego potrzebuje lmxxf. `LmxxfNrRuntime.pak`
starszy niż 0.3.3.2 daje ten sam stan; zastąp go plikiem z tego wydania.
`fk=fff-` w tej samej linii oznacza to samo (stary pak albo luźny folder): lmxxf nadal działa, z dawną
szybkością.

**danielblnc: styl NR nadal się zmienia, gdy NR resolution odchodzi od 100%?** Nadal otwarte od 0.3.4 do 0.3.5, a
wartość domyślna się nie zmienia. Przy 100% Residual strength 0.99 daje 99% wartości 1.00 (naprawione w 0.3.3.2); poza
100% (także w krokach Dynamic NR oraz presetach Balanced / Performance) strength, limit i edge fade nadal działają
na cały wynik, więc wygląd może się zmienić. 0.3.4 dodaje test A/B, który pomoże znaleźć właściwą poprawkę:
Neural > Diagnostics > **Edit shaper (A/B, not saved)** z Literal, F1 i F2 oraz Only below 100% i Carry cap (tylko
danielblnc; Save Settings tego nie zapisuje; klucze w ini to `[DlssNr] AmdEditShaper`, `AmdEditShaperLimit`,
`AmdEditShaperScope` i `AmdEditShaperCarryCap`). Jeśli któryś z nich sprawia, że 85% wygląda jak 100% w Twojej
grze, daj nam znać na Discordzie ze zrzutami ekranu. lmxxf nie jest dotknięty.

**Menu otwierało się i zamykało dwa razy po jednym naciśnięciu albo klawiatura i mysz przestawały działać na całym
pulpicie, gdy menu było otwarte (Assetto Corsa)?** Naprawione w 0.3.4: drugie naciśnięcie klawisza menu lub NR w
ciągu 400 ms jest ignorowane (`[Hotfix] MenuToggleDebounceMs`, 0 = dawne zachowanie), a przy otwartym menu
niskopoziomowy hook klawiatury lub myszy gry jest pomijany, ale klawisz nadal trafia do Windows
(`[Hotfix] MenuLowLevelHookPassThrough=false` = dawne zachowanie). Jeszcze niepotwierdzone w Assetto Corsa: jeśli
to nadal się dzieje, wyślij zip z raportem.

**Menu otwierało się samo na oknie wyboru runtime'u, kliknięcia nic nie robiły albo klawisz menu ukrywał je tylko na
czas przytrzymania (Assetto Corsa)?** Naprawione w 0.3.4.2: klawisz menu otwiera lub zamyka je tylko raz na fizyczne
naciśnięcie (spóźniony komunikat klawisza jest ignorowany), kliknięcia i klawisze menu krótsze niż jedna klatka są
odtwarzane, okno wyboru runtime'u reaguje na `1` / `2` / `Enter` / `Esc` i na X na swoim pasku tytułu, a zamknięcie
menu liczy się jako "Decide later"; okno wyboru nie otwiera już menu samo z siebie. Jeszcze niepotwierdzone
przez gracza Assetto Corsa: jeśli to nadal się dzieje, wyślij zip z raportem. Aby wrócić do klawisza menu i kliknięć z
0.3.4.1: dopisz `DiagInputHooksSkip=presslatch,clickreplay` pod `[Hotfix]` w swoim `OptiScaler.ini` (bez nowego
klucza; dołączony ini opisuje tę linię tylko w komentarzu).
0.3.5 dodaje dwie rzeczy dla Assetto Corsa: AMDNR ustępuje obcemu `nvngx.dll` w folderze gry i zabezpiecza tworzenie
urządzenia D3D11On12, a gra DX11, która nigdy nie wyświetla klatek przez D3D12, dostaje startową kolejkę D3D12 dla
przebiegu neuronowego (`[DlssNr] AmdBootstrapQueue`, auto). Jeszcze niepotwierdzone przez gracza Assetto Corsa.

**Uncharted: Legacy of Thieves Collection wywalał się kilka sekund po starcie na RX 9000 z włączonym Neural
Rendering?** Naprawione w 0.3.5: gra wykonuje swoją pracę na małych fiberach o rozmiarze 192 KiB, a pierwsza
inicjalizacja HIP (sterownik kompiluje swoje kernele pomocnicze wewnątrz gry) przepełniała ten stos przy pierwszej
klatce NR. Pierwsze użycie HIP przez most neuronowy działa teraz na własnym, dużym stosie (`[DlssNr] BigStackCall`,
auto). Jeśli ustawiłeś tam `[DlssNr] Enabled=false` dla 0.3.4.2, włącz je z powrotem. Jeszcze niepotwierdzone przez
gracza na RX 9000: jeśli to nadal się dzieje, wyślij zip z raportem.

**Gra z dynamiczną rozdzielczością (The Last of Us Part II) migocze z włączonym Neural Rendering?** Naprawione
w 0.3.5: gra zmieniała swój rozmiar renderowania setki razy na minutę, a każdy krok przebudowywał sieć i resetował
jej historię. Krok, który mieści się w istniejącej alokacji, jest teraz utrzymywany (bez stabilizacji, bez
rozgrzewki, bez resetu historii, oba runtime'y); większa klatka albo prawdziwy spadek nadal powoduje ponowną
alokację. Linia statystyk w raporcie liczy utrzymane kroki. Jeszcze niepotwierdzone w tej grze.

**Generowanie klatek: brak wzrostu fps albo wieczne "restart the game"?** W niemal każdym zgłoszeniu generowanie
klatek po prostu nie było włączone - wyłączone, a nie zepsute. Wymaga pięciu kroków, a od 0.3.5 zakładka Frame Gen
i log podają ten, którego brakuje:

1. Zakładka Frame Gen: wybierz **oba**, FG Input i FG Output. Gra DX12 z własnym generowaniem klatek: jej DLSS FG
   albo FSR 3.1 FG jest wejściem (gry z FSR 3.1 FG: "FSR 3.1 FG", nie "FSR 3.0 FG"); gra w ogóle bez FG: FG Input =
   OptiFG (Upscaler), HUD fix włączony. DX11: tylko OptiFG. Vulkan: bez wyjścia FSR FG / XeFG (użyj własnego z gry).
2. **Save Settings**.
3. **Zamknij grę całkowicie i uruchom ją ponownie** - FG nie może się włączyć w trakcie sesji.
4. We własnych opcjach graficznych gry **włącz** jej generowanie klatek: DLSS Frame Generation (z DLSS jako
   upscalerem) dla wejścia DLSSG, generowanie klatek FSR (z FSR) dla wejścia FSR 3.1 FG.
5. Otwórz znowu menu, zakładka Frame Gen, zaznacz **Active** w sekcji Frame Generation. Nic nie jest generowane,
   dopóki to pole nie jest zaznaczone (XeFG może poprosić o jeszcze jeden restart).

Potem **wyłącz** w grze: tryb pełnoekranowy na wyłączność (XeFG potrzebuje okna bez ramki), V-Sync i limity klatek
(albo ustaw limit na dwukrotność bazowego fps) oraz własne generowanie klatek XeSS w grze, jeśli je ma (jeden
generator klatek na okno; gra, która ładuje własne XeSS FG, dostaje notkę w zakładce). Brak wzrostu fps = FG jest
wyłączone - log mówi `... Enabled is off ...: no frames are generated. Frame generation off, not broken.`; połowa fps
= limit albo V-Sync powstrzymuje wygenerowane klatki. Jeśli nadal nie działa, naciśnij **Save report** po wystąpieniu
problemu i wyślij zip z nazwą gry, wybraną parą, informacją, która opcja generowania klatek w grze była włączona,
okno bez ramki czy pełny ekran, HDR włączony czy wyłączony, oraz tym, co widziałeś. Pełne FAQ jest przypięte na
kanale wsparcia na Discordzie.

**Inne mody (RED4ext z Cyberpunk 2077, Cyber Engine Tweaks) albo ReShade obok AMDNR?** Od 0.3.5 raport i log podają
loadery proxy innych modów (`Mod loaders:` w `report.txt`; RED4ext to `winmm.dll`, Cyber Engine Tweaks to
`version.dll` przez loader ASI) i ostrzegają, gdy AMDNR zajmuje nazwę znanego loadera tej gry bez łańcuchowania
(unchained). AMDNR Launcher nigdy nie zabiera ani nie przenosi pliku loadera: wybiera inną nazwę proxy (Cyberpunk
2077: `dxgi.dll`), a jego Doctor podaje każdy loader, który wcześniejsze INSTALL przeniosło na bok (`AMDNR_backup`).
ReShade obok AMDNR jest wykrywany po zasobie wersji modułu albo po eksportach add-onów ReShade, nigdy po nazwie
pliku, i podawany w raporcie i logu (`[Game] ReShade detected: <module>`); nic nie jest ładowane, hookowane ani
blokowane, a współdzielenie urządzenia D3D12 przez oba jest zaprojektowane, ale jeszcze niezbudowane.

**`No HIP adapter matches D3D12 LUID` w zakładce Neural albo w `amd_bridge.log`?** Od 0.3.5 ta linia podaje obie
strony - adapter D3D12 gry (nazwa, LUID) i każde urządzenie HIP (numer porządkowy, nazwa, gfx, LUID) - oraz
prawdopodobną przyczynę z tym, co zrobić: gra działa na zintegrowanym GPU albo na innej karcie (Ustawienia Windows >
System > Ekran > Grafika: przypisz grę do karty dedykowanej), runtime HIP bez tożsamości (zabłąkany
`amdhip64_7.dll` obok gry: usuń go), brak jakiegokolwiek urządzenia HIP (zainstaluj sterownik Adrenalin od AMD),
ta sama karta pod inną tożsamością albo adapter, który nie jest od AMD. Oba runtime'y zapisują ten sam tekst.

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

**Nie możesz znaleźć ustawień Ray Regeneration?** Od 0.3.5 są na własnej zakładce **Ray Regeneration**, zaraz po
Upscaling, rysowanej, gdy Ray Regeneration działa w danym tytule (i zachowanej do końca sesji, gdy raz zadziałało);
linia **Ray Regeneration** w zakładce Neural oferuje wtedy przycisk **Open Ray Regeneration**. Gdy nie działa,
zakładka nie jest rysowana, a ta linia w zakładce Neural mówi dlaczego: gra nie włączyła Ray Reconstruction, Ray
Regeneration zrezygnowało z tej gry i dlaczego, albo ile sekund temu działało ostatni raz. Na RX 7000 kolejna
przygaszona linijka dodaje, że jest tam oferowane jako preview: denoiser AMD nie ma providera dla RDNA 3, więc gdy tylko
gra włączy Ray Reconstruction, jej obraz jest skalowany bez odszumiania, a `[FSR-RR] FfxDenoiserAllowPreRdna4=false`
(albo odznaczone pole w zakładce Upscaling **Preview: Ray Regeneration on this card (restart)**) wyłącza preview, więc
gra zostaje przy swoim denoiserze. Na RX 6000 i starszych mówi, że na tej karcie nie jest oferowane, że AMD udostępnia denoiser dla RDNA 4
i że `[FSR-RR] FfxDenoiserAllowPreRdna4=true` udostępnia je mimo to. Aby je uruchomić, w ustawieniach graficznych gry: wybierz **DLSS** jako upscaler (nie FSR, nie XeSS),
włącz **ray tracing** lub path tracing i włącz **Ray Reconstruction** (DLSS-RR); zakładka Upscaling pokaże wtedy "FSR
Ray Regeneration". Co zmienić przy którym problemie, opisuje przewodnik po ustawieniach Ray Regeneration
**RR-BEST-SETTINGS.md** (nie ma go w zipie).

**Ray Regeneration wygląda ziarniście albo szumi?** Oceniaj go najpierw z **wyłączonym Neural Rendering** (odznacz **Enable Neural Rendering** na górze zakładki Neural, naciśnij Home w trakcie gry albo ustaw
`[DlssNr] Enabled=false`): przebieg neuronowy działa po Ray Regeneration, na jego wyjściu,
więc zrzut ekranu zrobiony z włączonym NR nie mówi nic o denoiserze. Potem, zależnie od rodzaju ziarna — ziarno pełzające
w nieruchomej scenie, jasne kropki, ziarno na twarzach, smugi za poruszającymi się postaciami — ustawienia do
wypróbowania są w tym samym przewodniku, **RR-BEST-SETTINGS.md**. **W 0.3.4.2 nie zmieniła się żadna domyślna
wartość denoisera ani wyostrzania**: liczby są te z 0.3.4.1. Zmieniło się to, że wyostrzanie, które AMDNR dodaje po
Ray Regeneration, ma teraz własny klucz w ini, `[Sharpness] RrDefaultSharpness` (ta sama wartość domyślna 0.25),
więc 0.15, 0.10 albo 0 to zmiana w ini, a nie nowy build. Zajrzyj do swojego ini, zanim zaczniesz gonić ziarno
suwakiem ostrości: wartość pod `[Sharpness] Sharpness` nic nie robi, dopóki `OverrideSharpness` jest wyłączone, a zaczyna
działać w chwili, gdy zaznaczysz **Override** w menu - dlatego Image > Sharpness oznacza ją teraz
("ini Sharpness 1.00 waits for Override"). Dwie rzeczy, których nie będziemy ukrywać: część ziarna to własne
próbkowanie promieni gry — denoiser AMD nie jest zrobiony do naprawiania szumu, który przychodzi skorelowany, a gra
oferująca DLSS Ray Reconstruction wyłącza swój własny denoiser i przekazuje nam surowy sygnał — a ziarno pełzające w
nieruchomej scenie ma po naszej stronie przyczynę strukturalną, której żaden suwak nie usunie do końca. To znany
problem. 0.3.5 wyraża go liczbą: blok Diagnostics zakładki Ray Regeneration pokazuje ziarno na wejściu i wyjściu oraz
migotanie przy nieruchomej kamerze (mierzone, gdy zakładka jest otwarta), a wiersz **Denoiser backend** tej zakładki
można ustawić na Off, co mówi grze, że Ray Reconstruction nie jest obsługiwane, więc gra zachowuje własny denoiser
(po restarcie). Własny denoiser AMDNR to praca na 0.3.6.

**Ray Reconstruction w grze jest włączone, ale zakładka Neural mówi "Ray Regeneration is off in this title"?**
Gra nie udostępnia tego, czego potrzebuje FSR Ray Regeneration: jej wtyczka DLSS przekazuje puste macierze kamery
(Satisfactory), które Ray Reconstruction od NVIDIA traktuje jako opcjonalne, a FSR Ray Regeneration ich
potrzebuje. W zamian działa upscaling FSR, a NR zajmuje swoje normalne miejsce przed SR; zakładka Upscaling mówi
to samo. Od 0.3.4 w tytule Unreal z tą sygnaturą pozostaje wyłączone przez całą sesję. Wyłącz Ray Reconstruction
w grze i przywróć ustawienia denoisera silnika.

**Gra na silniku Ubisoft Anvil (AC Black Flag Resynced, Shadows, Mirage) pokazuje "DX12 Error 0x80070057"?**
Te gry mają własne XeSS Frame Generation. Od 0.3.5 zakładka Frame Gen pokazuje tam notkę z radą (zostaw własne
XeSS FG gry wyłączone, inaczej dwa generatory dzielą jedno okno), a wyjście XeFG od AMDNR nadal działa; najprostsza
droga to opcja XeSS FG w samej grze przy wyłączonym generowaniu klatek AMDNR. Jeśli błąd nadal występuje, ustaw
`[FrameGen] Enabled=false` i `[fakenvapi] ForceXeLL=false` i zgłoś problem, dołączając log.

**The Last of Us Part I crashuje przy starcie?** Przyczyną jest inicjalizacja Streamline w samej grze, znany
problem OptiScaler: zmień nazwę `sl.common.dll` w folderze gry na `sl.common.dll.bak` i wybierz
**FSR 3.1** w ustawieniach gry zamiast DLSS.

Pełne informacje o każdej wersji: `CHANGELOG.md` (w zipie i w repozytorium).

## Plan rozwoju

- **0.3.5** (ten build) — **AMDNR Anywhere** (preview, RX 9000, przez launcher); zakładka Ray Regeneration z liniami
  stanu dla każdej karty i każdego API, wierszem Denoiser backend Off i liczbą szumu; przebieg neuronowy po
  upscalingu (`AmdPlacement`); zakładka Frame Gen i log mówią, dlaczego nic nie jest generowane; lmxxf 0.37
  autorstwa Kiena (MIT) na RX 9000; zestaw modułów 0.3.5
  (około 10 procent szybciej na RX 7000 i handheldach klasy Z1 Extreme, ten sam obraz); kroki dynamicznej
  rozdzielczości utrzymywane bez przebudowy sieci; poprawka przenoszenia na handheldach; wspólny folder runtime'u
  (`AmdRuntimePath`); odmowa adaptera HIP, która podaje obie strony; loadery innych modów i ReShade podawane
  w raporcie; procesy anti-cheatów i programów raportujących crashe pozostawione w spokoju; poprawki dla Uncharted
  (RX 9000), Assetto Corsa, F1 25 i Kingdom Come: Deliverance II (Game Pass), Control Resonant z generowaniem klatek,
  Tainted Grail: The Fall of Avalon i Dead Space, GTA V Enhanced, Half-Life 2 RTX i innych tytułów Vulkan oraz
  teksty o Linux / Proton; AMDNR Launcher 0.3.5.1.
- **0.3.4.2** — hotfix: menu w Assetto Corsa (klawisz menu otwiera lub zamyka je tylko raz na naciśnięcie,
  kliknięcia krótsze niż jedna klatka są odtwarzane, okno wyboru runtime'u reaguje na klawisze, a zamknięcie menu liczy
  się jako "Decide later"), okno wyboru runtime'u nie otwiera już menu samo z siebie, sekcja Ray Regeneration jest w
  zakładce Neural zawsze i mówi, dlaczego nie działa, o jeden przyjęty układ runtime'u danielblnc więcej,
  poprawki tekstu o Wine / Proton,
  uzupełnienia README (nazwy proxy, Uncharted, PC hybrydowe, `AmdLmxxfTierSnap` na RX 9000, dwa wpisy FAQ o Ray
  Regeneration); NR identyczny bajt w bajt
  z 0.3.4.1 poza tym jednym wierszem przyjętego układu.
- **0.3.4.1** — hotfix: wyostrzanie po Ray Regeneration, gdy gra nie przekazuje własnego (Windows;
  domyślnie żadnego na Linux / Proton), koniec z fałszywym komunikatem "Upscaler failed to run!" w Control
  Resonant, menu na Linux / Proton dołączane do okna gry, Save report podaje vkd3d-proton / DXVK; AMDNR Launcher
  0.3.4.1 (dziewięć języków, wyszukiwanie, ulubione, ukrywanie, zmiana nazwy, CHOOSE GAME .EXE, PLAY, pełne
  UNINSTALL); NR bez zmian.
- **0.3.4** — nowe menu (przebudowana zakładka Neural, ten sam wygląd wszystkich zakładek, Save
  report); lmxxf szybszy na RX 7000 (domyślnie poziomy rozmiaru sieci) oraz na RX 9070 / 9070 XT
  (kernele lmxxf 0.31); lmxxf na APU w handheldach (eksperymentalnie; nowe rozmiary sieci 360p i
  576p); lmxxf zyskuje Network output, Encoding, Residual edge fade, natywną maskę postaci i opcjonalny Fast mode; AMDNR Screen GI (preview);
  ustawienia runtime'u danielblnc (Network style, Tone curve, Black lift, Game exposure) i ochrona koloru jasnych
  partii; strojenie i diagnostyka Ray Regeneration; poprawki wejścia menu w Assetto Corsa, Shadow of the Tomb Raider, Marvel's Midnight Suns i The Last of Us Part II,
  czystego wyjścia i logów.
- **0.3.3.x** — lmxxf na RDNA 3 (RX 7000; własny backend AMDNR); kompozycja koloru RenoDX
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
- **0.3.6** — RX 6000 (RDNA 2): własne kernele AMDNR za bramką sprzętową, z poziomem 360 i Model interleave jako
  preview na Navi 21; preview denoisera AMDNR (ARD), własnego denoisera AMDNR za wywołaniem Ray Reconstruction, dla
  kart, na których denoiser AMD odmawia działania; AMDNR Anywhere na RX 7000, gdy zostanie tam przetestowane.
- **0.4.0** — poziom sieci 1440p; Ray Regeneration w tytułach Vulkan przez most; Neural Rendering między adapterami
  (gra na jednej karcie, sieć na karcie AMD); Anywhere poza fazą preview (tytuły bez własnego upscalera, w których
  OptiScaler dostarcza jednocześnie upscaler i przebieg neuronowy).
- **Później** — AMDNR w dowolnym oknie (pulpit).

---

## Podziękowania

Ten build to połączenie w całość pracy innych ludzi. Jeśli uważasz go za przydatny,
podziękowania należą się autorom projektów źródłowych (upstream).

- **TheAutomatic** — DLSS 5 AMD project — https://github.com/TheAutomatic/dlss-5-amd-project
- **danielblnc** — DLSS-NR on AMD by Daniel Blanco — https://github.com/danielblnc/DLSS-NR-on-AMD (pliki `*Runtime.zip`, bez modyfikacji)
- **lmxxf** (Kien) — https://github.com/lmxxf/dlss5-on-amd-9070xt-porting (port sieci, kernele i runtime HIP, MIT)
- **TheAutomatic** — `LmxxfNrRuntime.cpp`, `LmxxfNrApi.h`, `LmxxfProductionOptions.h`: portions contributed to lmxxf by TheAutomatic (MIT)
- **kernele lmxxf 0.31** w `LmxxfNrRuntime.pak` (the ViT projection (lmxxf031-vit-wide-deep), the C512 QKV and mix kernels (lmxxf031-c512-m32-mh, lmxxf031-c512-m32-deep) and one-wave-per-head attention (lmxxf031-c64-wave2)) — autorstwa lmxxf (Kien, MIT), zbudowane przez AMDNR ze źródeł i przepisu budowania lmxxf; część AMDNR to ładowanie, przypięte sumy SHA-256, filtrowanie według GPU i rozwiązania zapasowe
- **kernele lmxxf 0.37 i 0.39** w `LmxxfNrRuntime.pak` (moduły lmxxf037-* i lmxxf039-* dla RX 9000) i ich kod uruchamiający w `LmxxfNrRuntime.dll` — lmxxf 0.37 / 0.39 by Kien (MIT), dostarczane w takiej postaci, w jakiej zbudował je lmxxf; część AMDNR to ładowanie jako jedna przypięta grupa, przypięte sumy SHA-256, ustawienia domyślne według GPU, wyłączniki i rozwiązania zapasowe
- **c32w kernels** (0.3.3.2) — własne jednofalowe (one-wave) kernele AMDNR dla RDNA 4, przeznaczone dla sieci lmxxf, Copyright (c) 2026 3zwr1 (AMDNR); pomysły z publicznej dokumentacji AMD dotyczącej RDNA 4 WMMA (GPUOpen, ROCm matrix instruction calculator)
- **Backend RDNA 3 od AMDNR** (0.3.3; buildy dla handheldów gfx1103 / gfx1150 w 0.3.4), polityka poziomów rozmiaru sieci i małe rozmiary sieci (0.3.4) — Copyright (c) 2026 3zwr1 (AMDNR)
- **Matheus / dlss-5-amd** — https://github.com/MatheusGViana/dlss-5-amd-project
- **Dagherbou / OptiScaler_DLSSNR** — https://github.com/Dagherbou/OptiScaler_DLSSNR
- **wilsjo2 / OptiScaler-DLSSNR-PreSR-Multipass** — https://github.com/wilsjo2/OptiScaler-DLSSNR-PreSR-Multipass
- **Nukem9** — dlssg-to-fsr3 — https://github.com/Nukem9/dlssg-to-fsr3 (GPLv3, bez modyfikacji)
- **Magpie by Blinue, experimental fork by SAOG0721 (GPL-3.0), fetched from the author, not redistributed by AMDNR** — host przechwytujący okno, wewnątrz którego działa AMDNR Anywhere — https://github.com/Blinue/Magpie (fork: https://github.com/SAOG0721/Magpie)
- **RenoDX** — clshortfuse — https://github.com/clshortfuse/renodx (matematyka kompozycji koloru, MIT)
- **Coldwood1026** — XeFGUnlock (GPL-3.0), podstawa wbudowanego odblokowania generowania wielu klatek XeFG i jego frame pacingu
- **Zach Hembree (DarkHelmet)** — FSR Ray Regeneration dla OptiScalera, źródło ścieżki Ray Regeneration w AMDNR, kontynuowane przez **burak113**, z którego gałęzi AMDNR wykonał port (gałąź OptiScaler ffx-denoise-experimental, GPL-3.0)
- **Screen-space GI** (odziedziczony efekt; wycofany z menu w 0.3.4, `[AmdRtgi] Enabled` w ini) — efekt, który AMDNR odziedziczył z linii OptiScaler-AMD-PreSR; zasługa należy do jego pierwotnych autorów. Wymaga folderu `experimental_lighting` z paczki danielblnc, której AMDNR nie dostarcza.
- **AMDNR Screen GI** (preview 0.3.4) — własna praca AMDNR, Copyright (c) 2026 3zwr1 (AMDNR), napisana na podstawie opublikowanych prac (Therrien, Levesque i Gilet 2023; Jimenez i in. 2016; Schied i in. 2017; oraz pozostałe wymienione w `CHANGELOG.md` i `Licenses/AMDNR_NOTICE.txt`)
- **OptiScaler** — Overclockers — https://github.com/Overclockers/OptiScaler-Releases

## AMDNR Launcher

**AMDNR Launcher** (nowość w 0.3.4) to program dla Windows 10 / 11, który może też działać pod Protonem na Linuksie
(eksperymentalnie, jeszcze tego nie testowaliśmy: patrz "Linux / Proton"). Instaluje i aktualizuje AMDNR osobno dla
każdej gry: znajduje Twoje gry (Steam, Epic, aplikacja Xbox, Ubisoft Connect, aplikacja EA, GOG, Rockstar,
Battle.net i Amazon Games), dobiera nazwę DLL, pobiera build i wybrany przez Ciebie runtime danielblnc, sprawdza
każdą instalację swoim narzędziem Doctor, uruchamia AMDNR Anywhere dla gier bez własnego upscalera (PLAY ANYWHERE)
i sam się aktualizuje.
Pobierz `AMDNR-Launcher.exe`, czyli AMDNR Launcher z najnowszego wydania, ze strony wydań:
<https://github.com/3zwr1/AMD-NR---OptiScaler/releases>

**Nowość w Launcherze 0.3.5.1** (w wydaniu Alpha0.3.5; najpierw aktualizuje sam siebie, potem Twoje gry):

- **PLAY ANYWHERE** przy grze bez własnego upscalera (patrz "AMDNR Anywhere"): launcher pobiera host przechwytujący
  z wydania jego autora, uruchamia grę i uruchamia Neural Rendering na jej oknie, z podsumowaniem ustawień hosta
  tylko do odczytu obok przycisku (same ustawienia są w zakładce Anywhere w menu). W tym wydaniu RX 9000. Gry, do
  których mod nie może się załadować (32-bit, DirectX 9 / OpenGL bez upscalera), są odrzucane przy INSTALL z jasnym
  komunikatem i dostają propozycję Anywhere.
- **UPDATE ALL** i aktualizacja przy starcie; mirrory paczek; RETRY / OPEN DOWNLOAD / IMPORT PACKAGE, gdy pobieranie
  się nie powiedzie.
- **COLLECT LOGS** włącza obsługę crashy na jedno uruchomienie i dołącza `amdnr_crash.log`, najnowszy zrzut awarii
  i `dlssnr_on_amd.ini` od danielblnc.
- **PLAY** uruchamia gry z Xbox / Microsoft Store po ich identyfikatorze aplikacji (app id).
- **Jeden przycisk PLAY** z wyborem ścieżki (własny upscaler gry albo AMDNR Anywhere, zapamiętywany osobno dla
  każdej gry), a na stronie każdej gry także API graficzne (DX9 / 10 / 11 / 12 / Vulkan / OpenGL).
- **Dwanaście języków**: turecki, koreański i węgierski dołączają do dziewięciu wymienionych niżej.
- **Gry Rockstara uruchamiają się przez ich sklep** (Steam, Epic albo Rockstar Games Launcher), nigdy przez ich exe.
- **AMDNR Anywhere**: gra działa z priorytetem GPU niższym niż normalny, żeby host i Neural Rendering miały
  pierwszeństwo; ikona hosta przechwytującego w zasobniku systemowym jest ukryta.
- **Resident Evil 2 / 3 / 4 (2023) / Village**: informacja o konfiguracji (wymagają pluginu upscalera od PureDark
  z REFramework). Więcej gier bez DLSS, XeSS ani FSR 2+ jest oznaczanych jako niewspierane, z podaniem powodu,
  jeszcze przed jakimkolwiek pobieraniem.
- **FSR 4 (INT8)** jako eksperymentalna opcja do włączenia na własne życzenie na stronie gry na handheldach RDNA 3.
- **Loadery innych modów nigdy nie są zabierane ani przenoszone** (RED4ext, Cyber Engine Tweaks i podobne): launcher
  wybiera inną nazwę proxy, a jego Doctor podaje każdy loader, który wcześniejsze INSTALL przeniosło na bok.
- **GTA V Enhanced**: strona gry zawiera linię ścieżki (FSR 3.1 wybrany w grze jest wejściem; Neural Rendering
  działa przed nim) i notkę o `settings.xml`.

**Nowość w Launcherze 0.3.4.1: prosiliście, więc zbudowaliśmy** (na podstawie pierwszych opinii z Discorda):

- Dziewięć języków (dwanaście od 0.3.5.1): angielski, arabski, chiński (uproszczony), francuski, hiszpański,
  portugalski, włoski, rosyjski i polski, a także turecki, koreański i węgierski. Launcher używa języka Twojego Windows (w przeciwnym razie angielskiego); inny wybierzesz w LANGUAGE
  (JĘZYK) albo w SETTINGS (USTAWIENIA), gdzie wybór języka pojawia się też przy pierwszym uruchomieniu. Wyniki
  narzędzia Doctor, komunikaty instalacji i raport z COLLECT LOGS (ZBIERZ LOGI) zostają po angielsku, żeby osoby
  udzielające pomocy mogły je przeczytać.
- Wyszukiwanie w bibliotece; ulubione (gwiazdka, a gry z gwiazdką są na początku); ukrywanie gier (HIDDEN pokazuje je
  z powrotem); zmiana nazwy gry.
- CHOOSE GAME .EXE: sam wskaż plik exe gry, gdy launcher wybrał zły albo nie znalazł żadnego. Cyberpunk 2077 i
  The Witcher 3 (REDengine) są teraz znajdowane we właściwym folderze bez tego.
- PLAY i OPEN FOLDER na stronie każdej gry; STORES włącza i wyłącza całe sklepy; foldery dodane ręcznie zostają na
  liście, także gdy ich dysk jest odłączony, dopóki ich nie usuniesz.
- UNINSTALL najpierw pyta i usuwa wszystko, co umieścił mod, łącznie z tym, co zapisał podczas działania gry (logi,
  cache, zrzuty awarii, niedokończone raporty); zostawia gotowe zipy z Save report oraz DLL o nazwie proxy, który
  nie jest już OptiScalerem (własny plik gry).
- COLLECT LOGS dla dowolnej gry, zainstalowanej lub nie, z raportem ze skanowania.
- Handheldy i APU (ROG Ally Z1 Extreme i inne APU Ryzen) są rozpoznawane, z notką, że AMDNR jest na nich nadal
  w fazie testów.
- Linux (eksperymentalnie, jeszcze tego nie testowaliśmy): ten sam exe dla Windows może działać pod Protonem,
  pokazuje tam komunikat z informacją, co zrobić, i szuka też gier w Twojej linuksowej bibliotece Steam; kroki
  w "Linux / Proton". Daj nam znać na Discordzie, czy działa.

Jego kod źródłowy jest w `Launcher/OpenSource/` repozytorium GitHub tego projektu, z własną licencją,
`Launcher/OpenSource/LICENSE.txt`. **Nie** obejmuje go licencja GPL-3.0 (`LICENSE`) tego repozytorium: kod jest
udostępniony do wglądu (source-available), wszelkie prawa zastrzeżone, Copyright (c) 2026 3zwr1 (AMDNR). Manifest
launchera to `Launcher/manifest.json`. Zobacz też sekcję 7 pliku `Licenses/AMDNR_NOTICE.txt`.

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
