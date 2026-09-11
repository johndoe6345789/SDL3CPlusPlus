#include "services/interfaces/workflow/gta5/gta5_weapon_step.hpp"
#include "services/interfaces/workflow_context.hpp"
#include <btBulletDynamicsCommon.h>
#include <algorithm>

namespace sdl3cpp::services::impl {

void WorkflowGta5WeaponStep::Explode(WorkflowContext& context,
                                     const glm::vec3& at, float radius,
                                     float damage) {
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    const Gta5EffectsPtr effects = Gta5EffectsOf(context);
    SpawnGta5Explosion(*effects, at, radius);
    // The mark it burns into whatever lies below.
    Gta5Shot ground;
    if (Gta5ShootRay(world, at, at - glm::vec3(0.f, radius, 0.f), nullptr,
                     ground)) {
        SpawnGta5Scorch(*effects, ground.at, ground.normal, radius * 0.9f,
                        120.f);
    }
    // Cars near it are thrown; the player is hurt, armour taking it first.
    for (Gta5Vehicle& car : state_->vehicles) {
        if (!car.chassis) continue;
        const btVector3 to = car.chassis->getCenterOfMassPosition() -
                             btVector3(at.x, at.y, at.z);
        const float away = to.length();
        if (away > radius * 2.f) continue;
        const float share = 1.f - away / (radius * 2.f);
        car.chassis->activate(true);
        car.chassis->applyCentralImpulse(
            (to.normalized() + btVector3(0.f, 1.f, 0.f)) * share * damage *
            30.f);
    }
    const auto hurt = [&](const char* key, float& left, float take) {
        const float had = context.Get<float>(key, left);
        const float now = std::max(0.f, had - take);
        context.Set<float>(key, now);
        left = had - now;
    };
    const glm::vec3 player =
        context.Get<glm::vec3>("q3.player_pos", glm::vec3(0.f));
    const float away = glm::distance(player, at);
    if (away < radius * 2.f) {
        float took = damage * (1.f - away / (radius * 2.f)) * 0.25f;
        float soaked = 0.f;
        hurt("gta5.player.armour", soaked, took);
        hurt("gta5.player.health", soaked, took - soaked);
    }
    context.Set<int>("gta5.weapon.blast",
                     context.Get<int>("gta5.weapon.blast", 0) + 1);
}

}  // namespace sdl3cpp::services::impl
