#include "services/interfaces/workflow/gta5/gta5_instance_batch.hpp"

#include "services/interfaces/workflow/gta5/gta5_frustum.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

void Collect(const Gta5StreamState& state, const Gta5Frustum& frustum,
             const glm::vec3& camera, float sizeRatio,
             std::vector<const Gta5Instance*>& visible) {
    visible.clear();
    for (const auto& entry : state.resident) {
        for (const Gta5Instance& instance : entry.second.instances) {
            if (!instance.geometry || !instance.geometry->usable) continue;
            if (Gta5InstanceVisible(frustum, instance, camera, sizeRatio)) {
                visible.push_back(&instance);
            }
        }
    }
    for (const Gta5Vehicle& car : state.vehicles) {
        if (car.instance.geometry) visible.push_back(&car.instance);
        if (!car.hasWheels) continue;
        for (const Gta5Instance& wheel : car.wheels) {
            if (wheel.geometry) visible.push_back(&wheel);
        }
    }
}

}  // namespace

void BuildGta5InstanceBatch(const Gta5StreamState& state,
                            const glm::mat4& viewProj,
                            const glm::vec3& camera, float sizeRatio,
                            Gta5InstanceBatch& batch) {
    Collect(state, MakeGta5Frustum(viewProj), camera, sizeRatio,
            batch.visible);
    // Copies of one archetype side by side: each run is one group.
    std::sort(batch.visible.begin(), batch.visible.end(),
              [](const Gta5Instance* a, const Gta5Instance* b) {
                  return a->geometry < b->geometry;
              });
    batch.matrices.clear();
    batch.groups.clear();
    for (const Gta5Instance* instance : batch.visible) {
        if (batch.groups.empty() ||
            batch.groups.back().geometry != instance->geometry) {
            batch.groups.push_back(
                {instance->geometry,
                 static_cast<std::uint32_t>(batch.matrices.size()), 0});
        }
        batch.matrices.push_back(instance->modelMatrix);
        ++batch.groups.back().count;
    }
    // One archetype's materials mostly use different textures, so it is
    // the draws, not the groups, that are sorted: by arena block, whose
    // buffers are then bound once, then texture, then tint.
    batch.items.clear();
    for (const Gta5DrawGroup& group : batch.groups) {
        for (const Gta5SubMesh& sub : group.geometry->subMeshes) {
            if (sub.indexCount) batch.items.push_back({&sub, group.first,
                                                       group.count});
        }
    }
    std::sort(batch.items.begin(), batch.items.end(),
              [](const Gta5DrawItem& a, const Gta5DrawItem& b) {
                  if (a.sub->slot.block != b.sub->slot.block) {
                      return a.sub->slot.block < b.sub->slot.block;
                  }
                  if (a.sub->texture != b.sub->texture) {
                      return a.sub->texture < b.sub->texture;
                  }
                  return a.sub->surface < b.sub->surface;
              });
}

}  // namespace sdl3cpp::services::impl
