#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// A car Los Santos Customs can give: its model (vehicles.rpf, the _hi
/// drawn), its name, the wheel it wears and the sizes vehicles.meta
/// would give it.
struct Gta5GarageCar {
    std::string model;
    std::string name;
    std::string wheel{"wheel_spt_01"};
    float radius{0.36f};
    float mass{1600.f};
};

struct Gta5Colour {
    std::string name;
    glm::vec3 rgb{1.f};
};

struct Gta5Garage {
    std::vector<Gta5GarageCar> cars;
    std::vector<Gta5Colour> colours;
};

/// data/garage.json; empty when missing.
Gta5Garage LoadGta5Garage(const std::string& file);

}  // namespace sdl3cpp::services::impl
