// tests/ai/GameEngineTestNotes.md
// AI-focused sandbox plan for testing GameEngine without impacting production build.

- Scope: use overlay/box collider path only (no MusicEngine requirement).
- Build: reuse GameEngine Debug|x64; run executable from repo root.
- Expected behavior: overlay blue box centered; click = quit; ESC = quit; headless_output.bmp produced.
- Future hooks: add AI experiments here (new source files) and wire via separate vcxproj if needed.

## GameEngine.Test harness
- Project: `tests/GameEngine.Test/GameEngine.Test.vcxproj` (added to solution).
- What it does: headless render to `headless_output.bmp`, plays a system sound (or beep fallback), runs a render throughput sample, then shows the overlay UI; click the box or press ESC to exit.
- Leak + perf: Debug builds enable CRT leak checks; render loop prints messages-per-second; throughput sample prints blits/sec.
- How to run: build `GameEngine.Test` (Debug|x64 recommended) and start it; watch console logs plus the overlay window. Keep speaker on for audio ping.
- MusicEngine hook: swap the `RunAudioPing` implementation to call into MusicEngine once its API is available (uses winmm alias for now).
