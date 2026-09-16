#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/audio/gta5_sound_voice.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <random>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// One of GTA's ambient rules: a set of one-shots -- a bird, a car
/// passing a street away -- played now and then while the hour is right.
struct Gta5AmbientRule {
    glm::vec3 at{0.f};   // GTA metres; where it sounds when `placed`
    bool placed{false};  // else anywhere `inner`-`outer` from the player
    float inner{0.f};
    float outer{50.f};
    int start{0};  // minutes of the day it is heard between
    int end{1440};
    float perMinute{1.f};
    std::vector<std::string> files;  // relative to the ambience folder
    std::vector<Gta5Clip> clips;     // read the first time it plays
    bool read{false};
    float wait{0.f};  // seconds until it next plays
};

/// A box of the map (GTA metres, turned `angle` degrees about up) and
/// the rules that play inside it.
struct Gta5AmbientZone {
    glm::vec3 centre{0.f};
    glm::vec3 half{0.f};
    float angle{0.f};
    std::vector<int> rules;
};

struct Gta5Ambience {
    std::string dir;
    std::vector<Gta5AmbientZone> zones;
    std::vector<Gta5AmbientRule> rules;
};

/// dir/ambience.json, written by the audio export; empty without it.
Gta5Ambience LoadGta5Ambience(const std::string& dir,
                              const std::shared_ptr<ILogger>& logger);

/// Whether @p p is in the zone, and whether the rule plays at @p minutes
/// past midnight (a window may run over midnight).
bool Gta5InAmbientZone(const Gta5AmbientZone& zone, const glm::vec3& p);
bool Gta5AmbientRuleAwake(const Gta5AmbientRule& rule, float minutes);

/// What the listener hears this frame.
struct Gta5Listener {
    glm::vec3 at{0.f};  // GTA metres
    float minutes{720.f};
    float dt{0.f};
    float volume{1.f};
};

/// Plays the rules of every zone the listener is in, when they are due.
void PlayGta5Ambience(Gta5Ambience& ambience, const Gta5Listener& listener,
                      std::mt19937& rng, SDL_AudioDeviceID device,
                      const SDL_AudioSpec& spec,
                      std::vector<SDL_AudioStream*>& playing);

}  // namespace sdl3cpp::services::impl
