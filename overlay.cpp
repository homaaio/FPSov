/*
 * overlay.cpp — System Overlay for Windows (No external dependencies)
 * Uses simple .ini config file
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#include <dwmapi.h>
#include <fstream>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "dwmapi.lib")

// ─── CONFIGURATION STRUCTURE ──────────────────────────────────────────────
struct Config {
    // Window settings
    int width = 200;
    int height = 110;
    float alpha = 0.88f;
    int margin = 14;
    std::string corner = "top_right";
    
    // Color settings (stored as RGB values)
    int bg_r = 10, bg_g = 10, bg_b = 10;
    int fps_ok_r = 0, fps_ok_g = 220, fps_ok_b = 120;
    int fps_warn_r = 255, fps_warn_g = 170, fps_warn_b = 0;
    int fps_crit_r = 255, fps_crit_g = 50, fps_crit_b = 70;
    int text_dim_r = 60, text_dim_g = 100, text_dim_b = 75;
    int sep_r = 40, sep_g = 70, sep_b = 50;
    
    // Display settings
    bool show_fps = true;
    bool show_cpu = true;
    bool show_gpu = true;
    bool show_title = true;
    int font_fps = 18;
    int font_val = 14;
    int font_small = 10;
    
    // Update intervals
    int update_metrics = 800;
    int update_overlay = 33;
} g_config;

// Global config variables (for quick access)
static int WIN_W, WIN_H, MARGIN;
static COLORREF BG, C_OK, C_WARN, C_CRIT, C_DIM, C_SEP;
static int UPDATE_MS, TIMER_MS;
static float ALPHA_F;
static bool SHOW_FPS, SHOW_CPU, SHOW_GPU, SHOW_TITLE;
static int FONT_FPS, FONT_VAL, FONT_SMALL;
static std::string CORNER;

// ─── CONFIG FILE PARSER ───────────────────────────────────────────────────
static void LoadConfig() {
    std::ifstream file("overlay.ini");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;
            
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            val.erase(0, val.find_first_not_of(" \t\r\n"));
            val.erase(val.find_last_not_of(" \t\r\n") + 1);
            
            // Window settings
            if (key == "width") g_config.width = std::stoi(val);
            else if (key == "height") g_config.height = std::stoi(val);
            else if (key == "alpha") g_config.alpha = std::stof(val);
            else if (key == "margin") g_config.margin = std::stoi(val);
            else if (key == "corner") g_config.corner = val;
            
            // Color settings
            else if (key == "background") sscanf(val.c_str(), "%d,%d,%d", &g_config.bg_r, &g_config.bg_g, &g_config.bg_b);
            else if (key == "fps_ok") sscanf(val.c_str(), "%d,%d,%d", &g_config.fps_ok_r, &g_config.fps_ok_g, &g_config.fps_ok_b);
            else if (key == "fps_warn") sscanf(val.c_str(), "%d,%d,%d", &g_config.fps_warn_r, &g_config.fps_warn_g, &g_config.fps_warn_b);
            else if (key == "fps_crit") sscanf(val.c_str(), "%d,%d,%d", &g_config.fps_crit_r, &g_config.fps_crit_g, &g_config.fps_crit_b);
            else if (key == "text_dim") sscanf(val.c_str(), "%d,%d,%d", &g_config.text_dim_r, &g_config.text_dim_g, &g_config.text_dim_b);
            else if (key == "separator") sscanf(val.c_str(), "%d,%d,%d", &g_config.sep_r, &g_config.sep_g, &g_config.sep_b);
            
            // Display settings
            else if (key == "show_fps") g_config.show_fps = (val == "true" || val == "1");
            else if (key == "show_cpu") g_config.show_cpu = (val == "true" || val == "1");
            else if (key == "show_gpu") g_config.show_gpu = (val == "true" || val == "1");
            else if (key == "show_title") g_config.show_title = (val == "true" || val == "1");
            else if (key == "font_fps") g_config.font_fps = std::stoi(val);
            else if (key == "font_val") g_config.font_val = std::stoi(val);
            else if (key == "font_small") g_config.font_small = std::stoi(val);
            
            // Update settings
            else if (key == "update_metrics") g_config.update_metrics = std::stoi(val);
            else if (key == "update_overlay") g_config.update_overlay = std::stoi(val);
        }
        file.close();
    }
    
    // Apply config to global variables
    WIN_W = g_config.width;
    WIN_H = g_config.height;
    ALPHA_F = g_config.alpha;
    MARGIN = g_config.margin;
    CORNER = g_config.corner;
    
    BG = RGB(g_config.bg_r, g_config.bg_g, g_config.bg_b);
    C_OK = RGB(g_config.fps_ok_r, g_config.fps_ok_g, g_config.fps_ok_b);
    C_WARN = RGB(g_config.fps_warn_r, g_config.fps_warn_g, g_config.fps_warn_b);
    C_CRIT = RGB(g_config.fps_crit_r, g_config.fps_crit_g, g_config.fps_crit_b);
    C_DIM = RGB(g_config.text_dim_r, g_config.text_dim_g, g_config.text_dim_b);
    C_SEP = RGB(g_config.sep_r, g_config.sep_g, g_config.sep_b);
    
    UPDATE_MS = g_config.update_metrics;
    TIMER_MS = g_config.update_overlay;
    
    SHOW_FPS = g_config.show_fps;
    SHOW_CPU = g_config.show_cpu;
    SHOW_GPU = g_config.show_gpu;
    SHOW_TITLE = g_config.show_title;
    
    FONT_FPS = g_config.font_fps;
    FONT_VAL = g_config.font_val;
    FONT_SMALL = g_config.font_small;
}

static void SaveDefaultConfig() {
    std::ifstream test("overlay.ini");
    if (test.is_open()) {
        test.close();
        return;
    }
    
    std::ofstream file("overlay.ini");
    file << "; Overlay Configuration File\n";
    file << "; Edit this file or use settings.exe to change values\n\n";
    
    file << "[Window]\n";
    file << "width=200\n";
    file << "height=110\n";
    file << "alpha=0.88\n";
    file << "margin=14\n";
    file << "corner=top_right\n\n";
    
    file << "[Colors]\n";
    file << "background=10,10,10\n";
    file << "fps_ok=0,220,120\n";
    file << "fps_warn=255,170,0\n";
    file << "fps_crit=255,50,70\n";
    file << "text_dim=60,100,75\n";
    file << "separator=40,70,50\n\n";
    
    file << "[Display]\n";
    file << "show_fps=true\n";
    file << "show_cpu=true\n";
    file << "show_gpu=true\n";
    file << "show_title=true\n";
    file << "font_fps=18\n";
    file << "font_val=14\n";
    file << "font_small=10\n\n";
    
    file << "[Update]\n";
    file << "update_metrics=800\n";
    file << "update_overlay=33\n";
    
    file.close();
}

// ─── WINDOW POSITION ──────────────────────────────────────────────────────
enum Corner { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT };
static Corner g_corner = TOP_RIGHT;

static void MoveToCorner(HWND hwnd, Corner c) {
    g_corner = c;
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    int x, y;
    switch (c) {
        case TOP_LEFT:     x = MARGIN;            y = MARGIN;            break;
        case BOTTOM_LEFT:  x = MARGIN;            y = sh-WIN_H-MARGIN;   break;
        case BOTTOM_RIGHT: x = sw-WIN_W-MARGIN;   y = sh-WIN_H-MARGIN;   break;
        default:           x = sw-WIN_W-MARGIN;   y = MARGIN;            break;
    }
    SetWindowPos(hwnd, HWND_TOPMOST, x, y, WIN_W, WIN_H, SWP_NOSIZE);
}

// ─── SHARED DATA ──────────────────────────────────────────────────────────
static std::atomic<float>  g_cpu_pct   {0};
static std::atomic<float>  g_gpu_pct   {0};
static std::atomic<int>    g_fps       {0};
static std::atomic<bool>   g_running   {true};

// ═══════════════════════════════════════════════════════════════════════════
// CPU VIA PDH
// ═══════════════════════════════════════════════════════════════════════════
static PDH_HQUERY   hCpuQuery  = nullptr;
static PDH_HCOUNTER hCpuCtr    = nullptr;

static void InitCPU() {
    PdhOpenQueryA(nullptr, 0, &hCpuQuery);
    PdhAddEnglishCounterA(hCpuQuery, "\\Processor(_Total)\\% Processor Time", 0, &hCpuCtr);
    PdhCollectQueryData(hCpuQuery);
}

static float ReadCPU() {
    if (!hCpuQuery || !hCpuCtr) return 0;
    PdhCollectQueryData(hCpuQuery);
    PDH_FMT_COUNTERVALUE val;
    if (PdhGetFormattedCounterValue(hCpuCtr, PDH_FMT_DOUBLE, nullptr, &val) == ERROR_SUCCESS) {
        return (float)val.doubleValue;
    }
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// GPU VIA PDH
// ═══════════════════════════════════════════════════════════════════════════
static PDH_HQUERY   hGpuQuery  = nullptr;
static PDH_HCOUNTER hGpuCtr    = nullptr;

static void InitGPU() {
    PdhOpenQueryA(nullptr, 0, &hGpuQuery);
    const char* counters[] = {
        "\\GPU Engine(*_3D)\\Utilization Percentage",
        "\\GPU Engine(*)\\Utilization Percentage",
    };
    for (const char* counter : counters) {
        if (PdhAddEnglishCounterA(hGpuQuery, counter, 0, &hGpuCtr) == ERROR_SUCCESS)
            break;
    }
    if (hGpuCtr) PdhCollectQueryData(hGpuQuery);
}

static float ReadGPU() {
    if (!hGpuQuery || !hGpuCtr) return 0;
    PdhCollectQueryData(hGpuQuery);
    PDH_FMT_COUNTERVALUE val;
    if (PdhGetFormattedCounterValue(hGpuCtr, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, nullptr, &val) == ERROR_SUCCESS) {
        return (float)std::min(val.doubleValue, 100.0);
    }
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// FPS FROM ACTIVE GAME
// ═══════════════════════════════════════════════════════════════════════════
static HWND GetForegroundGameWindow() {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return nullptr;
    
    char className[256];
    GetClassNameA(hwnd, className, 256);
    
    if (strstr(className, "Overlay") || strstr(className, "Progman") ||
        strstr(className, "Shell") || strstr(className, "WorkerW")) {
        return nullptr;
    }
    
    return hwnd;
}

static int GetGameFPS() {
    static int last_fps = 60;
    static auto last_update = std::chrono::steady_clock::now();
    
    auto now = std::chrono::steady_clock::now();
    double dt = std::chrono::duration<double>(now - last_update).count();
    
    if (dt >= 0.5) {
        HWND gameWindow = GetForegroundGameWindow();
        int new_fps = 0;
        
        if (gameWindow) {
            DWM_TIMING_INFO timing{};
            timing.cbSize = sizeof(DWM_TIMING_INFO);
            
            if (SUCCEEDED(DwmGetCompositionTimingInfo(gameWindow, &timing))) {
                if (timing.rateRefresh.uiNumerator > 0 && timing.rateRefresh.uiDenominator > 0) {
                    new_fps = (int)(timing.rateRefresh.uiNumerator / 
                                   (double)timing.rateRefresh.uiDenominator);
                }
                
                if (new_fps <= 0 && timing.qpcRefreshPeriod > 0) {
                    new_fps = (int)(10000.0 / (timing.qpcRefreshPeriod / 10000.0));
                }
            }
            
            if (new_fps <= 0) {
                DEVMODE dm{};
                dm.dmSize = sizeof(dm);
                if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dm)) {
                    if (dm.dmDisplayFrequency > 0) {
                        new_fps = dm.dmDisplayFrequency;
                    }
                }
            }
        }
        
        if (new_fps > 0 && new_fps < 500) {
            last_fps = (int)(last_fps * 0.7 + new_fps * 0.3);
            if (last_fps < 1) last_fps = 1;
        }
        
        last_update = now;
    }
    
    return last_fps;
}

// ═══════════════════════════════════════════════════════════════════════════
// BACKGROUND COLLECTOR THREAD
// ═══════════════════════════════════════════════════════════════════════════
static void CollectorThread() {
    InitCPU();
    InitGPU();
    
    while (g_running) {
        if (SHOW_CPU) g_cpu_pct = ReadCPU();
        if (SHOW_GPU) g_gpu_pct = ReadGPU();
        if (SHOW_FPS) g_fps = GetGameFPS();
        std::this_thread::sleep_for(std::chrono::milliseconds(UPDATE_MS));
    }
    
    if (hCpuQuery) PdhCloseQuery(hCpuQuery);
    if (hGpuQuery) PdhCloseQuery(hGpuQuery);
}

// ═══════════════════════════════════════════════════════════════════════════
// DRAWING
// ═══════════════════════════════════════════════════════════════════════════
static COLORREF ColorForFPS(int fps) {
    if (fps < 45) return C_CRIT;
    if (fps < 70) return C_WARN;
    return C_OK;
}

static COLORREF ColorForUsage(float pct) {
    if (pct >= 90) return C_CRIT;
    if (pct >= 70) return C_WARN;
    return C_OK;
}

static void DrawLine(HDC hdc, HFONT font, int x, int y,
                     const std::wstring& text, COLORREF col) {
    SelectObject(hdc, font);
    SetTextColor(hdc, col);
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, x, y, text.c_str(), (int)text.size());
}

static void Paint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, WIN_W, WIN_H);
    HGDIOBJ oldBmp = SelectObject(memDC, bmp);
    
    RECT rc{0, 0, WIN_W, WIN_H};
    HBRUSH bgBrush = CreateSolidBrush(BG);
    FillRect(memDC, &rc, bgBrush);
    DeleteObject(bgBrush);
    
    HFONT fontFPS = CreateFontW(FONT_FPS, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    HFONT fontVal = CreateFontW(FONT_VAL, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    HFONT fontSmall = CreateFontW(FONT_SMALL, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    
    // Separator line
    HPEN pen = CreatePen(PS_SOLID, 1, C_SEP);
    SelectObject(memDC, pen);
    MoveToEx(memDC, 8, 22, nullptr);
    LineTo(memDC, WIN_W - 8, 22);
    DeleteObject(pen);
    
    // Title
    if (SHOW_TITLE) {
        DrawLine(memDC, fontSmall, 8, 4, L"MONITOR", C_DIM);
        DrawLine(memDC, fontSmall, WIN_W - 18, 4, L"X", C_DIM);
    }
    
    auto fmt1 = [](float v, int dec = 1) -> std::wstring {
        std::wostringstream ss;
        ss << std::fixed << std::setprecision(dec) << v;
        return ss.str();
    };
    
    int y = 26;
    int line_height = FONT_VAL + 8;
    
    // FPS
    if (SHOW_FPS) {
        int fps = g_fps.load();
        std::wstring fps_str = L"FPS  " + std::to_wstring(fps);
        DrawLine(memDC, fontFPS, 8, y, fps_str, ColorForFPS(fps));
        y += line_height + 4;
    }
    
    // CPU
    if (SHOW_CPU) {
        float cpu = g_cpu_pct.load();
        std::wstring cpu_str = L"CPU  " + fmt1(cpu) + L"%";
        DrawLine(memDC, fontVal, 8, y, cpu_str, ColorForUsage(cpu));
        y += line_height;
    }
    
    // GPU
    if (SHOW_GPU) {
        float gpu = g_gpu_pct.load();
        std::wstring gpu_str = L"GPU  " + fmt1(gpu) + L"%";
        DrawLine(memDC, fontVal, 8, y, gpu_str, ColorForUsage(gpu));
        y += line_height;
    }
    
    // Hint
    DrawLine(memDC, fontSmall, 8, WIN_H - 18, L"RMB menu  |  LMB drag", C_DIM);
    
    DeleteObject(fontFPS);
    DeleteObject(fontVal);
    DeleteObject(fontSmall);
    
    BitBlt(hdc, 0, 0, WIN_W, WIN_H, memDC, 0, 0, SRCCOPY);
    
    SelectObject(memDC, oldBmp);
    DeleteObject(bmp);
    DeleteDC(memDC);
    EndPaint(hwnd, &ps);
}

// ═══════════════════════════════════════════════════════════════════════════
// WINDOW PROCEDURE
// ═══════════════════════════════════════════════════════════════════════════
static POINT g_drag_start;
static bool  g_dragging = false;

#define IDM_CLOSE    1001
#define IDM_TL       1002
#define IDM_TR       1003
#define IDM_BL       1004
#define IDM_BR       1005

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT:
        Paint(hwnd);
        return 0;
        
    case WM_TIMER:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
        
    case WM_LBUTTONDOWN: {
        int x = LOWORD(lp);
        int y = HIWORD(lp);
        g_drag_start.x = x;
        g_drag_start.y = y;
        g_dragging = true;
        SetCapture(hwnd);
        return 0;
    }
        
    case WM_MOUSEMOVE:
        if (g_dragging) {
            int x = LOWORD(lp);
            int y = HIWORD(lp);
            RECT wr;
            GetWindowRect(hwnd, &wr);
            int nx = wr.left + x - g_drag_start.x;
            int ny = wr.top + y - g_drag_start.y;
            SetWindowPos(hwnd, HWND_TOPMOST, nx, ny, 0, 0, SWP_NOSIZE);
        }
        return 0;
        
    case WM_LBUTTONUP: {
        g_dragging = false;
        ReleaseCapture();
        int x = LOWORD(lp);
        int y = HIWORD(lp);
        if (x > WIN_W - 22 && y < 20) {
            PostQuitMessage(0);
        }
        return 0;
    }
        
    case WM_RBUTTONUP: {
        HMENU menu = CreatePopupMenu();
        AppendMenuA(menu, MF_STRING, IDM_CLOSE, "Close");
        AppendMenuA(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuA(menu, MF_STRING, IDM_TL, "Top Left");
        AppendMenuA(menu, MF_STRING, IDM_TR, "Top Right");
        AppendMenuA(menu, MF_STRING, IDM_BL, "Bottom Left");
        AppendMenuA(menu, MF_STRING, IDM_BR, "Bottom Right");
        
        POINT pt;
        GetCursorPos(&pt);
        int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                 pt.x, pt.y, 0, hwnd, nullptr);
        DestroyMenu(menu);
        
        Corner corner = TOP_RIGHT;
        switch (cmd) {
            case IDM_CLOSE: PostQuitMessage(0); break;
            case IDM_TL: corner = TOP_LEFT; MoveToCorner(hwnd, corner); break;
            case IDM_TR: corner = TOP_RIGHT; MoveToCorner(hwnd, corner); break;
            case IDM_BL: corner = BOTTOM_LEFT; MoveToCorner(hwnd, corner); break;
            case IDM_BR: corner = BOTTOM_RIGHT; MoveToCorner(hwnd, corner); break;
        }
        return 0;
    }
        
    case WM_DESTROY:
        g_running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

// ═══════════════════════════════════════════════════════════════════════════
// WINMAIN
// ═══════════════════════════════════════════════════════════════════════════
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    SaveDefaultConfig();
    LoadConfig();
    
    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "OverlayMonitor";
    
    if (!RegisterClassExA(&wc)) {
        MessageBoxA(nullptr, "Failed to register window class", "Error", MB_ICONERROR);
        return 1;
    }
    
    HWND hwnd = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "OverlayMonitor", "Overlay",
        WS_POPUP,
        0, 0, WIN_W, WIN_H,
        nullptr, nullptr, hInst, nullptr
    );
    
    if (!hwnd) {
        MessageBoxA(nullptr, "Failed to create window", "Error", MB_ICONERROR);
        return 1;
    }
    
    SetLayeredWindowAttributes(hwnd, 0, (BYTE)(ALPHA_F * 255), LWA_ALPHA);
    
    // Set corner based on config
    Corner corner = TOP_RIGHT;
    if (CORNER == "top_left") corner = TOP_LEFT;
    else if (CORNER == "bottom_left") corner = BOTTOM_LEFT;
    else if (CORNER == "bottom_right") corner = BOTTOM_RIGHT;
    MoveToCorner(hwnd, corner);
    
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    
    SetTimer(hwnd, 1, TIMER_MS, nullptr);
    
    std::thread collector(CollectorThread);
    
    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    
    g_running = false;
    if (collector.joinable()) collector.join();
    
    return 0;
}