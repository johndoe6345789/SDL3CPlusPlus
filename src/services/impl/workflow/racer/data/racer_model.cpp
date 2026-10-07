#include "services/interfaces/workflow/racer/data/racer_model.hpp"

#include "racer_model_context.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

using racer_model_detail::ModelWalk;

namespace {

constexpr std::size_t kSkySlot = 2;  // a 'Trak' header's skybox node

}  // namespace

bool RacerMaterialRef::operator==(const RacerMaterialRef& other) const {
    return textureIndex == other.textureIndex && format == other.format &&
           width == other.width && height == other.height &&
           doubleWidth == other.doubleWidth &&
           doubleHeight == other.doubleHeight;
}

RacerModel ParseRacerModel(const std::vector<std::uint8_t>& data,
                           RacerModelScope scope) {
    ModelWalk walk(data);
    walk.scope = scope;
    if (!walk.reader.Has(0, 8)) return {};
    walk.model.tag = walk.reader.U32(0);

    // The header's node list: one word per entry until 0xFFFFFFFF. A
    // negative word is a plain integer (a placeholder slot), zero is an
    // empty slot, and anything else points at a node.
    const glm::mat4 identity(1.f);
    std::vector<std::uint32_t> walked;
    for (std::size_t at = 4, slot = 0; walk.reader.Has(at, 4);
         at += 4, ++slot) {
        const std::uint32_t word = walk.reader.U32(at);
        if (word == 0xFFFFFFFFu) break;
        if (static_cast<std::int32_t>(word) <= 0) continue;
        // Some headers list a node twice; walking it twice would draw
        // every one of its triangles twice.
        if (std::find(walked.begin(), walked.end(), word) != walked.end()) {
            continue;
        }
        const bool sky = slot == kSkySlot;
        if ((scope == RacerModelScope::SkyOnly && !sky) ||
            (scope == RacerModelScope::TrackWithoutSky && sky)) {
            continue;
        }
        walked.push_back(word);
        racer_model_detail::WalkNode(walk, word, identity, 0);
    }
    if (scope == RacerModelScope::PodParts) {
        racer_model_detail::LayoutPodParts(walk);
    }
    walk.model.valid = walk.model.meshCount > 0;
    return walk.model;
}

}  // namespace sdl3cpp::services::impl
