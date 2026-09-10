#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// Copies every step parameter straight into a JSON object (Number/
/// String/Bool only — no hardcoded defaults, everything comes from the
/// workflow definition), exactly as spotlight.setup always has.
nlohmann::json CopySpotlightParameters(const WorkflowStepDefinition& step);

/// If any of `spotlight[xKey/yKey/zKey]` is present, combines them
/// (missing components default to 0.0) into a `spotlight[arrayKey]` array.
/// Used for spotlight.setup's color/offset/rotation/position/direction
/// component groups, whose per-axis keys don't share one naming scheme
/// (e.g. "color_r"/"color_g"/"color_b" vs "pos_x"/"pos_y"/"pos_z").
void CombineSpotlightVec3(nlohmann::json& spotlight, const char* xKey,
                          const char* yKey, const char* zKey,
                          const char* arrayKey);

}  // namespace sdl3cpp::services::impl
