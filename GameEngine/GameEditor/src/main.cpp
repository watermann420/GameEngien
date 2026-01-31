#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#include <filesystem>
#include <string>
#include <vector>
#include <sstream>

namespace fs = std::filesystem;

static std::vector<std::wstring> g_logs;
static const int kTopBarH = 36;
static HWND g_consoleWnd = nullptr;
static bool g_consoleVisible = true;
static const int kConsoleLinesMax = 200;
enum { ID_TOGGLE_CONSOLE = 1001 };

struct FileEntry
{
    std::wstring label;
    fs::path path;
    bool isDir{ false };
    int depth{ 0 };
};

static std::vector<FileEntry> g_files;
static int g_selectedIndex = -1;
static const int kProjectW = 280;
static const int kLineH = 16;
static const int kResizeBorder = 6;

static HMENU g_menuFile = nullptr;
static HMENU g_menuEdit = nullptr;
static HMENU g_menuView = nullptr;
static HMENU g_menuWindow = nullptr;
static HMENU g_menuHelp = nullptr;

static std::vector<RECT> g_menuRects;
static int g_hoverMenu = -1;
static RECT g_btnClose{};
static RECT g_btnMin{};

void AddLog(const std::wstring& line)
{
    g_logs.push_back(line);
    if (g_logs.size() > kConsoleLinesMax)
        g_logs.erase(g_logs.begin());
    if (g_consoleWnd)
        InvalidateRect(g_consoleWnd, nullptr, FALSE);
}

void EnableDarkTitleBar(HWND hwnd)
{
    BOOL useDark = TRUE;
    const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    if (FAILED(DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof(useDark))))
    {
        const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_OLD = 19;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, &useDark, sizeof(useDark));
    }
}

void BuildMenus()
{
    g_menuFile = CreatePopupMenu();
    AppendMenuW(g_menuFile, MF_STRING, 2001, L"New Folder");
    AppendMenuW(g_menuFile, MF_STRING, 2002, L"Refresh");
    AppendMenuW(g_menuFile, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g_menuFile, MF_STRING, 2003, L"Exit");

    g_menuEdit = CreatePopupMenu();
    AppendMenuW(g_menuEdit, MF_STRING, 2101, L"Preferences...");

    g_menuView = CreatePopupMenu();
    AppendMenuW(g_menuView, MF_STRING, ID_TOGGLE_CONSOLE, L"Console");

    g_menuWindow = CreatePopupMenu();
    AppendMenuW(g_menuWindow, MF_STRING, ID_TOGGLE_CONSOLE, L"Console");

    g_menuHelp = CreatePopupMenu();
    AppendMenuW(g_menuHelp, MF_STRING, 2401, L"About");
}

void ShowMenuPopup(HWND hwnd, HMENU menu, int x, int y)
{
    POINT pt{ x, y };
    ClientToScreen(hwnd, &pt);
    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, hwnd, nullptr);
}

void CollectTree(const fs::path& root, int depth, std::vector<FileEntry>& out)
{
    if (!fs::exists(root)) return;
    std::vector<fs::directory_entry> dirs;
    std::vector<fs::directory_entry> files;
    for (auto& p : fs::directory_iterator(root))
    {
        if (p.is_directory()) dirs.push_back(p);
        else files.push_back(p);
    }
    for (auto& p : dirs)
    {
        FileEntry e;
        e.depth = depth;
        e.isDir = true;
        e.path = p.path();
        e.label = L"[D] " + p.path().filename().wstring();
        out.push_back(e);
        CollectTree(p.path(), depth + 1, out);
    }
    for (auto& p : files)
    {
        FileEntry e;
        e.depth = depth;
        e.isDir = false;
        e.path = p.path();
        e.label = L"[F] " + p.path().filename().wstring();
        out.push_back(e);
    }
}

void RefreshFiles()
{
    g_files.clear();
    CollectTree(fs::path("EngineFiles"), 0, g_files);
}

LRESULT CALLBACK ConsoleWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        g_consoleVisible = false;
        return 0;
    case WM_LBUTTONDOWN:
        ReleaseCapture();
        SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        HBRUSH bg = CreateSolidBrush(RGB(10, 10, 12));
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(230, 230, 230));
        TextOutW(hdc, 10, 8, L"Console", 7);

        int y = 28;
        for (auto it = g_logs.rbegin(); it != g_logs.rend(); ++it)
        {
            if (y > rc.bottom - 14) break;
            TextOutW(hdc, 10, y, it->c_str(), (int)it->size());
            y += 14;
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        AddLog(L"[init] GameEditor started");
        fs::path root = fs::path("EngineFiles");
        if (fs::exists(root))
            AddLog(L"[workspace] EngineFiles found");
        else
            AddLog(L"[workspace] EngineFiles not found");
        RefreshFiles();

        const wchar_t CONSOLE_CLASS[] = L"GameEditorConsole";
        WNDCLASSW cwc{};
        cwc.lpfnWndProc = ConsoleWndProc;
        cwc.hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
        cwc.lpszClassName = CONSOLE_CLASS;
        cwc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassW(&cwc);

        g_consoleWnd = CreateWindowExW(
            WS_EX_TOOLWINDOW,
            CONSOLE_CLASS,
            L"GameEditor Console",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            50, 500, 900, 260,
            nullptr, nullptr, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), nullptr);
        g_consoleVisible = true;
        BuildMenus();
        EnableDarkTitleBar(hwnd);
        EnableDarkTitleBar(g_consoleWnd);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TOGGLE_CONSOLE)
        {
            g_consoleVisible = !g_consoleVisible;
            ShowWindow(g_consoleWnd, g_consoleVisible ? SW_SHOW : SW_HIDE);
        }
        else if (LOWORD(wParam) == 2001)
        {
            // New Folder
            fs::path base = fs::path("EngineFiles");
            if (g_selectedIndex >= 0 && g_selectedIndex < (int)g_files.size())
            {
                if (g_files[g_selectedIndex].isDir)
                    base = g_files[g_selectedIndex].path;
                else
                    base = g_files[g_selectedIndex].path.parent_path();
            }
            fs::path newDir = base / L"NewFolder";
            int suffix = 1;
            while (fs::exists(newDir))
                newDir = base / (L"NewFolder" + std::to_wstring(suffix++));
            fs::create_directories(newDir);
            AddLog(L"[project] created folder " + newDir.filename().wstring());
            RefreshFiles();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == 2002)
        {
            RefreshFiles();
            AddLog(L"[project] refreshed");
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (LOWORD(wParam) == 2003)
        {
            PostQuitMessage(0);
        }
        else if (LOWORD(wParam) == 2401)
        {
            MessageBoxW(hwnd, L"GameEditor template\n(Win32 UI stub)", L"About", MB_OK | MB_ICONINFORMATION);
        }
        break;
    case WM_RBUTTONDOWN:
    {
        // Right-click in project panel: create folder under selected dir or root.
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT rc; GetClientRect(hwnd, &rc);
        RECT project{ 0, kTopBarH, kProjectW, rc.bottom };
        if (PtInRect(&project, pt))
        {
            fs::path base = fs::path("EngineFiles");
            if (g_selectedIndex >= 0 && g_selectedIndex < (int)g_files.size())
            {
                if (g_files[g_selectedIndex].isDir)
                    base = g_files[g_selectedIndex].path;
                else
                    base = g_files[g_selectedIndex].path.parent_path();
            }
            fs::path newDir = base / L"NewFolder";
            int suffix = 1;
            while (fs::exists(newDir))
            {
                newDir = base / (L"NewFolder" + std::to_wstring(suffix++));
            }
            fs::create_directories(newDir);
            AddLog(L"[project] created folder " + newDir.filename().wstring());
            RefreshFiles();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        break;
    }
    case WM_KEYDOWN:
        if (wParam == VK_F1)
        {
            g_consoleVisible = !g_consoleVisible;
            ShowWindow(g_consoleWnd, g_consoleVisible ? SW_SHOW : SW_HIDE);
        }
        break;
    case WM_LBUTTONDOWN:
    {
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT rc; GetClientRect(hwnd, &rc);
        // top bar buttons
        if (PtInRect(&g_btnClose, pt))
        {
            PostQuitMessage(0);
            return 0;
        }
        if (PtInRect(&g_btnMin, pt))
        {
            ShowWindow(hwnd, SW_MINIMIZE);
            return 0;
        }

        // menu clicks
        for (int i = 0; i < (int)g_menuRects.size(); ++i)
        {
            if (PtInRect(&g_menuRects[i], pt))
            {
                int y = g_menuRects[i].bottom;
                int x = g_menuRects[i].left;
                switch (i)
                {
                case 0: ShowMenuPopup(hwnd, g_menuFile, x, y); break;
                case 1: ShowMenuPopup(hwnd, g_menuEdit, x, y); break;
                case 2: ShowMenuPopup(hwnd, g_menuView, x, y); break;
                case 3: ShowMenuPopup(hwnd, g_menuWindow, x, y); break;
                case 4: ShowMenuPopup(hwnd, g_menuHelp, x, y); break;
                }
                return 0;
            }
        }

        RECT project{ 0, kTopBarH, kProjectW, rc.bottom };
        if (PtInRect(&project, pt))
        {
            int y = pt.y - (kTopBarH + 8);
            int idx = y / kLineH;
            if (idx >= 0 && idx < (int)g_files.size())
            {
                g_selectedIndex = idx;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        ReleaseCapture();
        SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        int hover = -1;
        for (int i = 0; i < (int)g_menuRects.size(); ++i)
        {
            if (PtInRect(&g_menuRects[i], pt))
            {
                hover = i;
                break;
            }
        }
        if (hover != g_hoverMenu)
        {
            g_hoverMenu = hover;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        break;
    }
    case WM_MOUSELEAVE:
        g_hoverMenu = -1;
        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    case WM_NCHITTEST:
    {
        // resize borders
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hwnd, &pt);
        RECT rc; GetClientRect(hwnd, &rc);
        bool left = pt.x < kResizeBorder;
        bool right = pt.x > rc.right - kResizeBorder;
        bool top = pt.y < kResizeBorder;
        bool bottom = pt.y > rc.bottom - kResizeBorder;
        if (top && left) return HTTOPLEFT;
        if (top && right) return HTTOPRIGHT;
        if (bottom && left) return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left) return HTLEFT;
        if (right) return HTRIGHT;
        if (top) return HTTOP;
        if (bottom) return HTBOTTOM;
        if (pt.y < kTopBarH) return HTCAPTION;
        return HTCLIENT;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        RECT topbar{ 0,0,w,kTopBarH };
        RECT project{ 0,kTopBarH,kProjectW,h };
        RECT workspace{ kProjectW,kTopBarH,w,h };

        HBRUSH bg = CreateSolidBrush(RGB(8, 8, 10));
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        HBRUSH bar = CreateSolidBrush(RGB(18, 18, 20));
        FillRect(hdc, &topbar, bar);
        DeleteObject(bar);

        HBRUSH proj = CreateSolidBrush(RGB(14, 14, 16));
        FillRect(hdc, &project, proj);
        DeleteObject(proj);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(240, 240, 240));

        // Custom menu bar
        g_menuRects.clear();
        int x = 10;
        const wchar_t* labels[] = { L"File", L"Edit", L"View", L"Window", L"Help" };
        for (int i = 0; i < 5; ++i)
        {
            SIZE sz{};
            GetTextExtentPoint32W(hdc, labels[i], (int)wcslen(labels[i]), &sz);
            RECT r{ x - 4, topbar.top + 4, x + sz.cx + 8, topbar.bottom - 4 };
            if (g_hoverMenu == i)
            {
                HBRUSH hover = CreateSolidBrush(RGB(45, 45, 48));
                FillRect(hdc, &r, hover);
                DeleteObject(hover);
            }
            TextOutW(hdc, x, topbar.top + 8, labels[i], (int)wcslen(labels[i]));
            g_menuRects.push_back(r);
            x += sz.cx + 20;
        }

        // Window buttons (min/close)
        g_btnClose = { w - 40, 4, w - 8, kTopBarH - 4 };
        g_btnMin = { w - 80, 4, w - 48, kTopBarH - 4 };
        HBRUSH btn = CreateSolidBrush(RGB(40, 40, 44));
        FillRect(hdc, &g_btnMin, btn);
        FillRect(hdc, &g_btnClose, btn);
        DeleteObject(btn);
        SetTextColor(hdc, RGB(230, 230, 230));
        TextOutW(hdc, g_btnMin.left + 10, g_btnMin.top + 4, L"_", 1);
        TextOutW(hdc, g_btnClose.left + 8, g_btnClose.top + 4, L"X", 1);

        SetTextColor(hdc, RGB(210, 210, 210));
        TextOutW(hdc, project.left + 8, project.top + 6, L"Project", 7);

        int y = project.top + 24;
        for (int i = 0; i < (int)g_files.size(); ++i)
        {
            const auto& e = g_files[i];
            if (y > project.bottom - kLineH) break;
            if (i == g_selectedIndex)
            {
                RECT sel{ project.left + 2, y - 2, project.right - 2, y + kLineH };
                HBRUSH selBrush = CreateSolidBrush(RGB(40, 40, 45));
                FillRect(hdc, &sel, selBrush);
                DeleteObject(selBrush);
            }
            int x = project.left + 10 + e.depth * 12;
            TextOutW(hdc, x, y, e.label.c_str(), (int)e.label.size());
            y += kLineH;
        }

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_SIZE:
        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"GameEditorWindow";
    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    HMENU hMenu = CreateMenu();
    HMENU hWindow = CreatePopupMenu();
    AppendMenuW(hWindow, MF_STRING, ID_TOGGLE_CONSOLE, L"Console\tF1");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hWindow, L"Window");

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"GameEditor (Template)", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, nullptr, hMenu, hInstance, nullptr);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    EnableDarkTitleBar(hwnd);

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
