#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace sdl3cpp::q3 {

struct NavNode { glm::vec3 pos; std::vector<int> neighbors; };
struct NavGraph { std::vector<NavNode> nodes; };
using NavGraphPtr = std::shared_ptr<NavGraph>;
// context key: "q3.nav_graph" (NavGraphPtr)

}  // namespace sdl3cpp::q3
