#include "AudioPlayback.h"

#include "AppState.h"

#include <mmsystem.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <thread>

#pragma comment(lib, "winmm.lib")

// Generate and play a WAV-from-memory instantly for the given MIDI note.
static void PlayImmediateNote(int midiNote, int velocity, double seconds = 0.25)
{
    const int sampleRate = 48000;
    const double freq = 440.0 * pow(2.0, (midiNote - 69) / 12.0);
    const double vol = std::clamp(velocity / 127.0, 0.05, 1.0) * 0.6;
    const int samples = static_cast<int>(seconds * sampleRate);

    struct WavHeader
    {
        DWORD riff;
        DWORD size;
        DWORD wave;
        DWORD fmt;
        DWORD fmtSize;
        WORD  format;
        WORD  channels;
        DWORD sampleRateField;
        DWORD byteRate;
        WORD  blockAlign;
        WORD  bitsPerSample;
        DWORD dataId;
        DWORD dataSize;
    };

    static std::map<std::pair<int, int>, std::vector<BYTE>> cache;
    auto key = std::make_pair(midiNote, velocity);
    auto it = cache.find(key);
    if (it == cache.end())
    {
        WavHeader header{
            'FFIR',
            0,
            'EVAW',
            ' tmf',
            16,
            1,
            1,
            static_cast<DWORD>(sampleRate),
            static_cast<DWORD>(sampleRate * 2),
            2,
            16,
            'atad',
            0
        };
        header.dataSize = samples * 2;
        header.size = 4 + 8 + header.fmtSize + 8 + header.dataSize;

        std::vector<BYTE> buf(sizeof(header) + header.dataSize);
        memcpy(buf.data(), &header, sizeof(header));
        short* pcm = reinterpret_cast<short*>(buf.data() + sizeof(header));

        const double twoPi = 6.283185307179586;
        const double attack = 0.002;
        const double decay = 0.04;
        const double sustain = 0.3;
        const double release = 0.06;

        for (int i = 0; i < samples; ++i)
        {
            double t = static_cast<double>(i) / sampleRate;
            double env = 1.0;
            if (t < attack) env = t / attack;
            else if (t < attack + decay) env = 1.0 - (t - attack) / decay * (1.0 - sustain);
            else if (t > seconds - release) env = sustain * (seconds - t) / release;
            else env = sustain;

            double s = (sin(twoPi * freq * t) + 0.3 * sin(twoPi * freq * 2 * t)) * vol * env;
            pcm[i] = static_cast<short>(std::clamp(s, -1.0, 1.0) * 32767);
        }
        it = cache.emplace(key, std::move(buf)).first;
    }

    PlaySoundW(reinterpret_cast<LPCWSTR>(it->second.data()), nullptr, SND_ASYNC | SND_MEMORY);
}

static bool EnsureMidi()
{
    static bool tried = false;
    static bool ok = false;
    if (tried) return ok;
    tried = true;
    if (midiOutOpen(&g_app.midiOut, MIDI_MAPPER, 0, 0, 0) == MMSYSERR_NOERROR)
    {
        ok = true;
    }
    return ok;
}

static void PlayMidiPiano(int note = 60, int velocity = 100, int durationMs = 200)
{
    if (EnsureMidi())
    {
        DWORD on = 0;
        on |= 0x90; // note on, channel 0
        on |= (static_cast<DWORD>(note) & 0x7F) << 8;
        on |= (static_cast<DWORD>(velocity) & 0x7F) << 16;
        midiOutShortMsg(g_app.midiOut, on);

        std::thread([note]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            DWORD off = 0;
            off |= 0x80;
            off |= (static_cast<DWORD>(note) & 0x7F) << 8;
            midiOutShortMsg(g_app.midiOut, off);
        }).detach();
        return;
    }
    PlayImmediateNote(note, velocity, durationMs / 1000.0);
}

void PlayMusicEngineNote(int note, int velocity, double duration)
{
    PlayMidiPiano(note, velocity, static_cast<int>(duration * 1000));
}

void StopAudio()
{
    PlaySoundW(nullptr, nullptr, 0);
    g_app.audioOpen = false;
}
