#include "services/interfaces/workflow/gta5/gta5_weapon_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>

#include <algorithm>

namespace sdl3cpp::services::impl {

bool Gta5ShootRay(btDiscreteDynamicsWorld* world, const glm::vec3& from,
                  const glm::vec3& to, const btCollisionObject* skip,
                  Gta5Shot& shot) {
    if (!world) return false;
    const btVector3 start(from.x, from.y, from.z);
    const btVector3 end(to.x, to.y, to.z);
    btCollisionWorld::ClosestRayResultCallback hit(start, end);
    world->rayTest(start, end, hit);
    if (!hit.hasHit() || hit.m_collisionObject == skip) return false;
    shot.at = glm::vec3(hit.m_hitPointWorld.x(), hit.m_hitPointWorld.y(),
                        hit.m_hitPointWorld.z());
    shot.normal = glm::vec3(hit.m_hitNormalWorld.x(), hit.m_hitNormalWorld.y(),
                            hit.m_hitNormalWorld.z());
    shot.object = hit.m_collisionObject;
    return true;
}

void WorkflowGta5WeaponStep::Fire(WorkflowContext& context,
                                  const Gta5Weapon& weapon) {
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    const Gta5EffectsPtr effects = Gta5EffectsOf(context);
    // Along the view, from a little ahead of the camera: the shot goes
    // where the middle of the screen looks.
    const auto view =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    const glm::vec3 ahead =
        -glm::vec3(view[0][2], view[1][2], view[2][2]);
    const glm::vec3 eye =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f));
    const glm::vec3 muzzle = eye + ahead * 0.6f;
    SpawnGta5Muzzle(*effects, muzzle, ahead);
    std::uniform_real_distribution<float> scatter(-weapon.spread,
                                                  weapon.spread);
    const btCollisionObject* me = Gta5PlayerBody(context);
    for (int pellet = 0; pellet < std::max(1, weapon.pellets); ++pellet) {
        glm::vec3 way = ahead;
        if (weapon.spread > 0.f) {
            way = glm::normalize(ahead + glm::vec3(scatter(rng_),
                                                   scatter(rng_),
                                                   scatter(rng_)));
        }
        const glm::vec3 far = muzzle + way * weapon.range;
        Gta5Shot shot;
        const bool struck = Gta5ShootRay(world, muzzle, far, me, shot);
        SpawnGta5Tracer(*effects, muzzle, struck ? shot.at : far);
        if (!struck) continue;
        SpawnGta5Impact(*effects, shot.at, shot.normal);
        // A car takes the hit where it was struck, and rocks with it.
        for (Gta5Vehicle& car : state_->vehicles) {
            if (!car.chassis || car.chassis != shot.object) continue;
            car.chassis->activate(true);
            const btVector3 push(way.x * weapon.damage * 2.f,
                                 way.y * weapon.damage * 2.f,
                                 way.z * weapon.damage * 2.f);
            car.chassis->applyImpulse(
                push, btVector3(shot.at.x, shot.at.y, shot.at.z) -
                          car.chassis->getCenterOfMassPosition());
        }
    }
    context.Set<int>("gta5.weapon.shot",
                     context.Get<int>("gta5.weapon.shot", 0) + 1);
    context.Set<std::string>("gta5.weapon.shot_id", weapon.id);
}

}  // namespace sdl3cpp::services::impl
