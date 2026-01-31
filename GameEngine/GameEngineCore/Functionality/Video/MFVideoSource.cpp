#include "MFVideoSource.h"
#include <stdexcept>
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

MFVideoSource::MFVideoSource(const std::wstring& path)
{
    m_ready = Init(path);
}

MFVideoSource::~MFVideoSource()
{
    Shutdown();
}

bool MFVideoSource::Init(const std::wstring& path)
{
    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_FULL)))
        return false;

    IMFAttributes* attrs = nullptr;
    if (FAILED(MFCreateAttributes(&attrs, 2)))
        return false;
    attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    attrs->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);

    IMFSourceReader* reader = nullptr;
    if (FAILED(MFCreateSourceReaderFromURL(path.c_str(), attrs, &reader)))
    {
        if (attrs) attrs->Release();
        return false;
    }

    // Select first video stream and set output to RGB32.
    reader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
    reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);

    IMFMediaType* mt = nullptr;
    if (FAILED(MFCreateMediaType(&mt))) { reader->Release(); if (attrs) attrs->Release(); return false; }
    mt->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    mt->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, mt);
    mt->Release();

    IMFMediaType* outType = nullptr;
    if (FAILED(reader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &outType)))
    {
        reader->Release(); if (attrs) attrs->Release(); return false;
    }

    UINT32 w = 0, h = 0;
    MFGetAttributeSize(outType, MF_MT_FRAME_SIZE, &w, &h);
    m_desc.width = w;
    m_desc.height = h;
    m_desc.format = TextureFormat::BGRA8;
    m_desc.stride = w * 4;
    outType->Release();

    m_reader = reader;
    if (attrs) attrs->Release();
    return true;
}

void MFVideoSource::Shutdown()
{
    if (m_reader)
    {
        m_reader->Release();
        m_reader = nullptr;
    }
    MFShutdown();
}

bool MFVideoSource::TryReadFrame(std::vector<uint8_t>& buffer, uint64_t& timestampNs)
{
    if (!m_reader || m_eof) return false;

    IMFSample* sample = nullptr;
    DWORD flags = 0;
    LONGLONG ts = 0;
    HRESULT hr = m_reader->ReadSample(
        (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0,
        nullptr,
        &flags,
        &ts,
        &sample);

    if (FAILED(hr)) return false;
    if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
    {
        m_eof = true;
        if (sample) sample->Release();
        return false;
    }
    if (!sample) return false;

    IMFMediaBuffer* buf = nullptr;
    hr = sample->ConvertToContiguousBuffer(&buf);
    if (FAILED(hr) || !buf)
    {
        sample->Release();
        return false;
    }

    BYTE* data = nullptr;
    DWORD maxLen = 0, curLen = 0;
    buf->Lock(&data, &maxLen, &curLen);
    size_t needed = m_desc.BytesPerFrame();
    if (buffer.size() < needed) buffer.resize(needed);
    if (curLen >= needed)
    {
        memcpy(buffer.data(), data, needed);
    }
    else
    {
        memcpy(buffer.data(), data, curLen);
        memset(buffer.data() + curLen, 0, needed - curLen);
    }
    buf->Unlock();
    buf->Release();
    sample->Release();

    timestampNs = static_cast<uint64_t>(ts * 100); // MF timestamps are in 100ns units
    return true;
}
