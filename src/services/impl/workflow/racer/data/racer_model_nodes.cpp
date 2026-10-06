#include "racer_model_context.hpp"

#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl::racer_model_detail {
namespace {

constexpr int kMaxDepth = 64;
// flags, flags1, flags2, flags3 (s16) + light (s16), flags5, child
// count, child pointer array: 0x1C bytes before the kind's own fields.
constexpr std::uint32_t kHeaderBytes = 0x1C;
constexpr std::uint32_t kChildCount = 0x14;
constexpr std::uint32_t kChildren = 0x18;

}  // namespace

void WalkNode(ModelWalk& walk, std::uint32_t offset,
              const glm::mat4& parent, int depth) {
    const RacerBigEndianReader& r = walk.reader;
    if (depth > kMaxDepth || !walk.IsPointer(offset, kHeaderBytes)) return;
    if (--walk.nodeBudget < 0) return;

    const std::uint32_t flags = r.U32(offset);
    glm::mat4 transform = parent;
    const bool podParts = walk.scope == RacerModelScope::PodParts;
    bool enteredPart = false;
    if (flags == kTransformed || flags == kTransformedPivot) {
        const glm::mat4 local = ReadNodeTransform(r, offset);
        if (podParts && !walk.insidePart && IsPodPartScale(local)) {
            walk.insidePart = enteredPart = true;
            walk.parts.emplace_back();
            transform = glm::mat4(1.f);  // the game places the part
        } else {
            transform = parent * local;
        }
    } else if (flags != kMeshGroup && flags != kBasic &&
               flags != kSelector && flags != kLodSelector &&
               flags != kTransformedComputed) {
        return;  // not a node; a stray pointer
    }

    const std::int32_t count = r.I32(offset + kChildCount);
    const std::uint32_t children = r.U32(offset + kChildren);
    if (count <= 0 || count > 4096 ||
        !walk.IsPointer(children, 4u * count)) {
        return;
    }
    for (std::int32_t i = 0; i < count; ++i) {
        const std::uint32_t child = r.U32(children + 4u * i);
        if (child == 0 || !VisitNodeChild(r, offset, flags, i)) continue;
        if (flags == kMeshGroup) {
            if (!podParts) AppendMesh(walk, child, transform);
            if (podParts && walk.insidePart) {
                std::swap(walk.model, walk.parts.back());
                AppendMesh(walk, child, transform);
                std::swap(walk.model, walk.parts.back());
            }
        } else {
            WalkNode(walk, child, transform, depth + 1);
        }
        // An LOD selector draws only its most detailed child present.
        if (flags == kLodSelector) break;
    }
    if (enteredPart) walk.insidePart = false;
}

}  // namespace sdl3cpp::services::impl::racer_model_detail
