#include "AudioEngine.h"
#include <cmath>
#include <iostream>

#if defined(GECROSS_HAS_OPENAL)
#include <AL/al.h>
#include <AL/alc.h>
#endif

struct AudioEngine::Impl
{
#if defined(GECROSS_HAS_OPENAL)
    ALCdevice* device{ nullptr };
    ALCcontext* ctx{ nullptr };
#endif
    int sampleRate{ 48000 };
};

bool AudioEngine::Init(int sampleRate)
{
    m_impl = new Impl();
    m_impl->sampleRate = sampleRate;
#if defined(GECROSS_HAS_OPENAL)
    m_impl->device = alcOpenDevice(nullptr);
    if (!m_impl->device) { std::cerr << "OpenAL: no device\n"; return false; }
    m_impl->ctx = alcCreateContext(m_impl->device, nullptr);
    if (!m_impl->ctx) { std::cerr << "OpenAL: context fail\n"; return false; }
    alcMakeContextCurrent(m_impl->ctx);
    return true;
#else
    std::cerr << "OpenAL not available; audio disabled\n";
    return false;
#endif
}

void AudioEngine::PlayTestTone(double frequencyHz, double seconds, double volume)
{
#if defined(GECROSS_HAS_OPENAL)
    if (!m_impl || !m_impl->ctx) return;
    const int samples = static_cast<int>(seconds * m_impl->sampleRate);
    std::vector<short> pcm(samples);
    const double twopi = 6.283185307179586;
    for (int i = 0; i < samples; ++i)
    {
        double t = static_cast<double>(i) / m_impl->sampleRate;
        double s = std::sin(twopi * frequencyHz * t) * volume;
        pcm[i] = static_cast<short>(s * 32767);
    }

    ALuint buf = 0, src = 0;
    alGenBuffers(1, &buf);
    alBufferData(buf, AL_FORMAT_MONO16, pcm.data(), static_cast<ALsizei>(pcm.size() * sizeof(short)), m_impl->sampleRate);
    alGenSources(1, &src);
    alSourcei(src, AL_BUFFER, buf);
    alSourcePlay(src);
    // Non-blocking; fire-and-forget minimal check.
#else
    (void)frequencyHz; (void)seconds; (void)volume;
#endif
}

void AudioEngine::Shutdown()
{
#if defined(GECROSS_HAS_OPENAL)
    if (m_impl && m_impl->ctx)
    {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(m_impl->ctx);
    }
    if (m_impl && m_impl->device)
    {
        alcCloseDevice(m_impl->device);
    }
#endif
    delete m_impl;
    m_impl = nullptr;
}

