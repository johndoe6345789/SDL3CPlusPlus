#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

namespace sdl3cpp::q3 {

struct Q3Mover {
    std::string id;
    std::string classname;
    glm::vec3   pos1{};        // closed/bottom position
    glm::vec3   pos2{};        // open/top position
    float       travelTime{1.f};
    float       waitTime{2.f};
    float       stateProgress{0.f};  // 0=pos1, 1=pos2
    enum class State { AtPos1, MovingTo2, AtPos2, MovingTo1 } state{State::AtPos1};
    float       stateTimer{0.f};
    glm::vec3   currentPos{};
    glm::vec3   velocity{};    // current frame velocity (for player push)
};

using MoverList = std::shared_ptr<std::vector<Q3Mover>>;
// context key: "q3.movers" (MoverList)

}  // namespace sdl3cpp::q3
