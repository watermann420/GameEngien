#pragma once
#include <cstdint>
#include <vector>
#include <string>

// Minimal BMP writer (32-bit BGRA) for cross-platform headless output.
bool WriteBmpBGRA(const std::string& path, uint32_t width, uint32_t height, const std::vector<uint8_t>& data);

