#include "services/interfaces/workflow/rendering/bsp_texture_groups_context.hpp"

namespace sdl3cpp::services::impl {

std::shared_ptr<std::map<int, TextureGroup>> GetOrCreateBspTextureGroups(
    WorkflowContext& context) {
    auto groups = GetBspTextureGroups(context);
    if (!groups) {
        groups = std::make_shared<std::map<int, TextureGroup>>();
        context.Set(kBspTextureGroupsKey, groups);
    }
    return groups;
}

std::shared_ptr<std::map<int, TextureGroup>> GetBspTextureGroups(
    WorkflowContext& context) {
    return context.Get<std::shared_ptr<std::map<int, TextureGroup>>>(
        kBspTextureGroupsKey, nullptr);
}

}  // namespace sdl3cpp::services::impl
