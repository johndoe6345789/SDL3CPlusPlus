#include "services/interfaces/workflow/gta5/gta5_player_character_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_ped_pose.hpp"
#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"

#include <SDL3/SDL_mouse.h>

#include <cstdio>
#include <string>

namespace sdl3cpp::services::impl {
namespace {

std::string Three(const glm::vec3& v) {
    char out[64];
    std::snprintf(out, sizeof(out), "%+.2f %+.2f %+.2f", v.x, v.y, v.z);
    return out;
}

/// The way a bone runs to the next in the bind pose, or a note that
/// the skeleton has no such bone -- which would leave it unposed.
std::string Runs(const Gta5Skeleton& s, const char* from, const char* to) {
    const int a = FindGta5Bone(s, from), b = FindGta5Bone(s, to);
    if (a < 0 || b < 0) return std::string("no ") + (a < 0 ? from : to);
    return Three(glm::normalize(glm::vec3(s.rest[b][3] - s.rest[a][3])));
}

}  // namespace

void WorkflowGta5PlayerCharacterStep::Load(const WorkflowStepDefinition& step,
                                           SDL_GPUDevice* device) {
    tried_ = true;
    const Gta5PedSpec spec{
        Gta5ParameterOr(step, "ped_dir", "."),
        Gta5ParameterOr(step, "ped", "a_m_y_hipster_01"),
        {"head_000_r", "hair_000_r", "uppr_000_r", "lowr_000_u"}};
    LoadGta5Ped(*state_, device, spec, ped_, logger_);
    const Gta5Skeleton& s = ped_.skeleton;
    if (!logger_ || s.names.empty()) return;
    // How this ped turned out to be built. Everything that hangs an arm
    // or turns the body is counted off these, so if one reads wrong the
    // pose is wrong in exactly the way this says.
    const Gta5PedAxes axes = Gta5AxesOf(s);
    logger_->Info("gta5.player.character: hip to head " +
                  Runs(s, "SKEL_L_Thigh", "SKEL_Head") + ", hand " +
                  Runs(s, "SKEL_R_Forearm", "SKEL_R_Hand"));
    logger_->Info("gta5.player.character: up " + Three(axes.up) +
                  ", front " + Three(axes.front) + ", arm hangs " +
                  Three(Gta5ArmRest(s, axes, "SKEL_R_UpperArm",
                                    "SKEL_R_Forearm")) +
                  ", bind arm runs " +
                  Runs(s, "SKEL_R_UpperArm", "SKEL_R_Forearm"));
}

void WorkflowGta5PlayerCharacterStep::Watch(WorkflowContext& context) {
    const bool aiming = context.GetBool("gta5.weapon.aiming", false);
    if (!logger_ || aiming == aimed_) return;
    aimed_ = aiming;
    logger_->Info(std::string("gta5.player.character: aiming ") +
                  (aiming ? "on" : "off") + ", mouse buttons " +
                  std::to_string(SDL_GetMouseState(nullptr, nullptr)) +
                  ", hands " +
                  std::to_string(context.Get<int>("gta5.weapon.hands", 2)));
}

}  // namespace sdl3cpp::services::impl
