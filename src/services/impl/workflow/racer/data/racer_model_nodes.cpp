#include "racer_model_context.hpp"

namespace sdl3cpp::services::impl::racer_model_detail {
namespace {

constexpr int kMaxDepth = 64;
// flags, flags1, flags2, flags3 (s16) + light (s16), flags5, child
// count, child pointer array: 0x1C bytes before the kind's own fields.
constexpr std::uint32_t kHeaderBytes = 0x1C;
constexpr std::uint32_t kChildCount = 0x14;
constexpr std::uint32_t kChildren = 0x18;

/// Rows after the header: right, forward, up, then translation (3 floats each).
glm::mat4 ReadTransform(const RacerBigEndianReader& r, std::uint32_t at) {
    glm::mat4 m(1.f);
    for (int row = 0; row < 4; ++row) {
        for (int k = 0; k < 3; ++k) {
            m[row][k] = r.F32(at + kHeaderBytes + 4 * (3 * row + k));
        }
    }
    return m;
}

/// Which children to visit: all, one, or none (selector value -2).
bool VisitChild(const RacerBigEndianReader& r, std::uint32_t node,
                std::uint32_t flags, int child) {
    if (flags == kLodSelector) return child == 0;  // highest detail
    if (flags != kSelector) return true;
    const std::int32_t selection = r.I32(node + kHeaderBytes);
    if (selection == -2) return false;
    return selection < 0 || selection == child;
}

}  // namespace

void WalkNode(ModelWalk& walk, std::uint32_t offset,
              const glm::mat4& parent, int depth) {
    const RacerBigEndianReader& r = walk.reader;
    if (depth > kMaxDepth || !walk.IsPointer(offset, kHeaderBytes)) return;
    if (--walk.nodeBudget < 0) return;

    const std::uint32_t flags = r.U32(offset);
    glm::mat4 transform = parent;
    const bool lodOnly = walk.scope == RacerModelScope::FirstLodOnly;
    bool enteredLod = false;
    if (lodOnly && flags == kLodSelector && !walk.insideLod) {
        walk.insideLod = enteredLod = true;
        transform = glm::mat4(1.f);  // drop the runtime scale above it
    }
    if (flags == kTransformed || flags == kTransformedPivot) {
        transform = parent * ReadTransform(r, offset);
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
        if (child == 0 || !VisitChild(r, offset, flags, i)) continue;
        if (flags == kMeshGroup) {
            if (!lodOnly || walk.insideLod) AppendMesh(walk, child, transform);
        } else {
            WalkNode(walk, child, transform, depth + 1);
        }
    }
    if (enteredLod) walk.insideLod = false;
}

}  // namespace sdl3cpp::services::impl::racer_model_detail
