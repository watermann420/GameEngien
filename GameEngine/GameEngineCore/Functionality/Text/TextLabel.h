#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <memory>

// Lightweight GDI-based text sprite with per-character coloring, spacing, and basic glow.
// Designed for realtime overlay/demo use (no GDI+ / DirectWrite dependency).
class TextLabel
{
public:
    TextLabel(std::wstring text,
              int fontPx = 32,
              COLORREF baseColor = RGB(255, 255, 255),
              int letterSpacingPx = 1,
              int glowRadiusPx = 2,
              COLORREF glowColor = RGB(0, 0, 0));
    ~TextLabel();

    void SetText(std::wstring text);
    void SetPosition(float x, float y);
    void SetVelocity(float vx, float vy);
    void SetLetterSpacing(int px);
    void SetBaseColor(COLORREF color);
    void SetGlow(int radiusPx, COLORREF color);

    // Provide per-character colors; empty vector = use baseColor for all.
    void SetCharColors(std::vector<COLORREF> colors);

    // Advance position with simple axis-aligned bounce inside bounds.
    void Update(double dtSeconds, const RECT& bounds);

    // Draw text (and glow if set) into target DC.
    void Draw(HDC hdc) const;

    SIZE Measure(HDC hdc) const;
    size_t Length() const { return m_text.size(); }

private:
    std::wstring m_text;
    HFONT m_font{ nullptr };
    int m_fontPx{ 32 };
    COLORREF m_baseColor{ RGB(255, 255, 255) };
    int m_letterSpacing{ 1 };
    int m_glowRadius{ 2 };
    COLORREF m_glowColor{ RGB(0, 0, 0) };
    std::vector<COLORREF> m_charColors;
    float m_x{ 0.0f };
    float m_y{ 0.0f };
    float m_vx{ -120.0f }; // default leftward motion (pixels/s)
    float m_vy{ 0.0f };

    void RebuildFont();
    void DrawGlow(HDC hdc, int x, int y) const;
};
