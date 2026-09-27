# Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.

<#
.SYNOPSIS
Draws the launcher's icon and packs it as a real multi-image .ico.

.DESCRIPTION
The mark is the owner's: a dark rounded plate, AMD in white with the arrow, NR in the accent
red. Every size is drawn on its own rather than scaled down from one picture, so the small ones
stay sharp; at 16 and 24 px two lines of letters would be a smudge, so those two carry NR alone.

The sizes are the ones Windows asks for: 16 and 24 in lists and the title bar, 32 on the
taskbar, 48 and 64 in Explorer's icon views, 128 and 256 for the largest tiles and for scaling
between them. Every size below 256 is written as a 32-bit DIB with its mask, the form every
version of Windows reads; 256 is written as PNG, the form the format requires at that size.

The result is checked in as src\AmdnrLauncher.App\Assets\app.ico and named by <ApplicationIcon>
in the App csproj, which writes it into the exe. A test parses the file's directory, and another
asks the built assembly for its icon group.

The arrow is the two polygons Chrome.xaml's AmdArrowMark draws, on the same 100 x 100 box; the
colours are Palette.xaml's. Change them there and here together.

Windows PowerShell 5.1 and System.Drawing only, kept to ASCII: 5.1 reads a script without a BOM
in the system code page, so any other character would arrive garbled.

.PARAMETER OutFile
Where to write the .ico. Defaults to src\AmdnrLauncher.App\Assets\app.ico, found from this
script's own folder.

.PARAMETER PreviewDir
When given, each size is also written there as icon-<size>.png, to look at.

.EXAMPLE
powershell -ExecutionPolicy Bypass -File tools\make-icon.ps1 -PreviewDir $env:TEMP\amdnr-icon
#>
[CmdletBinding()]
param(
    [string]$OutFile,
    [string]$PreviewDir
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
Add-Type -AssemblyName System.Drawing

if (-not $OutFile) { $OutFile = Join-Path $PSScriptRoot '..\src\AmdnrLauncher.App\Assets\app.ico' }
if (-not [System.IO.Path]::IsPathRooted($OutFile)) { $OutFile = Join-Path (Get-Location).Path $OutFile }
$OutFile = [System.IO.Path]::GetFullPath($OutFile)

$Sizes = @(16, 24, 32, 48, 64, 128, 256)

# Palette.xaml: the raised and base surfaces for the plate, BorderStrong for its edge, the
# brand ink and the accent.
$PlateTop    = [System.Drawing.Color]::FromArgb(255, 0x22, 0x27, 0x32)
$PlateBottom = [System.Drawing.Color]::FromArgb(255, 0x13, 0x16, 0x1C)
$PlateEdge   = [System.Drawing.Color]::FromArgb(255, 0x33, 0x3B, 0x47)
$Ink         = [System.Drawing.Color]::FromArgb(255, 0xFF, 0xFF, 0xFF)
$Accent      = [System.Drawing.Color]::FromArgb(255, 0xE2, 0x3E, 0x2C)

function Get-BrandFontFamily {
    # Bahnschrift at full weight is the wordmark's face in the app (Palette.xaml, BrandFont), and
    # the same fallbacks follow it. GDI+ would quietly substitute a default face for a name it
    # does not know, so the choice is made here, where it can be seen.
    $installed = New-Object System.Drawing.Text.InstalledFontCollection
    foreach ($name in @('Bahnschrift', 'Segoe UI Black', 'Segoe UI', 'Arial')) {
        $family = $installed.Families | Where-Object { $_.Name -eq $name } | Select-Object -First 1
        if ($family) { return $family }
    }
    return [System.Drawing.FontFamily]::GenericSansSerif
}

function New-RoundedRectanglePath([single]$x, [single]$y, [single]$w, [single]$h, [single]$radius) {
    $d = $radius * 2
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $path.AddArc($x, $y, $d, $d, 180, 90)
    $path.AddArc($x + $w - $d, $y, $d, $d, 270, 90)
    $path.AddArc($x + $w - $d, $y + $h - $d, $d, $d, 0, 90)
    $path.AddArc($x, $y + $h - $d, $d, $d, 90, 90)
    $path.CloseFigure()
    return $path
}

function New-TextPath([string]$text, [System.Drawing.FontFamily]$family) {
    # An outline rather than DrawString, so it can be scaled to fill a box exactly: AMD and NR
    # then take the same width at every size, whatever the face's own metrics.
    # Winding, not the default alternate fill: Bahnschrift is a variable font whose outlines
    # overlap where strokes meet, and alternate fill leaves a slit through every overlap.
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath([System.Drawing.Drawing2D.FillMode]::Winding)
    $path.AddString($text, $family, [int][System.Drawing.FontStyle]::Bold, 100,
        [System.Drawing.PointF]::Empty, [System.Drawing.StringFormat]::GenericTypographic)
    return $path
}

function Move-PathIntoBox([System.Drawing.Drawing2D.GraphicsPath]$path, [System.Drawing.RectangleF]$box) {
    # Scale uniformly to fit the box, then centre in it.
    $bounds = $path.GetBounds()
    $scale = [math]::Min($box.Width / $bounds.Width, $box.Height / $bounds.Height)
    $matrix = New-Object System.Drawing.Drawing2D.Matrix
    $matrix.Translate(-$bounds.X, -$bounds.Y, [System.Drawing.Drawing2D.MatrixOrder]::Append)
    $matrix.Scale($scale, $scale, [System.Drawing.Drawing2D.MatrixOrder]::Append)
    $matrix.Translate($box.X + ($box.Width - $bounds.Width * $scale) / 2,
                      $box.Y + ($box.Height - $bounds.Height * $scale) / 2,
                      [System.Drawing.Drawing2D.MatrixOrder]::Append)
    $path.Transform($matrix)
}

function New-ArrowPath([System.Drawing.RectangleF]$box) {
    # Chrome.xaml's AmdArrowMark: the right and bottom bars of a square with their ends cut at
    # 45 degrees, and the top-left corner piece cut the same way, so the gap between the two is
    # the arrow, pointing up and left.
    $unit = $box.Width / 100
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $frame  = @(100, 0,   100, 100,   0, 100,   25, 75,   75, 75,   75, 25)
    $corner = @(0, 0,     75, 0,      50, 25,   25, 25,   25, 50,   0, 75)
    foreach ($polygon in @($frame, $corner)) {
        $points = New-Object 'System.Drawing.PointF[]' ($polygon.Count / 2)
        for ($i = 0; $i -lt $points.Count; $i++) {
            $points[$i] = New-Object System.Drawing.PointF(
                ($box.X + $polygon[2 * $i] * $unit), ($box.Y + $polygon[2 * $i + 1] * $unit))
        }
        $path.AddPolygon($points)
    }
    return $path
}

function New-IconBitmap([int]$size, [System.Drawing.FontFamily]$family) {
    $bitmap = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
        $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
        $g.Clear([System.Drawing.Color]::Transparent)
        $s = [single]$size

        # The plate: a rounded square with a hairline edge, so it keeps its shape on the dark
        # taskbar as well as on Explorer's white.
        $edge = [single][math]::Max(1, $size / 64)
        $plate = New-RoundedRectanglePath ($edge / 2) ($edge / 2) ($s - $edge) ($s - $edge) ($s * 0.2)
        $fill = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
            (New-Object System.Drawing.PointF(0, 0)), (New-Object System.Drawing.PointF(0, $s)), $PlateTop, $PlateBottom)
        $g.FillPath($fill, $plate)
        $g.DrawPath((New-Object System.Drawing.Pen($PlateEdge, $edge)), $plate)

        $inkBrush = New-Object System.Drawing.SolidBrush($Ink)
        $accentBrush = New-Object System.Drawing.SolidBrush($Accent)

        if ($size -lt 32) {
            # NR alone: at these sizes a second line of letters is a smudge.
            $nr = New-TextPath 'NR' $family
            Move-PathIntoBox $nr (New-Object System.Drawing.RectangleF(($s * 0.14), ($s * 0.20), ($s * 0.72), ($s * 0.60)))
            $g.FillPath($accentBrush, $nr)
        }
        else {
            # The wordmark stacked: AMD with the arrow on the upper line, NR larger beneath.
            $amd = New-TextPath 'AMD' $family
            Move-PathIntoBox $amd (New-Object System.Drawing.RectangleF(($s * 0.16), ($s * 0.19), ($s * 0.46), ($s * 0.23)))
            $amdBounds = $amd.GetBounds()
            $gap = $amdBounds.Height * 0.24
            $arrow = New-ArrowPath (New-Object System.Drawing.RectangleF(
                ($amdBounds.Right + $gap), $amdBounds.Y, $amdBounds.Height, $amdBounds.Height))

            # The pair together, centred on the plate.
            $groupWidth = $amdBounds.Width + $gap + $amdBounds.Height
            $shift = New-Object System.Drawing.Drawing2D.Matrix
            $shift.Translate((($s - $groupWidth) / 2 - $amdBounds.X), 0)
            $amd.Transform($shift)
            $arrow.Transform($shift)
            $g.FillPath($inkBrush, $amd)
            $g.FillPath($inkBrush, $arrow)

            $nr = New-TextPath 'NR' $family
            Move-PathIntoBox $nr (New-Object System.Drawing.RectangleF(($s * 0.16), ($s * 0.49), ($s * 0.68), ($s * 0.33)))
            $g.FillPath($accentBrush, $nr)
        }
    }
    finally {
        $g.Dispose()
    }
    return $bitmap
}

function ConvertTo-IconDib([System.Drawing.Bitmap]$bitmap) {
    # A 32-bit DIB as an icon holds it: a BITMAPINFOHEADER with the height doubled, the colour
    # rows bottom-up, then the one-bit mask, rows padded to four bytes, set where the pixel is
    # fully transparent for readers that ignore the alpha channel.
    $w = $bitmap.Width
    $h = $bitmap.Height
    $rowBytes = $w * 4
    $pixels = New-Object byte[] ($rowBytes * $h)

    $rect = New-Object System.Drawing.Rectangle(0, 0, $w, $h)
    $data = $bitmap.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        for ($y = 0; $y -lt $h; $y++) {
            $source = [IntPtr]::Add($data.Scan0, $y * $data.Stride)
            [System.Runtime.InteropServices.Marshal]::Copy($source, $pixels, ($h - 1 - $y) * $rowBytes, $rowBytes)
        }
    }
    finally {
        $bitmap.UnlockBits($data)
    }

    $maskStride = [int]([math]::Floor(($w + 31) / 32) * 4)
    $mask = New-Object byte[] ($maskStride * $h)
    for ($row = 0; $row -lt $h; $row++) {
        for ($x = 0; $x -lt $w; $x++) {
            if ($pixels[$row * $rowBytes + $x * 4 + 3] -eq 0) {
                $index = $row * $maskStride + ($x -shr 3)
                $mask[$index] = [byte]($mask[$index] -bor (0x80 -shr ($x -band 7)))
            }
        }
    }

    $stream = New-Object System.IO.MemoryStream
    $writer = New-Object System.IO.BinaryWriter($stream)
    $writer.Write([int32]40)
    $writer.Write([int32]$w)
    $writer.Write([int32]($h * 2))
    $writer.Write([int16]1)
    $writer.Write([int16]32)
    $writer.Write([int32]0)
    $writer.Write([int32]($pixels.Length + $mask.Length))
    $writer.Write([int32]0)
    $writer.Write([int32]0)
    $writer.Write([int32]0)
    $writer.Write([int32]0)
    $writer.Write($pixels)
    $writer.Write($mask)
    $writer.Flush()
    return ,$stream.ToArray()
}

function Get-PngBytes([System.Drawing.Bitmap]$bitmap) {
    $stream = New-Object System.IO.MemoryStream
    $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
    return ,$stream.ToArray()
}

$family = Get-BrandFontFamily
Write-Host "Face: $($family.Name)"

if ($PreviewDir -and -not (Test-Path $PreviewDir)) { New-Item -ItemType Directory -Path $PreviewDir | Out-Null }

$images = @()
foreach ($size in $Sizes) {
    $bitmap = New-IconBitmap $size $family
    try {
        if ($PreviewDir) { $bitmap.Save((Join-Path $PreviewDir "icon-$size.png"), [System.Drawing.Imaging.ImageFormat]::Png) }
        [byte[]]$bytes = if ($size -ge 256) { Get-PngBytes $bitmap } else { ConvertTo-IconDib $bitmap }
        $images += @{ Size = $size; Bytes = $bytes }
    }
    finally {
        $bitmap.Dispose()
    }
}

# ICONDIR, then one ICONDIRENTRY per image, then the images in the same order.
$stream = New-Object System.IO.MemoryStream
$writer = New-Object System.IO.BinaryWriter($stream)
$writer.Write([uint16]0)
$writer.Write([uint16]1)
$writer.Write([uint16]$images.Count)
$offset = 6 + 16 * $images.Count
foreach ($image in $images) {
    $side = if ($image.Size -ge 256) { 0 } else { $image.Size }   # 0 stands for 256
    $writer.Write([byte]$side)
    $writer.Write([byte]$side)
    $writer.Write([byte]0)          # colours: none for 32-bit
    $writer.Write([byte]0)          # reserved
    $writer.Write([uint16]1)        # planes
    $writer.Write([uint16]32)       # bits per pixel
    $writer.Write([uint32]$image.Bytes.Length)
    $writer.Write([uint32]$offset)
    $offset += $image.Bytes.Length
}
foreach ($image in $images) { $writer.Write([byte[]]$image.Bytes) }
$writer.Flush()

$folder = Split-Path -Parent $OutFile
if (-not (Test-Path $folder)) { New-Item -ItemType Directory -Path $folder | Out-Null }
[System.IO.File]::WriteAllBytes($OutFile, $stream.ToArray())

Write-Host ("Wrote {0}: {1} images ({2}), {3:N0} bytes" -f $OutFile, $images.Count, ($Sizes -join ', '), $stream.Length)
