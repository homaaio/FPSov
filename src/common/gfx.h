/*
 * gfx.h - tiny software rasterizer (premultiplied ARGB, anti-aliased shapes)
 * Pixel layout is 0xAARRGGBB (little-endian BGRA bytes), which is exactly what
 * X11 ARGB32 visuals and Windows 32-bit DIBs expect.
 */
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace gfx {

struct Canvas {
    int w = 0, h = 0;
    std::vector<uint32_t> px;
    void Resize(int W, int H) { w = W; h = H; px.assign((size_t)W * H, 0u); }
    void Clear() { std::fill(px.begin(), px.end(), 0u); }
};

inline uint32_t Div255(uint32_t v) { v += 128; return (v + (v >> 8)) >> 8; }
inline float Clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

// Source-over blend of a (non-premultiplied) color with coverage/alpha `a` (0..255).
inline void BlendPx(uint32_t& d, uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
    if (!a) return;
    uint32_t ia = 255 - a;
    uint32_t da = d >> 24, dr = (d >> 16) & 255, dg = (d >> 8) & 255, db = d & 255;
    uint32_t oa = std::min<uint32_t>(255, a + Div255(da * ia));
    uint32_t orr = std::min<uint32_t>(255, Div255(r * a) + Div255(dr * ia));
    uint32_t og = std::min<uint32_t>(255, Div255(g * a) + Div255(dg * ia));
    uint32_t ob = std::min<uint32_t>(255, Div255(b * a) + Div255(db * ia));
    d = (oa << 24) | (orr << 16) | (og << 8) | ob;
}

inline void FillRect(Canvas& c, int x, int y, int w, int h, int r, int g, int b, float alpha) {
    uint32_t a = (uint32_t)(Clamp01(alpha) * 255.0f + 0.5f);
    int x0 = std::max(0, x), y0 = std::max(0, y);
    int x1 = std::min(c.w, x + w), y1 = std::min(c.h, y + h);
    for (int yy = y0; yy < y1; yy++)
        for (int xx = x0; xx < x1; xx++)
            BlendPx(c.px[(size_t)yy * c.w + xx], r, g, b, a);
}

// Signed distance from point to a rounded rectangle (negative inside).
inline float RRectSDF(float px, float py, float cx, float cy, float hw, float hh, float rad) {
    float qx = std::fabs(px - cx) - (hw - rad);
    float qy = std::fabs(py - cy) - (hh - rad);
    float ox = std::max(qx, 0.0f), oy = std::max(qy, 0.0f);
    return std::sqrt(ox * ox + oy * oy) + std::min(std::max(qx, qy), 0.0f) - rad;
}

// Filled anti-aliased rounded rectangle.
inline void FillRRect(Canvas& c, float x, float y, float w, float h, float rad,
                      int r, int g, int b, float alpha) {
    rad = std::max(0.0f, std::min(rad, std::min(w, h) * 0.5f));
    float cx = x + w * 0.5f, cy = y + h * 0.5f, hw = w * 0.5f, hh = h * 0.5f;
    int x0 = std::max(0, (int)std::floor(x) - 1), y0 = std::max(0, (int)std::floor(y) - 1);
    int x1 = std::min(c.w, (int)std::ceil(x + w) + 1), y1 = std::min(c.h, (int)std::ceil(y + h) + 1);
    for (int yy = y0; yy < y1; yy++)
        for (int xx = x0; xx < x1; xx++) {
            float d = RRectSDF(xx + 0.5f, yy + 0.5f, cx, cy, hw, hh, rad);
            float cov = Clamp01(0.5f - d);
            if (cov <= 0) continue;
            BlendPx(c.px[(size_t)yy * c.w + xx], r, g, b, (uint32_t)(cov * Clamp01(alpha) * 255.0f + 0.5f));
        }
}

// Anti-aliased rounded-rectangle outline drawn inside the given rectangle.
inline void StrokeRRect(Canvas& c, float x, float y, float w, float h, float rad, float width,
                        int r, int g, int b, float alpha) {
    rad = std::max(0.0f, std::min(rad, std::min(w, h) * 0.5f));
    float cx = x + w * 0.5f, cy = y + h * 0.5f, hw = w * 0.5f, hh = h * 0.5f;
    int x0 = std::max(0, (int)std::floor(x) - 1), y0 = std::max(0, (int)std::floor(y) - 1);
    int x1 = std::min(c.w, (int)std::ceil(x + w) + 1), y1 = std::min(c.h, (int)std::ceil(y + h) + 1);
    for (int yy = y0; yy < y1; yy++)
        for (int xx = x0; xx < x1; xx++) {
            float d = RRectSDF(xx + 0.5f, yy + 0.5f, cx, cy, hw, hh, rad);
            float outer = Clamp01(0.5f - d);
            float inner = Clamp01(0.5f - (d + width));
            float cov = outer - inner;
            if (cov <= 0) continue;
            BlendPx(c.px[(size_t)yy * c.w + xx], r, g, b, (uint32_t)(cov * Clamp01(alpha) * 255.0f + 0.5f));
        }
}

// Blend an 8-bit coverage mask (e.g. a rasterized glyph) with a color.
inline void DrawMask(Canvas& c, int x, int y, const uint8_t* m, int w, int h, int pitch,
                     int r, int g, int b, float alpha) {
    uint32_t ai = (uint32_t)(Clamp01(alpha) * 255.0f + 0.5f);
    for (int row = 0; row < h; row++) {
        int yy = y + row;
        if (yy < 0 || yy >= c.h) continue;
        const uint8_t* src = m + (size_t)row * pitch;
        for (int col = 0; col < w; col++) {
            int xx = x + col;
            if (xx < 0 || xx >= c.w || !src[col]) continue;
            BlendPx(c.px[(size_t)yy * c.w + xx], r, g, b, Div255(src[col] * ai));
        }
    }
}

// Scale every (premultiplied) channel - used for whole-overlay opacity.
inline void ApplyOpacity(Canvas& c, float k) {
    if (k >= 0.999f) return;
    uint32_t ki = (uint32_t)(Clamp01(k) * 255.0f + 0.5f);
    for (auto& p : c.px) {
        uint32_t a = Div255((p >> 24) * ki), r = Div255(((p >> 16) & 255) * ki);
        uint32_t g = Div255(((p >> 8) & 255) * ki), b = Div255((p & 255) * ki);
        p = (a << 24) | (r << 16) | (g << 8) | b;
    }
}

// Minimal UTF-8 decoder. Returns the code point and advances `i`.
inline uint32_t NextCodepoint(const std::string& s, size_t& i) {
    unsigned char c = (unsigned char)s[i++];
    if (c < 0x80) return c;
    int extra = (c >= 0xF0) ? 3 : (c >= 0xE0) ? 2 : (c >= 0xC0) ? 1 : 0;
    uint32_t cp = c & (0x3F >> extra);
    while (extra-- > 0 && i < s.size() && ((unsigned char)s[i] & 0xC0) == 0x80)
        cp = (cp << 6) | ((unsigned char)s[i++] & 0x3F);
    return cp;
}

} // namespace gfx
