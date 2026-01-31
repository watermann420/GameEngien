#include "../RenderPipeline2D/Renderer2D.h"
#include "../textur/VideoRenderPipline/IFrameSource.h"
#include "../textur/VideoRenderPipline/VideoTexture.h"
#include "../textur/VideoRenderPipline/TexturePool.h"
#include "../Physics/Physics2D/PhysicsWorld2D.h"
#include "../Physics/Physics3D/PhysicsWorld3D.h"
#include <vector>
#include <memory>
#include <iostream>

// Minimal test scene as if a user was scripting the engine.
int RunBlueBox2DTest()
{
    // Physics sanity
    PhysicsWorld2D world2d;
    auto body = world2d.CreateBody();
    body->SetMass(1.0);
    world2d.Step(1.0 / 60.0);

    // Renderer-driven frame source for VideoTexture
    class BoxFrameSource : public IFrameSource
    {
    public:
        BoxFrameSource(uint32_t w, uint32_t h) : m_renderer(200, 200, RGB(0, 122, 255))
        {
            m_desc.width = w; m_desc.height = h; m_desc.stride = w * 4;
        }
        TextureDesc Describe() const override { return m_desc; }
        bool TryReadFrame(std::vector<uint8_t>& buffer, uint64_t& timestampNs) override
        {
            m_renderer.RenderToBuffer(static_cast<int>(m_desc.width), static_cast<int>(m_desc.height), buffer);
            timestampNs += 16'666'667;
            return true;
        }
    private:
        TextureDesc m_desc{};
        Renderer2D m_renderer;
    };

    const uint32_t w = 800, h = 600;
    auto src = std::make_unique<BoxFrameSource>(w, h);
    VideoTexture vt(std::move(src));
    vt.Start();
    FrameView view{};
    if (vt.TryGetFrame(view))
    {
        // Dump a BMP to prove output; reuse Renderer2D buffer (view points into VideoTexture buffer).
        // (Headless writer lives in CrossPlatform; here we just log presence.)
        std::cout << "BlueBox2DTest: got frame " << view.desc.width << "x" << view.desc.height << "\n";
    }
    vt.Stop();
    return 0;
}
