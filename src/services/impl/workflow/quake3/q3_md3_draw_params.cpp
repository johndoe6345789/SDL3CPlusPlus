#include "services/interfaces/workflow/quake3/q3_md3_draw_params.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {

Md3DrawParams ReadMd3DrawParams(const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    auto getStr = [&](const char* k, const std::string& def) {
        const auto* p = params.FindParameter(step, k);
        return (p && p->type == WorkflowParameterValue::Type::String)
                   ? p->stringValue
                   : def;
    };
    auto getNum = [&](const char* k, float def) {
        const auto* p = params.FindParameter(step, k);
        return (p && p->type == WorkflowParameterValue::Type::Number)
                   ? static_cast<float>(p->numberValue)
                   : def;
    };
    auto getBool = [&](const char* k, bool def) {
        const auto* p = params.FindParameter(step, k);
        return (p && p->type == WorkflowParameterValue::Type::Bool)
                   ? p->boolValue
                   : def;
    };

    Md3DrawParams out;
    out.prefix    = getStr("prefix", out.prefix);
    out.posKey    = getStr("pos_key", out.posKey);
    out.yawKey    = getStr("yaw_key", out.yawKey);
    out.frameKey  = getStr("frame_key", out.frameKey);
    out.fps       = getNum("fps", out.fps);
    out.animFirst = static_cast<int>(getNum("anim_first", 0.0f));
    out.animCount = static_cast<int>(getNum("anim_count", 0.0f));
    out.viewmodel = getBool("viewmodel", out.viewmodel);
    out.vmRight   = getNum("vm_right", out.vmRight);
    out.vmDown    = getNum("vm_down", out.vmDown);
    out.vmFwd     = getNum("vm_fwd", out.vmFwd);
    return out;
}

}  // namespace sdl3cpp::services::impl
