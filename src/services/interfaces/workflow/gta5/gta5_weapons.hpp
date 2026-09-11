#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

enum class Gta5WeaponKind { Bullet, Rocket, Grenade, Melee };

/// One of Ammu-Nation's weapons and how it behaves: see
/// data/weapons.json.
struct Gta5Weapon {
    std::string id;
    std::string name;
    std::string model;       // weapons.rpf .ydr, in the hand
    std::string projectile;  // what a rocket or grenade flies as
    Gta5WeaponKind kind{Gta5WeaponKind::Bullet};
    int clip{0};  // -1: none, as a bat has none
    int max{0};
    int pellets{1};
    float damage{0.f};
    float rate{1.f};
    float range{100.f};
    float spread{0.f};
    float speed{0.f};
    bool automatic{false};
};

std::vector<Gta5Weapon> LoadGta5Weapons(const std::string& file);

/// What the player carries (gta5.inventory), by catalogue index: rounds
/// loaded (-1 when not owned) and besides, and the weapon in hand.
struct Gta5Inventory {
    std::vector<int> clip;
    std::vector<int> reserve;
    int current{-1};  // unarmed
    void Fit(std::size_t count) {
        clip.resize(count, -1);
        reserve.resize(count, 0);
    }
    bool Owns(int i) const {
        return i >= 0 && i < static_cast<int>(clip.size()) && clip[i] >= 0;
    }
};

}  // namespace sdl3cpp::services::impl
