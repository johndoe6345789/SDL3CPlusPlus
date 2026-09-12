#include "services/interfaces/workflow/gta5/gta5_instance_batch.hpp"

#include "services/interfaces/workflow/gta5/gta5_frustum.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

void Collect(const Gta5StreamState& state, const Gta5Frustum& frustum,
             const glm::vec3& camera, const Gta5CullOptions& options,
             std::vector<const Gta5Instance*>& visible) {
    visible.clear();
    for (const auto& entry : state.resident) {
        for (const Gta5Instance& instance : entry.second.instances) {
            if (!instance.geometry || !instance.geometry->usable) continue;
            if (!((options.kinds >> instance.proxy) & 1u)) continue;
            const bool shown =
                Gta5InstanceVisible(frustum, instance, camera,
                                    options.sizeRatio, options.lodScale) ||
                (options.collision && instance.body &&
                 Gta5InstanceVisible(frustum, instance, camera, 0.f, 1e6f));
            if (shown) visible.push_back(&instance);
        }
    }
    for (const Gta5Vehicle& car : state.vehicles) {
        if (car.instance.geometry) visible.push_back(&car.instance);
        for (const Gta5Instance& wheel : car.wheels) {
            if (wheel.geometry) visible.push_back(&wheel);
        }
    }
    for (const Gta5Instance& part : state.character) visible.push_back(&part);
    for (const Gta5Instance& car : state.traffic) {
        if (car.geometry) visible.push_back(&car);
    }
}
}  // namespace

void BuildGta5InstanceBatch(const Gta5StreamState& state,
                            const glm::mat4& viewProj,
                            const glm::vec3& camera,
                            const Gta5CullOptions& options,
                            Gta5InstanceBatch& batch) {
    Collect(state, MakeGta5Frustum(viewProj), camera, options, batch.visible);
    std::sort(batch.visible.begin(), batch.visible.end(),
              [](auto* a, auto* b) { return a->geometry < b->geometry; });
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
    // Draws, not groups, are sorted: by pipeline, block, texture, tint.
    batch.items.clear();
    for (const Gta5DrawGroup& group : batch.groups) {
        for (const Gta5SubMesh& sub : group.geometry->subMeshes) {
            if (sub.indexCount) batch.items.push_back({&sub, group.first,
                                                       group.count});
        }
    }
    std::sort(batch.items.begin(), batch.items.end(),
              [](const Gta5DrawItem& a, const Gta5DrawItem& b) {
                  if (a.sub->DrawKind() != b.sub->DrawKind()) {
                      return a.sub->DrawKind() < b.sub->DrawKind();
                  }
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
