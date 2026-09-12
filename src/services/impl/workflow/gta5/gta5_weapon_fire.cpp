#include "services/interfaces/workflow/gta5/gta5_weapon_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_decal_stick.hpp"
#include "services/interfaces/workflow/gta5/gta5_effects_spawn.hpp"

#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
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
    const glm::vec3 aim = eye + ahead * 0.6f;
    const auto ps = context.Get<Q3PlayerState>("q3.ps", Q3PlayerState{});
    const glm::vec3 muzzle = Gta5MuzzlePoint(
        eye, ahead, ps.origin, ps.maxs.y,
        context.GetBool("gta5.third_person", false));
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
        const glm::vec3 far = aim + way * weapon.range;
        Gta5Shot shot;
        const bool struck = Gta5ShootRay(world, aim, far, me, shot);
        SpawnGta5Tracer(*effects, muzzle, struck ? shot.at : far);
        if (!struck) continue;
        // A car takes the hit where it was struck, and rocks with it.
        Gta5Stuck stuck;
        glm::vec3 at = shot.at, facing = shot.normal;
        for (Gta5Vehicle& car : state_->vehicles) {
            if (!car.chassis || car.chassis != shot.object) continue;
            // On the panel it actually hit, not on the box round the
            // car, and held there so it goes where the car goes.
            if (Gta5StickToCar(car, aim, far, stuck)) {
                at = Gta5StuckAt(stuck);
                const btVector3 out = car.chassis->getWorldTransform()
                                          .getBasis() *
                                      btVector3(stuck.normal.x,
                                                stuck.normal.y,
                                                stuck.normal.z);
                facing = glm::vec3(out.x(), out.y(), out.z());
            }
            car.chassis->activate(true);
            const btVector3 push(way.x * weapon.damage * 2.f,
                                 way.y * weapon.damage * 2.f,
                                 way.z * weapon.damage * 2.f);
            car.chassis->applyImpulse(
                push, btVector3(at.x, at.y, at.z) -
                          car.chassis->getCenterOfMassPosition());
        }
        SpawnGta5Impact(*effects, at, facing, stuck);
    }
    context.Set<int>("gta5.weapon.shot",
                     context.Get<int>("gta5.weapon.shot", 0) + 1);
    context.Set<std::string>("gta5.weapon.shot_id", weapon.id);
}

}  // namespace sdl3cpp::services::impl
