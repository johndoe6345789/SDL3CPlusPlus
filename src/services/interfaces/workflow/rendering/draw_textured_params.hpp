#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// draw.textured's resolved parameters, each with the original defaults.
struct DrawTexturedParams {
    std::string meshName    = "plane";
    std::string textureName = "texture";
    std::string facing;
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f;
    float scale     = 1.0f;
    float roughness = 0.8f;
    float metallic  = 0.0f;
};

DrawTexturedParams ReadDrawTexturedParams(const WorkflowStepDefinition& step);

}  // namespace sdl3cpp::services::impl
