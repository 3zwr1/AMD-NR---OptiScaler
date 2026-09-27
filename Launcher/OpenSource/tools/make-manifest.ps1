# Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.

<#
.SYNOPSIS
Writes launcher\manifest.json from the GitHub releases it points at.

.DESCRIPTION
Every url, size and sha256 in the manifest comes from the GitHub API (each release asset's
"digest"), never from a person copying it. A wrong hash in a published manifest fails every
download for every user at once, and hand-copied hashes are how that happens.

The script only reads from GitHub. It never downloads a package.

Everything that is not release data (build descriptions, exclude lists, accepted pass hashes,
the runtimes and the GPU generations each is offered to, proxy defaults and overrides, hidden
Steam app ids) lives in the tables in this file. Change it here, not in manifest.json: the next
run rewrites the whole file.

The manifest offers three of danielblnc's runtimes: 0.4.1 for RX 9000, 0.3.3 for every other
GPU, and 0.4.0 kept behind them; see $Runtimes. Every zip has to be in the release(s) named by
-RuntimeTags, or the script refuses: a manifest that names a runtime nobody can download fails
the install of everyone whose GPU picks it.

Windows PowerShell 5.1 compatible, and kept to ASCII: 5.1 reads a script without a BOM in the
system code page, so any other character would arrive garbled.

.PARAMETER AmdnrTag
Release tag in 3zwr1/AMD-NR---OptiScaler holding AMDNR-v<version>.zip.

.PARAMETER TheAutomaticTag
Release tag in TheAutomatic/dlss-5-amd-project holding OptiScaler-AMD-PreSR-<version>.zip.
Defaults to v1.9.1-alpha, the build the owner chose for 0.4.0. It is a GitHub pre-release, and
TheAutomatic's releases are accepted as pre-releases because the tag is always picked by hand.

.PARAMETER LauncherTag
Release tag in 3zwr1/AMD-NR---OptiScaler holding AMDNR-Launcher.exe and nvngx.dll_dlssnr.dll.

.PARAMETER RuntimeTags
Release tag(s) in 3zwr1/AMD-NR---OptiScaler holding the runtime zips: one tag for all of them, or
one per entry of $Runtimes in its order, comma-separated. Defaults to AmdnrTag, where they live today.

.PARAMETER RuntimeAssets
The runtime zips' asset names, one per entry of $Runtimes in its order. Default to the names in
the table (v0.4.1-Runtime.zip, v0.3.3-Runtime.zip, v0.4.0-Runtime.zip). Each must be
v<version>-Runtime.zip with the version its package id stands for (runtime-033 is 0.3.3), so they
cannot be swapped by mistake.

.PARAMETER LauncherVersion
Defaults to the version number at the end of LauncherTag (Launcher0.4.0 -> 0.4.0).

.PARAMETER ForwarderVersion
The forwarder's version as the manifest states it. Raise it whenever the forwarder's bytes
change: launchers re-download a cached forwarder whose hash no longer matches either way, but
the version is what install records and support conversations name.

.PARAMETER ForwarderPath
The local nvngx.dll_dlssnr.dll. Normally optional: when given, the uploaded asset's digest must
match it, which catches uploading the wrong build. Required with -SkipLauncher.

.PARAMETER ForwarderUrl
The address the forwarder is downloaded from when it is hosted somewhere other than the launcher's
GitHub release (the owner's own server). Its hash and size then come from -ForwarderPath, which is
required with it, and the launcher release is read for AMDNR-Launcher.exe only. Must be a direct,
permanent https link to the file itself, not a page.

.PARAMETER SkipLauncher
For before the launcher release exists. The launcher release is not read: the launcher and
forwarder URLs are built from LauncherTag, the forwarder hash and size come from ForwarderPath,
and the launcher sha256 is left as PENDING_UPLOAD. The result is NOT publishable.

.PARAMETER AllowPrerelease
Accept AMDNR and launcher releases GitHub marks as pre-release. Off by default so a test build
is never published by accident. TheAutomatic's releases do not need it: see TheAutomaticTag.

.EXAMPLE
powershell -ExecutionPolicy Bypass -File tools\make-manifest.ps1 -AmdnrTag Alpha0.3.3.1 -LauncherTag Launcher0.4.0 -ForwarderPath ..\OptiScaler-DLSSNR-PreSR-Multipass-main\x64\Release\a\nvngx.dll_dlssnr.dll
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$AmdnrTag,
    [string]$TheAutomaticTag = 'v1.9.1-alpha',
    [Parameter(Mandatory = $true)][string]$LauncherTag,
    [string[]]$RuntimeTags,
    [string[]]$RuntimeAssets,
    [string]$LauncherVersion,
    [string]$ForwarderVersion = '0.2.1',
    [string]$ForwarderPath,
    [string]$ForwarderUrl,
    [switch]$SkipLauncher,
    [switch]$AllowPrerelease,
    [string]$OutFile
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$AmdnrRepo = '3zwr1/AMD-NR---OptiScaler'
# TheAutomatic's repository has no licence, so their zip is only ever linked from their own
# release, never re-hosted.
$TheAutomaticRepo = 'TheAutomatic/dlss-5-amd-project'
$LauncherAssetName = 'AMDNR-Launcher.exe'
$ForwarderAssetName = 'nvngx.dll_dlssnr.dll'
$LauncherHashPending = 'PENDING_UPLOAD'

# The pass DLL hashes the mod accepts: kAmdLayouts[] in OptiScaler\dlssnr\amd\AmdLayout.h of the
# AMDNR source, in its order: 0.2.17, 0.3, 0.3.1, 0.3.2, 0.3.3, 0.4.0, 0.4.1 (published 26 Sep 2026),
# then one layout the mod already accepts for a runtime that is not published yet. They describe
# what the mod will load, not what a runtime zip contains, so they cannot be read from a release.
# Every runtime package carries the whole list: the Doctor checks the pass DLLs it finds against
# the package the install came from, and a shorter list has it call a good runtime unverified.
$AcceptedPassSha256 = @(
    'bc97f3b06718e19042acaf227bfe15d1e43d4977f9dc2e39994fcc511445ff4e', # kAmd0217
    '8321cae728d28cb7632d0d58d3d913e91132bf7645c126505698fbe4cd5a0138', # kAmd03
    'b108d6407eb7f094a4f9111edd778eee7b978b648d413a9fc7aeedfdd914c154', # kAmd031
    'b92f7481bc03fa41f443b1e1e54b502789df2bbbcefe48c680df7bb02a33fc1e', # kAmd032
    '907b30a61644a6d7e43e58a43a9d97a04a24b1a764a88bdef3954ac807e8d112', # kAmd033
    'd62be3d8b9fbb3c6c81982c4ddb3dfa00eb9662e3206925cbe5b7e1bc6798b80', # kAmd040
    '823063eb4c76b1334fd1800c41798873ae61d4016af0406f1f0b9dce57b1d376', # kAmd041
    '8aa2dcc5b6596aca97995dbfd4e0a9790d8c15108495e0ed154dd15dbb5b465a'  # a later layout, not published yet
)

# danielblnc's runtimes, in the order the launcher tries them: the first entry whose gpus lists
# the detected generation wins. 0.4.1 (published 26 Sep 2026) is the RX 9000 runtime, by the
# owner's decision that day; 0.3.3 runs on RX 7000 and RX 9000 and is what every other GPU gets.
# 0.4.0 stays listed behind both so its package and hashes remain in the manifest, but no card
# reaches it by default. Reordering this table is how a newer runtime reaches players: the Doctor
# offers REPAIR / UPDATE to every game installed with another runtime for its card, with no new
# launcher. rdna1 is on no list on purpose: no runtime runs there. The package id is 'runtime-' and
# the version without its dots, and the asset name must carry that version (checked when read).
$Runtimes = @(
    [ordered]@{ id = 'runtime-041'; asset = 'v0.4.1-Runtime.zip'; gpus = @('rdna4') },
    [ordered]@{ id = 'runtime-033'; asset = 'v0.3.3-Runtime.zip'; gpus = @('rdna2', 'rdna3', 'rdna4', 'unknown-amd', 'none') },
    [ordered]@{ id = 'runtime-040'; asset = 'v0.4.0-Runtime.zip'; gpus = @('rdna4') }
)

# The generation strings the launcher's GPU probe can report. Anything else in a gpus list would
# never be picked, silently.
$KnownGpus = @('rdna1', 'rdna2', 'rdna3', 'rdna4', 'unknown-amd', 'none')
$RuntimeAssetPattern = '^v(?<version>\d[^/]*)-Runtime\.zip$'

# TheAutomatic's own installer scripts would set things up a second way beside the launcher's
# install, and their uninstaller does not know about the launcher's backup record. 1.9.1-alpha
# has no tools\ folder any more; everything else in its zip is placed, lmxxf-modules\ included.
$TheAutomaticExclude = @('Setup.bat', 'Setup.ps1', 'Uninstall_OptiScaler_NR.bat', 'Uninstall_OptiScaler_NR.ps1')

# TheAutomatic's packaged OptiScaler.ini ships [DlssNr] Enabled=false and selects the lmxxf
# backend, whose model files are not in their release. With the DLSSNR AMD files installed,
# danielblnc's runtime is the backend the launcher can actually supply, so the seeded ini has to
# switch NR on and select it - what their own Setup.ps1 writes - or Neural Rendering stays off.
$TheAutomaticIniWhenRuntime = @(
    [ordered]@{ section = 'DlssNr'; key = 'Enabled'; value = 'true' },
    [ordered]@{ section = 'DlssNr'; key = 'NrBackend'; value = 'daniel' }
)

$ProxyDefaults = @('dxgi.dll', 'd3d12.dll', 'winmm.dll', 'version.dll', 'dbghelp.dll', 'd3d11.dll')

# Each from the owner's own working install, so the first name offered is the one known to load
# there. Resident Evil Requiem is the one that sent the owner round every name: REFramework holds
# dinput8.dll, and OptiScaler loads as dxgi.dll. ManifestParserTests checks this table against
# manifest.json, so add to both.
$ProxyOverrides = @(
    [ordered]@{ exe = 're9.exe'; proxy = 'dxgi.dll'; reason = 'REFramework holds dinput8.dll; OptiScaler loads as dxgi.dll' },
    [ordered]@{ exe = 'Spider-Man.exe'; proxy = 'dbghelp.dll'; reason = 'dxgi.dll does not load in this Nixxes port' },
    [ordered]@{ exe = 'forzahorizon6.exe'; proxy = 'dxgi.dll'; reason = "loads as dxgi.dll in the owner's own install" },
    [ordered]@{ exe = 'GTA5_Enhanced.exe'; proxy = 'dxgi.dll'; reason = "a tester's log on 0.3.3.2 reads 'OptiScaler working as dxgi.dll' (GTA V Enhanced, DX12)" },
    [ordered]@{ exe = 'SHProto-Win64-Shipping.exe'; proxy = 'dxgi.dll'; reason = "loads as dxgi.dll in the owner's own install" },
    [ordered]@{ exe = 'Stray-Win64-Shipping.exe'; proxy = 'dxgi.dll'; reason = "loads as dxgi.dll in the owner's own install" }
)

$HiddenSteamAppIds = @()

# Windows PowerShell 5.1 runs on .NET Framework, which can still default to TLS 1.0;
# api.github.com refuses anything below 1.2.
[Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12

function Get-Release([string]$Repo, [string]$Tag, [bool]$PrereleaseIsFine = $false) {
    $uri = "https://api.github.com/repos/$Repo/releases/tags/$([Uri]::EscapeDataString($Tag))"
    try {
        $release = Invoke-RestMethod -Method Get -Uri $uri -UserAgent 'AMDNR-make-manifest' `
            -Headers @{ Accept = 'application/vnd.github+json' }
    }
    catch {
        # A 404 here usually means the tag was never published: GitHub hides drafts from this
        # endpoint just as it hides them from users.
        throw "Could not read release '$Tag' of ${Repo} (is it published under exactly that tag?): $($_.Exception.Message)"
    }
    if ($release.draft) {
        throw "Release '$Tag' of $Repo is a draft; nobody else can download its assets."
    }
    if ($release.prerelease -and -not $AllowPrerelease -and -not $PrereleaseIsFine) {
        throw "Release '$Tag' of $Repo is marked pre-release. Pass -AllowPrerelease if offering it is deliberate."
    }
    $release
}

function Get-Asset($Release, [string]$Repo, [string]$Pattern) {
    $tag = $Release.tag_name
    $found = @($Release.assets | Where-Object { $_.name -match $Pattern })
    if ($found.Count -ne 1) {
        $names = @($Release.assets | ForEach-Object { $_.name }) -join ', '
        throw "Release '$tag' of $Repo has $($found.Count) assets matching '$Pattern'; expected exactly one. Assets: $names"
    }
    $asset = $found[0]

    # GitHub only records a digest for assets uploaded since mid-2025. Without one, the hash
    # would have to come from somewhere other than the file GitHub actually serves.
    $digest = $asset.PSObject.Properties['digest']
    if ($null -eq $digest -or $null -eq $digest.Value -or -not ($digest.Value -match '^sha256:([0-9a-fA-F]{64})$')) {
        throw "Asset '$($asset.name)' in release '$tag' of $Repo has no sha256 digest. Re-upload it so GitHub records one."
    }
    $sha256 = $Matches[1].ToLowerInvariant()

    $version = $null
    if ($asset.name -match $Pattern) { $version = $Matches['version'] }

    [pscustomobject]@{
        Name    = [string]$asset.name
        Version = $version
        Url     = [string]$asset.browser_download_url
        Sha256  = $sha256
        Size    = [long]$asset.size
    }
}

# One runtime zip by its exact name. The version comes from the name and has to be the one the
# package id stands for, so -RuntimeAssets given in the wrong order fails here, not on a user's PC.
function Get-RuntimeAsset($Release, [string]$Repo, [string]$Name, [string]$Id) {
    if (-not ($Name -match $RuntimeAssetPattern)) {
        throw "Runtime asset name '$Name' is not v<version>-Runtime.zip."
    }
    $version = $Matches['version']
    $expectedId = 'runtime-' + ($version -replace '\.', '')
    if ($expectedId -ne $Id) {
        throw "Runtime asset '$Name' is version $version, which is package '$expectedId', not '$Id'. Check the order of -RuntimeAssets."
    }
    $asset = Get-Asset $Release $Repo "^$([regex]::Escape($Name))$"
    $asset.Version = $version
    $asset
}

function Get-DownloadUrl([string]$Repo, [string]$Tag, [string]$Name) {
    "https://github.com/$Repo/releases/download/$([Uri]::EscapeDataString($Tag))/$([Uri]::EscapeDataString($Name))"
}

function Get-LocalFileFacts([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "File not found: $Path" }
    $item = Get-Item -LiteralPath $Path
    [pscustomobject]@{
        Sha256 = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        Size   = [long]$item.Length
    }
}

function Format-JsonString([string]$Text) {
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.Append('"')
    foreach ($ch in $Text.ToCharArray()) {
        if ($ch -eq [char]'"') { [void]$sb.Append('\"') }
        elseif ($ch -eq [char]'\') { [void]$sb.Append('\\') }
        elseif ([int]$ch -lt 0x20) { [void]$sb.AppendFormat('\u{0:x4}', [int]$ch) }
        else { [void]$sb.Append($ch) }
    }
    [void]$sb.Append('"')
    $sb.ToString()
}

# ConvertTo-Json is not used: in Windows PowerShell 5.1 it can write an array as
# {"value":[...],"Count":n}, it escapes apostrophes as backslash-u0027, and its indentation is hard to
# review in a diff. This writes the handful of types the manifest uses and nothing else.
function ConvertTo-ManifestJson($Value, [int]$Depth = 0) {
    $pad = '  ' * ($Depth + 1)
    $end = '  ' * $Depth
    if ($null -eq $Value) { return 'null' }
    if ($Value -is [bool]) { if ($Value) { return 'true' } else { return 'false' } }
    if ($Value -is [string]) { return (Format-JsonString $Value) }
    if ($Value -is [int] -or $Value -is [long]) { return $Value.ToString([Globalization.CultureInfo]::InvariantCulture) }
    if ($Value -is [System.Collections.IDictionary]) {
        if ($Value.Count -eq 0) { return '{}' }
        $members = @(foreach ($key in $Value.Keys) {
            $pad + (Format-JsonString ([string]$key)) + ': ' + (ConvertTo-ManifestJson $Value[$key] ($Depth + 1))
        })
        return "{`n" + ($members -join ",`n") + "`n$end}"
    }
    if ($Value -is [System.Collections.IEnumerable]) {
        $items = @(foreach ($item in $Value) { ConvertTo-ManifestJson $item ($Depth + 1) })
        if ($items.Count -eq 0) { return '[]' }
        $nested = @($Value | Where-Object { $_ -is [System.Collections.IEnumerable] -and $_ -isnot [string] })
        $inline = '[' + ($items -join ', ') + ']'
        if ($nested.Count -eq 0 -and ($end.Length + $inline.Length) -le 100) { return $inline }
        return "[`n" + (@($items | ForEach-Object { $pad + $_ }) -join ",`n") + "`n$end]"
    }
    throw "Cannot write a $($Value.GetType().FullName) to the manifest."
}

function Assert-Manifest([string]$Path, [bool]$LauncherPending) {
    try { $m = [IO.File]::ReadAllText($Path) | ConvertFrom-Json }
    catch { throw "The written manifest is not valid JSON: $($_.Exception.Message)" }

    $hex = '^[0-9a-f]{64}$'
    $release = '^https://github\.com/[^/]+/[^/]+/releases/download/[^/]+/[^/]+$'
    $problems = New-Object System.Collections.Generic.List[string]

    if ($m.schemaVersion -ne 1) { $problems.Add("schemaVersion is '$($m.schemaVersion)', not 1.") }
    if (-not ($m.launcher.version -match '^\d+(\.\d+)+$')) { $problems.Add("launcher.version '$($m.launcher.version)' is not a version number.") }
    if (-not ($m.launcher.url -match $release)) { $problems.Add("launcher.url '$($m.launcher.url)' is not a GitHub release asset.") }
    if (-not ($m.launcher.sha256 -match $hex) -and -not ($LauncherPending -and $m.launcher.sha256 -eq $LauncherHashPending)) {
        $problems.Add("launcher.sha256 '$($m.launcher.sha256)' is not a sha256.")
    }

    $ids = @($m.packages | ForEach-Object { $_.id })
    foreach ($dup in @($ids | Group-Object | Where-Object { $_.Count -gt 1 })) { $problems.Add("Package id '$($dup.Name)' appears more than once.") }
    foreach ($p in @($m.packages)) {
        # The forwarder may live on the owner's own server (-ForwarderUrl); everything else is a GitHub release asset.
        $elsewhere = ($p.id -eq 'forwarder') -and $ForwarderUrl -and ($p.url -eq $ForwarderUrl)
        if (-not $elsewhere -and -not ($p.url -match $release)) { $problems.Add("Package '$($p.id)' url '$($p.url)' is not a GitHub release asset.") }
        if (-not ($p.sha256 -match $hex)) { $problems.Add("Package '$($p.id)' sha256 '$($p.sha256)' is not a sha256.") }
        if ([long]$p.size -le 0) { $problems.Add("Package '$($p.id)' has no size.") }
    }

    $runtimeIds = @($Runtimes | ForEach-Object { $_.id })
    foreach ($r in $Runtimes) {
        if (@($r.gpus).Count -eq 0) { $problems.Add("Runtime '$($r.id)' lists no GPU generation, so nothing would ever pick it.") }
        foreach ($g in @($r.gpus)) {
            if ($KnownGpus -notcontains $g) { $problems.Add("Runtime '$($r.id)' names GPU generation '$g', which the launcher cannot detect. Known: $($KnownGpus -join ', ').") }
        }
        $found = @($m.packages | Where-Object { $_.id -eq $r.id })
        if ($found.Count -ne 1) { $problems.Add("There is no '$($r.id)' package."); continue }
        $pass = $found[0].PSObject.Properties['passSha256']
        $hashes = @()
        if ($null -ne $pass) { $hashes = @($pass.Value) }
        if ($hashes.Count -ne $AcceptedPassSha256.Count `
            -or @($hashes | Where-Object { -not ($_ -match $hex) }).Count -gt 0 `
            -or @($hashes | Select-Object -Unique).Count -ne $hashes.Count) {
            $problems.Add("Package '$($r.id)' needs exactly $($AcceptedPassSha256.Count) distinct passSha256 hashes, one per layout in AmdLayout.h.")
        }
    }

    $sources = @($m.sources)
    if ($sources.Count -eq 0) { $problems.Add('There are no sources.') }
    foreach ($s in $sources) {
        # Every build offers the same runtimes in the same order; a source whose list differs, or
        # whose one-generation list was written as a string, would pick a runtime nobody meant.
        $list = $s.PSObject.Properties['runtimes']
        $choices = @()
        if ($null -ne $list) { $choices = @($list.Value) }
        $names = @($choices | ForEach-Object { $_.package })
        if (($names -join ' ') -ne ($runtimeIds -join ' ')) {
            $problems.Add("Source '$($s.id)' runtimes are '$($names -join ', ')', not '$($runtimeIds -join ', ')' in that order.")
        }
        for ($i = 0; $i -lt [Math]::Min($choices.Count, $Runtimes.Count); $i++) {
            $gpus = $choices[$i].PSObject.Properties['gpus']
            if ($null -eq $gpus -or $gpus.Value -isnot [array]) {
                $problems.Add("Source '$($s.id)' runtime '$($names[$i])' gpus is not a list.")
            }
            elseif ((@($gpus.Value) -join ' ') -ne (@($Runtimes[$i].gpus) -join ' ')) {
                $problems.Add("Source '$($s.id)' runtime '$($names[$i])' lists GPUs '$(@($gpus.Value) -join ', ')', not '$(@($Runtimes[$i].gpus) -join ', ')'.")
            }
        }
    }
    $automatic = @($sources | Where-Object { $_.id -eq 'theautomatic' })
    if ($automatic.Count -eq 1) {
        # Read through PSObject.Properties: under Set-StrictMode 2.0 a missing member throws
        # instead of reading as null, and the message below would never be shown.
        $ini = $automatic[0].PSObject.Properties['iniWhenRuntime']
        $settings = @()
        if ($null -ne $ini) { $settings = @($ini.Value | ForEach-Object { "[$($_.section)] $($_.key)=$($_.value)" }) }
        foreach ($needed in @('[DlssNr] Enabled=true', '[DlssNr] NrBackend=daniel')) {
            if ($settings -notcontains $needed) {
                $problems.Add("Source 'theautomatic' iniWhenRuntime does not set $needed; its packaged ini ships NR off, on a backend with no model files.")
            }
        }
    }
    foreach ($s in $sources) {
        if ($ids -notcontains $s.package) { $problems.Add("Source '$($s.id)' names package '$($s.package)', which is not listed.") }
        if ($null -ne $s.forwarder -and $ids -notcontains $s.forwarder) { $problems.Add("Source '$($s.id)' names forwarder '$($s.forwarder)', which is not listed.") }
    }

    if ($problems.Count -gt 0) {
        throw ("The written manifest failed validation:`n  " + ($problems -join "`n  "))
    }
}

# --- read the releases ----------------------------------------------------------------------

# "a,b" arrives as one string when the script is run with powershell -File, as RELEASING.md says
# to; PowerShell only splits a comma list into an array when the call is made from PowerShell.
function Split-List([string[]]$Values) {
    @(foreach ($v in @($Values)) { foreach ($part in ([string]$v) -split ',') { if ($part.Trim()) { $part.Trim() } } })
}
if (-not $RuntimeTags) { $RuntimeTags = @($AmdnrTag) }
$RuntimeTags = Split-List $RuntimeTags
if (@($RuntimeTags).Count -eq 1 -and $Runtimes.Count -gt 1) { $RuntimeTags = @(foreach ($r in $Runtimes) { $RuntimeTags[0] }) }
if (@($RuntimeTags).Count -ne $Runtimes.Count) {
    throw "-RuntimeTags needs one tag, or one per runtime in this order: $($Runtimes | ForEach-Object { $_.id })."
}
if (-not $RuntimeAssets) { $RuntimeAssets = @(foreach ($r in $Runtimes) { $r.asset }) }
$RuntimeAssets = Split-List $RuntimeAssets
if (@($RuntimeAssets).Count -ne $Runtimes.Count) {
    throw "-RuntimeAssets needs one asset name per runtime in this order: $($Runtimes | ForEach-Object { $_.id })."
}
if (-not $LauncherVersion) {
    if ($LauncherTag -match '(?<version>\d+(\.\d+)+)$') { $LauncherVersion = $Matches['version'] }
    else { throw "Cannot read a version number from the end of '$LauncherTag'. Pass -LauncherVersion." }
}
if (-not $OutFile) { $OutFile = Join-Path $PSScriptRoot '..\manifest.json' }
$OutFile = [IO.Path]::GetFullPath($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutFile))

$amdnrRelease = Get-Release $AmdnrRepo $AmdnrTag
# Only AMDNR-v*.zip is matched. The 0.3.2 asset was briefly published as FSRNR-v0.3.2.zip and
# renamed back; a URL built from the old name is a 404, so a release still carrying it has to
# fail here rather than reach users.
$amdnr = Get-Asset $amdnrRelease $AmdnrRepo '^AMDNR-v(?<version>\d[^/]*)\.zip$'

# Both runtime zips, each from its release, read once per distinct tag. A missing one is a
# refusal (Get-Asset), not a manifest with one runtime fewer.
$releasesByTag = @{ $AmdnrTag = $amdnrRelease }
$runtimePackages = @()
for ($i = 0; $i -lt $Runtimes.Count; $i++) {
    $tag = [string]$RuntimeTags[$i]
    if (-not $releasesByTag.ContainsKey($tag)) { $releasesByTag[$tag] = Get-Release $AmdnrRepo $tag }
    $asset = Get-RuntimeAsset $releasesByTag[$tag] $AmdnrRepo ([string]$RuntimeAssets[$i]) ([string]$Runtimes[$i].id)
    $runtimePackages += [ordered]@{
        id = $Runtimes[$i].id; version = $asset.Version; url = $asset.Url; sha256 = $asset.Sha256; size = $asset.Size
        passSha256 = $AcceptedPassSha256
    }
}
$RuntimeChoices = @(foreach ($r in $Runtimes) { [ordered]@{ package = $r.id; gpus = $r.gpus } })

$theAutomatic = Get-Asset (Get-Release $TheAutomaticRepo $TheAutomaticTag $true) $TheAutomaticRepo '^OptiScaler-AMD-PreSR-(?<version>\d[^/]*)\.zip$'

$localForwarder = $null
if ($ForwarderPath) { $localForwarder = Get-LocalFileFacts $ForwarderPath }
if ($ForwarderUrl -and $null -eq $localForwarder) {
    throw '-ForwarderUrl needs -ForwarderPath: the hash and size of a file on another server come from the local copy.'
}
if ($ForwarderUrl -and $ForwarderUrl -notmatch '^https://') {
    throw "-ForwarderUrl must be an https address to the file itself; got '$ForwarderUrl'."
}

if ($SkipLauncher) {
    if ($null -eq $localForwarder) {
        throw '-SkipLauncher needs -ForwarderPath: with no launcher release to read, the forwarder hash has to come from the local file.'
    }
    $launcherUrl = Get-DownloadUrl $AmdnrRepo $LauncherTag $LauncherAssetName
    $launcherSha256 = $LauncherHashPending
    $forwarder = [pscustomobject]@{
        Url    = if ($ForwarderUrl) { $ForwarderUrl } else { Get-DownloadUrl $AmdnrRepo $LauncherTag $ForwarderAssetName }
        Sha256 = $localForwarder.Sha256
        Size   = $localForwarder.Size
    }
}
else {
    $launcherRelease = Get-Release $AmdnrRepo $LauncherTag
    $launcher = Get-Asset $launcherRelease $AmdnrRepo "^$([regex]::Escape($LauncherAssetName))$"
    $launcherUrl = $launcher.Url
    $launcherSha256 = $launcher.Sha256
    if ($ForwarderUrl) {
        # Hosted elsewhere: the release holds the launcher only, and the local file is the forwarder.
        $forwarder = [pscustomobject]@{ Url = $ForwarderUrl; Sha256 = $localForwarder.Sha256; Size = $localForwarder.Size }
    }
    else {
        $forwarder = Get-Asset $launcherRelease $AmdnrRepo "^$([regex]::Escape($ForwarderAssetName))$"
        if ($null -ne $localForwarder -and $localForwarder.Sha256 -ne $forwarder.Sha256) {
            throw "The uploaded $ForwarderAssetName ($($forwarder.Sha256)) is not the file at $ForwarderPath ($($localForwarder.Sha256))."
        }
    }
}

# --- write -----------------------------------------------------------------------------------

$manifest = [ordered]@{
    schemaVersion = 1
    launcher = [ordered]@{ version = $LauncherVersion; url = $launcherUrl; sha256 = $launcherSha256 }
    sources = @(
        [ordered]@{
            id = 'amdnr'; name = 'AMDNR'; author = '3zwr1'
            summary = 'OptiScaler with AMD Neural Rendering, the built-in lmxxf runtime for RX 7000 and RX 9000, and FSR frame generation.'
            homepage = 'https://github.com/3zwr1/AMD-NR---OptiScaler'
            package = 'amdnr'; forwarder = 'forwarder'
            runtimeRequired = $false
            runtimeNote = 'Recommended on every GPU. On RX 7000 and RX 9000, AMDNR can also use its built-in lmxxf runtime.'
            runtimes = $RuntimeChoices
        },
        [ordered]@{
            id = 'theautomatic'; name = 'OptiScaler AMD pre-SR'; author = 'TheAutomatic'
            summary = "TheAutomatic's multi-slot pre-SR build of OptiScaler for AMD."
            homepage = 'https://github.com/TheAutomatic/dlss-5-amd-project'
            package = 'theautomatic'; forwarder = $null
            runtimeRequired = $true
            runtimeNote = "Required: this build does not include danielblnc's runtime, and its lmxxf backend needs model files that are not in its release."
            iniWhenRuntime = $TheAutomaticIniWhenRuntime
            runtimes = $RuntimeChoices
        }
    )
    # lmxxf runs on RDNA 3 since 0.3.3, so LmxxfNrRuntime.pak is needed on RX 7000 as well: AMDNR
    # excludes nothing, whatever the GPU.
    packages = @(
        [ordered]@{ id = 'amdnr'; version = $amdnr.Version; url = $amdnr.Url; sha256 = $amdnr.Sha256; size = $amdnr.Size; exclude = @() },
        [ordered]@{ id = 'theautomatic'; version = $theAutomatic.Version; url = $theAutomatic.Url; sha256 = $theAutomatic.Sha256; size = $theAutomatic.Size; exclude = $TheAutomaticExclude }
    ) + $runtimePackages + @(
        [ordered]@{ id = 'forwarder'; version = $ForwarderVersion; url = $forwarder.Url; sha256 = $forwarder.Sha256; size = $forwarder.Size }
    )
    # Nothing new reads compatibility, but a launcher model that still declares it as a
    # non-nullable list would see null for a missing key; an empty list is safe either way.
    compatibility = @()
    proxyDefaults = $ProxyDefaults
    proxyOverrides = $ProxyOverrides
    hiddenSteamAppIds = $HiddenSteamAppIds
}

# Written beside the target and validated before it replaces anything, so a failed run never
# leaves a half-made manifest.json behind. UTF-8 without a BOM: Set-Content -Encoding UTF8 in
# 5.1 adds one.
$temp = "$OutFile.tmp"
[IO.File]::WriteAllText($temp, (ConvertTo-ManifestJson $manifest) + "`n", (New-Object System.Text.UTF8Encoding $false))
try { Assert-Manifest $temp ([bool]$SkipLauncher) }
catch { Remove-Item -LiteralPath $temp -ErrorAction SilentlyContinue; throw }
Move-Item -LiteralPath $temp -Destination $OutFile -Force

Write-Host "Wrote $OutFile"
foreach ($p in $manifest.packages) {
    Write-Host ('  {0,-13} {1,-12} {2,12:N0} B  {3}' -f $p.id, $p.version, $p.size, $p.sha256)
}
Write-Host ('  {0,-13} {1,-12} {2,14}  {3}' -f 'launcher', $LauncherVersion, '', $launcherSha256)
if ($SkipLauncher) {
    $also = if ($ForwarderUrl) { '' } else { " and $ForwarderAssetName" }
    Write-Warning "Not publishable yet: upload $LauncherAssetName$also to release '$LauncherTag', then run again without -SkipLauncher."
}
