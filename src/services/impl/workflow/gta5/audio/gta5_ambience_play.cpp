#include "services/interfaces/workflow/gta5/audio/gta5_ambience.hpp"
#include "services/interfaces/workflow/gta5/audio/gta5_sound.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// Seconds to the next play: random, so a rule never ticks like a clock.
float NextWait(const Gta5AmbientRule& r, std::mt19937& rng) {
    std::exponential_distribution<float> gap(std::max(r.perMinute, 0.05f) /
                                             60.f);
    return std::clamp(gap(rng), 0.5f, 600.f);
}

void Sound(Gta5Ambience& a, Gta5AmbientRule& r, const Gta5Listener& l,
           std::mt19937& rng, SDL_AudioDeviceID device,
           const SDL_AudioSpec& spec, std::vector<SDL_AudioStream*>& out) {
    if (!r.read) {
        r.read = true;
        for (const std::string& file : r.files) {
            Gta5Clip clip;
            if (LoadGta5Clip(a.dir + "/" + file, clip)) {
                r.clips.push_back(std::move(clip));
            }
        }
    }
    if (r.clips.empty()) return;
    std::uniform_real_distribution<float> unit(0.f, 1.f);
    const float d = r.placed ? glm::distance(r.at, l.at)
                             : r.inner + unit(rng) * (r.outer - r.inner);
    if (d > r.outer) return;
    const float gain = 0.5f * (1.f - d / r.outer) * l.volume;
    PlayGta5Clip(out, r.clips, rng, device, spec, gain,
                 0.95f + 0.1f * unit(rng));
}

}  // namespace

void PlayGta5Ambience(Gta5Ambience& a, const Gta5Listener& l,
                      std::mt19937& rng, SDL_AudioDeviceID device,
                      const SDL_AudioSpec& spec,
                      std::vector<SDL_AudioStream*>& playing) {
    for (const Gta5AmbientZone& zone : a.zones) {
        if (!Gta5InAmbientZone(zone, l.at)) continue;
        for (const int i : zone.rules) {
            if (i < 0 || i >= int(a.rules.size())) continue;
            Gta5AmbientRule& rule = a.rules[i];
            if (!Gta5AmbientRuleAwake(rule, l.minutes)) continue;
            rule.wait -= l.dt;
            if (rule.wait > 0.f) continue;
            rule.wait = NextWait(rule, rng);
            Sound(a, rule, l, rng, device, spec, playing);
        }
    }
}

}  // namespace sdl3cpp::services::impl
