param(
    [string]$MusicEnginePath = "",
    [int]$Note = 60,
    [int]$Velocity = 100,
    [int]$DurationMs = 500
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
function Find-MusicEngineDll {
    param([string]$Override)
    if ($Override -and (Test-Path $Override)) { return (Resolve-Path $Override).Path }

    $candidates = @(
        "..\\MusicEngine\\bin\\Debug\\net10.0-windows\\MusicEngine.dll",
        "..\\MusicEngine\\bin\\Release\\net10.0-windows\\MusicEngine.dll",
        "..\\..\\MusicEngine\\bin\\Debug\\net10.0-windows\\MusicEngine.dll",
        "..\\..\\MusicEngine\\bin\\Release\\net10.0-windows\\MusicEngine.dll"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return (Resolve-Path $c).Path }
    }
    return $null
}
$dll = Find-MusicEngineDll -Override $MusicEnginePath
if (-not $dll) {
    Write-Warning "MusicEngine.dll not found. Provide -MusicEnginePath or build MusicEngine first."
    [console]::beep(440, $DurationMs)
    return
}

Write-Host "Loading MusicEngine: $dll"
$asm = [System.Reflection.Assembly]::LoadFrom($dll)
$type = $asm.GetType("MusicEngine.Core.Synthesizers.OrganSynth", $true)
$synth = [System.Activator]::CreateInstance($type)
$noteOn = $type.GetMethod("NoteOn", [Type[]]@([int], [int], [int]))
$noteOff = $type.GetMethod("NoteOff", [Type[]]@([int], [int]))

$noteOn.Invoke($synth, @([int]0, [int]$Note, [int]$Velocity)) | Out-Null
Start-Sleep -Milliseconds $DurationMs
$noteOff.Invoke($synth, @([int]0, [int]$Note)) | Out-Null

$dispose = $type.GetMethod("Dispose")
if ($dispose) {
    $dispose.Invoke($synth, $null) | Out-Null
}