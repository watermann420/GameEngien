# EnginePlugins

This folder holds native plugin DLLs loaded by the engine at runtime.

## C plugin example
- Source: `EnginePlugins/ExampleCPlugin/ExampleCPlugin.c`
- Build it as a DLL and place the output in `EnginePlugins/`.
- The engine loads any `*.dll` in this folder and calls:
  - `GE_PluginInit(const EngineAPI* api)`
  - `GE_PluginUpdate(double dt)`
  - `GE_PluginShutdown()`

## Java support (planned)
Use JNI to host a JVM and call Java code from a native plugin DLL.
The recommended path is:
1) Build a native plugin (C/C++) that loads `jvm.dll` and creates a JVM.
2) Expose a thin C ABI for the engine, then bridge to Java internally.
3) Keep all engine calls in the C ABI so Java stays optional.
