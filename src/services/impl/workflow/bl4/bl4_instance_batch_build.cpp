#include "services/interfaces/workflow/bl4/bl4_instance_batch.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void BuildBl4InstanceBatch(Bl4InstanceBatch& batch) {
    batch.matrices.clear();
    batch.items.clear();
    // By archetype: every copy of one geometry lands in one run of the
    // storage buffer, which its submeshes then draw instanced.
    std::sort(batch.visible.begin(), batch.visible.end(),
              [](const Bl4Instance* a, const Bl4Instance* b) {
                  return a->geometry < b->geometry;
              });

    std::size_t start = 0;
    while (start < batch.visible.size()) {
        const Bl4Geometry* geometry = batch.visible[start]->geometry;
        std::size_t end = start;
        while (end < batch.visible.size() && batch.visible[end]->geometry == geometry) {
            batch.matrices.push_back(batch.visible[end]->modelMatrix);
            ++end;
        }
        for (const Bl4SubMesh& sub : geometry->subMeshes) {
            if (sub.indexCount == 0) continue;
            batch.items.push_back({&sub, static_cast<std::uint32_t>(start),
                                   static_cast<std::uint32_t>(end - start)});
        }
        start = end;
    }

    std::sort(batch.items.begin(), batch.items.end(),
              [](const Bl4DrawItem& a, const Bl4DrawItem& b) {
                  return a.sub->texture < b.sub->texture;
              });
}

}  // namespace sdl3cpp::services::impl
