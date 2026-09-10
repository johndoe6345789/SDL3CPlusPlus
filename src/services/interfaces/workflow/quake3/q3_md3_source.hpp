#pragma once

#include "services/interfaces/workflow/quake3/q3_skin_file.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <stdint.h>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief One MD3 model's raw bytes and resolved parameters.
 *
 * `q3.md3.read` fills this in and publishes a pointer to it under
 * "q3.md3.{prefix}_source"; the parse and upload steps consume it.  Keeping the
 * bytes in one place means the pk3 is opened once per model rather than once
 * per step.
 */
struct Q3Md3Source {
    std::string prefix;
    std::string path;
    std::string skin;
    std::string anim;
    std::string pk3Path;
    std::vector<uint8_t> bytes;
    q3::SkinMap skinMap;
};

/// Reads the `prefix`, `path`, `skin` and `anim` string parameters of a step.
struct Q3Md3StepParameters {
    std::string prefix;
    std::string path;
    std::string skin;
    std::string anim;
};

Q3Md3StepParameters ReadQ3Md3StepParameters(const WorkflowStepDefinition& step);

/// Context key holding the Q3Md3Source* for @p prefix.
std::string Q3Md3SourceKey(const std::string& prefix);

}  // namespace sdl3cpp::services::impl
