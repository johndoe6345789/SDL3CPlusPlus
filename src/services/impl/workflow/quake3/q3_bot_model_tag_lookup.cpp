#include "services/interfaces/workflow/quake3/q3_bot_model_render_internal.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_tags.hpp"

namespace sdl3cpp::services::impl::bot_model_detail {

glm::mat4 GetBotModelTagMatrix(const std::string& prefix, int frame,
                               const std::string& tagName,
                               WorkflowContext& context) {
    const auto* tagsJson =
        context.TryGet<nlohmann::json>("q3.md3." + prefix + "_tags");
    if (!tagsJson || !tagsJson->is_array() ||
        static_cast<int>(tagsJson->size()) <= frame) {
        return glm::mat4(1.0f);
    }
    const auto& frameObj = (*tagsJson)[static_cast<size_t>(frame)];
    if (!frameObj.contains(tagName)) return glm::mat4(1.0f);
    return q3::TagMatrix(frameObj[tagName]);
}

}  // namespace sdl3cpp::services::impl::bot_model_detail
