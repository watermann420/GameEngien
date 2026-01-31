#include "AudioDecoder.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

bool DecodeAudioPCM16(const std::wstring& path, DecodedAudio& out)
{
    out.samples.clear();
    out.channels = 0;
    out.sampleRate = 0;

    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_FULL))) return false;

    IMFAttributes* attrs = nullptr;
    if (FAILED(MFCreateAttributes(&attrs, 1)))
    {
        MFShutdown();
        return false;
    }
    attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);

    IMFSourceReader* reader = nullptr;
    if (FAILED(MFCreateSourceReaderFromURL(path.c_str(), attrs, &reader)))
    {
        attrs->Release();
        MFShutdown();
        return false;
    }
    attrs->Release();

    reader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
    reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);

    IMFMediaType* outType = nullptr;
    if (FAILED(MFCreateMediaType(&outType)))
    {
        reader->Release(); MFShutdown(); return false;
    }
    outType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    outType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    outType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2);
    outType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100);
    outType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
    outType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4);
    outType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100 * 4);
    outType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, outType);
    outType->Release();

    out.channels = 2;
    out.sampleRate = 44100;

    DWORD flags = 0;
    while (true)
    {
        IMFSample* sample = nullptr;
        LONGLONG ts = 0;
        HRESULT hr = reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, &ts, &sample);
        if (FAILED(hr)) break;
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
        {
            if (sample) sample->Release();
            break;
        }
        if (!sample) continue;

        IMFMediaBuffer* buf = nullptr;
        hr = sample->ConvertToContiguousBuffer(&buf);
        if (SUCCEEDED(hr) && buf)
        {
            BYTE* data = nullptr;
            DWORD maxLen = 0, curLen = 0;
            buf->Lock(&data, &maxLen, &curLen);
            size_t oldSize = out.samples.size();
            out.samples.resize(oldSize + curLen / 2);
            memcpy(out.samples.data() + oldSize, data, curLen);
            buf->Unlock();
            buf->Release();
        }
        sample->Release();
    }

    reader->Release();
    MFShutdown();
    return !out.samples.empty();
}
