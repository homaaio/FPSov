/*
 * overlay.cpp — Системный оверлей для Windows
 * С захватом FPS игры через DWM и Hook
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#include <dwmapi.h>
#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <vector>
#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// ─── НАСТРОЙКИ ────────────────────────────────────────────────────────────
static const int   WIN_W       = 280;
static const int   WIN_H       = 150;
static const int   MARGIN      = 14;
static const int   UPDATE_MS   = 800;
static const COLORREF BG        = RGB(10,  10,  10);
static const COLORREF C_OK      = RGB(0,   220, 120);
static const COLORREF C_WARN    = RGB(255, 170, 0);
static const COLORREF C_CRIT    = RGB(255, 50,  70);
static const COLORREF C_DIM     = RGB(60,  100, 75);
static const float    ALPHA_F   = 0.88f;
// ──────────────────────────────────────────────────────────────────────────

// ── Позиция окна ─────────────────────────────────────────────────────────
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

// ── Общие данные ──────────────────────────────────────────────────────────
static std::atomic<float>  g_cpu_pct   {0};
static std::atomic<float>  g_ram_used  {0};
static std::atomic<float>  g_ram_total {0};
static std::atomic<int>    g_ram_pct   {0};
static std::atomic<float>  g_gpu_pct   {0};
static std::atomic<int>    g_fps       {0};
static std::atomic<bool>   g_running   {true};
static std::wstring        g_fps_method;

// ═══════════════════════════════════════════════════════════════════════════
// 1. CPU через PDH
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
// 2. GPU через PDH
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
// 3. RAM
// ═══════════════════════════════════════════════════════════════════════════
static void ReadRAM() {
    MEMORYSTATUSEX ms{ sizeof(ms) };
    if (GlobalMemoryStatusEx(&ms)) {
        g_ram_total = ms.ullTotalPhys / (1024.f * 1024.f * 1024.f);
        g_ram_used = (ms.ullTotalPhys - ms.ullAvailPhys) / (1024.f * 1024.f * 1024.f);
        g_ram_pct = (int)ms.dwMemoryLoad;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 4. Получение FPS активного окна через DWM
// ═══════════════════════════════════════════════════════════════════════════
static HWND GetForegroundGameWindow() {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return nullptr;
    
    // Проверяем, что это не наш оверлей
    char className[256];
    GetClassNameA(hwnd, className, 256);
    
    // Игнорируем системные окна
    if (strstr(className, "Overlay") || strstr(className, "Progman") ||
        strstr(className, "Shell") || strstr(className, "WorkerW")) {
        return nullptr;
    }
    
    return hwnd;
}

static int GetFPSFromDWM(HWND targetWindow) {
    if (!targetWindow) return 0;
    
    DWM_TIMING_INFO timing{};
    timing.cbSize = sizeof(DWM_TIMING_INFO);
    
    HRESULT hr = DwmGetCompositionTimingInfo(targetWindow, &timing);
    if (SUCCEEDED(hr) && timing.rateRefresh.uiDenominator > 0) {
        if (timing.qpcRefreshPeriod > 0) {
            // Вычисляем FPS из периода обновления
            int fps = (int)(10000.0 / (timing.qpcRefreshPeriod / 10000.0));
            if (fps > 0 && fps < 500) {
                return fps;
            }
        }
        
        // Альтернативный метод через частоту обновления
        if (timing.rateRefresh.uiNumerator > 0) {
            int fps = (int)(timing.rateRefresh.uiNumerator / 
                           (double)timing.rateRefresh.uiDenominator);
            if (fps > 0 && fps < 500) {
                return fps;
            }
        }
    }
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// 5. Альтернативный метод через производительность процесса
// ═══════════════════════════════════════════════════════════════════════════
static int GetFPSFromPerformance() {
    static PDH_HQUERY hQuery = nullptr;
    static PDH_HCOUNTER hCounter = nullptr;
    static bool initialized = false;
    
    if (!initialized) {
        PdhOpenQueryA(nullptr, 0, &hQuery);
        if (hQuery) {
            // Пытаемся получить FPS через счётчик GPU
            PdhAddEnglishCounterA(hQuery, 
                "\\GPU Process Memory(*)\\Local Usage", 0, &hCounter);
            initialized = true;
        }
    }
    
    if (hQuery && hCounter) {
        PdhCollectQueryData(hQuery);
        PDH_FMT_COUNTERVALUE val;
        if (PdhGetFormattedCounterValue(hCounter, PDH_FMT_LONG, nullptr, &val) == ERROR_SUCCESS) {
            if (val.longValue > 0 && val.longValue < 500) {
                return val.longValue;
            }
        }
    }
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// 6. Получение FPS активной игры
// ═══════════════════════════════════════════════════════════════════════════
static int GetGameFPS() {
    static int last_fps = 60;
    static auto last_update = std::chrono::steady_clock::now();
    static int frame_count = 0;
    
    auto now = std::chrono::steady_clock::now();
    double dt = std::chrono::duration<double>(now - last_update).count();
    
    frame_count++;
    
    if (dt >= 0.5) { // Обновляем 2 раза в секунду
        HWND gameWindow = GetForegroundGameWindow();
        int new_fps = 0;
        
        if (gameWindow) {
            // Метод 1: DWM
            new_fps = GetFPSFromDWM(gameWindow);
            
            // Метод 2: Performance counters (fallback)
            if (new_fps <= 0) {
                new_fps = GetFPSFromPerformance();
            }
            
            // Если всё ещё 0, пытаемся угадать по частоте обновления монитора
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
            // Сглаживание
            last_fps = last_fps * 0.7 + new_fps * 0.3;
            if (last_fps < 1) last_fps = 1;
            g_fps_method = L"DWM";
        } else if (frame_count > 0) {
            // Fallback - показываем частоту обновления
            g_fps_method = L"REFRESH";
        }
        
        frame_count = 0;
        last_update = now;
    }
    
    return (int)last_fps;
}

// ═══════════════════════════════════════════════════════════════════════════
// 7. Фоновый поток сбора данных
// ═══════════════════════════════════════════════════════════════════════════
static void CollectorThread() {
    InitCPU();
    InitGPU();
    
    while (g_running) {
        g_cpu_pct = ReadCPU();
        ReadRAM();
        g_gpu_pct = ReadGPU();
        g_fps = GetGameFPS();
        std::this_thread::sleep_for(std::chrono::milliseconds(UPDATE_MS));
    }
    
    if (hCpuQuery) PdhCloseQuery(hCpuQuery);
    if (hGpuQuery) PdhCloseQuery(hGpuQuery);
}

// ═══════════════════════════════════════════════════════════════════════════
// 8. Отрисовка
// ═══════════════════════════════════════════════════════════════════════════
static COLORREF ColorFor(float pct) {
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
    
    HFONT fontLarge = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    HFONT fontSmall = CreateFontW(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(40, 70, 50));
    SelectObject(memDC, pen);
    MoveToEx(memDC, 8, 22, nullptr);
    LineTo(memDC, WIN_W - 8, 22);
    DeleteObject(pen);
    
    DrawLine(memDC, fontSmall, 8, 4, L"▶ GAME MONITOR", C_DIM);
    DrawLine(memDC, fontSmall, WIN_W - 18, 4, L"✕", C_DIM);
    
    auto fmt1 = [](float v, int dec = 1) -> std::wstring {
        std::wostringstream ss;
        ss << std::fixed << std::setprecision(dec) << v;
        return ss.str();
    };
    
    int fps = g_fps.load();
    float cpu = g_cpu_pct.load();
    float ram_u = g_ram_used.load();
    float ram_t = g_ram_total.load();
    int ram_p = g_ram_pct.load();
    float gpu = g_gpu_pct.load();
    
    std::wstring fps_str = L"FPS: ";
    if (fps > 0) {
        fps_str += std::to_wstring(fps);
        if (!g_fps_method.empty()) {
            fps_str += L" [" + g_fps_method + L"]";
        }
    } else {
        fps_str += L"--";
    }
    
    COLORREF fps_col = (fps > 0 && fps < 30) ? C_CRIT : (fps < 60 ? C_WARN : C_OK);
    
    std::wstring cpu_str = L"CPU: " + fmt1(cpu) + L"%";
    std::wstring ram_str = L"RAM: " + fmt1(ram_u) + L"/" + fmt1(ram_t, 0) + L" GB";
    std::wstring gpu_str = L"GPU: " + fmt1(gpu) + L"%";
    
    int y = 26;
    DrawLine(memDC, fontLarge, 8, y, fps_str, fps_col);
    y += 24;
    DrawLine(memDC, fontLarge, 8, y, cpu_str, ColorFor(cpu));
    y += 24;
    DrawLine(memDC, fontLarge, 8, y, ram_str, ColorFor((float)ram_p));
    y += 24;
    DrawLine(memDC, fontLarge, 8, y, gpu_str, ColorFor(gpu));
    y += 24;
    
    HWND game = GetForegroundGameWindow();
    if (game) {
        char title[256];
        GetWindowTextA(game, title, 256);
        if (strlen(title) > 0) {
            std::wstring wtitle(title, title + strlen(title));
            if (wtitle.length() > 25) {
                wtitle = wtitle.substr(0, 22) + L"...";
            }
            DrawLine(memDC, fontSmall, 8, y, L"Game: " + wtitle, C_DIM);
        }
    } else {
        DrawLine(memDC, fontSmall, 8, y, L"Нет активной игры", C_DIM);
    }
    
    DeleteObject(fontLarge);
    DeleteObject(fontSmall);
    
    BitBlt(hdc, 0, 0, WIN_W, WIN_H, memDC, 0, 0, SRCCOPY);
    
    SelectObject(memDC, oldBmp);
    DeleteObject(bmp);
    DeleteDC(memDC);
    EndPaint(hwnd, &ps);
}

// ═══════════════════════════════════════════════════════════════════════════
// 9. WinProc
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
        AppendMenuA(menu, MF_STRING, IDM_CLOSE, "Закрыть");
        AppendMenuA(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuA(menu, MF_STRING, IDM_TL, "Верхний левый");
        AppendMenuA(menu, MF_STRING, IDM_TR, "Верхний правый");
        AppendMenuA(menu, MF_STRING, IDM_BL, "Нижний левый");
        AppendMenuA(menu, MF_STRING, IDM_BR, "Нижний правый");
        
        POINT pt;
        GetCursorPos(&pt);
        int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                 pt.x, pt.y, 0, hwnd, nullptr);
        DestroyMenu(menu);
        
        switch (cmd) {
            case IDM_CLOSE: PostQuitMessage(0); break;
            case IDM_TL: MoveToCorner(hwnd, TOP_LEFT); break;
            case IDM_TR: MoveToCorner(hwnd, TOP_RIGHT); break;
            case IDM_BL: MoveToCorner(hwnd, BOTTOM_LEFT); break;
            case IDM_BR: MoveToCorner(hwnd, BOTTOM_RIGHT); break;
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
// 10. WinMain
// ═══════════════════════════════════════════════════════════════════════════
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
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
    MoveToCorner(hwnd, TOP_RIGHT);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    
    SetTimer(hwnd, 1, 33, nullptr);
    
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