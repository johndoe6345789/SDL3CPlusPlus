#include "services/interfaces/workflow/quake3/q3_md3_draw_params.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>

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

int ResolveMd3AnimFrame(const WorkflowContext& context,
                        const Md3DrawParams& params, int numFrames) {
    int frame = 0;
    if (!params.frameKey.empty()) {
        frame = context.Get<int>(params.frameKey, 0);
    } else {
        const double elapsed = context.GetDouble("frame.elapsed", 0.0);
        const int totalFrame = static_cast<int>(elapsed * params.fps);
        frame = (params.animCount > 0)
                    ? params.animFirst + (totalFrame % params.animCount)
                    : (totalFrame % numFrames);
    }
    return std::max(0, std::min(frame, numFrames - 1));
}

glm::mat4 BuildMd3ModelMatrix(const WorkflowContext& context,
                              const Md3DrawParams& params,
                              const glm::mat4& view,
                              const glm::vec3& camPos) {
    if (params.viewmodel) {
        const glm::vec3 right(view[0][0], view[1][0], view[2][0]);
        const glm::vec3 up(view[0][1], view[1][1], view[2][1]);
        const glm::vec3 forward(-view[0][2], -view[1][2], -view[2][2]);
        const glm::vec3 pos = camPos + right * params.vmRight +
                              up * params.vmDown + forward * params.vmFwd;

        glm::mat4 orient(1.0f);
        orient[0] = glm::vec4(forward, 0.0f);
        orient[1] = glm::vec4(-right, 0.0f);
        orient[2] = glm::vec4(up, 0.0f);
        return glm::translate(glm::mat4(1.0f), pos) * orient;
    }

    glm::vec3 pos(0.0f);
    if (!params.posKey.empty()) {
        const auto* pv = context.TryGet<nlohmann::json>(params.posKey);
        if (pv && pv->is_array() && pv->size() >= 3) {
            pos = glm::vec3((*pv)[0].get<float>(), (*pv)[1].get<float>(),
                            (*pv)[2].get<float>());
        }
    }
    float yaw = 0.0f;
    if (!params.yawKey.empty()) {
        yaw = context.Get<float>(params.yawKey, 0.0f);
    }
    // Same Z-up remap as the viewmodel: a bare yaw rotation about world Y
    // leaves a Z-up model lying on its side.
    const glm::vec3 f(-std::sin(yaw), 0.0f, -std::cos(yaw));
    const glm::vec3 u(0.0f, 1.0f, 0.0f);
    const glm::vec3 l = glm::cross(u, f);
    glm::mat4 orient(1.0f);
    orient[0] = glm::vec4(f, 0.0f);
    orient[1] = glm::vec4(l, 0.0f);
    orient[2] = glm::vec4(u, 0.0f);
    return glm::translate(glm::mat4(1.0f), pos) * orient;
}

}  // namespace sdl3cpp::services::impl
