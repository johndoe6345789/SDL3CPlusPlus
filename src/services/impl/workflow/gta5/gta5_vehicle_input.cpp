#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"

#include <string>

namespace sdl3cpp::services::impl {

bool Gta5KeyDown(const nlohmann::json* keys, const char* name) {
    return keys && keys->contains(name);
}

btRigidBody* Gta5PlayerBody(WorkflowContext& context) {
    const std::string name =
        context.GetString("physics_player_body", "player");
    return context.Get<btRigidBody*>("physics_body_" + name, nullptr);
}

}  // namespace sdl3cpp::services::impl
