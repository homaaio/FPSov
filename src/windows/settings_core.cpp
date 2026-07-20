/*
 * settings_core.cpp - Core settings library for FPS Overlay
 * Compiles to settings_core.dll
 * 
 * Compilation:
 * g++ -shared -O2 -o settings_core.dll settings_core.cpp -lws2_32
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

// Configuration structure
typedef struct {
    // Window settings
    int width;
    int height;
    float alpha;
    int margin;
    char corner[32];
    
    // Color settings
    int bg_r, bg_g, bg_b;
    int fps_ok_r, fps_ok_g, fps_ok_b;
    int fps_warn_r, fps_warn_g, fps_warn_b;
    int fps_crit_r, fps_crit_g, fps_crit_b;
    int text_dim_r, text_dim_g, text_dim_b;
    int sep_r, sep_g, sep_b;
    
    // Display settings
    int show_fps;
    int show_cpu;
    int show_gpu;
    int show_title;
    int font_fps;
    int font_val;
    int font_small;
    
    // Update settings
    int update_metrics;
    int update_overlay;
} OverlayConfig;

// Global config
static OverlayConfig g_config;

// Default configuration
static void SetDefaultConfig() {
    g_config.width = 200;
    g_config.height = 110;
    g_config.alpha = 0.88f;
    g_config.margin = 14;
    strcpy(g_config.corner, "top_right");
    
    g_config.bg_r = 10; g_config.bg_g = 10; g_config.bg_b = 10;
    g_config.fps_ok_r = 0; g_config.fps_ok_g = 220; g_config.fps_ok_b = 120;
    g_config.fps_warn_r = 255; g_config.fps_warn_g = 170; g_config.fps_warn_b = 0;
    g_config.fps_crit_r = 255; g_config.fps_crit_g = 50; g_config.fps_crit_b = 70;
    g_config.text_dim_r = 60; g_config.text_dim_g = 100; g_config.text_dim_b = 75;
    g_config.sep_r = 40; g_config.sep_g = 70; g_config.sep_b = 50;
    
    g_config.show_fps = 1;
    g_config.show_cpu = 1;
    g_config.show_gpu = 1;
    g_config.show_title = 1;
    g_config.font_fps = 18;
    g_config.font_val = 14;
    g_config.font_small = 10;
    
    g_config.update_metrics = 800;
    g_config.update_overlay = 33;
}

// Parse RGB from string "r,g,b"
static void ParseRGB(const std::string& str, int* r, int* g, int* b) {
    std::vector<int> rgb;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        rgb.push_back(std::stoi(item));
    }
    if (rgb.size() >= 3) {
        *r = rgb[0];
        *g = rgb[1];
        *b = rgb[2];
    }
}

// Load config from file
__declspec(dllexport) int LoadConfig() {
    SetDefaultConfig();
    
    std::ifstream file("overlay.ini");
    if (!file.is_open()) return 0;
    
    std::string line;
    while (std::getline(file, line)) {
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
        else if (key == "corner") strcpy(g_config.corner, val.c_str());
        
        // Color settings
        else if (key == "background") ParseRGB(val, &g_config.bg_r, &g_config.bg_g, &g_config.bg_b);
        else if (key == "fps_ok") ParseRGB(val, &g_config.fps_ok_r, &g_config.fps_ok_g, &g_config.fps_ok_b);
        else if (key == "fps_warn") ParseRGB(val, &g_config.fps_warn_r, &g_config.fps_warn_g, &g_config.fps_warn_b);
        else if (key == "fps_crit") ParseRGB(val, &g_config.fps_crit_r, &g_config.fps_crit_g, &g_config.fps_crit_b);
        else if (key == "text_dim") ParseRGB(val, &g_config.text_dim_r, &g_config.text_dim_g, &g_config.text_dim_b);
        else if (key == "separator") ParseRGB(val, &g_config.sep_r, &g_config.sep_g, &g_config.sep_b);
        
        // Display settings
        else if (key == "show_fps") g_config.show_fps = (val == "true" || val == "1") ? 1 : 0;
        else if (key == "show_cpu") g_config.show_cpu = (val == "true" || val == "1") ? 1 : 0;
        else if (key == "show_gpu") g_config.show_gpu = (val == "true" || val == "1") ? 1 : 0;
        else if (key == "show_title") g_config.show_title = (val == "true" || val == "1") ? 1 : 0;
        else if (key == "font_fps") g_config.font_fps = std::stoi(val);
        else if (key == "font_val") g_config.font_val = std::stoi(val);
        else if (key == "font_small") g_config.font_small = std::stoi(val);
        
        // Update settings
        else if (key == "update_metrics") g_config.update_metrics = std::stoi(val);
        else if (key == "update_overlay") g_config.update_overlay = std::stoi(val);
    }
    
    file.close();
    return 1;
}

// Save config to file
__declspec(dllexport) int SaveConfig() {
    std::ofstream file("overlay.ini");
    if (!file.is_open()) return 0;
    
    file << "; FPS Overlay Configuration File\n";
    file << "; Generated by Settings Core (C++)\n\n";
    
    file << "[Window]\n";
    file << "width=" << g_config.width << "\n";
    file << "height=" << g_config.height << "\n";
    file << "alpha=" << g_config.alpha << "\n";
    file << "margin=" << g_config.margin << "\n";
    file << "corner=" << g_config.corner << "\n\n";
    
    file << "[Colors]\n";
    file << "background=" << g_config.bg_r << "," << g_config.bg_g << "," << g_config.bg_b << "\n";
    file << "fps_ok=" << g_config.fps_ok_r << "," << g_config.fps_ok_g << "," << g_config.fps_ok_b << "\n";
    file << "fps_warn=" << g_config.fps_warn_r << "," << g_config.fps_warn_g << "," << g_config.fps_warn_b << "\n";
    file << "fps_crit=" << g_config.fps_crit_r << "," << g_config.fps_crit_g << "," << g_config.fps_crit_b << "\n";
    file << "text_dim=" << g_config.text_dim_r << "," << g_config.text_dim_g << "," << g_config.text_dim_b << "\n";
    file << "separator=" << g_config.sep_r << "," << g_config.sep_g << "," << g_config.sep_b << "\n\n";
    
    file << "[Display]\n";
    file << "show_fps=" << (g_config.show_fps ? "true" : "false") << "\n";
    file << "show_cpu=" << (g_config.show_cpu ? "true" : "false") << "\n";
    file << "show_gpu=" << (g_config.show_gpu ? "true" : "false") << "\n";
    file << "show_title=" << (g_config.show_title ? "true" : "false") << "\n";
    file << "font_fps=" << g_config.font_fps << "\n";
    file << "font_val=" << g_config.font_val << "\n";
    file << "font_small=" << g_config.font_small << "\n\n";
    
    file << "[Update]\n";
    file << "update_metrics=" << g_config.update_metrics << "\n";
    file << "update_overlay=" << g_config.update_overlay << "\n";
    
    file.close();
    return 1;
}

// Get individual settings
__declspec(dllexport) int GetWidth() { return g_config.width; }
__declspec(dllexport) int GetHeight() { return g_config.height; }
__declspec(dllexport) float GetAlpha() { return g_config.alpha; }
__declspec(dllexport) int GetMargin() { return g_config.margin; }
__declspec(dllexport) const char* GetCorner() { return g_config.corner; }

__declspec(dllexport) void GetBackground(int* r, int* g, int* b) {
    *r = g_config.bg_r; *g = g_config.bg_g; *b = g_config.bg_b;
}
__declspec(dllexport) void GetFpsOk(int* r, int* g, int* b) {
    *r = g_config.fps_ok_r; *g = g_config.fps_ok_g; *b = g_config.fps_ok_b;
}
__declspec(dllexport) void GetFpsWarn(int* r, int* g, int* b) {
    *r = g_config.fps_warn_r; *g = g_config.fps_warn_g; *b = g_config.fps_warn_b;
}
__declspec(dllexport) void GetFpsCrit(int* r, int* g, int* b) {
    *r = g_config.fps_crit_r; *g = g_config.fps_crit_g; *b = g_config.fps_crit_b;
}
__declspec(dllexport) void GetTextDim(int* r, int* g, int* b) {
    *r = g_config.text_dim_r; *g = g_config.text_dim_g; *b = g_config.text_dim_b;
}
__declspec(dllexport) void GetSeparator(int* r, int* g, int* b) {
    *r = g_config.sep_r; *g = g_config.sep_g; *b = g_config.sep_b;
}

__declspec(dllexport) int GetShowFps() { return g_config.show_fps; }
__declspec(dllexport) int GetShowCpu() { return g_config.show_cpu; }
__declspec(dllexport) int GetShowGpu() { return g_config.show_gpu; }
__declspec(dllexport) int GetShowTitle() { return g_config.show_title; }
__declspec(dllexport) int GetFontFps() { return g_config.font_fps; }
__declspec(dllexport) int GetFontVal() { return g_config.font_val; }
__declspec(dllexport) int GetFontSmall() { return g_config.font_small; }

__declspec(dllexport) int GetUpdateMetrics() { return g_config.update_metrics; }
__declspec(dllexport) int GetUpdateOverlay() { return g_config.update_overlay; }

// Set individual settings
__declspec(dllexport) void SetWidth(int val) { g_config.width = val; }
__declspec(dllexport) void SetHeight(int val) { g_config.height = val; }
__declspec(dllexport) void SetAlpha(float val) { g_config.alpha = val; }
__declspec(dllexport) void SetMargin(int val) { g_config.margin = val; }
__declspec(dllexport) void SetCorner(const char* val) { strcpy(g_config.corner, val); }

__declspec(dllexport) void SetBackground(int r, int g, int b) {
    g_config.bg_r = r; g_config.bg_g = g; g_config.bg_b = b;
}
__declspec(dllexport) void SetFpsOk(int r, int g, int b) {
    g_config.fps_ok_r = r; g_config.fps_ok_g = g; g_config.fps_ok_b = b;
}
__declspec(dllexport) void SetFpsWarn(int r, int g, int b) {
    g_config.fps_warn_r = r; g_config.fps_warn_g = g; g_config.fps_warn_b = b;
}
__declspec(dllexport) void SetFpsCrit(int r, int g, int b) {
    g_config.fps_crit_r = r; g_config.fps_crit_g = g; g_config.fps_crit_b = b;
}
__declspec(dllexport) void SetTextDim(int r, int g, int b) {
    g_config.text_dim_r = r; g_config.text_dim_g = g; g_config.text_dim_b = b;
}
__declspec(dllexport) void SetSeparator(int r, int g, int b) {
    g_config.sep_r = r; g_config.sep_g = g; g_config.sep_b = b;
}

__declspec(dllexport) void SetShowFps(int val) { g_config.show_fps = val; }
__declspec(dllexport) void SetShowCpu(int val) { g_config.show_cpu = val; }
__declspec(dllexport) void SetShowGpu(int val) { g_config.show_gpu = val; }
__declspec(dllexport) void SetShowTitle(int val) { g_config.show_title = val; }
__declspec(dllexport) void SetFontFps(int val) { g_config.font_fps = val; }
__declspec(dllexport) void SetFontVal(int val) { g_config.font_val = val; }
__declspec(dllexport) void SetFontSmall(int val) { g_config.font_small = val; }

__declspec(dllexport) void SetUpdateMetrics(int val) { g_config.update_metrics = val; }
__declspec(dllexport) void SetUpdateOverlay(int val) { g_config.update_overlay = val; }

// Reset to defaults
__declspec(dllexport) void ResetConfig() {
    SetDefaultConfig();
}

// Validate config (check for errors)
__declspec(dllexport) int ValidateConfig() {
    if (g_config.width < 100 || g_config.width > 500) return 0;
    if (g_config.height < 80 || g_config.height > 300) return 0;
    if (g_config.alpha < 0.3f || g_config.alpha > 1.0f) return 0;
    if (g_config.font_fps < 10 || g_config.font_fps > 40) return 0;
    if (g_config.font_val < 10 || g_config.font_val > 30) return 0;
    if (g_config.update_metrics < 200 || g_config.update_metrics > 2000) return 0;
    return 1;
}

// Get config as JSON string (for Python)
__declspec(dllexport) char* GetConfigJSON() {
    static char json[2048];
    sprintf(json, "{"
            "\"width\":%d,"
            "\"height\":%d,"
            "\"alpha\":%f,"
            "\"margin\":%d,"
            "\"corner\":\"%s\","
            "\"bg\":[%d,%d,%d],"
            "\"fps_ok\":[%d,%d,%d],"
            "\"fps_warn\":[%d,%d,%d],"
            "\"fps_crit\":[%d,%d,%d],"
            "\"text_dim\":[%d,%d,%d],"
            "\"separator\":[%d,%d,%d],"
            "\"show_fps\":%d,"
            "\"show_cpu\":%d,"
            "\"show_gpu\":%d,"
            "\"show_title\":%d,"
            "\"font_fps\":%d,"
            "\"font_val\":%d,"
            "\"font_small\":%d,"
            "\"update_metrics\":%d,"
            "\"update_overlay\":%d"
            "}",
            g_config.width, g_config.height, g_config.alpha, g_config.margin, g_config.corner,
            g_config.bg_r, g_config.bg_g, g_config.bg_b,
            g_config.fps_ok_r, g_config.fps_ok_g, g_config.fps_ok_b,
            g_config.fps_warn_r, g_config.fps_warn_g, g_config.fps_warn_b,
            g_config.fps_crit_r, g_config.fps_crit_g, g_config.fps_crit_b,
            g_config.text_dim_r, g_config.text_dim_g, g_config.text_dim_b,
            g_config.sep_r, g_config.sep_g, g_config.sep_b,
            g_config.show_fps, g_config.show_cpu, g_config.show_gpu, g_config.show_title,
            g_config.font_fps, g_config.font_val, g_config.font_small,
            g_config.update_metrics, g_config.update_overlay);
    return json;
}

#ifdef __cplusplus
}
#endif

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        LoadConfig();
    }
    return TRUE;
}