// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Text.Json;

namespace AmdnrLauncher.Core.Scanning;

public sealed class EpicScanner(string manifestsDirectory) : IGameScanner
{
    public static string DefaultManifestsDirectory => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData),
        "Epic", "EpicGamesLauncher", "Data", "Manifests");

    private sealed record Item(string? DisplayName, string? InstallLocation);

    public IEnumerable<GameCandidate> Scan()
    {
        // Materialised inside the guard on purpose: EnumerateFiles is lazy, so an access
        // error would otherwise surface mid-iteration and escape the iterator, killing the
        // whole scan. Same reasoning as SteamScanner.
        List<string> items;
        try
        {
            items = Directory.Exists(manifestsDirectory)
                ? Directory.EnumerateFiles(manifestsDirectory, "*.item").ToList()
                : [];
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            yield break;
        }

        foreach (var file in items)
        {
            GameCandidate? candidate = null;
            try
            {
                var item = JsonSerializer.Deserialize<Item>(
                    File.ReadAllText(file),
                    new JsonSerializerOptions { PropertyNameCaseInsensitive = true });

                if (!string.IsNullOrWhiteSpace(item?.DisplayName) &&
                    !string.IsNullOrWhiteSpace(item.InstallLocation) &&
                    Directory.Exists(item.InstallLocation))
                {
                    candidate = new GameCandidate(item.DisplayName, GameStore.Epic, item.InstallLocation);
                }
            }
            catch (Exception e) when (e is IOException or UnauthorizedAccessException or JsonException)
            {
                // Skip this manifest; keep scanning.
            }

            if (candidate is not null) yield return candidate;
        }
    }
}
