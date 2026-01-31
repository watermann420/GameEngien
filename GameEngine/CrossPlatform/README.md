# CrossPlatform Build (Windows & Linux)

This folder provides a single CMake target `GameEngineCross` that is free of Win32 APIs and should build on both Windows (MSVC/MinGW) and Linux/Clang/GCC.

What it does:
- Uses a platform-neutral 2D renderer (`Renderer2D`) and BMP writer to emit `headless_output_cross.bmp`.
- Compiles and lightly exercises the shared physics stubs from `GameEngineCore` (2D/3D bodies, gravity, N-body option).

Build & run:
```bash
cmake -S CrossPlatform -B build-cross
cmake --build build-cross
./build-cross/GameEngineCross   # Linux
build-cross\\Debug\\GameEngineCross.exe  # Windows MSVC default
```

Next steps to fully unify code:
- Move shared gameplay/physics/render abstractions into `GameEngineCore` with platform-specific backends behind interfaces.
- Add an SDL2/GL/Vulkan renderer for windowed output; keep this headless path for CI.
- Point C# P/Invoke to the cross-built shared library (`.so`/`.dll`) when scripting.

