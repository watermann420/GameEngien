#include "AudioMixer.h"
#include <avrt.h>
#include <cmath>
#include <audioclient.h>
#include <mmdeviceapi.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "mmdevapi.lib")

AudioMixer::AudioMixer() {}

AudioMixer::~AudioMixer()
{
    Stop();
}

static WAVEFORMATEXTENSIBLE MakeWaveExt(uint32_t rate, uint32_t ch)
{
    WAVEFORMATEXTENSIBLE w{};
    w.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    w.Format.nChannels = (WORD)ch;
    w.Format.nSamplesPerSec = rate;
    w.Format.wBitsPerSample = 32;
    w.Format.nBlockAlign = w.Format.wBitsPerSample / 8 * w.Format.nChannels;
    w.Format.nAvgBytesPerSec = w.Format.nBlockAlign * w.Format.nSamplesPerSec;
    w.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    w.Samples.wValidBitsPerSample = 32;
    w.dwChannelMask = (ch == 1) ? SPEAKER_FRONT_CENTER : SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    w.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    return w;
}

bool AudioMixer::InitDevice(uint32_t& bufferFrames)
{
    IMMDeviceEnumerator* enumr = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumr))))
        return false;
    if (FAILED(enumr->GetDefaultAudioEndpoint(eRender, eConsole, &m_device)))
    {
        enumr->Release();
        return false;
    }
    enumr->Release();

    if (FAILED(m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_client)))
        return false;

    if (FAILED(m_client->GetMixFormat(&m_mixFmt)))
        return false;

    WAVEFORMATEXTENSIBLE desired = MakeWaveExt(m_targetRate, m_targetCh);
    WAVEFORMATEX* closest = nullptr;
    HRESULT hr = m_client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, reinterpret_cast<WAVEFORMATEX*>(&desired), &closest);
    WAVEFORMATEX* chosen = nullptr;
    if (hr == S_OK)
    {
        chosen = reinterpret_cast<WAVEFORMATEX*>(&desired);
    }
    else if (hr == S_FALSE && closest)
    {
        chosen = closest;
    }
    else
    {
        chosen = m_mixFmt; // fallback to device mix
    }

    REFERENCE_TIME hns = 10000000; // 1s
    if (FAILED(m_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, hns, 0, chosen, nullptr)))
    {
        if (closest) CoTaskMemFree(closest);
        return false;
    }
    if (closest) CoTaskMemFree(closest);
    if (FAILED(m_client->GetBufferSize(&bufferFrames)))
        return false;
    if (FAILED(m_client->GetService(IID_PPV_ARGS(&m_render))))
        return false;
    return true;
}

bool AudioMixer::Start(uint32_t sampleRate, uint32_t channels)
{
    Stop();
    m_targetRate = sampleRate;
    m_targetCh = channels;
    if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) return false;
    if (!InitDevice(m_bufferFrames))
    {
        CoUninitialize();
        return false;
    }
    m_run = true;
    m_thread = std::thread(&AudioMixer::ThreadFunc, this);
    return true;
}

void AudioMixer::Stop()
{
    m_run = false;
    if (m_thread.joinable()) m_thread.join();
    if (m_client) m_client->Stop();
    if (m_render) { m_render->Release(); m_render = nullptr; }
    if (m_client) { m_client->Release(); m_client = nullptr; }
    if (m_device) { m_device->Release(); m_device = nullptr; }
    if (m_mixFmt) { CoTaskMemFree(m_mixFmt); m_mixFmt = nullptr; }
    CoUninitialize();
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sources.clear();
}

void AudioMixer::AddSource(const std::vector<float>& samples, uint32_t channels, uint32_t sampleRate)
{
    AudioSource s;
    s.samples = samples;
    s.channels = channels;
    s.sampleRate = sampleRate;
    s.frameCursor = 0.0;
    s.looping = true;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sources.push_back(std::move(s));
}

void AudioMixer::Mix(float* out, uint32_t frames)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::fill(out, out + frames * m_targetCh, 0.0f);
    for (auto& s : m_sources)
    {
        ResampleAndMix(s, out, frames);
    }
    // simple limiter
    for (uint32_t i = 0; i < frames * m_targetCh; ++i)
    {
        if (out[i] > 1.0f) out[i] = 1.0f;
        else if (out[i] < -1.0f) out[i] = -1.0f;
    }
}

void AudioMixer::ResampleAndMix(AudioSource& src, float* out, uint32_t frames)
{
    if (src.samples.empty()) return;
    double ratio = static_cast<double>(src.sampleRate) / m_targetRate;
    size_t totalFrames = src.samples.size() / src.channels;
    for (uint32_t f = 0; f < frames; ++f)
    {
        size_t srcFrame = static_cast<size_t>(src.frameCursor);
        if (srcFrame >= totalFrames)
        {
            if (src.looping)
                srcFrame = srcFrame % totalFrames;
            else
                break;
            src.frameCursor = static_cast<double>(srcFrame);
        }
        for (uint32_t ch = 0; ch < m_targetCh; ++ch)
        {
            uint32_t srcCh = ch < src.channels ? ch : src.channels - 1;
            float sample = src.samples[srcFrame * src.channels + srcCh];
            out[f * m_targetCh + ch] += sample;
        }
        src.frameCursor += ratio;
    }
}

void AudioMixer::ThreadFunc()
{
    HANDLE hTask = AvSetMmThreadCharacteristicsW(L"Pro Audio", nullptr);
    m_client->Start();

    const UINT32 period = 10; // ms
    std::vector<float> mixBuf;

    while (m_run)
    {
        UINT32 padding = 0;
        if (FAILED(m_client->GetCurrentPadding(&padding))) break;
        UINT32 avail = m_bufferFrames > padding ? m_bufferFrames - padding : 0;
        if (avail == 0)
        {
            Sleep(period);
            continue;
        }
        mixBuf.resize(avail * m_targetCh);
        Mix(mixBuf.data(), avail);

        BYTE* render = nullptr;
        if (SUCCEEDED(m_render->GetBuffer(avail, &render)))
        {
            memcpy(render, mixBuf.data(), avail * m_targetCh * sizeof(float));
            m_render->ReleaseBuffer(avail, 0);
        }
        Sleep(2);
    }

    m_client->Stop();
    if (hTask) AvRevertMmThreadCharacteristics(hTask);
}
