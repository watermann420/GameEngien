#pragma once
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <string>
#include <vector>
#include "../../textur/VideoRenderPipline/IFrameSource.h"

// Minimal Media Foundation source reader -> BGRA8 frame source.
class MFVideoSource : public IFrameSource
{
public:
    explicit MFVideoSource(const std::wstring& path);
    ~MFVideoSource();

    bool IsReady() const { return m_ready; }

    TextureDesc Describe() const override { return m_desc; }
    bool TryReadFrame(std::vector<uint8_t>& buffer, uint64_t& timestampNs) override;

private:
    bool Init(const std::wstring& path);
    void Shutdown();

    IMFSourceReader* m_reader{ nullptr };
    TextureDesc m_desc{};
    bool m_ready{ false };
    bool m_eof{ false };
};
