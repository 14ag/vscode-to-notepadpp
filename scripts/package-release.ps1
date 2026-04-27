param(
    [ValidateSet("x86", "x64", "arm64", "all")]
    [string]$Platform = "x64",
    [string]$Configuration = "Release",
    [string]$BuildDir = "build",
    [string]$DistDir = "dist",
    [string]$Version,
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot

function Get-Version {
    if ($Version) {
        return $Version
    }

    $cmakeText = Get-Content -Raw (Join-Path $repoRoot "CMakeLists.txt")
    $match = [regex]::Match($cmakeText, 'project\(VSCodeKeymapNpp VERSION ([0-9]+(?:\.[0-9]+){1,3})')
    if (-not $match.Success) {
        throw "Could not determine project version from CMakeLists.txt."
    }

    return $match.Groups[1].Value
}

function Get-PlatformSpec {
    param([string]$Name)

    switch ($Name) {
        "x86" {
            return @{
                Name = "x86"
                GeneratorPlatform = "Win32"
                Toolset = $null
            }
        }
        "x64" {
            return @{
                Name = "x64"
                GeneratorPlatform = "x64"
                Toolset = $null
            }
        }
        "arm64" {
            return @{
                Name = "arm64"
                GeneratorPlatform = "ARM64"
                Toolset = "v141"
            }
        }
        default {
            throw "Unsupported platform '$Name'."
        }
    }
}

function Invoke-CMake {
    param(
        [string[]]$Arguments,
        [string]$FailureMessage
    )

    & cmake @Arguments | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw $FailureMessage
    }
}

function Build-Platform {
    param([hashtable]$Spec)

    $buildDirName = "{0}-{1}" -f $BuildDir, $Spec.Name
    if ($Spec.Toolset) {
        $buildDirName += "-$($Spec.Toolset)"
    }
    $buildDir = Join-Path $repoRoot $buildDirName

    if ($SkipBuild) {
        return $buildDir
    }

    $configureFailure = "CMake configure failed for {0}." -f $Spec.Name
    if ($Spec.Name -eq "arm64") {
        $configureFailure += " Install Visual Studio C++ ARM64 build tools/components, then rerun."
    }

    $configureArgs = @("-S", $repoRoot, "-B", $buildDir, "-A", $Spec.GeneratorPlatform)
    if ($Spec.Toolset) {
        $configureArgs += @("-T", $Spec.Toolset)
    }

    Invoke-CMake `
        -Arguments $configureArgs `
        -FailureMessage $configureFailure

    $buildFailure = "Build failed for {0}." -f $Spec.Name
    if ($Spec.Name -eq "arm64") {
        $buildFailure += " Install Visual Studio C++ ARM64 build tools/components, then rerun."
    }

    Invoke-CMake `
        -Arguments @("--build", $buildDir, "--config", $Configuration) `
        -FailureMessage $buildFailure

    return $buildDir
}

function Copy-Docs {
    param([string]$Destination)

    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $repoRoot "README.MD") -Destination (Join-Path $Destination "README.MD")

    $docsDir = Join-Path $repoRoot "docs"
    if (Test-Path -LiteralPath $docsDir) {
        Get-ChildItem -LiteralPath $docsDir -File -Filter *.MD | ForEach-Object {
            Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $Destination $_.Name)
        }
    }
}

function Package-Platform {
    param(
        [hashtable]$Spec,
        [string]$BuildDirPath,
        [string]$ResolvedVersion,
        [string]$DistRoot
    )

    $dllPath = Join-Path $BuildDirPath (Join-Path $Configuration "VSCodeKeymapNpp.dll")
    if (-not (Test-Path -LiteralPath $dllPath)) {
        throw "Build output not found for $($Spec.Name): $dllPath"
    }

    $stageRoot = Join-Path $DistRoot ("_package-{0}" -f $Spec.Name)
    $docDir = Join-Path $stageRoot "doc"
    $zipPath = Join-Path $DistRoot ("VSCodeKeymapNpp-{0}-{1}.zip" -f $ResolvedVersion, $Spec.Name)

    if (Test-Path -LiteralPath $stageRoot) {
        Remove-Item -LiteralPath $stageRoot -Recurse -Force
    }
    if (Test-Path -LiteralPath $zipPath) {
        Remove-Item -LiteralPath $zipPath -Force
    }

    New-Item -ItemType Directory -Path $stageRoot -Force | Out-Null
    Copy-Item -LiteralPath $dllPath -Destination (Join-Path $stageRoot "VSCodeKeymapNpp.dll")
    Copy-Docs -Destination $docDir

    Compress-Archive `
        -Path (Join-Path $stageRoot "*") `
        -DestinationPath $zipPath `
        -CompressionLevel Optimal

    Remove-Item -LiteralPath $stageRoot -Recurse -Force
    return $zipPath
}

$resolvedVersion = Get-Version
$targets = @()
if ($Platform -eq "all") {
    $targets = @("x86", "x64", "arm64")
}
else {
    $targets = @($Platform)
}

$distRoot = Join-Path $repoRoot $DistDir
New-Item -ItemType Directory -Path $distRoot -Force | Out-Null

$artifacts = New-Object System.Collections.Generic.List[string]
foreach ($targetName in $targets) {
    $spec = Get-PlatformSpec -Name $targetName
    $buildDirPath = Build-Platform -Spec $spec
    $artifacts.Add((Package-Platform -Spec $spec -BuildDirPath $buildDirPath -ResolvedVersion $resolvedVersion -DistRoot $distRoot)) | Out-Null
}

$artifacts | ForEach-Object { Write-Output $_ }
