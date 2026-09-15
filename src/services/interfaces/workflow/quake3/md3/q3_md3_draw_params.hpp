#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// q3.md3.draw's tunable parameters, each with the original defaults.
struct Md3DrawParams {
    std::string prefix   = "model";
    std::string posKey   = "";
    std::string yawKey   = "";
    std::string frameKey = "";
    float fps            = 15.0f;
    int animFirst        = 0;
    int animCount        = 0;
    bool viewmodel       = false;
    float vmRight        = 0.35f;
    float vmDown         = -0.3f;
    float vmFwd          = 0.5f;
};

/// Parses `step`'s JSON parameters into an Md3DrawParams, applying the
/// struct's defaults for any parameter that is absent or the wrong type.
Md3DrawParams ReadMd3DrawParams(const WorkflowStepDefinition& step);

}  // namespace sdl3cpp::services::impl
