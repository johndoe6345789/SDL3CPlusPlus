#pragma once

#include "services/interfaces/workflow/compute/compute_pipeline_create.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// compute.pipeline.create's resource-count parameters plus the context key
/// it publishes the pipeline under.
struct ComputePipelineCreateParams {
    ComputePipelineResourceCounts counts;
    std::string pipelineKey = "compute_pipeline";
};

ComputePipelineCreateParams ReadComputePipelineCreateParams(
    const WorkflowStepDefinition& step);

}  // namespace sdl3cpp::services::impl
