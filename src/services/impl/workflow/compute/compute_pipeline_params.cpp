#include "services/interfaces/workflow/compute/compute_pipeline_params.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {

namespace {

int IntParameter(const WorkflowStepParameterResolver& params,
                 const WorkflowStepDefinition& step, const char* key,
                 int fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isNumber = p && p->type == WorkflowParameterValue::Type::Number;
    return isNumber ? static_cast<int>(p->numberValue) : fallback;
}

}  // namespace

ComputePipelineCreateParams ReadComputePipelineCreateParams(
    const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    ComputePipelineCreateParams p;
    auto& counts = p.counts;
    counts.numSamplers =
        IntParameter(params, step, "num_samplers", counts.numSamplers);
    counts.numReadWriteStorageBuffers = IntParameter(
        params, step, "num_storage_buffers", counts.numReadWriteStorageBuffers);
    counts.numUniformBuffers =
        IntParameter(params, step, "num_uniforms", counts.numUniformBuffers);
    counts.threadcountX =
        IntParameter(params, step, "threadcount_x", counts.threadcountX);
    counts.threadcountY =
        IntParameter(params, step, "threadcount_y", counts.threadcountY);
    counts.threadcountZ =
        IntParameter(params, step, "threadcount_z", counts.threadcountZ);

    const auto* pipelineKeyParam = params.FindParameter(step, "pipeline_key");
    if (pipelineKeyParam &&
        pipelineKeyParam->type == WorkflowParameterValue::Type::String) {
        p.pipelineKey = pipelineKeyParam->stringValue;
    }
    return p;
}

}  // namespace sdl3cpp::services::impl
