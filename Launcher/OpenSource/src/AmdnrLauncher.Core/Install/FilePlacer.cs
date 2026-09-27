// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
namespace AmdnrLauncher.Core.Install;

public enum PlacementMode { Copy, Hardlink }

public interface IFilePlacer
{
    PlacementMode Place(string source, string destination, CancellationToken ct);
}

/// <summary>Phase 1 placement. Hardlinking arrives in Phase 2 as a second
/// implementation of this interface; nothing else has to change.</summary>
public sealed class CopyFilePlacer : IFilePlacer
{
    public PlacementMode Place(string source, string destination, CancellationToken ct)
    {
        var directory = Path.GetDirectoryName(destination)!;
        Directory.CreateDirectory(directory);

        var temp = Path.Combine(directory, ".amdnr-" + Guid.NewGuid().ToString("N") + ".tmp");
        try
        {
            using (var from = new FileStream(source, FileMode.Open, FileAccess.Read, FileShare.Read))
            using (var to = new FileStream(temp, FileMode.Create, FileAccess.Write, FileShare.None))
            {
                var buffer = new byte[1024 * 1024];
                int read;
                while ((read = from.Read(buffer, 0, buffer.Length)) > 0)
                {
                    ct.ThrowIfCancellationRequested();
                    to.Write(buffer, 0, read);
                }
            }

            File.Move(temp, destination, overwrite: true);
            return PlacementMode.Copy;
        }
        catch
        {
            // A half-written DLL is worse than no DLL: the game would try to load it.
            // The cleanup is best-effort — letting it throw here would replace the real
            // failure with a misleading one from the delete.
            try { File.Delete(temp); }
            catch (Exception e) when (e is IOException or UnauthorizedAccessException) { }
            throw;
        }
    }
}
