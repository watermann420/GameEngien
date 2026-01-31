#pragma once
#include <string>
#include <vector>

struct DecodedAudio
{
    std::vector<int16_t> samples; // interleaved
    uint32_t channels{ 0 };
    uint32_t sampleRate{ 0 };
};

// Decode first audio stream to PCM16 interleaved buffer.
bool DecodeAudioPCM16(const std::wstring& path, DecodedAudio& out);
