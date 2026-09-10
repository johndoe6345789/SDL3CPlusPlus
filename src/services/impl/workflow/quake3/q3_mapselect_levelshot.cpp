#include "services/interfaces/workflow/quake3/q3_mapselect_levelshot.hpp"

#include <algorithm>
#include <cctype>

namespace sdl3cpp::services::impl {

using q3overlay::LoadTextureFromPk3;

std::string ToUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
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

}  // namespace sdl3cpp::services::impl
