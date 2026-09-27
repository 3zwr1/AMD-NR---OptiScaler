// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Collections.Concurrent;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Xml;
using System.Xml.Linq;

namespace AmdnrLauncher.Core.Scanning;

/// <summary>The kinds of picture a game's page is made of. Tile is the tall box art in the rail,
/// Hero the wide key art behind the stage, Logo the transparent wordmark drawn over it, Blur the
/// small pre-blurred hero the window is filled with. Header is Steam's 460×215 capsule: not shown
/// as itself, but it stands in for a missing tile or hero, as it always has.</summary>
public enum ArtKind { Tile, Hero, Header, Logo, Blur }

/// <summary>The pictures found for one game, each a file on disk or null. <paramref name="TileIsSquare"/>
/// says the tile is a square logo or an icon rather than 2:3 box art, so the view letterboxes it
/// on the coloured plate instead of cropping it to a spine.</summary>
public sealed record GameArt(string? Tile, bool TileIsSquare, string? Hero, string? Logo, string? Blur)
{
    public static readonly GameArt None = new(null, false, null, null, null);
}

/// <summary>One look for a game's pictures on this PC, and what the CDN is worth asking for
/// afterwards. <paramref name="Art"/> is what to show now. <paramref name="WantedFromCdn"/> names
/// the CDN's files for the kinds no step on this PC yielded as themselves: a header standing in
/// for a tile or a hero, or an icon standing in for a tile, leaves that kind wanted, while a
/// picture Steam's own cache holds is never asked for again. Empty when there is nothing the CDN
/// could add, when there is no fetcher, when the game has no Steam appid, or when this run has
/// asked for this game already.</summary>
public sealed record ArtLookup(GameArt Art, IReadOnlyList<string> WantedFromCdn)
{
    public bool NeedsFetch => WantedFromCdn.Count > 0;
}

/// <summary>What became of one picture asked of the CDN. Missing is a 404, which is remembered
/// so the same absent logo is not asked for at every start; Failed is anything else — no
/// network, a 5xx, a body that was not a picture — and is asked again next time.</summary>
public enum ArtFetch { Saved, Missing, Failed }

/// <summary>Where a picture comes from when it is not on this PC: the CDN in the app, a stub in
/// a test. Never throws; a fetch that could not be made is Failed.</summary>
public interface IArtFetcher
{
    /// <summary>Saves the picture at <paramref name="url"/> to <paramref name="destinationPath"/>,
    /// through a .part file, so a reader never sees half of one.</summary>
    Task<ArtFetch> FetchAsync(Uri url, string destinationPath, CancellationToken ct);
}

/// <summary>Finds a cover for every game — literally all of them. For each kind of picture the
/// first step that yields a file wins:
/// <list type="number">
/// <item>Steam's own cache, by file name, in both of the layouts the client has used.</item>
/// <item>Steam's cache under the content-hash names newer clients write, told apart by the
/// size in each file's header — nothing is decoded.</item>
/// <item>Steam's CDN, asked once per game for the kinds the steps before did not yield, and kept
/// under the launcher's own art folder; a 404 is remembered for a week. Only <see cref="FetchAsync"/>
/// reaches the network, and only the caller decides when — <see cref="Lookup"/> reads the cache,
/// says what is still wanted, and nothing else.</item>
/// <item>The Xbox app's ShellVisuals: a square logo as the tile, the splash screen as the hero.</item>
/// <item>The game exe's own icon, as a PNG the launcher writes once and keeps.</item>
/// </list>
/// Then the coloured plate with the initial, which is the view's and needs no file. Nothing in
/// here throws: a picture that cannot be found, read or written is a picture that is not shown.</summary>
public sealed class Artwork(string cacheRoot, string? steamRoot, IArtFetcher? fetcher = null)
{
    /// <summary>The pictures Steam's CDN serves for an appid, in the order they are asked for.
    /// header.jpg is asked for only when library_hero.jpg was not there, to stand in for it.</summary>
    public static readonly IReadOnlyList<string> CdnFiles = ["library_600x900.jpg", "library_hero.jpg", "logo.png", "header.jpg"];

    /// <summary>How long a 404 is believed before the CDN is asked again.</summary>
    public static readonly TimeSpan RememberMissingFor = TimeSpan.FromDays(7);

    private const string SteamTile = "library_600x900.jpg";
    private const string SteamHero = "library_hero.jpg";
    private const string SteamLogo = "logo.png";
    private const string SteamHeader = "header.jpg";
    private const string SteamBlur = "library_hero_blur.jpg";

    /// <summary>The widest a Steam logo.png is; the 1920-wide pictures with alpha are heroes
    /// exported as PNG, not wordmarks.</summary>
    private const int MaxLogoWidth = 1280;

    /// <summary>Narrower than this with alpha is an icon, not a wordmark. The rule as first written
    /// stopped at "a PNG with alpha and width ≤ 1280"; this lower bound is added on purpose, for the 32×32
    /// and 64×64 icons the client keeps under the same kind of hash name beside the art — a
    /// wordmark that narrow would be unreadable on the stage anyway.</summary>
    private const int MinLogoWidth = 100;

    /// <summary>A game's cache folder holds a handful of files; reading the headers of more than
    /// this is a folder that is not what it seems.</summary>
    private const int MaxHashedFiles = 64;

    /// <summary>The launcher stub at a game's root is one exe among few; a root with more is a
    /// tools folder — redistributables, crash handlers, a setup — and only an exe named for the
    /// game may lend an icon from it.</summary>
    private const int MaxRootExes = 4;

    /// <summary>The CDN is asked from many rows at once when a library is first seen; four at a
    /// time is quick without being a burst.</summary>
    private readonly SemaphoreSlim _fetches = new(4);

    /// <summary>Appids asked of the CDN in this run, whatever the answer: once means once.</summary>
    private readonly ConcurrentDictionary<string, byte> _asked = new(StringComparer.Ordinal);

    public static string DefaultCacheRoot => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "AMDNR", "art");

    public static Uri CdnUrl(string appId, string fileName)
        => new($"https://cdn.cloudflare.steamstatic.com/steam/apps/{appId}/{fileName}");

    public string CdnCachePath(string appId, string fileName) => Path.Combine(cacheRoot, "steam", appId, fileName);

    public string MissingMarkerPath(string appId, string fileName) => CdnCachePath(appId, fileName) + ".missing";

    /// <summary>What a Steam picture is, from its size alone, for the files the client stores
    /// under content-hash names. The sizes are Steam's own: the 600×900 capsule (cached at
    /// 300×450 by the current client), the 1920×620 hero and its 3840×1240 original, the 460×215
    /// header and its 920×430 original, the 192×62 pre-blurred hero. A PNG with alpha that is
    /// none of those, and wordmark-sized, is the logo. Anything else — the 32×32 icon the client
    /// keeps beside them, most of all — is nothing to show.</summary>
    public static ArtKind? Classify(ImageInfo info) => (info.Width, info.Height) switch
    {
        (600, 900) or (300, 450) => ArtKind.Tile,
        (1920, 620) or (3840, 1240) => ArtKind.Hero,
        (460, 215) or (920, 430) => ArtKind.Header,
        (192, 62) or (384, 124) => ArtKind.Blur,
        _ when info is { Format: ImageFormat.Png, HasAlpha: true, Width: >= MinLogoWidth and <= MaxLogoWidth }
            => ArtKind.Logo,
        _ => null,
    };

    /// <summary>Everything on this PC for the game, in the order above, reading the CDN's cache
    /// but never the CDN — and, beside it, what the CDN would be asked for: the kinds none of
    /// this yielded. Cheap enough for every row of a library, and safe from any thread; the
    /// marker and cache checks that decide the CDN's part run here, with the rest, so a caller on
    /// a UI thread has nothing left to look up.</summary>
    public ArtLookup Lookup(GameCandidate candidate, string? exeDirectory, string? exeName)
    {
        var found = new Found();
        var appId = AppIdOf(candidate);

        if (appId is not null)
        {
            TakeSteamNamed(found, appId);
            TakeSteamHashed(found, appId);
            TakeCdnCached(found, appId);
        }

        if (candidate.Store == GameStore.Xbox)
        {
            var xbox = XboxArtwork.Read(candidate.InstallRoot);
            found.TakeSquare(xbox.Tile);
            found.Take(ArtKind.Hero, xbox.Hero);
        }

        if (found.Tile is null && found.Header is null && found.Square is null)
            found.TakeSquare(IconTile(candidate, exeDirectory, exeName));

        var wanted = appId is not null && fetcher is not null && !_asked.ContainsKey(appId)
            ? Wanted(appId, found)
            : [];

        return new ArtLookup(found.ToGameArt(), wanted);
    }

    /// <summary>The pictures alone: for a caller that has asked the CDN already, or never will.</summary>
    public GameArt Resolve(GameCandidate candidate, string? exeDirectory, string? exeName)
        => Lookup(candidate, exeDirectory, exeName).Art;

    /// <summary>Asks the CDN, once in this run, for what <paramref name="lookup"/> found wanting,
    /// and says whether any of it landed — when it did, <see cref="Resolve"/> will now find it.
    /// Nothing is asked for a game whose pictures were all on this PC. Off whatever thread the
    /// caller is on; nothing waits for this. Never throws.</summary>
    public async Task<bool> FetchAsync(GameCandidate candidate, ArtLookup lookup, CancellationToken ct)
    {
        if (fetcher is null || !lookup.NeedsFetch || AppIdOf(candidate) is not { } appId) return false;
        if (!_asked.TryAdd(appId, 0)) return false;

        var landed = false;
        var heroLanded = false;
        foreach (var name in lookup.WantedFromCdn)
        {
            // Only the CDN's own file names are ever joined onto its address.
            if (!CdnFiles.Contains(name)) continue;

            // The header only stands in for a hero; with the hero here, it is not wanted after all.
            if (name == SteamHeader && heroLanded) continue;

            var result = await FetchOneAsync(appId, name, ct);
            if (result == ArtFetch.Saved)
            {
                landed = true;
                if (name == SteamHero) heroLanded = true;
            }
        }

        return landed;
    }

    /// <summary>The CDN's files for the kinds nothing on this PC yielded — not Steam's cache under
    /// either of its names, not the CDN's own cache — less those remembered as missing, in the
    /// order they are asked for. A header standing in for the tile and the hero leaves both
    /// wanted: the cover and the key art are better than the capsule cropped to either. The
    /// header itself is wanted only while there is neither a hero nor a header for it to stand
    /// in for. An icon standing in for the tile leaves the tile wanted too.</summary>
    private List<string> Wanted(string appId, Found found)
    {
        var wanted = new List<string>();
        foreach (var name in CdnFiles)
        {
            var have = name switch
            {
                SteamTile => found.Tile is not null,
                SteamHero => found.Hero is not null,
                SteamLogo => found.Logo is not null,
                SteamHeader => found.Hero is not null || found.Header is not null,
                _ => true,
            };
            if (have || IsRememberedMissing(appId, name)) continue;

            wanted.Add(name);
        }

        return wanted;
    }

    private async Task<ArtFetch> FetchOneAsync(string appId, string name, CancellationToken ct)
    {
        var destination = CdnCachePath(appId, name);
        ArtFetch result;

        await _fetches.WaitAsync(ct).ConfigureAwait(false);
        try
        {
            result = await fetcher!.FetchAsync(CdnUrl(appId, name), destination, ct).ConfigureAwait(false);
        }
        finally
        {
            _fetches.Release();
        }

        try
        {
            switch (result)
            {
                case ArtFetch.Saved when ImageHeader.Read(destination) is null:
                    // A captive portal's login page, or an error page served with a 200: not a
                    // picture, not kept, and not remembered as missing either — next start asks again.
                    File.Delete(destination);
                    return ArtFetch.Failed;

                case ArtFetch.Saved:
                    // A marker left from before the picture existed would only expire on its own.
                    File.Delete(MissingMarkerPath(appId, name));
                    return ArtFetch.Saved;

                case ArtFetch.Missing:
                    var marker = MissingMarkerPath(appId, name);
                    Directory.CreateDirectory(Path.GetDirectoryName(marker)!);
                    File.WriteAllText(marker, DateTime.UtcNow.ToString("o"));
                    return ArtFetch.Missing;

                default:
                    return ArtFetch.Failed;
            }
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            // The art folder is the launcher's own; if it cannot be written, there is no art to
            // remember and nothing worth telling the user about.
            return ArtFetch.Failed;
        }
    }

    private bool IsRememberedMissing(string appId, string name)
    {
        try
        {
            var marker = MissingMarkerPath(appId, name);
            if (!File.Exists(marker)) return false;

            return DateTime.UtcNow - File.GetLastWriteTimeUtc(marker) < RememberMissingFor;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return false;
        }
    }

    /// <summary>Step 1: the files Steam names, in either of its layouts. Each kind by its own
    /// name, so a header found here stands in only after every better source has been tried.</summary>
    private void TakeSteamNamed(Found found, string appId)
    {
        found.Take(ArtKind.Tile, SteamArtwork.Find(steamRoot, appId, SteamTile));
        found.Take(ArtKind.Hero, SteamArtwork.Find(steamRoot, appId, SteamHero));
        found.Take(ArtKind.Logo, SteamArtwork.Find(steamRoot, appId, SteamLogo));
        found.Take(ArtKind.Blur, SteamArtwork.Find(steamRoot, appId, SteamBlur));
        found.Take(ArtKind.Header, SteamArtwork.Find(steamRoot, appId, SteamHeader, "library_header.jpg"));
    }

    /// <summary>Step 2: everything else in the game's cache folder — hash-named files at the top,
    /// files inside hash-named folders, hash-named files in the old flat layout — each told apart
    /// by the size in its header. Of two of a kind, the larger.</summary>
    private void TakeSteamHashed(Found found, string appId)
    {
        if (string.IsNullOrEmpty(steamRoot)) return;

        var best = new Dictionary<ArtKind, (string Path, int Width)>();
        try
        {
            var cache = Path.Combine(steamRoot, "appcache", "librarycache");
            var folder = Path.Combine(cache, appId);

            IEnumerable<string> files = [];
            if (Directory.Exists(folder))
            {
                files = files
                    .Concat(Directory.EnumerateFiles(folder))
                    .Concat(Directory.EnumerateDirectories(folder).SelectMany(EnumerateFilesSafely));
            }
            if (Directory.Exists(cache))
                files = files.Concat(Directory.EnumerateFiles(cache, appId + "_*"));

            foreach (var file in files.Where(IsPictureName).Take(MaxHashedFiles))
            {
                if (ImageHeader.Read(file) is not { } info || Classify(info) is not { } kind) continue;
                if (!best.TryGetValue(kind, out var current) || info.Width > current.Width)
                    best[kind] = (file, info.Width);
            }
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            // An unreadable art cache is not a reason to lose the game, or the pictures found so far.
        }

        foreach (var (kind, (path, _)) in best) found.Take(kind, path);
    }

    /// <summary>Step 3's local half: what earlier runs fetched from the CDN.</summary>
    private void TakeCdnCached(Found found, string appId)
    {
        found.Take(ArtKind.Tile, Existing(CdnCachePath(appId, SteamTile)));
        found.Take(ArtKind.Hero, Existing(CdnCachePath(appId, SteamHero)));
        found.Take(ArtKind.Logo, Existing(CdnCachePath(appId, SteamLogo)));
        found.Take(ArtKind.Header, Existing(CdnCachePath(appId, SteamHeader)));
    }

    /// <summary>Step 5: the icon of the game's exe, or failing that of an exe at the game's root —
    /// the stub Unreal games start from, which carries the icon when the shipping exe under
    /// Binaries does not. Written once per exe and build (the key holds the exe's write time), read
    /// from the art folder after that.</summary>
    private string? IconTile(GameCandidate candidate, string? exeDirectory, string? exeName)
    {
        var exes = new List<string>();
        if (exeDirectory is not null && exeName is not null) exes.Add(Path.Combine(exeDirectory, exeName));
        exes.AddRange(RootExes(candidate));

        foreach (var exe in exes.Distinct(StringComparer.OrdinalIgnoreCase))
        {
            if (IconPathFor(exe) is not { } png) continue;
            if (Exists(png) || ExeIcon.ExtractPng(exe, png)) return png;
        }

        return null;
    }

    /// <summary>The exes at the game's root worth asking for an icon, best first: the one the
    /// store names and the one called what the folder is called, then the rest by name — Spider-
    /// Man's root holds crs-handler.exe beside the game, and alphabetically the crash handler
    /// comes first. A root crowded with exes is a tools folder, and only an exe named for the
    /// game is asked there. The root exes are a second chance, not a requirement: a root that
    /// cannot be read lends nothing.</summary>
    private static IEnumerable<string> RootExes(GameCandidate candidate)
    {
        try
        {
            if (!Directory.Exists(candidate.InstallRoot)) return [];

            var all = Directory
                .EnumerateFiles(candidate.InstallRoot, "*.exe", new EnumerationOptions { IgnoreInaccessible = true })
                .OrderBy(Path.GetFileName, StringComparer.OrdinalIgnoreCase)
                .ToList();
            var named = all.Where(exe => IsNamedForTheGame(candidate, exe)).ToList();

            return all.Count > MaxRootExes ? named : named.Concat(all);
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or ArgumentException)
        {
            return [];
        }
    }

    /// <summary>The exe the store itself names — the Xbox config's executable — or one called
    /// what the folder is called, spaces aside: Stray\Stray.exe is, SILENT HILL 2\SHProto.exe is not.</summary>
    private static bool IsNamedForTheGame(GameCandidate candidate, string exePath)
    {
        var fileName = Path.GetFileName(exePath);
        if (candidate.ExeHint is { Length: > 0 } hint
            && string.Equals(Path.GetFileName(hint), fileName, StringComparison.OrdinalIgnoreCase))
            return true;

        var stem = Path.GetFileNameWithoutExtension(exePath);
        var folder = Path.GetFileName(Path.TrimEndingDirectorySeparator(candidate.InstallRoot));
        return string.Equals(stem, folder, StringComparison.OrdinalIgnoreCase)
               || string.Equals(stem, folder.Replace(" ", ""), StringComparison.OrdinalIgnoreCase);
    }

    /// <summary>Where the exe's icon is kept: under the art folder, named by a hash of the exe's
    /// path, size and write time, so a game update — a new exe — gets a new picture and the old
    /// one is simply never read again. Null when the exe cannot be described at all.</summary>
    private string? IconPathFor(string exePath)
    {
        try
        {
            var info = new FileInfo(exePath);
            if (!info.Exists) return null;

            var key = $"{info.FullName.ToUpperInvariant()}|{info.Length}|{info.LastWriteTimeUtc.Ticks}|{ExeIcon.DefaultSize}";
            var hash = Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(key)))[..32].ToLowerInvariant();
            return Path.Combine(cacheRoot, "exe", hash + ".png");
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or ArgumentException
                                    or NotSupportedException or System.Security.SecurityException)
        {
            return null;
        }
    }

    /// <summary>A Steam appid is a number, and nothing else may be joined onto a path or an
    /// address: another store's id comes from a config file anyone can write.</summary>
    private static string? AppIdOf(GameCandidate candidate)
        => candidate.Store == GameStore.Steam
           && candidate.StoreId is { Length: > 0 } id
           && id.All(char.IsAsciiDigit)
            ? id
            : null;

    private static bool IsPictureName(string path)
        => Path.GetExtension(path) is { } extension
           && (extension.Equals(".jpg", StringComparison.OrdinalIgnoreCase)
               || extension.Equals(".jpeg", StringComparison.OrdinalIgnoreCase)
               || extension.Equals(".png", StringComparison.OrdinalIgnoreCase));

    private static IEnumerable<string> EnumerateFilesSafely(string folder)
    {
        try
        {
            return Directory.EnumerateFiles(folder).ToList();
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return [];
        }
    }

    private static string? Existing(string path) => Exists(path) ? path : null;

    private static bool Exists(string path)
    {
        try
        {
            return File.Exists(path);
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return false;
        }
    }

    /// <summary>The first file found for each kind, in the order the steps run. A square tile —
    /// an Xbox logo or an icon — is kept apart from box art, and ranks after the header, which is
    /// at least the game's own picture.</summary>
    private sealed class Found
    {
        public string? Tile;
        public string? Hero;
        public string? Header;
        public string? Logo;
        public string? Blur;
        public string? Square;

        public void Take(ArtKind kind, string? path)
        {
            if (path is null) return;

            switch (kind)
            {
                case ArtKind.Tile: Tile ??= path; break;
                case ArtKind.Hero: Hero ??= path; break;
                case ArtKind.Header: Header ??= path; break;
                case ArtKind.Logo: Logo ??= path; break;
                case ArtKind.Blur: Blur ??= path; break;
            }
        }

        public void TakeSquare(string? path)
        {
            if (path is not null) Square ??= path;
        }

        public GameArt ToGameArt()
        {
            var tile = Tile ?? Header ?? Square;
            return tile is null && Hero is null && Header is null && Logo is null && Blur is null
                ? GameArt.None
                : new GameArt(tile, TileIsSquare: tile is not null && Tile is null && Header is null, Hero ?? Header, Logo, Blur);
        }
    }
}

/// <summary>The default fetcher: Steam's CDN over the launcher's HttpClient. Saves through a
/// .part file, refuses anything larger than a piece of art could be, gives up on a connection
/// that stalls — the client's own timeout is 100 s, and a picture that never comes would hold one
/// of the four fetch slots for that long — and never throws.</summary>
public sealed class HttpArtFetcher(HttpClient client, TimeSpan? perFileTimeout = null) : IArtFetcher
{
    /// <summary>Larger than any picture Steam serves for a game; a body past this is not art.</summary>
    public const long MaxBytes = 8 * 1024 * 1024;

    /// <summary>Long enough for a 1 MB hero on a slow connection; short enough that a CDN that is
    /// not answering does not hold up the covers of every other game for the whole start.</summary>
    public static readonly TimeSpan DefaultPerFileTimeout = TimeSpan.FromSeconds(15);

    private readonly TimeSpan _perFileTimeout = perFileTimeout ?? DefaultPerFileTimeout;

    public async Task<ArtFetch> FetchAsync(Uri url, string destinationPath, CancellationToken ct)
    {
        var part = destinationPath + ".part";
        try
        {
            // One clock for the whole file, headers and body alike: a stall after the headers is
            // still a stall.
            using var timeout = CancellationTokenSource.CreateLinkedTokenSource(ct);
            timeout.CancelAfter(_perFileTimeout);
            var token = timeout.Token;

            using var response = await client.GetAsync(url, HttpCompletionOption.ResponseHeadersRead, token).ConfigureAwait(false);
            if (response.StatusCode == HttpStatusCode.NotFound) return ArtFetch.Missing;
            if (!response.IsSuccessStatusCode) return ArtFetch.Failed;
            if (response.Content.Headers.ContentLength is > MaxBytes) return ArtFetch.Failed;

            Directory.CreateDirectory(Path.GetDirectoryName(destinationPath)!);

            await using (var source = await response.Content.ReadAsStreamAsync(token).ConfigureAwait(false))
            await using (var sink = new FileStream(part, FileMode.Create, FileAccess.Write, FileShare.None))
            {
                var buffer = new byte[81920];
                long total = 0;
                int read;
                while ((read = await source.ReadAsync(buffer, token).ConfigureAwait(false)) > 0)
                {
                    total += read;
                    if (total > MaxBytes)
                    {
                        // A server that lied about, or did not state, its length.
                        await sink.DisposeAsync().ConfigureAwait(false);
                        File.Delete(part);
                        return ArtFetch.Failed;
                    }

                    await sink.WriteAsync(buffer.AsMemory(0, read), token).ConfigureAwait(false);
                }
            }

            File.Move(part, destinationPath, overwrite: true);
            return ArtFetch.Saved;
        }
        // OperationCanceledException covers the TaskCanceledException HttpClient raises, for the
        // timeout here and for a caller's own cancellation alike.
        catch (Exception e) when (e is HttpRequestException
                                    or OperationCanceledException
                                    or IOException
                                    or UnauthorizedAccessException
                                    or InvalidOperationException
                                    or NotSupportedException
                                    or ArgumentException)
        {
            try { File.Delete(part); }
            catch (Exception cleanup) when (cleanup is IOException or UnauthorizedAccessException) { }
            return ArtFetch.Failed;
        }
    }
}

/// <summary>The pictures the Xbox app ships with a game, named by its MicrosoftGame.config
/// under ShellVisuals: square logos in three sizes and a splash screen. The largest square logo
/// is the tile — square, so the view letterboxes it — and the splash the hero. Forza Horizon 6
/// on this PC names Storelogo.png, Logo.png (150×150), SmallLogo.png and SplashScreen.png
/// (1920×1080), all beside the config.</summary>
public static class XboxArtwork
{
    private const string ConfigName = "MicrosoftGame.config";

    /// <summary>Largest first: 480 where a game ships it, then the 150 tile, then the store logo
    /// (100×100). The 44×44 icon is too small to be worth a tile.</summary>
    private static readonly string[] TileAttributes = ["Square480x480Logo", "Square150x150Logo", "StoreLogo"];

    /// <summary>The art the config in <paramref name="contentRoot"/> names and that exists, as
    /// files inside that folder. Never throws: a folder without a config, a config that does not
    /// parse, or one naming files that are not there is a game without Xbox art.</summary>
    public static GameArt Read(string contentRoot)
    {
        try
        {
            if (!Directory.Exists(contentRoot)) return GameArt.None;

            // Matched without regard to case: Call of Duty ships MicrosoftGame.Config.
            var config = Directory.EnumerateFiles(contentRoot)
                .FirstOrDefault(f => string.Equals(Path.GetFileName(f), ConfigName, StringComparison.OrdinalIgnoreCase));
            if (config is null) return GameArt.None;

            // No DTD: the config comes from a folder anyone can write to, and an entity
            // expansion there would be a way to make every launcher start hang.
            var settings = new XmlReaderSettings { DtdProcessing = DtdProcessing.Prohibit, XmlResolver = null };
            using var reader = XmlReader.Create(config, settings);
            var visuals = XDocument.Load(reader).Root?.Descendants("ShellVisuals").FirstOrDefault();
            if (visuals is null) return GameArt.None;

            var tile = TileAttributes
                .Select(attribute => Picture(contentRoot, visuals.Attribute(attribute)?.Value))
                .FirstOrDefault(path => path is not null);
            var hero = Picture(contentRoot, visuals.Attribute("SplashScreenImage")?.Value);

            return tile is null && hero is null
                ? GameArt.None
                : new GameArt(tile, TileIsSquare: tile is not null, hero, Logo: null, Blur: null);
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or XmlException
                                    or ArgumentException or NotSupportedException)
        {
            return GameArt.None;
        }
    }

    /// <summary>The file a visual names, when it exists and sits inside the game's own folder.
    /// The config is in a folder any local user can write to, and the launcher — often elevated —
    /// would otherwise probe, and WPF would load, whatever file it named: a rooted path replaces
    /// the folder outright, and a ".." walks out of it.</summary>
    private static string? Picture(string contentRoot, string? relativePath)
    {
        if (string.IsNullOrWhiteSpace(relativePath) || Path.IsPathRooted(relativePath)) return null;

        var root = Path.GetFullPath(contentRoot).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
        var full = Path.GetFullPath(Path.Combine(root, relativePath));
        if (!full.StartsWith(root, StringComparison.OrdinalIgnoreCase)) return null;

        return File.Exists(full) ? full : null;
    }
}
