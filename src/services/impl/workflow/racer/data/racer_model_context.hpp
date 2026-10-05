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
    /// Nodes may be shared (instanced) under several parents, so they
    /// are not de-duplicated; this budget stops a malformed cycle.
    int nodeBudget = 200000;

    /// A pointer is an offset into the item; 0 means none.
    bool IsPointer(std::uint32_t offset, std::size_t bytes) const {
        return offset != 0 && offset % 4 == 0 && reader.Has(offset, bytes);
    }
};

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

RacerMaterialRef ReadMaterial(const ModelWalk& walk, std::uint32_t offset);

}  // namespace sdl3cpp::services::impl::racer_model_detail
