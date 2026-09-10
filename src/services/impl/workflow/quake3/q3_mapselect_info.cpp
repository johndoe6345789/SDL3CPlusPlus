#include "services/interfaces/workflow/quake3/q3_mapselect_info.hpp"
#include "services/interfaces/workflow/quake3/q3_mapselect_levelshot.hpp"

#include <algorithm>
#include <cctype>

namespace sdl3cpp::services::impl {
namespace {

using q3overlay::DrawPropText;
using q3overlay::LoadTextureFromPk3;

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

}  // namespace

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

}  // namespace sdl3cpp::services::impl
