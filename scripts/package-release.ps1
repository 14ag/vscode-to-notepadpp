param(
    [string]$Configuration = "Release",
    [string]$Platform = "x64",
    [string]$BuildDir = "build",
    [string]$DistDir = "dist",
    [string]$Version
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot

if (-not $Version) {
    $cmakeText = Get-Content -Raw (Join-Path $repoRoot "CMakeLists.txt")
    $match = [regex]::Match($cmakeText, 'project\(VSCodeKeymapNpp VERSION ([0-9]+(?:\.[0-9]+){1,3})')
    if (-not $match.Success) {
        throw "Could not determine project version from CMakeLists.txt."
    }

    $Version = $match.Groups[1].Value
}

$dllPath = Join-Path $repoRoot (Join-Path $BuildDir (Join-Path $Configuration "VSCodeKeymapNpp.dll"))
if (-not (Test-Path -LiteralPath $dllPath)) {
    throw "Build output not found: $dllPath"
}

$distRoot = Join-Path $repoRoot $DistDir
$stageRoot = Join-Path $distRoot "_package"
$docDir = Join-Path $stageRoot "doc"
$zipPath = Join-Path $distRoot ("VSCodeKeymapNpp-{0}-{1}.zip" -f $Version, $Platform)

if (Test-Path -LiteralPath $distRoot) {
    Remove-Item -LiteralPath $distRoot -Recurse -Force
}

New-Item -ItemType Directory -Path $docDir -Force | Out-Null
Copy-Item -LiteralPath $dllPath -Destination (Join-Path $stageRoot "VSCodeKeymapNpp.dll")
Copy-Item -LiteralPath (Join-Path $repoRoot "README.MD") -Destination (Join-Path $docDir "README.MD")

Compress-Archive `
    -Path (Join-Path $stageRoot "*") `
    -DestinationPath $zipPath `
    -CompressionLevel Optimal

Remove-Item -LiteralPath $stageRoot -Recurse -Force

Write-Output $zipPath
