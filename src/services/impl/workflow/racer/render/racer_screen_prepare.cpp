#include "services/interfaces/workflow/racer/render/racer_screen_step.hpp"

#include "services/interfaces/workflow/racer/flow/racer_flow_text.hpp"
#include "services/interfaces/workflow/racer/racer_step_params.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowRacerScreenStep::WorkflowRacerScreenStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

WorkflowRacerScreenStep::~WorkflowRacerScreenStep() {
    DestroyRacerPanel(title_);
    DestroyRacerPanel(hangar_);
    DestroyRacerPanel(text_);
}

std::string WorkflowRacerScreenStep::GetPluginId() const {
    return "racer.screen.draw";
}

void WorkflowRacerScreenStep::Prepare(const WorkflowStepDefinition& step,
                                      SDL_GPUDevice* device,
                                      SDL_GPUCommandBuffer* cmd,
                                      float aspect) {
    prepared_ = true;
    const std::filesystem::path images =
        std::filesystem::path(
            RacerStringParam(step, "racer_dir", "RACER_DIR", "")) /
        "data" / "images";
    for (auto [panel, file] : {std::pair{&title_, "splash.TGA"},
                               std::pair{&hangar_, "podhangar_backdrop.TGA"}}) {
        if (!CreateRacerImagePanel(device, images / file, *panel)) continue;
        const float imageAspect =
            static_cast<float>(panel->width) / panel->height;
        UploadRacerPanelRect(*panel, cmd, RacerCoverRect(imageAspect, aspect));
    }
    if (CreateRacerTextPanel(device, kRacerScreenWidth, kRacerScreenHeight,
                             text_)) {
        UploadRacerPanelRect(text_, cmd, RacerScreenRect{});
    } else if (logger_) {
        logger_->Warn("racer.screen.draw: text panel creation failed");
    }
}

}  // namespace sdl3cpp::services::impl
