#include "services/interfaces/workflow/gta5/weapon/gta5_weapon_step.hpp"
#include "services/interfaces/workflow/gta5/effects/gta5_effects_spawn.hpp"

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>

#include <algorithm>

namespace sdl3cpp::services::impl {

void WorkflowGta5WeaponStep::Throw(WorkflowContext& context,
                                   const Gta5Weapon& weapon) {
    const auto view =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    const glm::vec3 ahead = -glm::vec3(view[0][2], view[1][2], view[2][2]);
    const glm::vec3 eye =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f));
    const auto ps = context.Get<Q3PlayerState>("q3.ps", Q3PlayerState{});
    const glm::vec3 muzzle = Gta5MuzzlePoint(
        eye, ahead, ps.origin, ps.maxs.y,
        context.GetBool("gta5.third_person", false));
    Gta5Projectile shot;
    shot.rocket = weapon.kind == Gta5WeaponKind::Rocket;
    // Clear of the player, so it is not set off in their own face.
    shot.at = muzzle + ahead * 0.6f;
    // A rocket flies flat and fast; a grenade is lobbed and falls.
    shot.velocity = ahead * weapon.speed +
                    (shot.rocket ? glm::vec3(0.f) : glm::vec3(0.f, 3.f, 0.f));
    shot.fuse = shot.rocket ? weapon.range / std::max(weapon.speed, 1.f) : 3.f;
    shot.damage = weapon.damage;
    shot.radius = shot.rocket ? 8.f : 6.f;
    flying_.push_back(shot);
    SpawnGta5Muzzle(*Gta5EffectsOf(context), shot.at, ahead);
}

void WorkflowGta5WeaponStep::Advance(WorkflowContext& context, float dt) {
    if (flying_.empty()) return;
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    const Gta5EffectsPtr effects = Gta5EffectsOf(context);
    const btCollisionObject* me = Gta5PlayerBody(context);
    std::vector<Gta5Projectile> still;
    for (Gta5Projectile& shot : flying_) {
        const glm::vec3 was = shot.at;
        if (!shot.rocket) shot.velocity.y -= 9.81f * dt;
        shot.at += shot.velocity * dt;
        shot.fuse -= dt;
        // The smoke it leaves behind, and what it runs into.
        Gta5Particle trail;
        trail.at = shot.at;
        trail.colour = glm::vec3(0.6f);
        trail.size = 0.25f;
        trail.growth = 1.2f;
        trail.life = 0.9f;
        trail.fade = 0.5f;
        trail.sprite = kGta5Smoke;
        effects->particles.push_back(trail);
        Gta5Shot into;
        if (Gta5ShootRay(world, was, shot.at, me, into)) {
            Explode(context, into.at, shot.radius, shot.damage);
            continue;
        }
        if (shot.fuse <= 0.f) {
            Explode(context, shot.at, shot.radius, shot.damage);
            continue;
        }
        still.push_back(shot);
    }
    flying_ = still;
}

}  // namespace sdl3cpp::services::impl
