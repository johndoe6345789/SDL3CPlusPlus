#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_pause_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_play_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_resume_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_seek_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_set_looping_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_set_volume_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_stop_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterAudioSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                       std::shared_ptr<ILogger> logger,
                       std::shared_ptr<IAudioService> audioSvc) {
    if (!registry) return 0;

    int count = 0;

    // ── Audio (service-dependent, nullptr until wired) ─────────
    registry->RegisterStep(
        std::make_shared<WorkflowAudioPauseStep>(audioSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowAudioPlayStep>(audioSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowAudioResumeStep>(audioSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowAudioSeekStep>(audioSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowAudioSetLoopingStep>(audioSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowAudioSetVolumeStep>(audioSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowAudioStopStep>(audioSvc, logger));
    count += 7;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
