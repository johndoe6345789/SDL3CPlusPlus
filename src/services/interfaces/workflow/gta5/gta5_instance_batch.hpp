#pragma once

#include "services/interfaces/workflow/gta5/gta5_geometry.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

struct Gta5StreamState;

/// Every visible copy of one archetype: matrices [first, first + count).
struct Gta5DrawGroup {
    const Gta5Geometry* geometry{nullptr};
    std::uint32_t first{0};
    std::uint32_t count{0};
};

/// One instanced draw: a group's copies of one of its materials.
struct Gta5DrawItem {
    const Gta5SubMesh* sub{nullptr};
    std::uint32_t first{0};
    std::uint32_t count{0};
};

/// This frame's visible instances, grouped by archetype, and the storage
/// buffer their model matrices are uploaded to for the instanced draw.
struct Gta5InstanceBatch {
    std::vector<const Gta5Instance*> visible;
    std::vector<std::array<float, 16>> matrices;
    std::vector<Gta5DrawGroup> groups;
    /// Every group's materials, sorted by texture so each is bound once.
    std::vector<Gta5DrawItem> items;
    SDL_GPUBuffer* buffer{nullptr};
    SDL_GPUTransferBuffer* transfer{nullptr};
    std::uint32_t capacity{0};  // in matrices
};

/// Matches gta5_model.vert's VertexUniforms, std140: 144 bytes.
struct Gta5InstancedUniforms {
    float viewProj[16];
    float shadowVP[16];
    float cameraPos[4];
};

/// How gta5.tiles.cull picks what to draw.
struct Gta5CullOptions {
    float sizeRatio{0.003f};  // cull what is smaller than this of its range
    float lodScale{1.f};      // above 1, finer models are kept further out
    /// Also draw every instance in view with a physics body, whatever LOD
    /// would show: F2. A collider nothing visible accounts for -- a lump
    /// in the road -- then appears on top of what is normally drawn.
    bool collision{false};
    /// Which instances: a bit per Gta5ProxyKind. The view draws scenery.
    std::uint32_t kinds{1u};
};

/// Cull resident instances to the view, group the rest by archetype and
/// list their draws. Vehicles are always in: few, and the camera follows
/// one.
void BuildGta5InstanceBatch(const Gta5StreamState& state,
                            const glm::mat4& viewProj,
                            const glm::vec3& camera,
                            const Gta5CullOptions& options,
                            Gta5InstanceBatch& batch);

/// Upload the batch's matrices on a command buffer of its own, submitted
/// before the frame's, growing the buffers as needed.
bool UploadGta5InstanceBatch(SDL_GPUDevice* device, Gta5InstanceBatch& batch);

}  // namespace sdl3cpp::services::impl
