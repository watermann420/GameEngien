#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "msimg32.lib")
#include <filesystem>
#include <memory>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <map>
#include <cmath>
#include <filesystem>
#include <mmreg.h>
#include "RenderPipeline2D/Renderer2D.h"
#include "Colliders/BoxCollider2D.h"
#include "Functionality/Video/MFVideoSource.h"
#include "Functionality/Video/VideoPlane.h"
#include "Functionality/Audio/AudioDecoder.h"
#include "textur/VideoRenderPipline/VideoTexture.h"
#include "Scripts/BlueBox2DTest.h"

static const int kWindowWidth = 800;
static const int kWindowHeight = 600;
static Renderer2D g_renderer(200, 200, RGB(0, 122, 255));
static BoxCollider2D g_collider;
static HWND g_overlay = nullptr;
static HMIDIOUT g_midiOut = nullptr;
static HDC g_memDC = nullptr;
static HBITMAP g_memBmp = nullptr;
static HGDIOBJ g_oldBmp = nullptr;
static RECT g_bounds{};
static std::unique_ptr<VideoTexture> g_videoTexture;
static bool g_videoAudioStarted = false;
static std::wstring g_videoPath;
static std::wstring g_audioPath;
static VideoPlane g_plane;
static std::vector<VideoPlane> g_planes;
static double g_spawnAccumulator = 0.0;
static std::vector<BYTE> g_audioWavBytes; // one WAV buffer
static bool g_audioOpen = false;
static std::vector<BYTE> g_audioWav;
namespace fs = std::filesystem;

// Generate and play a WAV-from-memory instantly for the given MIDI note.
void PlayImmediateNote(int midiNote, int velocity, double seconds = 0.25)
{
    const int sampleRate = 48000;
    const double freq = 440.0 * pow(2.0, (midiNote - 69) / 12.0);
    const double vol = std::clamp(velocity / 127.0, 0.05, 1.0) * 0.6;
    const int samples = static_cast<int>(seconds * sampleRate);

    // cache per note/velocity to keep buffers alive for SND_MEMORY
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

bool StartVideoAudio()
{
    if (g_videoPath.empty()) return false;
    // Open unique alias for stacked playback.
    return true; // mixer removed
}

void StopAudio()
{
    PlaySoundW(nullptr, nullptr, 0);
    g_audioOpen = false;
}

bool EnsureMidi()
{
    static bool tried = false;
    static bool ok = false;
    if (tried) return ok;
    tried = true;
    if (midiOutOpen(&g_midiOut, MIDI_MAPPER, 0, 0, 0) == MMSYSERR_NOERROR)
    {
        ok = true;
    }
    return ok;
}

void PlayMidiPiano(int note = 60, int velocity = 100, int durationMs = 200)
{
    if (EnsureMidi())
    {
        DWORD on = 0;
        on |= 0x90;                 // note on, channel 0
        on |= (static_cast<DWORD>(note) & 0x7F) << 8;
        on |= (static_cast<DWORD>(velocity) & 0x7F) << 16;
        midiOutShortMsg(g_midiOut, on);

        // schedule note off
        std::thread([note]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            DWORD off = 0;
            off |= 0x80;
            off |= (static_cast<DWORD>(note) & 0x7F) << 8;
            midiOutShortMsg(g_midiOut, off);
        }).detach();
        return;
    }
    // Fallback to synthesized tone if MIDI init failed
    PlayImmediateNote(note, velocity, durationMs / 1000.0);
}

// Play immediate synthesized note (zero latency).
void PlayMusicEngineNote(int note = 60, int velocity = 100, double duration = 0.6)
{
    PlayMidiPiano(note, velocity, static_cast<int>(duration * 1000));
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_LBUTTONDOWN:
    {
        POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }; // client coords
        if (g_collider.Contains(p))
        {
            PlayMusicEngineNote(); // Trigger C4 via MusicEngine when the box is clicked.
        }
        return 0;
    }
    case WM_RBUTTONDOWN:
        PostQuitMessage(0); // right-click to close
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0); // ESC to close
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Creates a borderless layered overlay window to host the box without flicker.
HWND CreateOverlayWindow(HINSTANCE hInstance, int width, int height)
{
    const wchar_t* CLASS_NAME = L"GameEngineOverlayClass";

    WNDCLASSW wc{};
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        CLASS_NAME,
        L"",
        WS_POPUP,
        0, 0, width, height,
        nullptr, nullptr, hInstance, nullptr);

    return hwnd;
}

void ShowBoxOverlay(HWND hwnd, int width, int height, COLORREF color)
{
    HDC screen = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screen);
    HBITMAP bmp = CreateCompatibleBitmap(screen, width, height);
    HGDIOBJ old = SelectObject(memDC, bmp);

    RECT area{ 0,0,width,height };
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(memDC, &area, brush);
    DeleteObject(brush);

    POINT pos{
        (GetSystemMetrics(SM_CXSCREEN) - width) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - height) / 2
    };
    SIZE size{ width, height };
    POINT src{ 0,0 };
    BLENDFUNCTION bf{};
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = 0;

    UpdateLayeredWindow(hwnd, screen, &pos, &size, memDC, &src, 0, &bf, ULW_ALPHA);

    // collider in client coords (same as box since window is box-sized)
    g_collider.Set(0, 0, width, height);

    SelectObject(memDC, old);
    DeleteObject(bmp);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screen);

    ShowWindow(hwnd, SW_SHOW);
}

int main()
{
    // Prefer low-latency timers and slightly elevated priority for real-time feel.
    timeBeginPeriod(1);
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    SetProcessDPIAware(); // avoid DPI scaling shifts when centering on desktop

    // Headless/offscreen test: render into a BMP on disk
    g_renderer.RenderToBitmapFile(L"headless_output.bmp", kWindowWidth, kWindowHeight);

    // Optional: load MP4 from files\video.mp4 to texture the 2D plane.
    // Locate media in files/ (pick first mp4 for video, first mp3 as audio fallback).
    g_videoPath.clear();
    g_audioPath.clear();
    const fs::path filesDir = fs::path("files");
    if (fs::exists(filesDir) && fs::is_directory(filesDir))
    {
        for (auto& p : fs::directory_iterator(filesDir))
        {
            if (p.path().extension() == L".mp4")
            {
                g_videoPath = p.path().wstring();
                break;
            }
        }
        for (auto& p : fs::directory_iterator(filesDir))
        {
            if (p.path().extension() == L".mp3")
            {
                g_audioPath = p.path().wstring();
                break;
            }
        }
    }
    if (!g_videoPath.empty())
    {
        auto src = std::make_unique<MFVideoSource>(g_videoPath);
        if (src->IsReady())
        {
            g_videoTexture = std::make_unique<VideoTexture>(std::move(src));
            g_videoTexture->Start();
            std::wstringstream ss; ss << L"[video] playing " << g_videoPath << L"\n";
            OutputDebugStringW(ss.str().c_str());
        }
        // Decode audio once to PCM16 and wrap into WAV header for PlaySound
        DecodedAudio pcm16;
        if (DecodeAudioPCM16(g_videoPath, pcm16))
        {
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

            g_audioWavBytes.resize(sizeof(hdr) + hdr.dataSize);
            memcpy(g_audioWavBytes.data(), &hdr, sizeof(hdr));
            memcpy(g_audioWavBytes.data() + sizeof(hdr), pcm16.samples.data(), hdr.dataSize);
        }
    }

    // Flicker-free overlay: fullscreen layered window; click inside box to exit.
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    g_overlay = CreateOverlayWindow(hInstance, screenW, screenH);
    g_bounds = RECT{ 0,0,screenW,screenH };

    // Prepare offscreen buffer for layered updates (compatible DC/bitmap).
    HDC screen = GetDC(nullptr);
    g_memDC = CreateCompatibleDC(screen);
    g_memBmp = CreateCompatibleBitmap(screen, screenW, screenH);
    g_oldBmp = SelectObject(g_memDC, g_memBmp);

    // Position collider center screen.
    g_collider.Set((screenW - g_renderer.BoxWidth()) / 2,
                   (screenH - g_renderer.BoxHeight()) / 2,
                   g_renderer.BoxWidth(),
                   g_renderer.BoxHeight());

    // Initialize text via BlueBox test helpers.
    InitBlueBoxText(g_bounds);

    ShowWindow(g_overlay, SW_SHOW);

    MSG msg{};
    ULONGLONG prevTicks = GetTickCount64();
    bool running = true;
    while (running)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        ULONGLONG now = GetTickCount64();
        double dt = static_cast<double>(now - prevTicks) / 1000.0;
        prevTicks = now;

        // Clear with magenta colorkey (will be made transparent).
        HBRUSH clearBrush = CreateSolidBrush(RGB(255, 0, 255));
        RECT clearRect{ 0,0,screenW,screenH };
        FillRect(g_memDC, &clearRect, clearBrush);
        DeleteObject(clearBrush);

    // If video available, blit onto bouncing planes and use first as collider.
    if (g_videoTexture)
    {
        FrameView view{};
        if (g_videoTexture->TryGetFrame(view))
        {
            // Ensure at least one plane
            if (g_planes.empty())
            {
                VideoPlane p;
                p.InitFromFrame(view.desc, g_bounds, 0.35f);
                p.SetVelocity(-220.0f, 140.0f);
                g_planes.push_back(p);
                // start audio once when first plane appears
                if (!g_videoAudioStarted && !g_audioWavBytes.empty())
                {
                    PlaySoundW(reinterpret_cast<LPCWSTR>(g_audioWavBytes.data()), nullptr, SND_ASYNC | SND_MEMORY | SND_LOOP);
                    g_videoAudioStarted = true;
                }
            }

            // Spawn additional plane every ~2s (no hard cap)
            g_spawnAccumulator += dt;
            if (g_spawnAccumulator >= 2.0)
            {
                g_spawnAccumulator = 0.0;
                VideoPlane p;
                p.InitFromFrame(view.desc, g_bounds, 0.35f);
                p.SetPosition(static_cast<float>(g_bounds.right - p.Bounds().right),
                              static_cast<float>((g_bounds.bottom) * 0.2f));
                p.SetVelocity(-250.0f - 10.0f * static_cast<float>(g_planes.size()), 120.0f);
                g_planes.push_back(p);

                // only one audio instance; nothing to do for later planes
            }

            // Update/render planes
            for (auto& p : g_planes)
            {
                p.Update(dt, g_bounds);
                p.Render(g_memDC, view);
            }

            // Collider = first plane
            RECT r = g_planes.front().Bounds();
            g_collider.Set(r.left, r.top, r.right - r.left, r.bottom - r.top);

            g_videoAudioStarted = true;
        }
        else
        {
            static int miss = 0;
            if (++miss % 60 == 0)
            {
                OutputDebugStringW(L"[video] no frame yet\n");
            }
        }
    }
        else
        {
            // fallback: dim background with a simple gradient to show overlay is alive
            TRIVERTEX vert[2] = {
                {0, 0, 0x1000, 0x1000, 0x1000, 0x0000},
                {g_bounds.right, g_bounds.bottom, 0x4000, 0x4000, 0x5000, 0x0000}
            };
            GRADIENT_RECT gr{0,1};
            GradientFill(g_memDC, vert, 2, &gr, 1, GRADIENT_FILL_RECT_V);

            // collider center small box for exit
            int bw = 200, bh = 200;
            int bx = (g_bounds.right - bw)/2;
            int by = (g_bounds.bottom - bh)/2;
            RECT box{ bx, by, bx + bw, by + bh };
            HBRUSH b = CreateSolidBrush(RGB(0,122,255));
            FillRect(g_memDC, &box, b);
            DeleteObject(b);
            g_collider.Set(bx, by, bw, bh);
        }

        // Push to layered window using colorkey magenta so only plane is visible.
        POINT pos{ 0,0 };
        SIZE size{ screenW, screenH };
        POINT src{ 0,0 };
        COLORREF key = RGB(255, 0, 255);
        UpdateLayeredWindow(g_overlay, screen, &pos, &size, g_memDC, &src, key, nullptr, ULW_COLORKEY);

        Sleep(1);
    }

    // Cleanup
    if (g_oldBmp) SelectObject(g_memDC, g_oldBmp);
    if (g_memBmp) DeleteObject(g_memBmp);
    if (g_memDC) DeleteDC(g_memDC);
    ReleaseDC(nullptr, screen);
    DestroyWindow(g_overlay);

    timeEndPeriod(1);
    CoUninitialize();
    return 0;
}
