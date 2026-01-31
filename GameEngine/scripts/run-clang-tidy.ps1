param(
    [string]$CompileCommands = "compile_commands.json",
    [string]$Output = "scripts/reports/clang-tidy.txt"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not (Test-Path $CompileCommands)) {
    Write-Error "compile_commands.json not found. Generate one (e.g., CMake or vswhere + Ninja) and re-run."
}

$clangTidy = Get-Command clang-tidy -ErrorAction SilentlyContinue
if (-not $clangTidy) {
    Write-Error "clang-tidy not found. Install LLVM/clang tools."
}

$targets = @(
    "GameEngineCore/GameEngine.cpp",
    "GameEngineCore/RenderPipeline2D/Renderer2D.cpp"
)

Write-Host "Running clang-tidy on targets..."
$results = @()
foreach ($t in $targets) {
    $r = & $clangTidy --quiet -p $CompileCommands $t 2>&1
    $results += $r
    $results += "----"
}
$results | Set-Content -Path $Output -Encoding ASCII
Write-Host "clang-tidy log -> $Output"
