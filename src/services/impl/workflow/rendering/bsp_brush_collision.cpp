#include "services/interfaces/workflow/rendering/bsp_brush_collision.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {

std::vector<btVector3> ComputeBrushVertices(const BspBrushSide* sides,
                                            int numSides,
                                            const BspPlane* allPlanes,
                                            float scale) {
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

BspBrushCollisionShapes BuildBspBrushCollisionShapes(
    const std::vector<uint8_t>& bspData, float scale) {
    BspBrushCollisionShapes out;

    auto* lumps =
        reinterpret_cast<const BspLump*>(bspData.data() + sizeof(BspHeader));

    const auto& texLump = lumps[LUMP_TEXTURES];
    int numTextures = texLump.length / static_cast<int>(sizeof(BspTexture));
    auto* bspTextures =
        reinterpret_cast<const BspTexture*>(bspData.data() + texLump.offset);

    const auto& brushLump = lumps[LUMP_BRUSHES];
    int numBrushes = brushLump.length / static_cast<int>(sizeof(BspBrush));
    auto* bspBrushes =
        reinterpret_cast<const BspBrush*>(bspData.data() + brushLump.offset);

    const auto& brushSideLump = lumps[LUMP_BRUSHSIDES];
    int numBrushSides =
        brushSideLump.length / static_cast<int>(sizeof(BspBrushSide));
    auto* bspBrushSides = reinterpret_cast<const BspBrushSide*>(
        bspData.data() + brushSideLump.offset);

    const auto& planeLump = lumps[LUMP_PLANES];
    auto* bspPlanes =
        reinterpret_cast<const BspPlane*>(bspData.data() + planeLump.offset);

    out.solid = new btCompoundShape();
    // Player-clip brushes go in their own body so that only pmove sees
    // them, matching Quake's MASK_PLAYERSOLID / MASK_SHOT split.
    auto* clipCompound = new btCompoundShape();

    for (int b = 0; b < numBrushes; ++b) {
        const auto& brush = bspBrushes[b];

        bool playerClip = false;
        if (brush.shaderIndex >= 0 && brush.shaderIndex < numTextures) {
            const auto& tex = bspTextures[brush.shaderIndex];
            playerClip = (tex.contents & CONTENTS_PLAYERCLIP) != 0;
            if (!(tex.contents & CONTENTS_SOLID) && !playerClip) {
                ++out.skippedBrushes;
                continue;
            }
            // SURF_NODRAW is a rendering property. Player-clip brushes
            // are always nodraw, so skipping on it dropped exactly the
            // geometry that makes curved surfaces walkable.
            if ((tex.flags & SURF_NODRAW) && !playerClip) {
                ++out.skippedBrushes;
                continue;
            }
        } else {
            ++out.skippedBrushes;
            continue;
        }

        if (brush.firstSide < 0 ||
            brush.firstSide + brush.numSides > numBrushSides) {
            ++out.skippedBrushes;
            continue;
        }

        auto hullVerts = ComputeBrushVertices(
            &bspBrushSides[brush.firstSide], brush.numSides, bspPlanes,
            scale);

        if (hullVerts.size() < 4) {
            ++out.skippedBrushes;
            continue;
        }

        auto* convex = new btConvexHullShape();
        for (const auto& v : hullVerts) {
            convex->addPoint(v, false);
        }
        convex->recalcLocalAabb();
        convex->setMargin(0.01f);
        // Keep the brush's own face planes so pmove traces can report a
        // face normal rather than Bullet's edge separation direction.
        convex->initializePolyhedralFeatures();

        btTransform childTransform;
        childTransform.setIdentity();
        if (playerClip) {
            clipCompound->addChildShape(childTransform, convex);
            ++out.clipBrushes;
        } else {
            out.solid->addChildShape(childTransform, convex);
            ++out.solidBrushes;
        }
    }

    if (out.clipBrushes > 0) {
        out.clip = clipCompound;
    } else {
        delete clipCompound;
    }

    return out;
}

void RemoveBspCollisionBody(btDiscreteDynamicsWorld* world,
                            btRigidBody*& body) {
    if (!body) return;

    world->removeRigidBody(body);
    auto* shape = body->getCollisionShape();
    auto* ms = body->getMotionState();
    delete body;
    delete ms;
    if (shape) {
        if (auto* compound = dynamic_cast<btCompoundShape*>(shape)) {
            for (int i = compound->getNumChildShapes() - 1; i >= 0; --i) {
                delete compound->getChildShape(i);
            }
        }
        delete shape;
    }
    body = nullptr;
}

btRigidBody* AddStaticCollisionBody(btDiscreteDynamicsWorld* world,
                                    btCollisionShape* shape, float friction) {
    btTransform startTransform;
    startTransform.setIdentity();
    auto* motionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, motionState,
                                                    shape);
    rbInfo.m_friction = friction;
    auto* body = new btRigidBody(rbInfo);
    body->setCollisionFlags(body->getCollisionFlags() |
                            btCollisionObject::CF_STATIC_OBJECT);
    world->addRigidBody(body);
    return body;
}

btRigidBody* AddFilteredStaticCollisionBody(btDiscreteDynamicsWorld* world,
                                            btCollisionShape* shape,
                                            int group, int mask) {
    btTransform startTransform;
    startTransform.setIdentity();
    auto* motionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, motionState,
                                                    shape);
    auto* body = new btRigidBody(rbInfo);
    body->setCollisionFlags(body->getCollisionFlags() |
                            btCollisionObject::CF_STATIC_OBJECT);
    world->addRigidBody(body, group, mask);
    return body;
}

}  // namespace sdl3cpp::services::impl
