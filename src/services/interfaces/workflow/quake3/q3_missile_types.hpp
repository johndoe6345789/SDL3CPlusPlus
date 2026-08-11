#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace sdl3cpp::q3 {

enum class MissileType { Rocket, Grenade, Plasma, BFG };

struct Q3Missile {
    uint32_t    id{0};
    MissileType type{MissileType::Rocket};
    glm::vec3   origin{};
    glm::vec3   velocity{};     // world-units/s
    float       damage{0};
    float       splashDamage{0};
    float       splashRadius{0};
    float       lifetimeLeft{10.f};  // seconds
    bool        fromPlayer{true};
    bool        exploded{false};
};

using MissileList = std::shared_ptr<std::vector<Q3Missile>>;
// context key: "q3.missiles" (MissileList)

}  // namespace sdl3cpp::q3
