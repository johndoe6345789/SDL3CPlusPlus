#include "services/interfaces/workflow/quake3/q3_mapselect_draw.hpp"

#include <algorithm>
#include <cctype>

namespace sdl3cpp::services::impl {
namespace {

using q3overlay::DrawPropText;
using q3overlay::DrawQ3Text;
using q3overlay::kH;
using q3overlay::kW;
using q3overlay::LoadTextureFromPk3;

std::string ToUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

/// Returns the cached texture for `cacheKey`, loading and caching it from
/// `pk3RelativePath` inside `pk3` on first use (cache still gets a null
/// entry when the load fails, matching the original's behavior).
SDL_Texture* GetOrLoadCachedTexture(SDL_Renderer* r,
                                    q3overlay::LevelshotCache& cache,
                                    const std::string& pk3,
                                    const std::string& cacheKey,
                                    const std::string& pk3RelativePath) {
    auto it = cache.find(cacheKey);
    if (it != cache.end()) return it->second;
    SDL_Texture* tex = LoadTextureFromPk3(r, pk3, pk3RelativePath.c_str());
    cache[cacheKey]  = tex;
    return tex;
}

void DrawLevelshot(const Q3MapSelectAssets& a, const std::string& mapName,
                   float x, float y, float w, float h) {
    SDL_Texture* shot = nullptr;
    if (a.shots && !mapName.empty() && !a.pk3Path.empty()) {
        const std::string key = ToUpper(mapName);
        auto it               = a.shots->find(key);
        if (it != a.shots->end()) {
            shot = it->second;
        } else {
            shot = LoadTextureFromPk3(a.renderer, a.pk3Path,
                                      ("levelshots/" + key + ".jpg").c_str());
            if (!shot) {
                shot = LoadTextureFromPk3(
                    a.renderer, a.pk3Path,
                    ("levelshots/" + mapName + ".jpg").c_str());
            }
            (*a.shots)[key] = shot;
        }
    }

    if (shot) {
        SDL_FRect dst{x, y, w, h};
        SDL_SetTextureAlphaMod(shot, 255);
        SDL_SetTextureBlendMode(shot, SDL_BLENDMODE_NONE);
        SDL_RenderTexture(a.renderer, shot, nullptr, &dst);
    } else {
        SDL_SetRenderDrawColor(a.renderer, 30, 20, 15, 255);
        SDL_FRect fb{x, y, w, h};
        SDL_RenderFillRect(a.renderer, &fb);
    }
    SDL_SetRenderDrawColor(a.renderer, 180, 50, 20, 255);
    SDL_FRect border{x - 2.f, y - 2.f, w + 4.f, h + 4.f};
    SDL_RenderRect(a.renderer, &border);
}

void DrawArenaInfo(const Q3MapSelectAssets& a, const std::string& mapName,
                   float centerX, float infoY) {
    if (!a.arenas) return;
    auto aIt = a.arenas->find(ToLower(mapName));
    if (aIt == a.arenas->end() || aIt->second.longname.empty()) return;

    const std::string ln =
        ToUpper(ToUpper(mapName) + ": " + aIt->second.longname);
    DrawPropText(a.renderer, a.prop, centerX, infoY, ln.c_str(),
                 {200, 130, 40, 255}, 0.75f, true);

    if (aIt->second.bot.empty() || !a.shots || a.pk3Path.empty()) return;
    const std::string botKey = aIt->second.bot;
    const std::string bk     = ToLower(botKey);
    SDL_Texture* botIcon =
        GetOrLoadCachedTexture(a.renderer, *a.shots, a.pk3Path, "bot_" + bk,
                               "models/players/" + bk + "/icon_default.tga");
    if (botIcon) {
        SDL_FRect dst{centerX - 22.f, infoY + 42.f, 44.f, 44.f};
        SDL_SetTextureAlphaMod(botIcon, 255);
        SDL_SetTextureBlendMode(botIcon, SDL_BLENDMODE_BLEND);
        SDL_RenderTexture(a.renderer, botIcon, nullptr, &dst);
    }
    DrawPropText(a.renderer, a.prop, centerX, infoY + 90.f,
                 ToUpper(botKey).c_str(), {180, 130, 50, 220}, 0.65f, true);
}

void DrawNavArrows(const Q3MapSelectAssets& a, int idx, int nMaps, float y) {
    if (a.arrowLeft && idx > 0) {
        SDL_FRect d{8.f, y, 64.f, 32.f};
        SDL_RenderTexture(a.renderer, a.arrowLeft, nullptr, &d);
    }
    if (a.arrowRight && idx < nMaps - 1) {
        SDL_FRect d{kW - 72.f, y, 64.f, 32.f};
        SDL_RenderTexture(a.renderer, a.arrowRight, nullptr, &d);
    }
}

void DrawMapSelectButtons(const Q3MapSelectAssets& a) {
    constexpr float kBY = static_cast<float>(kH) - 52.f, kBW = 128.f,
                    kBH = 46.f;
    auto drawBtn        = [&](SDL_Texture* t, float x) {
        if (!t) return;
        SDL_SetTextureAlphaMod(t, 255);
        SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
        SDL_FRect d{x, kBY, kBW, kBH};
        SDL_RenderTexture(a.renderer, t, nullptr, &d);
    };
    drawBtn(a.btnBack, kW * 0.12f - kBW * 0.5f);
    drawBtn(a.btnSkirmish, kW * 0.50f - kBW * 0.5f);
    drawBtn(a.btnFight, kW * 0.88f - kBW * 0.5f);
}

}  // namespace

void DrawQ3MapSelectScreen(const Q3MapSelectAssets& a) {
    SDL_SetRenderDrawColor(a.renderer, 0, 0, 0, 255);
    SDL_FRect bg{0, 0, static_cast<float>(kW), static_cast<float>(kH)};
    SDL_RenderFillRect(a.renderer, &bg);

    constexpr float kCX = kW * 0.5f;
    DrawQ3Text(a.renderer, a.bigchars, kCX - 96.f, 12.f, "CHOOSE LEVEL",
               {200, 50, 20, 255}, 1.0f);

    const int nMaps = static_cast<int>(a.maps.size());
    const int idx =
        (nMaps > 0) ? std::max(0, std::min(a.selectedItem, nMaps - 1)) : 0;
    const std::string mapName =
        (nMaps > 0) ? a.maps[static_cast<size_t>(idx)].get<std::string>()
                    : "q3dm7";

    constexpr float kLW = 162.f, kLH = 162.f, kLX = (kW - kLW) * 0.5f,
                    kLY = 58.f;
    DrawLevelshot(a, mapName, kLX, kLY, kLW, kLH);

    const std::string mU = ToUpper(mapName);
    DrawPropText(a.renderer, a.prop, kCX, kLY + kLH + 6.f, mU.c_str(),
                 {255, 200, 50, 255}, 0.7f, true);
    DrawArenaInfo(a, mapName, kCX, kLY + kLH + 34.f);

    DrawNavArrows(a, idx, nMaps, kLY + kLH * 0.5f - 16.f);
    DrawMapSelectButtons(a);
}

}  // namespace sdl3cpp::services::impl
