#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {

GeometryPlaneParams ReadGeometryPlaneParams(
    const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;

    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number)
                   ? static_cast<float>(p->numberValue)
                   : def;
    };
    auto getInt = [&](const char* pname, int def) -> int {
        const auto* p = params.FindParameter(step, pname);
        return (p && p->type == WorkflowParameterValue::Type::Number)
                   ? static_cast<int>(p->numberValue)
                   : def;
    };
    auto getStr = [&](const char* pname, const std::string& def) {
        const auto* p = params.FindParameter(step, pname);
        return (p && p->type == WorkflowParameterValue::Type::String)
                   ? p->stringValue
                   : def;
    };

    GeometryPlaneParams result;
    result.width         = getNum("width", 10.0f);
    result.depth         = getNum("depth", 10.0f);
    result.uvScaleX      = getNum("uv_scale_x", 1.0f);
    result.uvScaleY      = getNum("uv_scale_y", 1.0f);
    result.subdivisionsX = getInt("subdivisions_x", 1);
    result.subdivisionsY = getInt("subdivisions_y", 1);
    result.name          = getStr("name", "plane");
    return result;
}

}  // namespace sdl3cpp::services::impl
