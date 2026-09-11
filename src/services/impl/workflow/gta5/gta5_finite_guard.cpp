#include "services/interfaces/workflow/gta5/gta5_finite_guard.hpp"

#include "services/interfaces/workflow/gta5/gta5_player_pin.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"

#include <cmath>
#include <cstdio>
#include <string>

namespace sdl3cpp::services::impl {
namespace {

bool Finite(float x, float y, float z) {
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}

std::string Text(float x, float y, float z) {
    char out[64];
    std::snprintf(out, sizeof(out), "(%.2f, %.2f, %.2f)", x, y, z);
    return out;
}

std::string Text(const btVector3& v) { return Text(v.x(), v.y(), v.z()); }

btVector3 g_lastAt(0.f, 0.f, 0.f);
btVector3 g_lastVelocity(0.f, 0.f, 0.f);
bool g_known = false;
bool g_reported = false;

}  // namespace

void GuardGta5PlayerFinite(Gta5StreamState& state, WorkflowContext& context,
                           btRigidBody* player,
                           const std::shared_ptr<ILogger>& logger) {
    if (!player) return;
    const btVector3 at = player->getWorldTransform().getOrigin();
    const btVector3 velocity = player->getLinearVelocity();
    const auto* ps = context.TryGet<Q3PlayerState>("q3.ps");
    const bool psFinite =
        !ps || (Finite(ps->origin.x, ps->origin.y, ps->origin.z) &&
                Finite(ps->velocity.x, ps->velocity.y, ps->velocity.z));
    if (Finite(at.x(), at.y(), at.z()) &&
        Finite(velocity.x(), velocity.y(), velocity.z()) && psFinite) {
        g_lastAt = at;
        g_lastVelocity = velocity;
        g_known = true;
        return;
    }
    if (!g_reported && logger) {
        g_reported = true;
        std::string line = "gta5.guard: player went non-finite: at " +
                           Text(at) + " v " + Text(velocity);
        if (ps) {
            const glm::vec3 o = ps->origin, v = ps->velocity;
            line += ", q3.ps " + Text(o.x, o.y, o.z) + " v " +
                    Text(v.x, v.y, v.z);
        }
        line += "; last finite " + Text(g_lastAt) + " v " +
                Text(g_lastVelocity) + "; seated " +
                std::to_string(state.seated);
        for (const Gta5Vehicle& car : state.vehicles) {
            if (!car.chassis) continue;
            const btRigidBody& body = *car.chassis;
            line += "; car " + Text(body.getWorldTransform().getOrigin()) +
                    " v " + Text(body.getLinearVelocity());
        }
        logger->Warn(line);
    }
    if (!g_known) return;
    PinGta5Player(context, player,
                  glm::vec3(g_lastAt.x(), g_lastAt.y() + 1.f, g_lastAt.z()));
}

}  // namespace sdl3cpp::services::impl
