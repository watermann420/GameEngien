#include "TextLabel.h"
#include <cmath>

TextLabel::TextLabel(std::wstring text, int fontPx, COLORREF baseColor, int letterSpacingPx, int glowRadiusPx, COLORREF glowColor)
    : m_text(std::move(text)),
      m_fontPx(fontPx),
      m_baseColor(baseColor),
      m_letterSpacing(letterSpacingPx),
      m_glowRadius(glowRadiusPx),
      m_glowColor(glowColor)
{
    RebuildFont();
}

TextLabel::~TextLabel()
{
    if (m_font)
    {
        DeleteObject(m_font);
        m_font = nullptr;
    }
}

void TextLabel::RebuildFont()
{
    if (m_font)
    {
        DeleteObject(m_font);
        m_font = nullptr;
    }
    // Bold, anti-aliased; negative height means character height in logical units.
    m_font = CreateFontW(
        -m_fontPx, 0, 0, 0,
        FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");
}

void TextLabel::SetText(std::wstring text)
{
    m_text = std::move(text);
    m_charColors.clear();
}

void TextLabel::SetPosition(float x, float y)
{
    m_x = x;
    m_y = y;
}

void TextLabel::SetVelocity(float vx, float vy)
{
    m_vx = vx;
    m_vy = vy;
}

void TextLabel::SetLetterSpacing(int px)
{
    m_letterSpacing = px;
}

void TextLabel::SetBaseColor(COLORREF color)
{
    m_baseColor = color;
}

void TextLabel::SetGlow(int radiusPx, COLORREF color)
{
    m_glowRadius = radiusPx;
    m_glowColor = color;
}

void TextLabel::SetCharColors(std::vector<COLORREF> colors)
{
    m_charColors = std::move(colors);
    if (m_charColors.size() < m_text.size())
    {
        m_charColors.resize(m_text.size(), m_baseColor);
    }
}

SIZE TextLabel::Measure(HDC hdc) const
{
    SIZE sz{ 0,0 };
    if (m_text.empty()) return sz;

    HFONT old = (HFONT)SelectObject(hdc, m_font);
    // Account for extra spacing manually.
    GetTextExtentPoint32W(hdc, m_text.c_str(), static_cast<int>(m_text.size()), &sz);
    sz.cx += m_letterSpacing * static_cast<int>(m_text.size() ? (m_text.size() - 1) : 0);
    SelectObject(hdc, old);
    return sz;
}

void TextLabel::Update(double dtSeconds, const RECT& bounds)
{
    m_x += static_cast<float>(m_vx * dtSeconds);
    m_y += static_cast<float>(m_vy * dtSeconds);

    const float left = static_cast<float>(bounds.left);
    const float top = static_cast<float>(bounds.top);
    const float right = static_cast<float>(bounds.right);
    const float bottom = static_cast<float>(bounds.bottom);

    // For width/height we need a DC; use screen DC briefly.
    HDC screen = GetDC(nullptr);
    SIZE sz = Measure(screen);
    ReleaseDC(nullptr, screen);

    if (m_x < left)
    {
        m_x = left;
        m_vx = std::fabs(m_vx);
    }
    else if (m_x + sz.cx > right)
    {
        m_x = right - sz.cx;
        m_vx = -std::fabs(m_vx);
    }

    if (m_y < top)
    {
        m_y = top;
        m_vy = std::fabs(m_vy);
    }
    else if (m_y + sz.cy > bottom)
    {
        m_y = bottom - sz.cy;
        m_vy = -std::fabs(m_vy);
    }
}

void TextLabel::DrawGlow(HDC hdc, int x, int y) const
{
    if (m_glowRadius <= 0) return;
    HFONT oldFont = (HFONT)SelectObject(hdc, m_font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, m_glowColor);

    const int r = m_glowRadius;
    for (int dy = -r; dy <= r; ++dy)
    {
        for (int dx = -r; dx <= r; ++dx)
        {
            if (dx == 0 && dy == 0) continue;
            TextOutW(hdc, x + dx, y + dy, m_text.c_str(), static_cast<int>(m_text.size()));
        }
    }
    SelectObject(hdc, oldFont);
}

void TextLabel::Draw(HDC hdc) const
{
    if (m_text.empty()) return;

    HFONT oldFont = (HFONT)SelectObject(hdc, m_font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextCharacterExtra(hdc, m_letterSpacing);

    const int x = static_cast<int>(std::lround(m_x));
    const int y = static_cast<int>(std::lround(m_y));

    // Glow pass first
    DrawGlow(hdc, x, y);

    // Main text with per-character color if provided.
    if (m_charColors.empty())
    {
        SetTextColor(hdc, m_baseColor);
        TextOutW(hdc, x, y, m_text.c_str(), static_cast<int>(m_text.size()));
    }
    else
    {
        int penX = x;
        SIZE glyph{};
        for (size_t i = 0; i < m_text.size(); ++i)
        {
            wchar_t ch = m_text[i];
            SetTextColor(hdc, m_charColors[i]);
            GetTextExtentPoint32W(hdc, &ch, 1, &glyph);
            TextOutW(hdc, penX, y, &ch, 1);
            penX += glyph.cx + m_letterSpacing;
        }
    }

    SetTextCharacterExtra(hdc, 0);
    SelectObject(hdc, oldFont);
}
