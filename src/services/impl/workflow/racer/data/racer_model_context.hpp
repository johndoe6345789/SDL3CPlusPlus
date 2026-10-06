#pragma once

#include "services/interfaces/workflow/racer/data/racer_big_endian.hpp"
#include "services/interfaces/workflow/racer/data/racer_model.hpp"

#include <glm/glm.hpp>

#include <array>


namespace sdl3cpp::services::impl::racer_model_detail {

/// Shared state while one model item is walked.
struct ModelWalk {
    explicit ModelWalk(const std::vector<std::uint8_t>& data)
        : reader(data) {}

    RacerBigEndianReader reader;
    RacerModel model;
    RacerModelScope scope = RacerModelScope::Everything;
    bool insideLod = false;   ///< the walk is below the first LOD

    /// Nodes may be shared (instanced) under several parents, so they
    /// are not de-duplicated; this budget stops a malformed cycle.
    int nodeBudget = 200000;

    /// A pointer is an offset into the item; 0 means none.
    bool IsPointer(std::uint32_t offset, std::size_t bytes) const {
        return offset != 0 && offset % 4 == 0 && reader.Has(offset, bytes);
    }
};

// Node kinds, from the flags word at +0x00 of every node.
constexpr std::uint32_t kMeshGroup = 0x3064;
constexpr std::uint32_t kBasic = 0x5064;
constexpr std::uint32_t kSelector = 0x5065;
constexpr std::uint32_t kLodSelector = 0x5066;
constexpr std::uint32_t kTransformed = 0xD064;
constexpr std::uint32_t kTransformedPivot = 0xD065;
constexpr std::uint32_t kTransformedComputed = 0xD066;

constexpr int kVertexSlots = 64;  // the F3DEX2 vertex cache

/// One mesh's display list being replayed into a batch.
struct MeshCursor {
    ModelWalk& walk;
    std::uint32_t vertices;
    std::int32_t vertexCount;
    glm::mat4 transform;
    RacerModelBatch* batch;
    std::array<std::int32_t, kVertexSlots> slots;
};

/// gSPVertex: loads `n` vertices from an item offset into cache slots.
void LoadVertices(MeshCursor& cursor, std::uint32_t command);

/// gSP1Triangle / one half of gSP2Triangles, by cache slot.
void Triangle(MeshCursor& cursor, std::uint8_t a, std::uint8_t b,
              std::uint8_t c);

void WalkNode(ModelWalk& walk, std::uint32_t offset,
              const glm::mat4& parent, int depth);

void AppendMesh(ModelWalk& walk, std::uint32_t offset,
                const glm::mat4& transform);

/// Appends a mesh's collision triangles (its separate s16 vertex list:
/// triangles, quads, or strips per its primitive type) to the model.
void AppendCollision(ModelWalk& walk, std::uint32_t mesh,
                     const glm::mat4& transform);

RacerMaterialRef ReadMaterial(const ModelWalk& walk, std::uint32_t offset);

}  // namespace sdl3cpp::services::impl::racer_model_detail
