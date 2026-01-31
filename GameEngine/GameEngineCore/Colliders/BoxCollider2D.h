#pragma once
#include <windows.h>

// Axis-aligned 2D rectangle collider used for simple hit tests.
class BoxCollider2D
{
public:
    BoxCollider2D() : m_rect{ 0,0,0,0 } {}
    BoxCollider2D(int x, int y, int w, int h) { Set(x, y, w, h); }

    void Set(int x, int y, int w, int h)
    {
        m_rect.left = x;
        m_rect.top = y;
        m_rect.right = x + w;
        m_rect.bottom = y + h;
    }

    bool Contains(POINT p) const
    {
        return p.x >= m_rect.left && p.x < m_rect.right &&
               p.y >= m_rect.top && p.y < m_rect.bottom;
    }

    const RECT& Rect() const { return m_rect; }

private:
    RECT m_rect;
};
