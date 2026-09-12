#include "services/interfaces/workflow/gta5/gta5_effects.hpp"
#include "services/interfaces/workflow/gta5/gta5_effects_spawn.hpp"

#include <btBulletDynamicsCommon.h>

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

/// A mark's place in the world now, from where it sits on what it hit.
/// Held in that thing's own space, it is carried wherever the thing
/// goes: shoot a car and the hole drives off with it.
void Carry(Gta5Particle& p) {
    const btTransform& at = p.on->getWorldTransform();
    const btVector3 point =
        at * btVector3(p.local.x, p.local.y, p.local.z);
    const btVector3 way = at.getBasis() * btVector3(
        p.localNormal.x, p.localNormal.y, p.localNormal.z);
    p.at = glm::vec3(point.x(), point.y(), point.z());
    p.normal = glm::vec3(way.x(), way.y(), way.z());
}

}  // namespace

void UpdateGta5Effects(Gta5Effects& effects, float dt) {
    for (Gta5Particle& p : effects.particles) {
        p.age += dt;
        if (p.on) Carry(p);
        if (p.length > 0.f || p.normal != glm::vec3(0.f)) continue;
        p.velocity.y -= p.gravity * dt;
        p.velocity -= p.velocity * std::min(1.f, p.drag * dt);
        p.at += p.velocity * dt;
    }
    const auto spent = std::remove_if(
        effects.particles.begin(), effects.particles.end(),
        [](const Gta5Particle& p) { return p.age >= p.life; });
    effects.particles.erase(spent, effects.particles.end());
}

void DropGta5MarksOn(Gta5Effects& effects, const btCollisionObject* body) {
    if (!body) return;
    const auto gone = std::remove_if(
        effects.particles.begin(), effects.particles.end(),
        [body](const Gta5Particle& p) { return p.on == body; });
    effects.particles.erase(gone, effects.particles.end());
}

}  // namespace sdl3cpp::services::impl
