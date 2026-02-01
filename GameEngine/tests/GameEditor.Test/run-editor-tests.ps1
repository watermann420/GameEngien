param(
    [switch]$Editor,
    [switch]$Engine,
    [switch]$All,
    [switch]$Audio,
    [switch]$Perf,
    [switch]$Screenshot,
    [string]$Configuration = "Debug",
    [string]$Platform = "x64"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ($All) { $Editor = $true; $Engine = $true; $Audio = $true; $Perf = $true; $Screenshot = $true }

$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$editorExe = Join-Path $root "x64\$Configuration\GameEditor.exe"
$engineExe = Join-Path $root "x64\$Configuration\GameEngine.exe"
$project = Join-Path $env:USERPROFILE "GameEngineProjects\StarterProject"
$artifacts = Join-Path $PSScriptRoot "artifacts"
New-Item -ItemType Directory -Force -Path $artifacts | Out-Null

if ($Editor) {
    if (-not (Test-Path $editorExe)) {
        Write-Error "GameEditor.exe not found: $editorExe"
    }
    Write-Host "[editor] launching $editorExe"
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $editorProc = Start-Process -FilePath $editorExe -PassThru
    if ($Perf) {
        $sw.Stop()
        $line = "[perf] editor launch ms: $($sw.ElapsedMilliseconds)"
        $line | Out-File -FilePath (Join-Path $artifacts "perf.log") -Append -Encoding ASCII
    }
    if ($Screenshot) {
        Start-Sleep -Milliseconds 800
        Add-Type -AssemblyName System.Windows.Forms
        Add-Type -AssemblyName System.Drawing
        $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
        $bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $g.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
        $path = Join-Path $artifacts "editor_screenshot.png"
        $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
        $g.Dispose()
        $bmp.Dispose()
        Write-Host "[editor] screenshot -> $path"
    }
}

if ($Engine) {
    if (-not (Test-Path $engineExe)) {
        Write-Error "GameEngine.exe not found: $engineExe"
    }
    $args = @("--project", $project)
    Write-Host "[engine] launching $engineExe --project $project"
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $engineProc = Start-Process -FilePath $engineExe -ArgumentList $args -PassThru
    if ($Perf) {
        $sw.Stop()
        $line = "[perf] engine launch ms: $($sw.ElapsedMilliseconds)"
        $line | Out-File -FilePath (Join-Path $artifacts "perf.log") -Append -Encoding ASCII
    }
    if ($Audio) {
        $files = Join-Path $project "Files"
        $hasMedia = Test-Path (Join-Path $files "*.mp3") -or Test-Path (Join-Path $files "*.mp4")
        if ($hasMedia) {
            Write-Host "[audio] media found in project Files/. Engine should play audio if supported."
        }
        else {
            Write-Host "[audio] no media found. Beep fallback."
            [console]::beep(440, 300)
        }
    }
}

Write-Host "[done]"
