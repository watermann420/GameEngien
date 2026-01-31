#include "TestScene.h"
#include "Renderer2D.h"
#include "BmpWriter.h"
#include "VulkanRenderer.h"
#include "AudioEngine.h"
#include "../../GameEngineCore/Physics/Physics2D/PhysicsWorld2D.h"
#include "../../GameEngineCore/Physics/Physics3D/PhysicsWorld3D.h"
#include "../../GameEngineCore/textur/VideoRenderPipline/VideoTexture.h"
#include "../../GameEngineCore/textur/VideoRenderPipline/TexturePool.h"
#include "../../GameEngineCore/textur/VideoRenderPipline/IFrameSource.h"
#include <vector>
#include <iostream>
#include <memory>

namespace
{
    // Simple frame source that reuses Renderer2D to supply BGRA frames for the Vulkan blit.
    class BoxFrameSource : public IFrameSource
    {
    public:
        BoxFrameSource(uint32_t w, uint32_t h)
            : m_renderer(200, 200, 0, 122, 255)
        {
            m_desc.width = w;
            m_desc.height = h;
            m_desc.stride = w * 4;
        }

        TextureDesc Describe() const override { return m_desc; }

        bool TryReadFrame(std::vector<uint8_t>& buffer, uint64_t& timestampNs) override
        {
            m_renderer.RenderToBuffer(m_desc.width, m_desc.height, buffer);
            timestampNs += 16'666'667; // pretend ~60 fps
            return true;
        }

    private:
        TextureDesc m_desc{};
        Renderer2D m_renderer;
    };
}

int RunSmokeTests()
{
    // Physics
    PhysicsWorld2D world2d;
    auto b2d = world2d.CreateBody();
    b2d->SetMass(5.0);
    b2d->enableNBodyGravity = true;
    world2d.SetNBodyGravityEnabled(true);
    world2d.Step(1.0 / 60.0);

    PhysicsWorld3D world3d;
    auto b3d = world3d.CreateBody();
    b3d->SetMass(10.0);
    b3d->enableNBodyGravity = true;
    world3d.SetNBodyGravityEnabled(true);
    world3d.Step(1.0 / 60.0);

    // Audio
    AudioEngine audio;
    audio.Init();
    audio.PlayTestTone();

    // Headless BGRA buffer (blue box)
    const uint32_t width = 800;
    const uint32_t height = 600;
    Renderer2D renderer(200, 200, 0, 122, 255);
    std::vector<uint8_t> buffer;
    renderer.RenderToBuffer(width, height, buffer);
    if (!WriteBmpBGRA("headless_output_cross.bmp", width, height, buffer))
    {
        std::cerr << "Failed to write headless_output_cross.bmp\n";
        audio.Shutdown();
        return 1;
    }

    // Vulkan blit (if available) using VideoTexture pipeline
    VulkanRenderer vk;
    if (vk.Init(width, height, "GameEngineCross Vulkan"))
    {
        // Drive a VideoTexture backed by BoxFrameSource (one frame)
        auto src = std::make_unique<BoxFrameSource>(width, height);
        VideoTexture vt(std::move(src));
        vt.Start();
        FrameView view{};
        if (vt.TryGetFrame(view))
        {
            vk.RenderBGRAFrame(view.desc.width, view.desc.height, view.data);
        }
        else
        {
            vk.RenderOnce(0.1f, 0.2f, 0.6f);
        }
        vt.Stop();
        vk.Shutdown();
    }

    audio.Shutdown();
    return 0;
}

