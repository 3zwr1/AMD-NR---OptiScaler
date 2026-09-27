// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Net;
using System.Net.Http.Headers;

namespace AmdnrLauncher.Core.Assets;

public readonly record struct DownloadProgress(long BytesReceived, long TotalBytes);

public sealed class HashMismatchException(string expected, string actual)
    : Exception($"Downloaded file failed verification. Expected {expected}, got {actual}.")
{
    public string Expected { get; } = expected;
    public string Actual { get; } = actual;
}

public sealed class ResumableDownloader(HttpClient client)
{
    public async Task DownloadAsync(
        string url,
        string destinationPath,
        string expectedSha256,
        long expectedSize,
        IProgress<DownloadProgress>? progress,
        CancellationToken ct)
    {
        var partPath = destinationPath + ".part";
        Directory.CreateDirectory(Path.GetDirectoryName(destinationPath)!);

        var already = File.Exists(partPath) ? new FileInfo(partPath).Length : 0;
        if (already >= expectedSize && expectedSize > 0) already = 0;   // corrupt: start over

        using var request = new HttpRequestMessage(HttpMethod.Get, url);
        if (already > 0) request.Headers.Range = new RangeHeaderValue(already, null);

        using var response = await client.SendAsync(
            request, HttpCompletionOption.ResponseHeadersRead, ct);
        response.EnsureSuccessStatusCode();

        // A server that ignores Range answers 200 with the whole body; restart cleanly.
        if (already > 0 && response.StatusCode != HttpStatusCode.PartialContent)
        {
            File.Delete(partPath);
            already = 0;
        }

        var total = expectedSize > 0
            ? expectedSize
            : already + (response.Content.Headers.ContentLength ?? 0);

        await using (var source = await response.Content.ReadAsStreamAsync(ct))
        await using (var sink = new FileStream(
            partPath, already > 0 ? FileMode.Append : FileMode.Create, FileAccess.Write, FileShare.None))
        {
            var buffer = new byte[81920];
            var received = already;
            int read;
            while ((read = await source.ReadAsync(buffer, ct)) > 0)
            {
                await sink.WriteAsync(buffer.AsMemory(0, read), ct);
                received += read;
                progress?.Report(new DownloadProgress(received, total));
            }
        }

        var actual = Hashing.Sha256OfFile(partPath);
        if (!Hashing.Matches(actual, expectedSha256))
        {
            File.Delete(partPath);
            throw new HashMismatchException(expectedSha256, actual);
        }

        File.Move(partPath, destinationPath, overwrite: true);
    }
}
