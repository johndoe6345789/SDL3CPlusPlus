#include "services/interfaces/workflow/gta5/gta5_player_character_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_ped_frame.hpp"
#include "services/interfaces/workflow/gta5/gta5_ped_stance.hpp"
#include "services/interfaces/workflow/gta5/gta5_shown_transform.hpp"
#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5PlayerCharacterStep::WorkflowGta5PlayerCharacterStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}
std::string WorkflowGta5PlayerCharacterStep::GetPluginId() const {
    return "gta5.player.character";
}
void WorkflowGta5PlayerCharacterStep::Load(const WorkflowStepDefinition& step,
                                           SDL_GPUDevice* device) {
    tried_ = true;
    const Gta5PedSpec spec{
        Gta5ParameterOr(step, "ped_dir", "."),
        Gta5ParameterOr(step, "ped", "a_m_y_hipster_01"),
        {"head_000_r", "hair_000_r", "uppr_000_r", "lowr_000_u"}};
    LoadGta5Ped(*state_, device, spec, ped_, logger_);
}
void WorkflowGta5PlayerCharacterStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!state_) return;
    state_->character.clear();
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    btRigidBody* player = Gta5PlayerBody(context);
    if (!device || !player || state_->seated >= 0 ||
        !context.GetBool("gta5.third_person", false)) {
        return;
    }
    if (!tried_) Load(step, device);
    if (ped_.parts.empty()) return;

    const auto ps = context.Get<Q3PlayerState>("q3.ps", Q3PlayerState{});
    const float dt =
        std::clamp(context.Get<float>("physics_dt", 1.f / 60.f), 0.f, 0.1f);
    const glm::vec2 run(ps.velocity.x, ps.velocity.z);
    const float speed = glm::length(run);
    SettleGta5Stance(stance_, context, run, dt);
    PoseGta5Ped(ped_.skeleton, walk_, speed, dt, stance_.aim,
                context.Get<float>("camera_pitch", 0.f),
                context.Get<int>("gta5.weapon.hands", 2) == 2, skin_);
    StageGta5PedFrame(*state_, device, ped_, skin_, frame_++ % kGta5PedRing,
                      skinned_);
    // Feet on the floor q3 stands the player on: its box's bottom.
    const btVector3 body = Gta5ShownTransform(player).getOrigin();
    const glm::vec3 feet(body.x(), body.y() + ps.mins.y - ped_.feet, body.z());
    const glm::mat4 model =
        glm::rotate(glm::translate(glm::mat4(1.f), feet), stance_.yaw,
                    glm::vec3(0.f, 1.f, 0.f));
    Gta5Instance instance;
    instance.geometry = &ped_.geometry;
    std::memcpy(instance.modelMatrix.data(), glm::value_ptr(model),
                sizeof(float) * 16);
    state_->character.push_back(instance);
    AddGta5HeldWeapon(*state_, device, ped_, skin_, model,
                      Gta5ParameterOr(step, "weapons_dir", ""),
                      context.GetString("gta5.weapon.model", ""), weapon_,
                      weaponModel_, logger_);
}

}  // namespace sdl3cpp::services::impl
