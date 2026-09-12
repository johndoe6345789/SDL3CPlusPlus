#include "services/interfaces/workflow/gta5/gta5_traffic_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_drawable_geometry.hpp"
#include "services/interfaces/workflow/gta5/gta5_mesh_transform.hpp"
#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

bool WorkflowGta5TrafficStep::Load(const WorkflowStepDefinition& step,
                                   WorkflowContext& context) {
    // The map has to be indexed before a car can be read out of it.
    if (!state_->assets) return false;
    tried_ = true;
    traffic_.cycle = Gta5NumberOr(step, "cycle", traffic_.cycle);
    traffic_.amber = Gta5NumberOr(step, "amber", traffic_.amber);
    traffic_.near = Gta5NumberOr(step, "near", traffic_.near);
    traffic_.far = Gta5NumberOr(step, "far", traffic_.far);
    traffic_.want = Gta5ParameterOrInt(step, "cars", traffic_.want);
    const std::string dir = Gta5ParameterOr(step, "roads_dir", "");
    if (!dir.empty()) LoadGta5Roads(dir, roads_);
    if (!roads_.loaded && logger_) {
        logger_->Warn("gta5.traffic: no road network in '" + dir +
                      "'; nothing to drive on");
    }
    const std::string model = Gta5ParameterOr(step, "model", "taxi_hi");
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    Gta5Geometry& body = state_->geometryCache["vehicle:" + model];
    if (!body.usable && device) {
        Gta5MeshData mesh =
            ReadGta5ArchetypeMesh(*state_, Gta5Hash(model),
                                  glm::vec3(0.55f, 0.57f, 0.60f));
        // Vehicles are modelled facing +y, which is the engine's -z.
        TurnGta5MeshAround(mesh);
        // With collision: the player's own car shares this cache
        // entry and measures its chassis box off these vertices, so
        // loading it without would leave whichever came second blind.
        UploadGta5MeshGeometry(*state_, mesh, device, body, true, model,
                               logger_);
    }
    traffic_.body = body.usable ? &body : nullptr;
    // A car's origin sits about axle height, not on the road, so the
    // mesh has to be lifted by however far it hangs below that or it
    // is drawn buried to the sills in the tarmac.
    float low = 0.f;
    for (std::size_t i = 1; i < body.collisionVertices.size(); i += 3) {
        low = std::min(low, static_cast<float>(body.collisionVertices[i]));
    }
    traffic_.lift = -low;
    if (logger_) {
        logger_->Info("gta5.traffic: " + std::to_string(roads_.nodes.size()) +
                      " road nodes, driving " + model + ", lifted " +
                      std::to_string(traffic_.lift) + " m" +
                      (traffic_.body ? "" : " (no mesh)"));
    }
    return traffic_.body != nullptr;
}

}  // namespace sdl3cpp::services::impl
