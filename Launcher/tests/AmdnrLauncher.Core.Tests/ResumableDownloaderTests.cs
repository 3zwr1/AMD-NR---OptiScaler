// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Net;
using AmdnrLauncher.Core.Assets;

namespace AmdnrLauncher.Core.Tests;

public class ResumableDownloaderTests
{
    // SHA256("hello world")
    private const string HelloSha = "B94D27B9934D3E08A52E52D7DA7DABFAC484EFE37A5380EE9088F7ACE2EFCDE9";
    private static readonly byte[] Hello = "hello world"u8.ToArray();

    private sealed class StubHandler(Func<HttpRequestMessage, HttpResponseMessage> respond)
        : HttpMessageHandler
    {
        public List<HttpRequestMessage> Requests { get; } = [];

        protected override Task<HttpResponseMessage> SendAsync(
            HttpRequestMessage request, CancellationToken cancellationToken)
        {
            Requests.Add(request);
            return Task.FromResult(respond(request));
        }
    }

    private static HttpResponseMessage Full() => new(HttpStatusCode.OK)
    {
        Content = new ByteArrayContent(Hello),
    };

    [Fact]
    public async Task Downloads_verifies_and_moves_into_place()
    {
        using var dir = new TempDir();
        var handler = new StubHandler(_ => Full());
        var sut = new ResumableDownloader(new HttpClient(handler));
        var dest = Path.Combine(dir.Path, "out.bin");

        await sut.DownloadAsync("https://h/f", dest, HelloSha, Hello.Length, null, default);

        Assert.Equal("hello world", File.ReadAllText(dest));
        Assert.False(File.Exists(dest + ".part"));
    }

    [Fact]
    public async Task Resumes_from_an_existing_partial_file()
    {
        using var dir = new TempDir();
        var dest = Path.Combine(dir.Path, "out.bin");
        File.WriteAllBytes(dest + ".part", Hello[..6]);          // "hello "

        var handler = new StubHandler(_ => new HttpResponseMessage(HttpStatusCode.PartialContent)
        {
            Content = new ByteArrayContent(Hello[6..]),           // "world"
        });
        var sut = new ResumableDownloader(new HttpClient(handler));

        await sut.DownloadAsync("https://h/f", dest, HelloSha, Hello.Length, null, default);

        Assert.Equal("hello world", File.ReadAllText(dest));
        Assert.Equal(6, handler.Requests.Single().Headers.Range!.Ranges.Single().From);
    }

    [Fact]
    public async Task Hash_mismatch_throws_and_discards_the_partial_file()
    {
        using var dir = new TempDir();
        var handler = new StubHandler(_ => Full());
        var sut = new ResumableDownloader(new HttpClient(handler));
        var dest = Path.Combine(dir.Path, "out.bin");

        var wrong = new string('0', 64);
        await Assert.ThrowsAsync<HashMismatchException>(() =>
            sut.DownloadAsync("https://h/f", dest, wrong, Hello.Length, null, default));

        Assert.False(File.Exists(dest));
        Assert.False(File.Exists(dest + ".part"));   // so the retry is not poisoned
    }

    [Fact]
    public async Task Reports_progress()
    {
        using var dir = new TempDir();
        var handler = new StubHandler(_ => Full());
        var sut = new ResumableDownloader(new HttpClient(handler));
        var seen = new List<DownloadProgress>();

        await sut.DownloadAsync("https://h/f", Path.Combine(dir.Path, "out.bin"),
            HelloSha, Hello.Length, new Progress<DownloadProgress>(seen.Add), default);

        // Progress is posted asynchronously; assert the terminal report eventually lands.
        for (var i = 0; i < 50 && (seen.Count == 0 || seen[^1].BytesReceived != Hello.Length); i++)
            await Task.Delay(10);

        Assert.Equal(Hello.Length, seen[^1].BytesReceived);
        Assert.Equal(Hello.Length, seen[^1].TotalBytes);
    }
}
