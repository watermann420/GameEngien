
![image logo banner](https://github.com/user-attachments/assets/0fbe14bc-1648-47a8-94d6-97bbd80e8556)




# GameEngine
![Status](https://img.shields.io/badge/status-Work_in_Progress-orange)
[![Join Discord](https://img.shields.io/badge/Discord-Join%20Server-5865F2?logo=discord&logoColor=white)](https://discord.gg/tWkqHMsB6a)




Lightweight C++ prototype for a Win32 overlay/renderer with cross‑platform stubs. Ships with a headless renderer, simple physics helpers, and placeholders for future MusicEngine audio integration.

## Features
- Borderless overlay window with a clickable blue box; exit via click, ESC, or right‑click.
- Headless render on startup to `headless_output.bmp` (800x600) via `Renderer2D`.
- Audio hook: `PlayMusicEngineNote` currently triggers a MIDI/WinMM ping; ready to swap to MusicEngine DLL.
- Cross-platform target in `CrossPlatform/` using CMake, free of Win32 APIs.
- Script samples (C# and C++) for MusicEngine DLL calls and smoke tests.

## Project Layout
- `GameEngine/` – Win32 entry point and overlay composition.
+- `GameEngineCore/` – Core renderer, colliders, physics stubs, audio hook, sample scenes.
- `CrossPlatform/` – CMake target `GameEngineCross` (Windows/Linux); writes `headless_output_cross.bmp`.
- `MusicEngine_Linux/` – Stub placeholder awaiting the real MusicEngine sources.
- `scripts/` – Smoke tests and scripting examples (`run_smoke_tests.cpp`, C# integration scripts).
- `tests/` – AI-oriented harness notes and `GameEngine.Test` project.

## Build & Run (Windows, Visual Studio/Rider)
1. Open `GameEngine.sln`.
2. Build `GameEngine` (Debug|x64 recommended).
3. Run: overlay appears centered; click/ESC/right‑click closes; `headless_output.bmp` is produced in repo root.
4. Optional: build `tests/GameEngine.Test` for a smoke sequence (audio ping, render throughput, overlay).

## Build & Run (CrossPlatform / Linux)
```bash
cmake -S CrossPlatform -B build-cross
cmake --build build-cross
./build-cross/GameEngineCross          # Linux
build-cross\\Debug\\GameEngineCross.exe  # Windows MSVC
```
Outputs `headless_output_cross.bmp` and exercises core physics/render stubs.

## MusicEngine Integration Notes
- Windows: swap `PlayMusicEngineNote` to call the MusicEngine DLL (see `scripts/csharp/MusicEngineIntegration.csx`).
- Linux: replace `MusicEngine_Linux/src` with real sources and adjust `CMakeLists.txt`.

## License
This project is released under the **Music Engine License (MEL)**. See `LICENSE` for terms.
