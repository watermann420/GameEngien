#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include <filesystem>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <map>
#include <cmath>
#include "RenderPipeline2D/Renderer2D.h"
#include "Colliders/BoxCollider2D.h"

static const int kWindowWidth = 800;
static const int kWindowHeight = 600;
static Renderer2D g_renderer(200, 200, RGB(0, 122, 255));
static BoxCollider2D g_collider;
static HWND g_overlay = nullptr;
static HMIDIOUT g_midiOut = nullptr;
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
    SetProcessDPIAware(); // avoid DPI scaling shifts when centering on desktop

    // Headless/offscreen test: render into a BMP on disk
    g_renderer.RenderToBitmapFile(L"headless_output.bmp", kWindowWidth, kWindowHeight);

    // Flicker-free overlay: borderless layered window, topmost; click inside box to exit.
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    g_overlay = CreateOverlayWindow(hInstance, g_renderer.BoxWidth(), g_renderer.BoxHeight());
    ShowBoxOverlay(g_overlay, g_renderer.BoxWidth(), g_renderer.BoxHeight(), g_renderer.Color());

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyWindow(g_overlay);

    return 0;
}
