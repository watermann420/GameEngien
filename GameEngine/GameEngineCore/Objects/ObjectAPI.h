#pragma once
#include <cstdint>

// C API for GameObject hierarchy, enabling C# or other languages via P/Invoke.
extern "C"
{
__declspec(dllexport) uint64_t GE_CreateObject(const char* name, const char* tag);
__declspec(dllexport) void     GE_DestroyObject(uint64_t id);
__declspec(dllexport) uint64_t GE_AddChild(uint64_t parentId, const char* name, const char* tag);
__declspec(dllexport) uint32_t GE_GetChildCount(uint64_t parentId);
__declspec(dllexport) uint64_t GE_GetChildAt(uint64_t parentId, uint32_t index);
__declspec(dllexport) uint64_t GE_FindByName(const char* name);
__declspec(dllexport) void     GE_SetName(uint64_t id, const char* name);
__declspec(dllexport) const char* GE_GetName(uint64_t id);
__declspec(dllexport) void     GE_SetTag(uint64_t id, const char* tag);
__declspec(dllexport) const char* GE_GetTag(uint64_t id);
}

