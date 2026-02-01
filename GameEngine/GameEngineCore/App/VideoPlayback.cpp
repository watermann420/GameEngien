#include "VideoPlayback.h"

#include "AppState.h"

#include "../Functionality/Video/MFVideoSource.h"
#include "../Functionality/Audio/AudioDecoder.h"
#include "../textur/VideoRenderPipline/FrameTypes.h"

#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

static fs::path GetFilesRoot()
{
    if (!g_app.projectPath.empty())
    {
        fs::path p = g_app.projectPath;
        fs::path filesUpper = p / L"Files";
        fs::path filesLower = p / L"files";
        if (fs::exists(filesUpper)) return filesUpper;
        if (fs::exists(filesLower)) return filesLower;
        return p;
    }
    return fs::path("files");
}

static void FindMediaPaths()
{
    g_app.videoPath.clear();
    g_app.audioPath.clear();
    const fs::path filesDir = GetFilesRoot();
    if (!fs::exists(filesDir) || !fs::is_directory(filesDir)) return;

    for (auto& p : fs::directory_iterator(filesDir))
    {
        if (p.path().extension() == L".mp4")
        {
            g_app.videoPath = p.path().wstring();
            break;
        }
    }
    for (auto& p : fs::directory_iterator(filesDir))
    {
        if (p.path().extension() == L".mp3")
        {
            g_app.audioPath = p.path().wstring();
            break;
        }
    }
}

static void BuildAudioWavFromVideo()
{
    if (g_app.videoPath.empty()) return;
    DecodedAudio pcm16;
    if (!DecodeAudioPCM16(g_app.videoPath, pcm16)) return;

    struct WavHeader
    {
        DWORD riff = 'FFIR';
        DWORD size = 0;
        DWORD wave = 'EVAW';
        DWORD fmt = ' tmf';
        DWORD fmtSize = 16;
        WORD  format = 1;
        WORD  channels = 2;
        DWORD sampleRate = 44100;
        DWORD byteRate = 0;
        WORD  blockAlign = 0;
        WORD  bitsPerSample = 16;
        DWORD data = 'atad';
        DWORD dataSize = 0;
    } hdr;

    hdr.channels = (WORD)pcm16.channels;
    hdr.sampleRate = pcm16.sampleRate;
    hdr.bitsPerSample = 16;
    hdr.blockAlign = hdr.channels * hdr.bitsPerSample / 8;
    hdr.byteRate = hdr.sampleRate * hdr.blockAlign;
    hdr.dataSize = static_cast<DWORD>(pcm16.samples.size() * sizeof(int16_t));
    hdr.size = 4 + 8 + hdr.fmtSize + 8 + hdr.dataSize;

    g_app.audioWavBytes.resize(sizeof(hdr) + hdr.dataSize);
    memcpy(g_app.audioWavBytes.data(), &hdr, sizeof(hdr));
    memcpy(g_app.audioWavBytes.data() + sizeof(hdr), pcm16.samples.data(), hdr.dataSize);
}

bool InitVideoFromFiles()
{
    FindMediaPaths();
    if (g_app.videoPath.empty()) return false;

    auto src = std::make_unique<MFVideoSource>(g_app.videoPath);
    if (!src->IsReady()) return false;

    g_app.videoTexture = std::make_unique<VideoTexture>(std::move(src));
    g_app.videoTexture->Start();
    std::wstringstream ss;
    ss << L"[video] playing " << g_app.videoPath << L"\n";
    OutputDebugStringW(ss.str().c_str());

    BuildAudioWavFromVideo();
    return true;
}

static void EnsurePlaneForFrame(const FrameView& view)
{
    if (!g_app.planes.empty()) return;

    VideoPlane p;
    p.InitFromFrame(view.desc, g_app.bounds, 0.35f);
    p.SetVelocity(-220.0f, 140.0f);
    g_app.planes.push_back(p);
}

static void SpawnPlaneIfNeeded(double dt, const FrameView& view)
{
    g_app.spawnAccumulator += dt;
    if (g_app.spawnAccumulator < 2.0) return;

    g_app.spawnAccumulator = 0.0;
    VideoPlane p;
    p.InitFromFrame(view.desc, g_app.bounds, 0.35f);
    p.SetPosition(static_cast<float>(g_app.bounds.right - p.Bounds().right),
                  static_cast<float>((g_app.bounds.bottom) * 0.2f));
    p.SetVelocity(-250.0f - 10.0f * static_cast<float>(g_app.planes.size()), 120.0f);
    g_app.planes.push_back(p);
}

static void UpdatePlanes(double dt, HDC dc, const FrameView& view)
{
    for (auto& p : g_app.planes)
    {
        p.Update(dt, g_app.bounds);
        p.Render(dc, view);
    }
}

static void UpdateColliderFromFirstPlane()
{
    if (g_app.planes.empty()) return;
    RECT r = g_app.planes.front().Bounds();
    g_app.collider.Set(r.left, r.top, r.right - r.left, r.bottom - r.top);
}

bool UpdateVideoAndRender(double dt, HDC targetDC)
{
    if (!g_app.videoTexture) return false;

    FrameView view{};
    if (!g_app.videoTexture->TryGetFrame(view))
    {
        static int miss = 0;
        if (++miss % 60 == 0)
            OutputDebugStringW(L"[video] no frame yet\n");
        return true;
    }

    EnsurePlaneForFrame(view);
    SpawnPlaneIfNeeded(dt, view);
    UpdatePlanes(dt, targetDC, view);
    UpdateColliderFromFirstPlane();

    if (!g_app.videoAudioStarted && !g_app.audioWavBytes.empty())
    {
        PlaySoundW(reinterpret_cast<LPCWSTR>(g_app.audioWavBytes.data()), nullptr, SND_ASYNC | SND_MEMORY | SND_LOOP);
        g_app.videoAudioStarted = true;
    }

    return true;
}

void RenderFallback(HDC targetDC)
{
    TRIVERTEX vert[2] = {
        {0, 0, 0x1000, 0x1000, 0x1000, 0x0000},
        {g_app.bounds.right, g_app.bounds.bottom, 0x4000, 0x4000, 0x5000, 0x0000}
    };
    GRADIENT_RECT gr{ 0,1 };
    GradientFill(targetDC, vert, 2, &gr, 1, GRADIENT_FILL_RECT_V);

    int bw = 200;
    int bh = 200;
    int bx = (g_app.bounds.right - bw) / 2;
    int by = (g_app.bounds.bottom - bh) / 2;
    RECT box{ bx, by, bx + bw, by + bh };
    HBRUSH b = CreateSolidBrush(RGB(0, 122, 255));
    FillRect(targetDC, &box, b);
    DeleteObject(b);
    g_app.collider.Set(bx, by, bw, bh);
}
