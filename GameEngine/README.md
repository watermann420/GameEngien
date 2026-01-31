# GameEngine

Lightweight C++ prototype for a Win32 overlay/renderer with cross‑platform stubs. Ships with a headless renderer, simple physics helpers, and placeholders for future MusicEngine audio integration.

## Features
- Borderless overlay with a clickable (now 3D wireframe) cube; exit via click, ESC, or right‑click.
- Headless render on startup to `headless_output.bmp` (800x600) via `Renderer2D`.
- Audio hook: `PlayMusicEngineNote` currently triggers a MIDI/WinMM ping; ready to swap to MusicEngine DLL.
- Cross-platform target in `CrossPlatform/` using CMake, free of Win32 APIs.
- Script samples (C# and C++) for MusicEngine DLL calls and smoke tests.
- Real-time tweaks: cached GDI brush (no per-frame alloc), high-priority process/thread, 1 ms timer resolution for smoother input/render.
- Optional video plane: place `files/video.mp4` and the overlay will texture it onto a 2D plane (with best-effort audio via MCI) alongside the rotating cube.

## Project Layout
- `GameEngine/` – Win32 entry point and overlay composition.
- `GameEngineCore/` – Core renderer, colliders, physics stubs, audio hook, sample scenes.
- `CrossPlatform/` – CMake target `GameEngineCross` (Windows/Linux); writes `headless_output_cross.bmp`.
- `MusicEngine_Linux/` – Stub placeholder awaiting the real MusicEngine sources.
- `scripts/` – Smoke tests and scripting examples (`run_smoke_tests.cpp`, C# integration scripts).
- `tests/` – AI-oriented harness notes and `GameEngine.Test` project (runs BlueBox2DTest bridge).

## Build & Run (Windows, Visual Studio/Rider)
1. Open `GameEngine.sln`. (Both Win32 and x64 configs are available.)
2. Build `GameEngine` for your target:
   - 32-bit: `Debug|Win32` or `Release|Win32`
   - 64-bit: `Debug|x64` or `Release|x64`
3. Run: overlay appears centered; click/ESC/right‑click closes; `headless_output.bmp` is produced in repo root.
4. Optional: build `tests/GameEngine.Test` to run the headless BlueBox2D demo and emit `BlueBox2DTest_output.bmp`.

## Build & Run (CrossPlatform / Linux)
```bash
cmake -S CrossPlatform -B build-cross
cmake --build build-cross
./build-cross/GameEngineCross          # Linux
build-cross\Debug\GameEngineCross.exe  # Windows MSVC
```
Outputs `headless_output_cross.bmp` and exercises core physics/render stubs.

## MusicEngine Integration Notes
- Windows: swap `PlayMusicEngineNote` to call the MusicEngine DLL (see `scripts/csharp/MusicEngineIntegration.csx`).
- Linux: replace `MusicEngine_Linux/src` with real sources and adjust `CMakeLists.txt`.

## Tests & Samples
- `scripts/run_smoke_tests.cpp` uses `CrossPlatform/src/TestScene.cpp` for end-to-end smoke.
- `GameEngineCore/Scripts/BlueBox2DTest.cpp` renders the rotating 3D cube + bouncing, color-shifting text and saves `BlueBox2DTest_output.bmp`.
- `tests/ai/GameEngineTestNotes.md` documents the AI overlay harness.
- `tests/GameEngine.Test` bridges to `RunBlueBox2DTest` so you can run it from the solution.

## License
This project is released under the **Music Engine License (MEL)**. See `LICENSE` for terms.
