#include "services/interfaces/workflow/quake3/q3_bot_model_render_internal.hpp"

namespace sdl3cpp::services::impl::bot_model_detail {

glm::mat4 TagMatrix(const nlohmann::json& tag) {
    const auto& ax = tag["axis"];
    const auto& o  = tag["origin"];
    auto axis      = [&](int i) {
        return glm::vec4(ax[i][0].get<float>(), ax[i][1].get<float>(),
                              ax[i][2].get<float>(), 0.0f);
    };
    return glm::mat4(axis(0), axis(1), axis(2),
                     glm::vec4(o[0].get<float>(), o[1].get<float>(),
                               o[2].get<float>(), 1.0f));
}

glm::mat4 GetBotModelTagMatrix(const std::string& prefix, int frame,
                               const std::string& tagName,
                               WorkflowContext& context) {
    const auto* tagsJson =
        context.TryGet<nlohmann::json>("q3.md3." + prefix + "_tags");
    if (!tagsJson || !tagsJson->is_array() || (int)tagsJson->size() <= frame) {
        return glm::mat4(1.0f);
    }
    const auto& frameObj = (*tagsJson)[(size_t)frame];
    if (!frameObj.contains(tagName)) return glm::mat4(1.0f);
    return TagMatrix(frameObj[tagName]);
}

}  // namespace sdl3cpp::services::impl::bot_model_detail
