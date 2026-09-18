#include "services/interfaces/workflow/fs2024/world/fs2024_world_open_step.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world_gpu.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

/// Where this machine's Steam copy keeps FS2024's packages; used when no
/// install root reaches the step (FS2024_INSTALL_DIR unset).
constexpr const char* kDefaultInstallRoot = "D:/Games/Official/Steam";

}  // namespace

WorkflowFs2024WorldOpenStep::WorkflowFs2024WorldOpenStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024WorldOpenStep::GetPluginId() const {
    return "fs2024.world.open";
}

void WorkflowFs2024WorldOpenStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    if (state_->world) return;
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) throw std::runtime_error("fs2024.world.open: no GPU device");

    auto world = std::make_shared<Fs2024World>();
    std::string root = Fs2024StringOr(step, "install_root", "");
    if (root.empty()) root = kDefaultInstallRoot;
    world->paths = ResolveFs2024InstallPaths(root);
    const double lat = Fs2024NumberOr(step, "spawn_lat", 51.5007f);
    const double lon = Fs2024NumberOr(step, "spawn_lon", -0.1246f);
    world->origin = MakeFs2024GeoOrigin(lat, lon);
    Fs2024EngineOfLatLon(world->origin, lat, lon, world->spawnX,
                         world->spawnZ);
    world->spawnHeading = Fs2024NumberOr(step, "spawn_heading", 0.f);
    world->dem = std::make_unique<Fs2024DemSampler>(world->paths.cglRoot);
    world->classes = std::make_unique<Fs2024ClassSampler>(world->paths.cglRoot);
    world->buildings =
        std::make_unique<sdl3cpp::fs2024::BldLibrary>(world->paths.cglRoot);
    world->vectors =
        std::make_unique<sdl3cpp::fs2024::VecLibrary>(world->paths.cglRoot);
    if (!world->paths.landmarkLibrary.empty()) {
        BucketFs2024Landmarks(*world, sdl3cpp::fs2024::BuildLandmarkIndex(
                                          world->paths.landmarkLibrary,
                                          world->paths.landmarkScenery));
    }

    const int climate =
        static_cast<int>(Fs2024NumberOr(step, "climate", 2.f));
    UploadFs2024GroundMaterials(device, *world, climate);
    UploadFs2024RoadTexture(device, *world);
    PublishFs2024BuildingKit(device, *world, context);

    state_->tileSize = world->origin.TileSize();
    if (logger_) {
        logger_->Info("fs2024.world.open: " + world->paths.cglRoot +
                      ", origin tile (" + std::to_string(world->origin.tileX) +
                      ", " + std::to_string(world->origin.tileY) +
                      "), tiles " + std::to_string(state_->tileSize) +
                      " m, landmarks in " +
                      std::to_string(world->landmarks.size()) + " tiles");
    }
    state_->world = std::move(world);
}

}  // namespace sdl3cpp::services::impl
