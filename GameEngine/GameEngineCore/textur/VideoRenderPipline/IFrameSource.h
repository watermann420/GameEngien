#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include "FrameTypes.h"

// Interface for anything that can populate a raw frame buffer.
class IFrameSource
{
public:
    virtual ~IFrameSource() = default;

    // Returns static description of the frames this source produces.
    virtual TextureDesc Describe() const = 0;

    // Attempts to write the next frame into 'buffer'. Implementations must resize the buffer if needed.
    // Returns true when a new frame is written. False means no frame was produced (e.g., end of stream).
    virtual bool TryReadFrame(std::vector<uint8_t>& buffer, uint64_t& timestampNs) = 0;
};

