/*
 * overlay.cpp — System Overlay for Linux (X11/Xlib)
 * No external dependencies beyond X11
 */

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <X11/keysym.h>

#include <fstream>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>

// CONFIGURATION STRUCTURE ----------------------------------------------
struct Config {
    int width = 200;
    int height = 110;
    float alpha = 0.88f;
    int margin = 14;
    std::string corner = "top_right";
    
    int bg_r = 10, bg_g = 10, bg_b = 10;
    int fps_ok_r = 0, fps_ok_g = 220, fps_ok_b = 120;
    int fps_warn_r = 255, fps_warn_g = 170, fps_warn_b = 0;
    int fps_crit_r = 255, fps_crit_g = 50, fps_crit_b = 70;
    int text_dim_r = 60, text_dim_g = 100, text_dim_b = 75;
    int sep_r = 40, sep_g = 70, sep_b = 50;
    
    bool show_fps = true;
    bool show_cpu = true;
    bool show_gpu = true;
    bool show_title = true;
    int font_fps = 18;
    int font_val = 14;
    int font_small = 10;
    
    int update_metrics = 800;
    int update_overlay = 33;
} g_config;

// Global config variables
static int WIN_W, WIN_H, MARGIN;
static unsigned long BG, C_OK, C_WARN, C_CRIT, C_DIM, C_SEP;
static int UPDATE_MS, TIMER_MS;
static float ALPHA_F;
static bool SHOW_FPS, SHOW_CPU, SHOW_GPU, SHOW_TITLE;
static int FONT_FPS, FONT_VAL, FONT_SMALL;
static std::string CORNER;

// X11 globals
static Display* g_display = nullptr;
static Window g_window = 0;
static GC g_gc = nullptr;
static XFontStruct* g_font_fps = nullptr;
static XFontStruct* g_font_val = nullptr;
static XFontStruct* g_font_small = nullptr;
static Colormap g_colormap = 0;

// Shared metrics
static std::atomic<float> g_cpu_pct{0};
static std::atomic<float> g_gpu_pct{0};
static std::atomic<int>   g_fps{60};
static std::atomic<bool>  g_running{true};

// COLOR HELPERS ---------------------------------------------------------
static unsigned long RGB(int r, int g, int b) {
    return (r << 16) | (g << 8) | b;
}

static unsigned long AllocColor(int r, int g, int b) {
    XColor color;
    color.red = (r * 65535) / 255;
    color.green = (g * 65535) / 255;
    color.blue = (b * 65535) / 255;
    color.flags = DoRed | DoGreen | DoBlue;
    XAllocColor(g_display, g_colormap, &color);
    return color.pixel;
}

static unsigned long ColorForFPS(int fps) {
    if (fps < 45) return C_CRIT;
    if (fps < 70) return C_WARN;
    return C_OK;
}

static unsigned long ColorForUsage(float pct) {
    if (pct >= 90) return C_CRIT;
    if (pct >= 70) return C_WARN;
    return C_OK;
}

// CONFIG FILE PARSER ---------------------------------------------------
static void LoadConfig() {
    std::ifstream file("overlay.ini");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;
            
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            val.erase(0, val.find_first_not_of(" \t\r\n"));
            val.erase(val.find_last_not_of(" \t\r\n") + 1);
            
            if (key == "width") g_config.width = std::stoi(val);
            else if (key == "height") g_config.height = std::stoi(val);
            else if (key == "alpha") g_config.alpha = std::stof(val);
            else if (key == "margin") g_config.margin = std::stoi(val);
            else if (key == "corner") g_config.corner = val;
            else if (key == "background") sscanf(val.c_str(), "%d,%d,%d", &g_config.bg_r, &g_config.bg_g, &g_config.bg_b);
            else if (key == "fps_ok") sscanf(val.c_str(), "%d,%d,%d", &g_config.fps_ok_r, &g_config.fps_ok_g, &g_config.fps_ok_b);
            else if (key == "fps_warn") sscanf(val.c_str(), "%d,%d,%d", &g_config.fps_warn_r, &g_config.fps_warn_g, &g_config.fps_warn_b);
            else if (key == "fps_crit") sscanf(val.c_str(), "%d,%d,%d", &g_config.fps_crit_r, &g_config.fps_crit_g, &g_config.fps_crit_b);
            else if (key == "text_dim") sscanf(val.c_str(), "%d,%d,%d", &g_config.text_dim_r, &g_config.text_dim_g, &g_config.text_dim_b);
            else if (key == "separator") sscanf(val.c_str(), "%d,%d,%d", &g_config.sep_r, &g_config.sep_g, &g_config.sep_b);
            else if (key == "show_fps") g_config.show_fps = (val == "true" || val == "1");
            else if (key == "show_cpu") g_config.show_cpu = (val == "true" || val == "1");
            else if (key == "show_gpu") g_config.show_gpu = (val == "true" || val == "1");
            else if (key == "show_title") g_config.show_title = (val == "true" || val == "1");
            else if (key == "font_fps") g_config.font_fps = std::stoi(val);
            else if (key == "font_val") g_config.font_val = std::stoi(val);
            else if (key == "font_small") g_config.font_small = std::stoi(val);
            else if (key == "update_metrics") g_config.update_metrics = std::stoi(val);
            else if (key == "update_overlay") g_config.update_overlay = std::stoi(val);
        }
        file.close();
    }
    
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
    if (access("overlay.ini", F_OK) == 0) return;
    
    std::ofstream file("overlay.ini");
    file << "; Overlay Configuration File\n";
    file << "; Edit this file or use settings.py to change values\n\n";
    file << "[Window]\nwidth=200\nheight=110\nalpha=0.88\nmargin=14\ncorner=top_right\n\n";
    file << "[Colors]\nbackground=10,10,10\nfps_ok=0,220,120\nfps_warn=255,170,0\n";
    file << "fps_crit=255,50,70\ntext_dim=60,100,75\nseparator=40,70,50\n\n";
    file << "[Display]\nshow_fps=true\nshow_cpu=true\nshow_gpu=true\nshow_title=true\n";
    file << "font_fps=18\nfont_val=14\nfont_small=10\n\n";
    file << "[Update]\nupdate_metrics=800\nupdate_overlay=33\n";
    file.close();
}

// CPU USAGE -------------------------------------------------------------
struct CpuStats {
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
};

static CpuStats ReadCpuStats() {
    CpuStats stats{};
    std::ifstream file("/proc/stat");
    if (file.is_open()) {
        std::string line;
        std::getline(file, line);
        sscanf(line.c_str(), "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
               &stats.user, &stats.nice, &stats.system, &stats.idle,
               &stats.iowait, &stats.irq, &stats.softirq, &stats.steal);
        file.close();
    }
    return stats;
}

static float GetCpuUsage() {
    static CpuStats prev{};
    static bool first = true;
    
    CpuStats curr = ReadCpuStats();
    if (first) {
        prev = curr;
        first = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        curr = ReadCpuStats();
    }
    
    unsigned long long prevIdle = prev.idle + prev.iowait;
    unsigned long long currIdle = curr.idle + curr.iowait;
    
    unsigned long long prevTotal = prev.user + prev.nice + prev.system + prev.idle + prev.iowait + prev.irq + prev.softirq + prev.steal;
    unsigned long long currTotal = curr.user + curr.nice + curr.system + curr.idle + curr.iowait + curr.irq + curr.softirq + curr.steal;
    
    unsigned long long totalDiff = currTotal - prevTotal;
    unsigned long long idleDiff = currIdle - prevIdle;
    
    prev = curr;
    
    if (totalDiff == 0) return 0;
    return 100.0f * (totalDiff - idleDiff) / totalDiff;
}

// GPU USAGE -------------------------------------------------------------
static std::string FindGpuPath() {
    // NVIDIA
    DIR* dir = opendir("/sys/class/drm");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir))) {
            if (strstr(entry->d_name, "card") && strstr(entry->d_name, "NV")) {
                std::string path = "/sys/class/drm/" + std::string(entry->d_name);
                closedir(dir);
                return path;
            }
        }
        closedir(dir);
    }
    
    // AMD GPU
    if (access("/sys/class/drm/card0/device/gpu_busy_percent", F_OK) == 0) {
        return "/sys/class/drm/card0";
    }
    
    return "";
}

static float GetGpuUsage() {
    static std::string gpuPath = FindGpuPath();
    if (gpuPath.empty()) return 0;
    
    // AMD GPU
    std::ifstream file(gpuPath + "/device/gpu_busy_percent");
    if (file.is_open()) {
        int usage;
        file >> usage;
        file.close();
        return static_cast<float>(usage);
    }
    
    // NVIDIA (через nvidia-smi как fallback)
    FILE* pipe = popen("nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits 2>/dev/null", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            pclose(pipe);
            return std::stof(buffer);
        }
        pclose(pipe);
    }
    
    return 0;
}

// FPS DETECTION ---------------------------------------------------------
static int GetFps() {
    // Определяем через частоту обновления X11
    static int lastFps = 60;
    static auto lastUpdate = std::chrono::steady_clock::now();
    
    auto now = std::chrono::steady_clock::now();
    double dt = std::chrono::duration<double>(now - lastUpdate).count();
    
    if (dt >= 0.5) {
        // Получаем частоту обновления через XRandR (простая реализация)
        FILE* pipe = popen("xrandr --query 2>/dev/null | grep '*' | awk '{print $2}' | cut -d'.' -f1 | head -1", "r");
        if (pipe) {
            char buffer[32];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                int refresh = std::stoi(buffer);
                if (refresh > 0 && refresh < 500) {
                    lastFps = static_cast<int>(lastFps * 0.7 + refresh * 0.3);
                }
            }
            pclose(pipe);
        }
        
        // DWM-like detection через XPresent (если доступен)
        // Для простоты используем значение xrandr
        lastUpdate = now;
    }
    
    return lastFps;
}

// COLLECTOR THREAD -----------------------------------------------------
static void CollectorThread() {
    while (g_running) {
        if (SHOW_CPU) g_cpu_pct = GetCpuUsage();
        if (SHOW_GPU) g_gpu_pct = GetGpuUsage();
        if (SHOW_FPS) g_fps = GetFps();
        
        const int step = 50;
        for (int i = 0; i < UPDATE_MS / step && g_running; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(step));
        }
    }
}

// DRAWING --------------------------------------------------------------
static void DrawText(int x, int y, const std::string& text, unsigned long color, XFontStruct* font) {
    XSetForeground(g_display, g_gc, color);
    XSetFont(g_display, g_gc, font->fid);
    XDrawString(g_display, g_window, g_gc, x, y, text.c_str(), text.length());
}

static void Paint() {
    // Clear background
    XSetForeground(g_display, g_gc, BG);
    XFillRectangle(g_display, g_window, g_gc, 0, 0, WIN_W, WIN_H);
    
    // Separator line
    XSetForeground(g_display, g_gc, C_SEP);
    XDrawLine(g_display, g_window, g_gc, 8, 22, WIN_W - 8, 22);
    
    // Title
    if (SHOW_TITLE) {
        DrawText(8, 16, "MONITOR", C_DIM, g_font_small);
        DrawText(WIN_W - 20, 16, "X", C_DIM, g_font_small);
    }
    
    int y = 38;
    int line_height = FONT_VAL + 12;
    
    // FPS
    if (SHOW_FPS) {
        int fps = g_fps.load();
        std::string fps_str = "FPS  " + std::to_string(fps);
        DrawText(8, y, fps_str, ColorForFPS(fps), g_font_fps);
        y += line_height + 4;
    }
    
    // CPU
    if (SHOW_CPU) {
        float cpu = g_cpu_pct.load();
        char buf[32];
        snprintf(buf, sizeof(buf), "CPU  %.1f%%", cpu);
        DrawText(8, y, buf, ColorForUsage(cpu), g_font_val);
        y += line_height;
    }
    
    // GPU
    if (SHOW_GPU) {
        float gpu = g_gpu_pct.load();
        char buf[32];
        snprintf(buf, sizeof(buf), "GPU  %.1f%%", gpu);
        DrawText(8, y, buf, ColorForUsage(gpu), g_font_val);
        y += line_height;
    }
    
    // Hint
    DrawText(8, WIN_H - 6, "RMB menu  |  LMB drag", C_DIM, g_font_small);
    
    XFlush(g_display);
}

// WINDOW POSITION ------------------------------------------------------
static void MoveToCorner(const std::string& corner) {
    int screen = DefaultScreen(g_display);
    int sw = DisplayWidth(g_display, screen);
    int sh = DisplayHeight(g_display, screen);
    
    int x, y;
    if (corner == "top_left")           { x = MARGIN; y = MARGIN; }
    else if (corner == "bottom_left")   { x = MARGIN; y = sh - WIN_H - MARGIN; }
    else if (corner == "bottom_right")  { x = sw - WIN_W - MARGIN; y = sh - WIN_H - MARGIN; }
    else                                { x = sw - WIN_W - MARGIN; y = MARGIN; }
    
    XMoveResizeWindow(g_display, g_window, x, y, WIN_W, WIN_H);
}

// MAIN -----------------------------------------------------------------
int main() {
    SaveDefaultConfig();
    LoadConfig();
    
    // Open display
    g_display = XOpenDisplay(nullptr);
    if (!g_display) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }
    
    int screen = DefaultScreen(g_display);
    Window root = RootWindow(g_display, screen);
    g_colormap = DefaultColormap(g_display, screen);
    
    // Allocate colors
    BG = AllocColor(g_config.bg_r, g_config.bg_g, g_config.bg_b);
    C_OK = AllocColor(g_config.fps_ok_r, g_config.fps_ok_g, g_config.fps_ok_b);
    C_WARN = AllocColor(g_config.fps_warn_r, g_config.fps_warn_g, g_config.fps_warn_b);
    C_CRIT = AllocColor(g_config.fps_crit_r, g_config.fps_crit_g, g_config.fps_crit_b);
    C_DIM = AllocColor(g_config.text_dim_r, g_config.text_dim_g, g_config.text_dim_b);
    C_SEP = AllocColor(g_config.sep_r, g_config.sep_g, g_config.sep_b);
    
    // Create window
    XSetWindowAttributes attrs;
    attrs.override_redirect = True;  // Без рамок
    attrs.background_pixel = BG;
    attrs.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | 
                       ButtonMotionMask | PointerMotionMask | KeyPressMask;
    
    g_window = XCreateWindow(
        g_display, root,
        0, 0, WIN_W, WIN_H, 0,
        CopyFromParent, InputOutput, CopyFromParent,
        CWOverrideRedirect | CWBackPixel | CWEventMask, &attrs
    );
    
    // Set window properties
    Atom wm_type = XInternAtom(g_display, "_NET_WM_WINDOW_TYPE", False);
    Atom wm_type_dock = XInternAtom(g_display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(g_display, g_window, wm_type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&wm_type_dock, 1);
    
    // Stay on top
    Atom wm_state = XInternAtom(g_display, "_NET_WM_STATE", False);
    Atom wm_state_above = XInternAtom(g_display, "_NET_WM_STATE_ABOVE", False);
    XChangeProperty(g_display, g_window, wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char*)&wm_state_above, 1);
    
    // Load fonts
    g_font_fps = XLoadQueryFont(g_display, "-*-*-medium-r-*-*-*-120-*-*-*-*-*-*");
    g_font_val = XLoadQueryFont(g_display, "-*-*-medium-r-*-*-*-100-*-*-*-*-*-*");
    g_font_small = XLoadQueryFont(g_display, "-*-*-medium-r-*-*-*-80-*-*-*-*-*-*");
    
    if (!g_font_fps) g_font_fps = XLoadQueryFont(g_display, "fixed");
    if (!g_font_val) g_font_val = XLoadQueryFont(g_display, "fixed");
    if (!g_font_small) g_font_small = XLoadQueryFont(g_display, "fixed");
    
    // Create GC
    g_gc = XCreateGC(g_display, g_window, 0, nullptr);
    
    // Set opacity
    Atom opacity_atom = XInternAtom(g_display, "_NET_WM_WINDOW_OPACITY", False);
    unsigned long opacity = (unsigned long)(ALPHA_F * 0xFFFFFFFF);
    XChangeProperty(g_display, g_window, opacity_atom, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&opacity, 1);
    
    // Show window
    XMapWindow(g_display, g_window);
    MoveToCorner(CORNER);
    
    // Start collector thread
    std::thread collector(CollectorThread);
    
    // Event loop
    bool dragging = false;
    int drag_start_x = 0, drag_start_y = 0;
    XEvent ev;
    
    while (g_running) {
        while (XPending(g_display)) {
            XNextEvent(g_display, &ev);
            
            switch (ev.type) {
                case Expose:
                    Paint();
                    break;
                    
                case ButtonPress:
                    if (ev.xbutton.button == Button1) {
                        dragging = true;
                        drag_start_x = ev.xbutton.x;
                        drag_start_y = ev.xbutton.y;
                    } else if (ev.xbutton.button == Button3) {
                        // Close on right click
                        g_running = false;
                    }
                    break;
                    
                case ButtonRelease:
                    if (ev.xbutton.button == Button1) {
                        dragging = false;
                        // Check if clicking X button
                        if (ev.xbutton.x > WIN_W - 22 && ev.xbutton.y < 20) {
                            g_running = false;
                        }
                    }
                    break;
                    
                case MotionNotify:
                    if (dragging) {
                        int dx = ev.xmotion.x - drag_start_x;
                        int dy = ev.xmotion.y - drag_start_y;
                        Window root_ret, child_ret;
                        int root_x, root_y, win_x, win_y;
                        unsigned int mask;
                        XQueryPointer(g_display, g_window, &root_ret, &child_ret,
                                     &root_x, &root_y, &win_x, &win_y, &mask);
                        XMoveWindow(g_display, g_window, root_x - drag_start_x, root_y - drag_start_y);
                    }
                    break;
                    
                case KeyPress:
                    if (XLookupKeysym(&ev.xkey, 0) == XK_Escape) {
                        g_running = false;
                    }
                    break;
            }
        }
        
        // Timer-based redraw
        Paint();
        std::this_thread::sleep_for(std::chrono::milliseconds(TIMER_MS));
    }
    
    // Cleanup
    g_running = false;
    if (collector.joinable()) collector.join();
    
    if (g_gc) XFreeGC(g_display, g_gc);
    if (g_font_fps) XFreeFont(g_display, g_font_fps);
    if (g_font_val) XFreeFont(g_display, g_font_val);
    if (g_font_small) XFreeFont(g_display, g_font_small);
    if (g_window) XDestroyWindow(g_display, g_window);
    if (g_display) XCloseDisplay(g_display);
    
    return 0;
}
