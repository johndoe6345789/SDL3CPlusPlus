#pragma once
#include <SDL3/SDL_render.h>
#include <stb_image.h>
#include <zip.h>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::q3overlay {

struct ArenaInfo { std::string longname; std::string bot; };
using ArenaMap       = std::unordered_map<std::string, ArenaInfo>;
using LevelshotCache = std::unordered_map<std::string, SDL_Texture*>;

static constexpr int kW = 640, kH = 480;  // ioq3 native virtual resolution
static constexpr int kGlyphSrc   = 16;
static constexpr int kPropHeight = 27;
static constexpr int kPropGap    = 3;
static constexpr int kPropSpace  = 8;

// Q3 proportional font character map [128][3] = {src_x, src_y, width}
// From ioquake3 code/q3_ui/ui_atoms.c propMap
inline constexpr int kPropMap[128][3] = {
    {0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},
    {0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},
    {0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},
    {0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},
    {0,0,8},
    {11,122,7},{154,181,14},{55,122,17},{79,122,18},{101,122,23},{153,122,18},{9,93,7},
    {207,122,8},{230,122,9},{177,122,18},{30,152,18},{85,181,7},{34,93,11},{110,181,6},{130,152,14},
    {22,64,17},{41,64,12},{58,64,17},{78,64,18},{98,64,19},{120,64,18},{141,64,18},{204,64,16},{162,64,17},{182,64,18},
    {59,181,7},{35,181,7},{203,152,14},{56,93,14},{228,152,14},{177,181,18},{28,122,22},
    {5,4,18},{27,4,18},{48,4,18},{69,4,17},{90,4,13},{106,4,13},{121,4,18},{143,4,17},
    {164,4,8},{175,4,16},{195,4,18},{216,4,12},{230,4,23},{6,34,18},{27,34,18},{48,34,18},
    {68,34,18},{90,34,17},{110,34,18},{130,34,14},{146,34,18},{166,34,19},{185,34,29},
    {215,34,18},{234,34,18},{5,64,14},
    {60,152,7},{106,151,13},{83,152,7},{128,122,17},{4,152,21},{134,181,5},
    {5,4,18},{27,4,18},{48,4,18},{69,4,17},{90,4,13},{106,4,13},{121,4,18},{143,4,17},
    {164,4,8},{175,4,16},{195,4,18},{216,4,12},{230,4,23},{6,34,18},{27,34,18},{48,34,18},
    {68,34,18},{90,34,17},{110,34,18},{130,34,14},{146,34,18},{166,34,19},{185,34,29},
    {215,34,18},{234,34,18},{5,64,14},
    {153,152,13},{11,181,5},{180,152,13},{79,93,17},{0,0,-1}
};

inline SDL_Texture* LoadTextureFromPk3(
        SDL_Renderer* renderer, const std::string& pk3Path, const char* entry) {
    if (pk3Path.empty() || !renderer || !entry) return nullptr;
    int ze = 0;
    zip_t* arc = zip_open(pk3Path.c_str(), ZIP_RDONLY, &ze);
    if (!arc) return nullptr;
    zip_stat_t st;
    if (zip_stat(arc, entry, 0, &st) != 0) { zip_close(arc); return nullptr; }
    std::vector<uint8_t> buf(st.size);
    zip_file_t* zf = zip_fopen(arc, entry, 0);
    if (!zf) { zip_close(arc); return nullptr; }
    zip_fread(zf, buf.data(), st.size);
    zip_fclose(zf); zip_close(arc);
    int w = 0, h = 0, ch = 0;
    unsigned char* px = stbi_load_from_memory(buf.data(), (int)buf.size(), &w, &h, &ch, 4);
    if (!px) return nullptr;
    SDL_Surface* surf = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, px, w * 4);
    SDL_Texture* tex = surf ? SDL_CreateTextureFromSurface(renderer, surf) : nullptr;
    if (surf) SDL_DestroySurface(surf);
    stbi_image_free(px);
    return tex;
}

inline float PropStringWidth(const char* text) {
    if (!text) return 0.f;
    float w = 0.f;
    for (const char* p = text; *p; ++p) {
        int ch = (unsigned char)*p & 127;
        int cw = kPropMap[ch][2];
        if (cw < 0) continue;
        w += (float)(cw == kPropSpace ? kPropSpace : cw + kPropGap);
    }
    return w > 0.f ? w - kPropGap : 0.f;
}

inline void DrawPropText(SDL_Renderer* r, SDL_Texture* font,
        float x, float y, const char* text, SDL_Color col, float scale, bool center) {
    if (!text || !r || !font) return;
    if (center) x -= PropStringWidth(text) * scale * 0.5f;
    SDL_SetTextureColorMod(font, col.r, col.g, col.b);
    SDL_SetTextureAlphaMod(font, col.a);
    SDL_SetTextureBlendMode(font, SDL_BLENDMODE_BLEND);
    float cx = x;
    for (const char* p = text; *p; ++p) {
        int ch = (unsigned char)*p & 127;
        int cw = kPropMap[ch][2];
        if (cw < 0) continue;
        if (cw == kPropSpace) { cx += kPropSpace * scale; continue; }
        SDL_FRect src{(float)kPropMap[ch][0], (float)kPropMap[ch][1], (float)cw, (float)kPropHeight};
        SDL_FRect dst{cx, y, cw * scale, kPropHeight * scale};
        SDL_RenderTexture(r, font, &src, &dst);
        cx += (cw + kPropGap) * scale;
    }
}

inline void DrawQ3Text(SDL_Renderer* r, SDL_Texture* bigchars,
        float x, float y, const char* text, SDL_Color col, float scale) {
    if (!text || !r) return;
    if (bigchars) {
        SDL_SetTextureColorMod(bigchars, col.r, col.g, col.b);
        SDL_SetTextureAlphaMod(bigchars, col.a);
        SDL_SetTextureBlendMode(bigchars, SDL_BLENDMODE_BLEND);
        float cx = x;
        for (const char* p = text; *p; ++p) {
            int code = (unsigned char)*p;
            SDL_FRect src{(float)((code%16)*kGlyphSrc), (float)((code/16)*kGlyphSrc),
                          (float)kGlyphSrc, (float)kGlyphSrc};
            SDL_FRect dst{cx, y, kGlyphSrc * scale, kGlyphSrc * scale};
            SDL_RenderTexture(r, bigchars, &src, &dst);
            cx += kGlyphSrc * scale;
        }
    } else {
        SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
        SDL_RenderDebugText(r, x, y, text);
    }
}

inline float DrawHudNumber(SDL_Renderer* r, SDL_Texture* digits[11],
        float x, float y, int value, float scale) {
    if (!r) return x;
    std::string s = (value < 0) ? ("-" + std::to_string(-value)) : std::to_string(value);
    const float dw = 32.f * scale;
    for (char c : s) {
        int idx = (c == '-') ? 10 : (c - '0');
        if (idx >= 0 && idx <= 10 && digits[idx]) {
            SDL_SetTextureColorMod(digits[idx], 255, 220, 60);
            SDL_SetTextureAlphaMod(digits[idx], 255);
            SDL_SetTextureBlendMode(digits[idx], SDL_BLENDMODE_BLEND);
            SDL_FRect dst{x, y, dw, dw};
            SDL_RenderTexture(r, digits[idx], nullptr, &dst);
        }
        x += dw;
    }
    return x;
}

} // namespace sdl3cpp::q3overlay
