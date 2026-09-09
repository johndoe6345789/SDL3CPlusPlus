// Movement against the real q3dm1 collision, not a flat plane. The
// pmove chain and the input layer both measure symmetric on their own,
// so a direction that feels reluctant in game has to be coming from the
// map's geometry.
//
// Needs the game data; skipped when QUAKE3_PAK0 is not set.

#include "services/interfaces/workflow/quake3/workflow_q3_pm_accelerate_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_friction_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_ground_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_step_slide_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_constants.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_build_collision_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_load_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_parse_spawn_step.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdlib>
#include <memory>

namespace impl = sdl3cpp::services::impl;
namespace q3 = sdl3cpp::q3;
using sdl3cpp::services::WorkflowContext;
using sdl3cpp::services::WorkflowParameterValue;
using sdl3cpp::services::WorkflowStepDefinition;

namespace {

const char* Pk3Path() { return std::getenv("QUAKE3_PAK0"); }

WorkflowStepDefinition LoadStep(const char* pk3, const char* map) {
    WorkflowStepDefinition step;
    auto& path = step.parameters["pk3_path"];
    path.type = WorkflowParameterValue::Type::String;
    path.stringValue = pk3;
    auto& name = step.parameters["map_name"];
    name.type = WorkflowParameterValue::Type::String;
    name.stringValue = map;
    auto& scale = step.parameters["scale"];
    scale.type = WorkflowParameterValue::Type::Number;
    scale.numberValue = 1.0 / 32.0;
    return step;
}

struct World {
    btDefaultCollisionConfiguration config;
    btCollisionDispatcher dispatcher{&config};
    btDbvtBroadphase broadphase;
    btSequentialImpulseConstraintSolver solver;
    btDiscreteDynamicsWorld world{&dispatcher, &broadphase, &solver, &config};
};

}  // namespace

class MapMovement : public ::testing::Test {
protected:
    void SetUp() override {
        if (!Pk3Path()) GTEST_SKIP() << "QUAKE3_PAK0 not set";
        context_.Set<btDiscreteDynamicsWorld*>("physics_world",
                                               &world_.world);
        const auto step = LoadStep(Pk3Path(), "q3dm1");
        impl::WorkflowBspLoadStep(nullptr).Execute(step, context_);
        impl::WorkflowBspBuildCollisionStep(nullptr).Execute(step, context_);
        impl::WorkflowBspParseSpawnStep(nullptr).Execute(step, context_);
    }

    // Travel from the map's spawn point, facing `yaw`, pressing forward.
    float Travel(float yaw) {
        const auto* spawn = context_.TryGet<nlohmann::json>("bsp.spawn");
        impl::Q3PlayerState ps;
        if (spawn) {
            ps.origin = glm::vec3(spawn->value("x", 0.0f),
                                  spawn->value("y", 2.0f),
                                  spawn->value("z", 0.0f));
        }
        WorkflowContext run = context_;
        run.Set("q3.ps", ps);
        run.Set<double>("frame.delta_time", 1.0 / 125.0);
        run.Set<float>("q3.player_yaw", yaw);
        run.Set<float>("input.move_forward", 1.0f);
        run.Set<float>("input.move_right", 0.0f);

        impl::WorkflowQ3PmGroundStep ground(nullptr);
        impl::WorkflowQ3PmFrictionStep friction(nullptr);
        impl::WorkflowQ3PmAccelerateStep accelerate(nullptr);
        impl::WorkflowQ3PmStepSlideStep slide(nullptr);

        const WorkflowStepDefinition step;
        // Let the player settle onto the floor before measuring.
        for (int i = 0; i < 60; ++i) {
            ground.Execute(step, run);
            slide.Execute(step, run);
        }
        // Path length, not displacement: sliding along a wall in a
        // corner curves back toward the start, so displacement makes
        // working movement look broken.
        auto previous =
            run.Get<impl::Q3PlayerState>("q3.ps", impl::Q3PlayerState{}).origin;
        float path = 0.0f;
        for (int i = 0; i < 125; ++i) {
            ground.Execute(step, run);
            friction.Execute(step, run);
            accelerate.Execute(step, run);
            slide.Execute(step, run);
            const auto now = run.Get<impl::Q3PlayerState>(
                                    "q3.ps", impl::Q3PlayerState{}).origin;
            const glm::vec3 delta = now - previous;
            path += std::sqrt(delta.x * delta.x + delta.z * delta.z);
            previous = now;
        }
        return path;
    }

    World world_;
    WorkflowContext context_;
};

TEST_F(MapMovement, CollisionGeometryLoaded) {
    EXPECT_NE(context_.Get<btRigidBody*>("bsp_collision_body", nullptr),
              nullptr);
}

TEST_F(MapMovement, ThePlayerCanMoveAtAllFromTheSpawn) {
    EXPECT_GT(Travel(0.0f), 0.5f);
}

TEST_F(MapMovement, MostFacingsMoveFreelyAndTheBestHitsQuakeSpeed) {
    // The spawn sits in a corner, so a few facings run head on into a
    // wall and correctly keep only the wish component along it. What
    // matters is that the open directions reach Quake's speed and that
    // the blocked ones are the minority, rather than everything being
    // dragged down by geometry.
    float best = 0.0f;
    int free = 0;
    for (int i = 0; i < 16; ++i) {
        const float yaw = static_cast<float>(i) * 0.3927f;
        const float travelled = Travel(yaw);
        best = std::max(best, travelled);
        std::printf("  yaw %5.2f rad -> %6.3f m\n", yaw, travelled);
    }
    for (int i = 0; i < 16; ++i) {
        if (Travel(static_cast<float>(i) * 0.3927f) > best * 0.33f) ++free;
    }
    std::printf("  best %.3f m, %d of 16 facings unobstructed\n", best, free);

    EXPECT_GT(best, q3::kMaxSpeed * 0.9f)
        << "an open direction should reach Quake's running speed";
    EXPECT_GE(free, 11) << "too many facings are being blocked";
}
