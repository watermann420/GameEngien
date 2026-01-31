#pragma once
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>

struct AudioSource
{
    std::vector<float> samples; // interleaved float32
    uint32_t channels{ 2 };
    uint32_t sampleRate{ 44100 };
    double frameCursor{ 0.0 };
    bool looping{ true };
};

class AudioMixer
{
public:
    AudioMixer();
    ~AudioMixer();

    bool Start(uint32_t sampleRate = 44100, uint32_t channels = 2);
    void Stop();

    void AddSource(const std::vector<float>& samples, uint32_t channels, uint32_t sampleRate);

private:
    void ThreadFunc();
    bool InitDevice(uint32_t& bufferFrames);
    void Mix(float* out, uint32_t frames);
    void ResampleAndMix(AudioSource& src, float* out, uint32_t frames);

    IMMDevice* m_device{ nullptr };
    IAudioClient* m_client{ nullptr };
    IAudioRenderClient* m_render{ nullptr };
    WAVEFORMATEX* m_mixFmt{ nullptr };
    uint32_t m_bufferFrames{ 0 };
    std::mutex m_mutex;
    std::vector<AudioSource> m_sources;
    std::thread m_thread;
    std::atomic<bool> m_run{ false };
    uint32_t m_targetRate{ 44100 };
    uint32_t m_targetCh{ 2 };
};
