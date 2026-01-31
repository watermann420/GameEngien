# GameEditor (work-in-progress)

Goal: Lightweight Unity-like editor for this engine—file browser, scene view, and code workspace in one app.

Planned modules
- Core: host window, dockable panes (scene, inspector, console).
- File browser: mirrors `EngineFiles/` (meshes, textures, audio, video).
- Code pane: launches external editor (e.g., Rider/VSCode) and shows read-only preview; later integrate a text editor.
- Scene graph: list of entities/components; drag-drop from file browser to scene.
- Play/Stop: run current game scene via existing GameEngine executable.

Current state
- Skeleton folders only (src/, editor/, assets/, plugins/). No code yet.
- No build integration in the solution yet—kept separate to avoid breaking existing projects.

Next steps
1) Add a new project to the solution (`GameEditor.vcxproj` or C# WinUI/WPF) under `GameEditor/src/`.
2) Wire file browser to `EngineFiles/` root; show previews for PNG/JPG and basic metadata for FBX/OBJ/GLTF.
3) Launch GameEngine as a child process for Play mode; stream logs into the console pane.
4) Add a minimal scene JSON format that the engine can load; serialize transforms/material refs.
