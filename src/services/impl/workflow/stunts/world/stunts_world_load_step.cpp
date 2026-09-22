#include "services/interfaces/workflow/stunts/world/stunts_world_load_step.hpp"

#include "services/interfaces/workflow/stunts/data/stunts_car_body.hpp"
#include "services/interfaces/workflow/stunts/stunts_step_params.hpp"
#include "services/interfaces/workflow/stunts/world/stunts_start_line.hpp"
#include "services/interfaces/workflow/stunts/world/stunts_world_upload.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr const char* kDefaultTable =
    "packages/stunts/assets/stunts_tiles.json";
constexpr const char* kDefaultMaterials =
    "packages/stunts/assets/stunts_materials.json";
constexpr const char* kDefaultTrack = "DEFAULT.TRK";
constexpr const char* kDefaultCar = "CARANSX.RES";

void Publish(WorkflowContext& context, const StuntsWorldState& state) {
    context.Set("stunts.car_name", state.car.name);
    context.Set("stunts.car_spec", state.car.spec);
    context.Set("stunts.start_pos", state.start.position);
    context.Set("stunts.start_heading", state.start.heading);
    context.Set("stunts.road_tiles", state.roadTiles);
}

}  // namespace

WorkflowStuntsWorldLoadStep::WorkflowStuntsWorldLoadStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<StuntsWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowStuntsWorldLoadStep::GetPluginId() const {
    return "stunts.world.load";
}

void WorkflowStuntsWorldLoadStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    if (state_->loaded) return;
    const std::string dir =
        StuntsStringOr(step, "game_dir", "STUNTS_DIR", "");
    state_->install = OpenStuntsInstall(dir);
    if (!state_->install.valid) {
        if (logger_) {
            logger_->Error("stunts.world.load: no track files in '" + dir +
                           "'; set STUNTS_DIR to a Stunts directory");
        }
        return;
    }

    state_->params.tileSize = StuntsNumberOr(step, "tile_size", 24.f);
    state_->params.roadWidth = StuntsNumberOr(step, "road_width", 8.f);
    state_->table = LoadStuntsTileTable(
        StuntsStringOr(step, "tile_table", nullptr, kDefaultTable));
    const std::string trackName =
        StuntsStringOr(step, "track", "STUNTS_TRACK", kDefaultTrack);
    state_->track =
        LoadStuntsTrack(FindStuntsFile(state_->install, trackName));
    if (!state_->track.loaded || !state_->table.loaded) {
        if (logger_) {
            logger_->Error("stunts.world.load: could not read track '" +
                           trackName + "' or its tile table");
        }
        return;
    }

    const std::string carName =
        StuntsStringOr(step, "car", "STUNTS_CAR", kDefaultCar);
    const std::string carPath = FindStuntsFile(state_->install, carName);
    state_->car = LoadStuntsCar(carPath, carName);
    state_->materials = LoadStuntsMaterialTable(
        StuntsStringOr(step, "materials", nullptr, kDefaultMaterials));
    const std::string bodyName = StuntsCarBodyFileFor(carName);
    state_->carBody =
        LoadStuntsCarBody(FindStuntsFile(state_->install, bodyName));
    state_->start =
        FindStuntsStartLine(state_->track, state_->table, state_->params);
    context.Set("stunts.track_name", trackName);

    UploadStuntsWorld(context, *state_, logger_);
    Publish(context, *state_);
    if (logger_) {
        logger_->Info("stunts.world.load: " + trackName + ", " +
                      std::to_string(state_->roadTiles) + " road tiles, car " +
                      state_->car.name);
    }
}

}  // namespace sdl3cpp::services::impl
