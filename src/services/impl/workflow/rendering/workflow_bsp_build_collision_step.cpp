#include "services/interfaces/workflow/rendering/workflow_bsp_build_collision_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

// Compute convex hull vertices from plane intersections of a brush.
static std::vector<btVector3> ComputeBrushVertices(
    const BspBrushSide* sides, int numSides,
    const BspPlane* allPlanes, float scale)
{
    std::vector<btVector3> verts;
    if (numSides < 3) return verts;

    struct Plane { btVector3 n; float d; };
    std::vector<Plane> planes(numSides);
    for (int i = 0; i < numSides; ++i) {
        const auto& p = allPlanes[sides[i].planeIndex];
        planes[i].n = btVector3(p.normal[0], p.normal[2], -p.normal[1]);
        planes[i].d = p.dist * scale;
    }

    for (int i = 0; i < numSides - 2; ++i) {
        for (int j = i + 1; j < numSides - 1; ++j) {
            for (int k = j + 1; k < numSides; ++k) {
                const auto& p1 = planes[i];
                const auto& p2 = planes[j];
                const auto& p3 = planes[k];

                btVector3 cross23 = p2.n.cross(p3.n);
                float denom = p1.n.dot(cross23);
                if (std::fabs(denom) < 1e-6f) continue;

                btVector3 point = (cross23 * p1.d +
                                   p3.n.cross(p1.n) * p2.d +
                                   p1.n.cross(p2.n) * p3.d) / denom;

                bool inside = true;
                for (int m = 0; m < numSides; ++m) {
                    if (m == i || m == j || m == k) continue;
                    float dist = planes[m].n.dot(point) - planes[m].d;
                    if (dist > 0.1f * scale) {
                        inside = false;
                        break;
                    }
                }

                if (inside) {
                    verts.push_back(point);
                }
            }
        }
    }

    return verts;
}

WorkflowBspBuildCollisionStep::WorkflowBspBuildCollisionStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspBuildCollisionStep::GetPluginId() const {
    return "bsp.build_collision";
}

void WorkflowBspBuildCollisionStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    auto bspDataPtr = context.Get<std::shared_ptr<std::vector<uint8_t>>>("bsp_raw_data", nullptr);
    if (!bspDataPtr) throw std::runtime_error("bsp.build_collision: bsp_raw_data not in context");

    auto bspConfig = context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    float scale = bspConfig.value("scale", 1.0f / 32.0f);
    std::string map_name = bspConfig.value("map_name", std::string("q3dm17"));

    auto* world = context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!world) {
        if (logger_) logger_->Info("bsp.build_collision: No physics world — skipping");
        return;
    }

    // Remove previous BSP collision body to prevent ghost geometry on map reload
    auto* prevBody = context.Get<btRigidBody*>("bsp_collision_body", nullptr);
    if (prevBody) {
        world->removeRigidBody(prevBody);
        auto* shape = prevBody->getCollisionShape();
        auto* ms = prevBody->getMotionState();
        delete prevBody;
        delete ms;
        if (shape) {
            auto* compound = dynamic_cast<btCompoundShape*>(shape);
            if (compound) {
                for (int i = compound->getNumChildShapes() - 1; i >= 0; --i) {
                    delete compound->getChildShape(i);
                }
            }
            delete shape;
        }
        context.Set<btRigidBody*>("bsp_collision_body", nullptr);
    }

    const auto& bspData = *bspDataPtr;
    auto* lumps = reinterpret_cast<const BspLump*>(bspData.data() + sizeof(BspHeader));

    const auto& texLump = lumps[LUMP_TEXTURES];
    int numTextures = texLump.length / static_cast<int>(sizeof(BspTexture));
    auto* bspTextures = reinterpret_cast<const BspTexture*>(bspData.data() + texLump.offset);

    const auto& brushLump = lumps[LUMP_BRUSHES];
    int numBrushes = brushLump.length / static_cast<int>(sizeof(BspBrush));
    auto* bspBrushes = reinterpret_cast<const BspBrush*>(bspData.data() + brushLump.offset);

    const auto& brushSideLump = lumps[LUMP_BRUSHSIDES];
    int numBrushSides = brushSideLump.length / static_cast<int>(sizeof(BspBrushSide));
    auto* bspBrushSides = reinterpret_cast<const BspBrushSide*>(bspData.data() + brushSideLump.offset);

    const auto& planeLump = lumps[LUMP_PLANES];
    auto* bspPlanes = reinterpret_cast<const BspPlane*>(bspData.data() + planeLump.offset);

    auto* compound = new btCompoundShape();
    int solidBrushes = 0;
    int skippedBrushes = 0;

    for (int b = 0; b < numBrushes; ++b) {
        const auto& brush = bspBrushes[b];

        if (brush.shaderIndex >= 0 && brush.shaderIndex < numTextures) {
            const auto& tex = bspTextures[brush.shaderIndex];
            if (!(tex.contents & CONTENTS_SOLID)) {
                ++skippedBrushes;
                continue;
            }
            if (tex.flags & 0x4000) {
                ++skippedBrushes;
                continue;
            }
        } else {
            ++skippedBrushes;
            continue;
        }

        if (brush.firstSide < 0 || brush.firstSide + brush.numSides > numBrushSides) {
            ++skippedBrushes;
            continue;
        }

        auto hullVerts = ComputeBrushVertices(
            &bspBrushSides[brush.firstSide], brush.numSides,
            bspPlanes, scale);

        if (hullVerts.size() < 4) {
            ++skippedBrushes;
            continue;
        }

        auto* convex = new btConvexHullShape();
        for (const auto& v : hullVerts) {
            convex->addPoint(v, false);
        }
        convex->recalcLocalAabb();
        convex->setMargin(0.01f);

        btTransform childTransform;
        childTransform.setIdentity();
        compound->addChildShape(childTransform, convex);
        ++solidBrushes;
    }

    if (solidBrushes > 0) {
        btTransform startTransform;
        startTransform.setIdentity();
        auto* motionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, motionState, compound);
        rbInfo.m_friction = 1.0f;
        auto* body = new btRigidBody(rbInfo);
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
        world->addRigidBody(body);
        context.Set<btRigidBody*>("bsp_collision_body", body);
    }

    if (logger_) {
        logger_->Info("bsp.build_collision: " + std::to_string(solidBrushes) + " solid brushes, " +
                     std::to_string(skippedBrushes) + " skipped");
    }
}

}  // namespace sdl3cpp::services::impl
