#pragma once
#include <cstdint>
#include <vector>

class AudioEngine
{
public:
    bool Init(int sampleRate = 48000);
    void Shutdown();

    // Play a short generated sine tone (non-streaming smoke test).
    void PlayTestTone(double frequencyHz = 440.0, double seconds = 0.25, double volume = 0.2);

private:
    struct Impl;
    Impl* m_impl{ nullptr };
};

