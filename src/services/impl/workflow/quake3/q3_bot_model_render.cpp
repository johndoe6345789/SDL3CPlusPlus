#include "services/interfaces/workflow/quake3/q3_bot_model_render_internal.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace sdl3cpp::services::impl {

void DrawBotModelChain(const nlohmann::json& bot,
                       const BotModelPrefixes& prefixes, const glm::mat4& view,
                       const glm::mat4& proj, const glm::vec3& camPos,
                       const glm::mat4& shadowVP,
                       const rendering::FragmentUniformData& fu,
                       SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                       SDL_GPUTexture* shadowTex, SDL_GPUSampler* shadowSamp,
                       WorkflowContext& context) {
    namespace detail = bot_model_detail;
    if (bot.value("state", std::string{}) == "dead") return;

    const auto& posJ = bot["pos"];
    const glm::vec3 bpos(posJ[0].get<float>(), posJ[1].get<float>(),
                         posJ[2].get<float>());
    const float yaw      = bot.value("yaw", 0.0f);
    const int legFrame   = bot.value("leg_frame", 0);
    const int torsoFrame = bot.value("torso_frame", 0);

    auto draw = [&](const std::string& pfx, int frame,
                    const glm::mat4& modelMat) {
        detail::DrawBotModelPart(pfx, frame, modelMat, view, proj, camPos,
                                 shadowVP, fu, pass, cmd, shadowTex, shadowSamp,
                                 context);
    };

    // ── lower.md3: root transform ──────────────────────────────────
    // MD3 is Quake Z-up (X forward, Y left, Z up); a bare yaw about
    // world Y would leave the model on its side. Same remap as
    // q3.md3.draw, matching ioq3's AnglesToAxis().
    const glm::vec3 bf(-std::sin(yaw), 0.0f, -std::cos(yaw));
    const glm::vec3 bu(0.0f, 1.0f, 0.0f);
    glm::mat4 bOrient(1.0f);
    bOrient[0]               = glm::vec4(bf, 0.0f);
    bOrient[1]               = glm::vec4(glm::cross(bu, bf), 0.0f);
    bOrient[2]               = glm::vec4(bu, 0.0f);
    const glm::mat4 lowerMat = glm::translate(glm::mat4(1.0f), bpos) * bOrient;
    draw(prefixes.lower, legFrame, lowerMat);
    if (!prefixes.hasUpper) return;

    // upper.md3: attached at tag_torso from lower.
    const glm::mat4 upperMat =
        lowerMat * detail::GetBotModelTagMatrix(prefixes.lower, legFrame,
                                                "tag_torso", context);
    draw(prefixes.upper, torsoFrame, upperMat);
    if (!prefixes.hasHead) return;

    // head.md3: attached at tag_head from upper.
    draw(prefixes.head, 0,
         upperMat * detail::GetBotModelTagMatrix(prefixes.upper, torsoFrame,
                                                 "tag_head", context));
    if (!prefixes.hasWeapon) return;

    // weapon.md3: attached at tag_weapon from upper.
    draw(prefixes.weapon, 0,
         upperMat * detail::GetBotModelTagMatrix(prefixes.upper, torsoFrame,
                                                 "tag_weapon", context));
}

}  // namespace sdl3cpp::services::impl
