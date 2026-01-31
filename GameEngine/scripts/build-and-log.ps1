param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [string]$Output = "scripts/reports/build.log"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$msbuild = Get-Command msbuild -ErrorAction SilentlyContinue
if (-not $msbuild) {
    Write-Error "msbuild not found. Run from a Developer Command Prompt or add VS Build Tools to PATH."
}

$arguments = @(
    "GameEngine.sln",
    "/m",
    "/p:Configuration=$Configuration",
    "/p:Platform=$Platform"
)

Write-Host "Building $Configuration|$Platform ..."
& $msbuild $arguments 2>&1 | Tee-Object -FilePath $Output
Write-Host "Build log -> $Output"
