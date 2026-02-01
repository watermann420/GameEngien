# GameEditor.Test

Purpose: Manual smoke harness for GameEditor UI + GameEngine runtime checks.
This is not an automated test suite yet; it provides repeatable launch scripts and a checklist.

What you can test here
- UI: editor window opens, panels, drag/drop, menus, rename, console, splitters, zoom
- Game world: GameEngine runs with a project
- Audio: basic playback (if media exists)
- Performance: timed runs + log output

Run
1) Editor UI smoke:
   `powershell -ExecutionPolicy Bypass -File tests/GameEditor.Test/run-editor-tests.ps1 -Editor`
2) Engine smoke (project):
   `powershell -ExecutionPolicy Bypass -File tests/GameEditor.Test/run-editor-tests.ps1 -Engine`
3) Full pass:
   `powershell -ExecutionPolicy Bypass -File tests/GameEditor.Test/run-editor-tests.ps1 -All`

Extra options
- `-Audio` : check for media in project Files/ (beep fallback)
- `-Perf` : logs launch times to tests/GameEditor.Test/artifacts/perf.log
- `-Screenshot` : captures a screen image to tests/GameEditor.Test/artifacts/editor_screenshot.png

Notes
- Editor uses external project path by default: %USERPROFILE%\GameEngineProjects\StarterProject
- Engine supports: `--project <path>` (see App/ProjectConfig.cpp)
