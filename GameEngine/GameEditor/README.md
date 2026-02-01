# GameEditor (work-in-progress)

Goal: Lightweight Unity-like editor for this engine -- file browser, scene view, and code workspace in one app.

Planned modules
- Core: host window, dockable panes (scene, inspector, console).
- File browser: mirrors `EngineFiles/` (meshes, textures, audio, video).
- Code pane: launches external editor (e.g., Rider/VSCode) and shows read-only preview; later integrate a text editor.
- Scene graph: list of entities/components; drag-drop from file browser to scene.
- Play/Stop: run current game scene via existing GameEngine executable.

Current state
- Win32 editor shell with project browser, console panel, and animated scene preview.
- No build integration in the solution yet -- kept separate to avoid breaking existing projects.

Smoke test (UI + animation)
1) Build and run: `powershell -ExecutionPolicy Bypass -File scripts/run-gameeditor-smoke.ps1`
2) The editor opens, plays an animated preview, then auto-closes after a few seconds.
3) Use `--seconds=10` to keep it open longer, or run the exe without `--smoke` for normal use.

Next steps
1) Wire file browser to `EngineFiles/` root; show previews for PNG/JPG and basic metadata for FBX/OBJ/GLTF.
2) Launch GameEngine as a child process for Play mode; stream logs into the console pane.
3) Add a minimal scene JSON format that the engine can load; serialize transforms/material refs.
