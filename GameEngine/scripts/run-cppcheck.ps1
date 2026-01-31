param(
    [string]$Output = "scripts/reports/cppcheck.xml"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$candidates = @("cppcheck", "C:\\Program Files\\Cppcheck\\cppcheck.exe")
$cppcheck = $null
foreach ($c in $candidates) {
    if (Get-Command $c -ErrorAction SilentlyContinue) { $cppcheck = $c; break }
}
if (-not $cppcheck) {
    Write-Error "cppcheck not found. Install cppcheck and re-run."
}

$includes = @(
    "GameEngineCore",
    "tests/GameEngine.Test"
)
$excludes = @(".idea", "x64", "scripts/reports")

$excludeArgs = $excludes | ForEach-Object { "-i" + $_ }
$includeArgs = $includes

$cmd = @($cppcheck,
    "--enable=warning,performance,style",
    "--inline-suppr",
    "--std=c++17",
    "--xml",
    "--output-file=$Output",
    "--force",
    "-j", [Environment]::ProcessorCount)
$cmd += $excludeArgs
$cmd += $includeArgs

Write-Host "Running cppcheck ..."
& $cmd
Write-Host "cppcheck report -> $Output"
