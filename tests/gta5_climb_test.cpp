#include "services/interfaces/workflow/gta5/player/gta5_climb.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace {

using sdl3cpp::services::impl::FindGta5Ledge;
using sdl3cpp::services::impl::Gta5Ledge;

/// A floor at y = 0 and boxes on it, as static bodies.
struct World {
    btDefaultCollisionConfiguration config;
    btCollisionDispatcher dispatcher{&config};
    btDbvtBroadphase broadphase;
    btSequentialImpulseConstraintSolver solver;
    btDiscreteDynamicsWorld world{&dispatcher, &broadphase, &solver,
                                  &config};
    std::vector<std::unique_ptr<btCollisionShape>> shapes;
    std::vector<std::unique_ptr<btCollisionObject>> bodies;

    World() {
        Add({50, 0.5f, 50}, {0, -0.5f, 0});
    }
    ~World() {
        for (auto& body : bodies) {
            world.removeCollisionObject(body.get());
        }
    }
    void Add(btVector3 half, btVector3 at) {
        shapes.push_back(std::make_unique<btBoxShape>(half));
        auto body = std::make_unique<btCollisionObject>();
        body->setCollisionShape(shapes.back().get());
        body->getWorldTransform().setOrigin(at);
        world.addCollisionObject(body.get());
        bodies.push_back(std::move(body));
    }
    /// A wall `high` tall and 1 m deep, its near face `ahead` down -z.
    void Wall(float high, float ahead) {
        Add({2, high / 2, 0.5f}, {0, high / 2, -(ahead + 0.5f)});
    }
    bool Find(Gta5Ledge& out) {
        world.updateAabbs();
        return FindGta5Ledge(&world, nullptr, {0, 0, 0}, {0, 0, -1}, 1.8f,
                             out);
    }
};

TEST(Gta5ClimbTest, ClimbsAKneeHighPlanter) {
    World w;
    w.Wall(0.9f, 0.5f);
    Gta5Ledge ledge;
    ASSERT_TRUE(w.Find(ledge));
    EXPECT_NEAR(ledge.rise, 0.9f, 0.01f);
    EXPECT_LT(ledge.top.z, -0.5f);  // past the face, on top
}

TEST(Gta5ClimbTest, LeavesKerbsToTheStepUp) {
    World w;
    w.Wall(0.25f, 0.5f);
    Gta5Ledge ledge;
    EXPECT_FALSE(w.Find(ledge));
}

TEST(Gta5ClimbTest, WillNotScaleABuilding) {
    World w;
    w.Wall(3.0f, 0.5f);
    Gta5Ledge ledge;
    EXPECT_FALSE(w.Find(ledge));
}

TEST(Gta5ClimbTest, NeedsRoomOnTop) {
    World w;
    w.Wall(1.0f, 0.5f);
    w.Add({2, 0.1f, 2}, {0, 2.0f, -1.0f});  // a slab 1 m over the top
    Gta5Ledge ledge;
    EXPECT_FALSE(w.Find(ledge));
}

TEST(Gta5ClimbTest, NothingAheadIsAJump) {
    World w;
    Gta5Ledge ledge;
    EXPECT_FALSE(w.Find(ledge));
}

}  // namespace
