# Scripts

- `run_smoke_tests.cpp`: Example/test entry point that uses the engine like a user. Calls `RunSmokeTests()` from `CrossPlatform/src/TestScene.cpp`, covering physics, audio, headless render, and Vulkan blit.
- `run-musicengine-example.ps1`: Loads `MusicEngine.dll` (if available) and plays a test note via `OrganSynth`. Use this as a minimal integration example.

Goal: Keep small example or test scenes here to demonstrate how to use engine APIs (GameObjects, VideoTexture, editor hooks, etc.).
