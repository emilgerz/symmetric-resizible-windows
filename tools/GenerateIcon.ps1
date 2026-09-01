param(
    [Parameter(Mandatory = $false)]
    [string]$SourceSvg = "$PSScriptRoot\..\assets\cross-cursor.svg",
    [Parameter(Mandatory = $false)]
    [string]$OutputIco = "$PSScriptRoot\..\assets\ResizeSymmetrically.ico"
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$svg = Get-Content -Raw -LiteralPath $SourceSvg
$pointsMatch = [regex]::Match($svg, 'points="([^"]+)"')
if (-not $pointsMatch.Success) {
    throw 'The SVG polygon points were not found.'
}
$colorMatch = [regex]::Match($svg, 'fill:\s*(#[0-9a-fA-F]{6})')
$color = if ($colorMatch.Success) {
    [System.Drawing.ColorTranslator]::FromHtml($colorMatch.Groups[1].Value)
} else {
    [System.Drawing.Color]::FromArgb(255, 136, 0)
}

$numbers = [regex]::Matches($pointsMatch.Groups[1].Value, '-?\d+(?:\.\d+)?') |
    ForEach-Object { [double]::Parse($_.Value, [Globalization.CultureInfo]::InvariantCulture) }
if (($numbers.Count % 2) -ne 0) {
    throw 'The SVG polygon has an invalid point count.'
}

$sizes = @(16, 20, 24, 32, 40, 48, 64, 128, 256)
$images = [System.Collections.Generic.List[byte[]]]::new()

foreach ($size in $sizes) {
    $bitmap = [System.Drawing.Bitmap]::new($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.Clear([System.Drawing.Color]::Transparent)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality

    $padding = [Math]::Max(1.0, $size * 0.035)
    $scale = ($size - 2.0 * $padding) / 512.0
    $points = [System.Collections.Generic.List[System.Drawing.PointF]]::new()
    for ($index = 0; $index -lt $numbers.Count; $index += 2) {
        $points.Add([System.Drawing.PointF]::new(
            [single]($padding + $numbers[$index] * $scale),
            [single]($padding + $numbers[$index + 1] * $scale)))
    }
    $brush = [System.Drawing.SolidBrush]::new($color)
    $graphics.FillPolygon($brush, $points.ToArray())

    $stream = [System.IO.MemoryStream]::new()
    $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
    $images.Add($stream.ToArray())
    $stream.Dispose()
    $brush.Dispose()
    $graphics.Dispose()
    $bitmap.Dispose()
}

$outputDirectory = Split-Path -Parent $OutputIco
[System.IO.Directory]::CreateDirectory($outputDirectory) | Out-Null
$file = [System.IO.File]::Open($OutputIco, [System.IO.FileMode]::Create)
$writer = [System.IO.BinaryWriter]::new($file)
$writer.Write([uint16]0)
$writer.Write([uint16]1)
$writer.Write([uint16]$sizes.Count)
$offset = 6 + 16 * $sizes.Count
for ($index = 0; $index -lt $sizes.Count; $index++) {
    $size = $sizes[$index]
    $dimensionByte = if ($size -eq 256) { [byte]0 } else { [byte]$size }
    $writer.Write($dimensionByte)
    $writer.Write($dimensionByte)
    $writer.Write([byte]0)
    $writer.Write([byte]0)
    $writer.Write([uint16]1)
    $writer.Write([uint16]32)
    $writer.Write([uint32]$images[$index].Length)
    $writer.Write([uint32]$offset)
    $offset += $images[$index].Length
}
foreach ($image in $images) {
    $writer.Write($image)
}
$writer.Dispose()
$file.Dispose()

Write-Host "Generated $OutputIco"
