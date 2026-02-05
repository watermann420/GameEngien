#include "EditorDraw.h"

#include <algorithm>
#include <cmath>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

void DrawHeader(HDC hdc, const RECT& rect, const wchar_t* label)
{
    HBRUSH bar = CreateSolidBrush(RGB(22, 22, 26));
    FillRect(hdc, &rect, bar);
    DeleteObject(bar);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(210, 210, 210));
    TextOutW(hdc, rect.left + 8, rect.top + 4, label, (int)wcslen(label));
}

struct Vec3
{
    float x;
    float y;
    float z;
};

static void ComputeBasis(float yaw, float pitch, Vec3& forward, Vec3& right, Vec3& up)
{
    float cy = cosf(yaw);
    float sy = sinf(yaw);
    float cp = cosf(pitch);
    float sp = sinf(pitch);

    forward = { sy * cp, sp, cy * cp };
    right = { cy, 0.0f, -sy };
    up = { -sy * sp, cp, -cy * sp };
}

static bool ProjectPoint(const RECT& rect, const Vec3& world, float yaw, float pitch, float camX, float camY, float camZ, POINT& out)
{
    Vec3 rel{ world.x - camX, world.y - camY, world.z - camZ };
    Vec3 forward{}, right{}, up{};
    ComputeBasis(yaw, pitch, forward, right, up);
    float viewX = rel.x * right.x + rel.y * right.y + rel.z * right.z;
    float viewY = rel.x * up.x + rel.y * up.y + rel.z * up.z;
    float viewZ = rel.x * forward.x + rel.y * forward.y + rel.z * forward.z;
    if (viewZ < 0.1f)
        return false;

    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    float fov = 220.0f;
    float sx = (viewX / viewZ) * fov + (rect.left + w / 2.0f);
    float sy = (-viewY / viewZ) * fov + (rect.top + h / 2.0f);
    out.x = (int)sx;
    out.y = (int)sy;
    return true;
}

static void DrawLine3D(HDC hdc, const RECT& rect, const Vec3& a, const Vec3& b, float yaw, float pitch, float camX, float camY, float camZ)
{
    POINT pa{}, pb{};
    if (!ProjectPoint(rect, a, yaw, pitch, camX, camY, camZ, pa)) return;
    if (!ProjectPoint(rect, b, yaw, pitch, camX, camY, camZ, pb)) return;
    MoveToEx(hdc, pa.x, pa.y, nullptr);
    LineTo(hdc, pb.x, pb.y);
}

static void DrawQuad3D(HDC hdc, const RECT& rect, const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, float yaw, float pitch, float camX, float camY, float camZ)
{
    POINT pa{}, pb{}, pc{}, pd{};
    if (!ProjectPoint(rect, a, yaw, pitch, camX, camY, camZ, pa)) return;
    if (!ProjectPoint(rect, b, yaw, pitch, camX, camY, camZ, pb)) return;
    if (!ProjectPoint(rect, c, yaw, pitch, camX, camY, camZ, pc)) return;
    if (!ProjectPoint(rect, d, yaw, pitch, camX, camY, camZ, pd)) return;
    POINT pts[4]{ pa, pb, pc, pd };
    Polygon(hdc, pts, 4);
}

void DrawScenePreview(HDC hdc,
    const RECT& rect,
    bool playing,
    float zoom,
    int panX,
    int panY,
    bool view3D,
    float camYaw,
    float camPitch,
    float camX,
    float camY,
    float camZ,
    bool showCenterCube,
    bool showOrbiter)
{
    HBRUSH bg = CreateSolidBrush(RGB(12, 12, 14));
    FillRect(hdc, &rect, bg);
    DeleteObject(bg);

    if (view3D)
    {
        HPEN grid = CreatePen(PS_SOLID, 1, RGB(28, 28, 36));
        HGDIOBJ oldGrid = SelectObject(hdc, grid);
        for (int i = -10; i <= 10; ++i)
        {
            DrawLine3D(hdc, rect, { (float)i, 0.0f, -10.0f }, { (float)i, 0.0f, 10.0f }, camYaw, camPitch, camX, camY, camZ);
            DrawLine3D(hdc, rect, { -10.0f, 0.0f, (float)i }, { 10.0f, 0.0f, (float)i }, camYaw, camPitch, camX, camY, camZ);
        }
        SelectObject(hdc, oldGrid);
        DeleteObject(grid);

        if (showCenterCube)
        {
            Vec3 center{ 0.0f, 0.0f, 0.0f };
            float half = 0.8f;
            HBRUSH ballBrush = CreateSolidBrush(RGB(80, 160, 240));
            HGDIOBJ oldBrush = SelectObject(hdc, ballBrush);
            Vec3 a{ center.x - half, center.y, center.z - half };
            Vec3 b{ center.x + half, center.y, center.z - half };
            Vec3 c{ center.x + half, center.y, center.z + half };
            Vec3 d{ center.x - half, center.y, center.z + half };
            DrawQuad3D(hdc, rect, a, b, c, d, camYaw, camPitch, camX, camY, camZ);
            SelectObject(hdc, oldBrush);
            DeleteObject(ballBrush);
        }

        if (showOrbiter)
        {
            double t = playing ? (GetTickCount64() / 1000.0) : 0.0;
            float orbitR = 2.4f;
            Vec3 orb{ (float)cos(t) * orbitR, 0.0f, (float)sin(t) * orbitR };
            float half = 0.25f;
            HBRUSH orbBrush = CreateSolidBrush(RGB(180, 210, 255));
            HGDIOBJ oldOrb = SelectObject(hdc, orbBrush);
            Vec3 a{ orb.x - half, orb.y, orb.z - half };
            Vec3 b{ orb.x + half, orb.y, orb.z - half };
            Vec3 c{ orb.x + half, orb.y, orb.z + half };
            Vec3 d{ orb.x - half, orb.y, orb.z + half };
            DrawQuad3D(hdc, rect, a, b, c, d, camYaw, camPitch, camX, camY, camZ);
            SelectObject(hdc, oldOrb);
            DeleteObject(orbBrush);
        }
        return;
    }

    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    int cx = rect.left + w / 2 + panX;
    int cy = rect.top + h / 2 + panY;

    HPEN grid = CreatePen(PS_SOLID, 1, RGB(25, 25, 30));
    HGDIOBJ oldPen = SelectObject(hdc, grid);
    for (int x = rect.left + 12; x < rect.right; x += 24)
    {
        MoveToEx(hdc, x, rect.top, nullptr);
        LineTo(hdc, x, rect.bottom);
    }
    for (int y = rect.top + 12; y < rect.bottom; y += 24)
    {
        MoveToEx(hdc, rect.left, y, nullptr);
        LineTo(hdc, rect.right, y);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(grid);

    HPEN axis = CreatePen(PS_SOLID, 1, RGB(80, 80, 90));
    HGDIOBJ oldAxis = SelectObject(hdc, axis);
    MoveToEx(hdc, rect.left + 10, cy, nullptr);
    LineTo(hdc, rect.right - 10, cy);
    MoveToEx(hdc, cx, rect.top + 10, nullptr);
    LineTo(hdc, cx, rect.bottom - 10);
    SelectObject(hdc, oldAxis);
    DeleteObject(axis);

    if (showCenterCube)
    {
        int r = (int)(36 * zoom);
        HBRUSH ballBrush = CreateSolidBrush(RGB(80, 160, 240));
        HGDIOBJ oldBrush = SelectObject(hdc, ballBrush);
        Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
        SelectObject(hdc, oldBrush);
        DeleteObject(ballBrush);
    }

    int orbitR = (int)(80 * zoom);

    if (showOrbiter)
    {
        double t = playing ? (GetTickCount64() / 1000.0) : 0.0;
        int ox = cx + (int)(std::cos(t) * orbitR);
        int oy = cy + (int)(std::sin(t) * orbitR);
        int r = (int)(10 * zoom);
        HBRUSH orbBrush = CreateSolidBrush(RGB(180, 210, 255));
        HGDIOBJ oldBrush = SelectObject(hdc, orbBrush);
        Ellipse(hdc, ox - r, oy - r, ox + r, oy + r);
        SelectObject(hdc, oldBrush);
        DeleteObject(orbBrush);
    }

    SetTextColor(hdc, RGB(200, 200, 200));
    TextOutW(hdc, rect.left + 10, rect.bottom - 20, L"Editor preview (orbit example)", 32);
}

void DrawInspector(HDC hdc, const RECT& rect, bool playing)
{
    HBRUSH bg = CreateSolidBrush(RGB(16, 16, 20));
    FillRect(hdc, &rect, bg);
    DeleteObject(bg);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(200, 200, 200));
    int y = rect.top + 8;
    TextOutW(hdc, rect.left + 10, y, L"Selection: SceneRoot", 22);
    y += 18;
    TextOutW(hdc, rect.left + 10, y, L"Transform: (0,0,0)", 22);
    y += 18;
    TextOutW(hdc, rect.left + 10, y, L"Material: Default", 18);
    y += 18;
    const wchar_t* playLabel = playing ? L"Play Mode: On" : L"Play Mode: Paused";
    TextOutW(hdc, rect.left + 10, y, playLabel, (int)wcslen(playLabel));
}
