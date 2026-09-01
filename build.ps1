param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [switch]$SkipTests,
    [switch]$Installer
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot

function Invoke-CleanProcess {
    param(
        [Parameter(Mandatory = $true)][string]$FileName,
        [Parameter(Mandatory = $false)][string[]]$Arguments = @()
    )
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FileName
    $startInfo.UseShellExecute = $false
    foreach ($argument in $Arguments) {
        $startInfo.ArgumentList.Add($argument)
    }
    # Some desktop hosts expose both Path and PATH. MSBuild 18 rejects that
    # environment before launching cl.exe, so pass a normalized environment.
    $startInfo.Environment.Clear()
    foreach ($item in Get-ChildItem Env:) {
        if (-not $startInfo.Environment.ContainsKey($item.Name)) {
            $startInfo.Environment.Add($item.Name, $item.Value)
        }
    }
    $process = [System.Diagnostics.Process]::Start($startInfo)
    $process.WaitForExit()
    return $process.ExitCode
}

if (-not (Test-Path -LiteralPath "$root\assets\ResizeSymmetrically.ico")) {
    & "$root\tools\GenerateIcon.ps1"
}

$vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw 'Visual Studio Build Tools were not found.'
}
$installation = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) {
    throw 'The MSVC x64 toolchain was not found.'
}
$msbuild = Join-Path $installation 'MSBuild\Current\Bin\MSBuild.exe'

if (-not $SkipTests) {
    $exitCode = Invoke-CleanProcess $msbuild @("$root\GeometryTests.vcxproj", '/m', "/p:Configuration=$Configuration", '/p:Platform=x64', '/p:TrackFileAccess=false', '/nologo')
    if ($exitCode -ne 0) { throw 'GeometryTests build failed.' }
    $exitCode = Invoke-CleanProcess "$root\bin\x64\$Configuration\GeometryTests.exe"
    if ($exitCode -ne 0) { throw 'Geometry tests failed.' }
    $exitCode = Invoke-CleanProcess $msbuild @("$root\TestWindow.vcxproj", '/m', "/p:Configuration=$Configuration", '/p:Platform=x64', '/p:TrackFileAccess=false', '/nologo')
    if ($exitCode -ne 0) { throw 'Integration test window build failed.' }
}

$exitCode = Invoke-CleanProcess $msbuild @("$root\ResizeSymmetrically.vcxproj", '/m', "/p:Configuration=$Configuration", '/p:Platform=x64', '/p:TrackFileAccess=false', '/nologo')
if ($exitCode -ne 0) { throw 'Application build failed.' }

if ($Installer) {
    $isccCandidates = @(
        (Get-Command ISCC.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue),
        'C:\Program Files (x86)\Inno Setup 6\ISCC.exe',
        'C:\Program Files\Inno Setup 6\ISCC.exe'
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }
    if (-not $isccCandidates) {
        throw 'Inno Setup 6 was not found. Install it or build without -Installer.'
    }
    $exitCode = Invoke-CleanProcess $isccCandidates[0] @("$root\installer\ResizeSymmetrically.iss")
    if ($exitCode -ne 0) { throw 'Installer build failed.' }
}

Write-Host "Build completed: $root\bin\x64\$Configuration\ResizeSymmetrically.exe"
