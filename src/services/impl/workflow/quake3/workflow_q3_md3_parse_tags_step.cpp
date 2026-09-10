#include "services/interfaces/workflow/quake3/workflow_q3_md3_parse_tags_step.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_source.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_format.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_tags.hpp"

namespace sdl3cpp::services::impl {

WorkflowQ3Md3ParseTagsStep::WorkflowQ3Md3ParseTagsStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3Md3ParseTagsStep::GetPluginId() const {
    return "q3.md3.parse_tags";
}

void WorkflowQ3Md3ParseTagsStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    const Q3Md3StepParameters params = ReadQ3Md3StepParameters(step);
    auto* source =
        context.Get<Q3Md3Source*>(Q3Md3SourceKey(params.prefix), nullptr);
    if (!source || source->bytes.size() < sizeof(q3::Md3Header)) {
        return;
    }

    const auto& header =
        *reinterpret_cast<const q3::Md3Header*>(source->bytes.data());
    if (header.numFrames <= 0 || header.numSurfaces <= 0) {
        return;
    }

    const std::string base = "q3.md3." + params.prefix + "_";
    context.Set(base + "tags", q3::BuildMd3TagsJson(source->bytes));
    context.Set<int>(base + "num_frames", header.numFrames);
    context.Set<int>(base + "num_surfs", header.numSurfaces);
    context.Set<int>(base + "num_tags", header.numTags);

    if (logger_) {
        logger_->Trace("WorkflowQ3Md3ParseTagsStep", "Execute",
                       "prefix=" + params.prefix +
                           ", frames=" + std::to_string(header.numFrames) +
                           ", tags=" + std::to_string(header.numTags),
                       "Published MD3 tag transforms");
    }
}

}  // namespace sdl3cpp::services::impl
