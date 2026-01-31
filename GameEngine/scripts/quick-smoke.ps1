param(
    [switch]$NoClangTidy,
    [switch]$NoCppcheck,
    [switch]$NoInspect
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$reports = Join-Path $PSScriptRoot 'reports'
if (-not (Test-Path $reports)) { New-Item -ItemType Directory -Path $reports | Out-Null }

$summary = @()

if (-not $NoCppcheck) {
    $cpp = Get-Command cppcheck -ErrorAction SilentlyContinue
    if ($cpp) {
        $out = Join-Path $reports 'quick-cppcheck.txt'
        & $cpp --enable=warning,performance --template=gcc --quiet --inline-suppr GameEngineCore 2> $out
        $summary += "cppcheck: $(Get-Content $out | Measure-Object -Line).Lines lines logged -> $out"
    } else {
        $summary += "cppcheck: not found"
    }
}

if (-not $NoClangTidy) {
    $ct = Get-Command clang-tidy -ErrorAction SilentlyContinue
    $cc = Join-Path $PSScriptRoot '..' 'compile_commands.json'
    if ($ct -and (Test-Path $cc)) {
        $out = Join-Path $reports 'quick-clang-tidy.txt'
        $targets = @('GameEngineCore/GameEngine.cpp','GameEngineCore/RenderPipeline2D/Renderer2D.cpp')
        $log = foreach ($t in $targets) { & $ct --quiet -p $cc $t 2>&1 }
        $log | Set-Content -Path $out -Encoding ASCII
        $summary += "clang-tidy: $(($log | Measure-Object -Line).Lines) lines logged -> $out"
    } else {
        $summary += "clang-tidy: missing tool or compile_commands.json"
    }
}

if (-not $NoInspect) {
    $inspect = $null
    if ($env:RIDERCMD -and (Test-Path $env:RIDERCMD)) { $inspect = $env:RIDERCMD }
    else {
        $cand = Get-ChildItem "$env:LOCALAPPDATA\JetBrains\Toolbox\apps\Rider\*\bin\inspectcode.exe" -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 1
        if ($cand) { $inspect = $cand.FullName }
    }
    if ($inspect) {
        $out = Join-Path $reports 'quick-inspect.xml'
        & $inspect GameEngine.sln /no-build /swea /o=$out /profile=Default /disable-settings-layers JetBrains.ReSharper.Settings.Ide /caches-home="$(Join-Path $PSScriptRoot '..' '.idea' 'resharper-caches')" | Out-Null
        $summary += "inspectcode: wrote $out"
    } else {
        $summary += "inspectcode: not found (set RIDERCMD)"
    }
}

$summary | Set-Content (Join-Path $reports 'quick-summary.txt') -Encoding ASCII
Write-Host ($summary -join "`n")
