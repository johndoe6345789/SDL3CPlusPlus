#include "services/interfaces/workflow/rendering/viewmodel_draw.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

std::optional<ViewmodelMesh> TryGetViewmodelMesh(
    const WorkflowContext& context, const std::string& meshName,
    const std::shared_ptr<ILogger>& logger) {
    ViewmodelMesh mesh;
    mesh.vertexBuffer =
        context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_vb", nullptr);
    mesh.indexBuffer =
        context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_ib", nullptr);
    const auto* meta = context.TryGet<nlohmann::json>("plane_" + meshName);

    if (!mesh.vertexBuffer || !mesh.indexBuffer || !meta) {
        if (logger) {
            logger->Warn("draw.viewmodel: Mesh '" + meshName + "' not found");
        }
        return std::nullopt;
    }
    mesh.indexCount = (*meta)["index_count"];
    return mesh;
}

}  // namespace sdl3cpp::services::impl
