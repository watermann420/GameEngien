# Workflows (CrossPlatform)

1) Runtime Smoke (headless + optional Vulkan + audio)  
   - Build: `cmake -S CrossPlatform -B build-cross -DGECROSS_ENABLE_VULKAN=ON -DGECROSS_ENABLE_SDL2=ON -DGECROSS_ENABLE_OPENAL=ON`  
   - Run: `./build-cross/GameEngineCross` (Linux) or `build-cross\\Debug\\GameEngineCross.exe` (Windows)  
   - Output: `headless_output_cross.bmp`, optional Vulkan window with BGRA blit/clear, short 440 Hz tone.

2) Editor Track (to-be-built)  
   - Goal: SDL2/Vulkan window hosting custom UI (world editor, 3D viewport, sprite painter, code editor).  
   - Next steps: add `EditorApp` target using VulkanRenderer as backend; embed immediate-mode UI (e.g., ImGui) or custom UI; wire GameObject tree + PhysicsWorld + VideoTexture previews.

3) Desktop/Overlay Track  
   - Goal: render to desktop/background or frameless window.  
   - Next steps: add platform abstraction for window flags (transparent, borderless, always-on-top) and optional direct-to-desktop modes; reuse VulkanRenderer swapchain + BGRA blit path.

Test script entry point: `RunSmokeTests()` in `src/TestScene.cpp` keeps all automated checks in one place (physics, audio, headless render, Vulkan blit).

