#include "services/interfaces/workflow/quake3/workflow_q3_movers_init_step.hpp"
#include "services/interfaces/workflow/quake3/q3_mover_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>
#include <string>

namespace {
// BSP entity fields are always JSON strings — helper to safely read as float.
float EntFloat(const nlohmann::json& ent, const char* key, float def) {
    if (!ent.contains(key)) return def;
    const auto& v = ent[key];
    if (v.is_number()) return v.get<float>();
    if (v.is_string()) { try { return std::stof(v.get<std::string>()); } catch (...) {} }
    return def;
}
}

namespace sdl3cpp::services::impl {

WorkflowQ3MoversInitStep::WorkflowQ3MoversInitStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3MoversInitStep::GetPluginId() const { return "q3.movers.init"; }

void WorkflowQ3MoversInitStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    if (context.GetBool("q3.movers_initialized", false)) return;

    const auto* entities = context.TryGet<nlohmann::json>("bsp.entities");
    auto movers = std::make_shared<std::vector<sdl3cpp::q3::Q3Mover>>();

    if (entities && entities->is_array()) {
        int idx = 0;
        for (const auto& ent : *entities) {
            const std::string cls = ent.value("classname", std::string{});
            if (cls != "func_door" && cls != "func_plat") continue;

            // Parse origin
            glm::vec3 origin(0.f);
            if (ent.contains("origin") && ent["origin"].is_string()) {
                const std::string os = ent["origin"].get<std::string>();
                float x = 0.f, y = 0.f, z = 0.f;
                if (std::sscanf(os.c_str(), "%f %f %f", &x, &y, &z) == 3) {
                    // Apply Q3 unit-to-world scale (same 0.03125 factor used by bsp.load)
                    constexpr float kScale = 0.03125f;
                    origin = glm::vec3(x * kScale, y * kScale, z * kScale);
                }
            }

            // Parse angle (degrees, Q3 convention: 0=+X, 90=-Z in XZ plane)
            const float angleDeg = EntFloat(ent, "angle", 0.f);
            const float angleRad = angleDeg * (3.14159265f / 180.f);
            const glm::vec3 moveDir(std::cos(angleRad), 0.f, -std::sin(angleRad));

            // Parse speed and distance — all stored as strings in BSP entity lump
            const float speed    = EntFloat(ent, "speed",    100.f);
            const float distRaw  = EntFloat(ent, "distance", 128.f);
            constexpr float kScale = 0.03125f;
            const float dist     = distRaw * kScale;

            const float wait       = EntFloat(ent, "wait", 2.f);
            const float travelTime = (speed > 0.f) ? dist / (speed * kScale) : 1.f;

            sdl3cpp::q3::Q3Mover m;
            m.id         = cls + "_" + std::to_string(idx++);
            m.classname  = cls;
            m.pos1       = origin;
            m.pos2       = origin + moveDir * dist;
            m.travelTime = travelTime;
            m.waitTime   = wait;
            m.currentPos = origin;
            movers->push_back(m);
        }
    }

    context.Set("q3.movers", movers);
    context.Set("q3.movers_initialized", true);

    if (logger_)
        logger_->Info("q3.movers.init: created " + std::to_string(movers->size()) + " movers");
}

}  // namespace sdl3cpp::services::impl
