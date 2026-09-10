#pragma once

#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace sdl3cpp::services::impl {

/// MD3 prefixes for one bot's model chain, plus which optional links
/// (upper/head/weapon) actually have frames loaded.
struct BotModelPrefixes {
    std::string lower;
    std::string upper;
    std::string head;
    std::string weapon;
    bool hasUpper  = false;
    bool hasHead   = false;
    bool hasWeapon = false;
};

/**
 * @brief Draws one bot's full model chain: lower.md3 (legs) at the bot's
 *        world position, then upper.md3 attached via lower's tag_torso,
 *        then head.md3 via upper's tag_head, then weapon.md3 (optional)
 *        via upper's tag_weapon.
 *
 * A no-op for a bot whose "state" is "dead". Links beyond lower are
 * skipped when their `prefixes` flag says the MD3 has no frames loaded.
 */
void DrawBotModelChain(const nlohmann::json& bot,
                       const BotModelPrefixes& prefixes,
                       const glm::mat4& view, const glm::mat4& proj,
                       const glm::vec3& camPos, const glm::mat4& shadowVP,
                       const rendering::FragmentUniformData& fu,
                       SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                       SDL_GPUTexture* shadowTex, SDL_GPUSampler* shadowSamp,
                       WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
