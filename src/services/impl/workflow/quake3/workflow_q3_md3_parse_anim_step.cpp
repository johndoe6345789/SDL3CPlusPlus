#include "services/interfaces/workflow/quake3/workflow_q3_md3_parse_anim_step.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_source.hpp"
#include "services/interfaces/workflow/quake3/q3_anim_cfg.hpp"
#include "services/interfaces/workflow/quake3/q3_pk3_reader.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

WorkflowQ3Md3ParseAnimStep::WorkflowQ3Md3ParseAnimStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3Md3ParseAnimStep::GetPluginId() const {
    return "q3.md3.parse_anim";
}

void WorkflowQ3Md3ParseAnimStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    const Q3Md3StepParameters params = ReadQ3Md3StepParameters(step);
    if (params.anim.empty()) {
        return;
    }

    auto* source =
        context.Get<Q3Md3Source*>(Q3Md3SourceKey(params.prefix), nullptr);
    if (!source) {
        return;
    }

    const auto animBytes = q3::ReadPk3Entry(source->pk3Path, params.anim);
    if (animBytes.empty()) {
        return;
    }

    const auto clips = q3::ParseAnimCfg(std::string(
        reinterpret_cast<const char*>(animBytes.data()), animBytes.size()));

    nlohmann::json animJson = nlohmann::json::array();
    for (const auto& clip : clips) {
        animJson.push_back({{"first", clip.firstFrame},
                            {"num", clip.numFrames},
                            {"loop", clip.loopingFrames},
                            {"fps", clip.fps}});
    }
    context.Set("q3.md3." + params.prefix + "_anim", animJson);

    if (logger_) {
        logger_->Trace("WorkflowQ3Md3ParseAnimStep", "Execute",
                       "prefix=" + params.prefix +
                           ", clips=" + std::to_string(clips.size()),
                       "Parsed MD3 animation.cfg");
    }
}

}  // namespace sdl3cpp::services::impl
