#include "services/interfaces/workflow/gta5/gta5_garage.hpp"
#include "services/interfaces/workflow/gta5/gta5_weapons.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

namespace sdl3cpp::services::impl {
namespace {

nlohmann::json Read(const std::string& file, const char* list) {
    std::ifstream in(file);
    const nlohmann::json doc = nlohmann::json::parse(in, nullptr, false);
    if (!doc.is_object()) return nlohmann::json::array();
    return doc.value(list, nlohmann::json::array());
}

Gta5WeaponKind Kind(const std::string& kind) {
    if (kind == "rocket") return Gta5WeaponKind::Rocket;
    if (kind == "grenade") return Gta5WeaponKind::Grenade;
    if (kind == "melee") return Gta5WeaponKind::Melee;
    return Gta5WeaponKind::Bullet;
}

}  // namespace

Gta5Garage LoadGta5Garage(const std::string& file) {
    Gta5Garage garage;
    for (const auto& c : Read(file, "cars")) {
        Gta5GarageCar car;
        car.model = c.value("model", std::string());
        car.name = c.value("name", car.model);
        car.wheel = c.value("wheel", car.wheel);
        car.radius = c.value("radius", car.radius);
        car.mass = c.value("mass", car.mass);
        if (!car.model.empty()) garage.cars.push_back(car);
    }
    for (const auto& c : Read(file, "colours")) {
        const auto rgb = c.value("rgb", nlohmann::json::array());
        if (rgb.size() < 3) continue;
        garage.colours.push_back(
            {c.value("name", std::string("PAINT")),
             glm::vec3(rgb[0].get<float>(), rgb[1].get<float>(),
                       rgb[2].get<float>())});
    }
    return garage;
}

std::vector<Gta5Weapon> LoadGta5Weapons(const std::string& file) {
    std::vector<Gta5Weapon> out;
    for (const auto& w : Read(file, "weapons")) {
        Gta5Weapon weapon;
        weapon.id = w.value("id", std::string());
        weapon.name = w.value("name", weapon.id);
        weapon.model = w.value("model", std::string());
        weapon.projectile = w.value("projectile", std::string());
        weapon.kind = Kind(w.value("kind", std::string("bullet")));
        weapon.clip = w.value("clip", 0);
        weapon.max = w.value("max", 0);
        weapon.pellets = w.value("pellets", 1);
        weapon.hands = w.value("hands", 2);
        weapon.damage = w.value("damage", 0.f);
        weapon.rate = w.value("rate", 1.f);
        weapon.range = w.value("range", 100.f);
        weapon.spread = w.value("spread", 0.f);
        weapon.speed = w.value("speed", 0.f);
        weapon.automatic = w.value("automatic", false);
        if (!weapon.id.empty()) out.push_back(weapon);
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
