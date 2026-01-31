param(
    [string]$Solution = "GameEngine.sln",
    [string]$Output = "scripts/reports/inspectcode.xml"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Find-InspectCode {
    if ($env:RIDERCMD -and (Test-Path $env:RIDERCMD)) { return $env:RIDERCMD }
    $candidates = @(
        "$env:LOCALAPPDATA\JetBrains\Toolbox\apps\Rider\*\bin\inspectcode.exe",
        "$env:ProgramFiles\JetBrains\Rider\bin\inspectcode.exe",
        "$env:ProgramFiles(x86)\JetBrains\Rider\bin\inspectcode.exe"
    )
    foreach ($pattern in $candidates) {
        $path = Get-ChildItem -Path $pattern -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 1
        if ($path) { return $path.FullName }
    }
    return $null
}

$tool = Find-InspectCode
if (-not $tool) {
    Write-Error "InspectCode.exe not found. Set RIDERCMD to full path of inspectcode.exe."
}

Write-Host "Running InspectCode on $Solution ..."
& $tool $Solution /o=$Output /no-build /swea /disable-settings-layers JetBrains.ReSharper.Settings.Ide /profile=Default /caches-home="$(Join-Path $PSScriptRoot '..' '.idea' 'resharper-caches')"
Write-Host "InspectCode report -> $Output"
