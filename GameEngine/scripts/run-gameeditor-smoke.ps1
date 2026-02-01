param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [int]$Seconds = 6
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$msbuild = Get-Command msbuild -ErrorAction SilentlyContinue
if (-not $msbuild) {
    Write-Error "msbuild not found. Run from a Developer Command Prompt or add VS Build Tools to PATH."
}

$project = "GameEditor/GameEditor.vcxproj"
$arguments = @(
    $project,
    "/m",
    "/p:Configuration=$Configuration",
    "/p:Platform=$Platform"
)

Write-Host "Building GameEditor $Configuration|$Platform ..."
& $msbuild $arguments

$exe = Join-Path "GameEditor" ("$Platform/$Configuration/GameEditor.exe")
if (-not (Test-Path $exe)) {
    Write-Error "GameEditor executable not found at $exe"
}

Write-Host "Launching GameEditor smoke ($Seconds seconds)..."
& $exe "--smoke" "--seconds=$Seconds"
