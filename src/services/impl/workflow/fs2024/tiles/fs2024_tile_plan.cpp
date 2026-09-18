#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_plan.hpp"

namespace sdl3cpp::services::impl {
namespace {

bool HasAncestorIn(const Fs2024TileKey& key,
                   const std::unordered_set<Fs2024TileKey>& tiles) {
    for (Fs2024TileKey up = key; up.level > kFs2024CoarsestLevel;) {
        up = Fs2024TileParent(up);
        if (tiles.count(up)) return true;
    }
    return false;
}

/// Draws what is resident over a wanted tile that is not.
void Cover(const Fs2024TileKey& wanted,
           const std::unordered_set<Fs2024TileKey>& resident,
           std::unordered_set<Fs2024TileKey>& draw) {
    for (Fs2024TileKey up = wanted; up.level > kFs2024CoarsestLevel;) {
        up = Fs2024TileParent(up);
        if (resident.count(up)) {
            draw.insert(up);
            return;
        }
    }
    for (const Fs2024TileKey& finer : resident) {
        if (finer.level > wanted.level &&
            Fs2024TileContains(wanted, finer)) {
            draw.insert(finer);
        }
    }
}

}  // namespace

Fs2024TilePlan PlanFs2024Tiles(
    const std::vector<Fs2024TileKey>& wanted,
    const std::unordered_set<Fs2024TileKey>& resident,
    const std::unordered_set<Fs2024TileKey>& empty) {
    Fs2024TilePlan plan;
    for (const Fs2024TileKey& key : wanted) {
        if (resident.count(key)) {
            plan.draw.insert(key);
        } else if (!empty.count(key)) {
            plan.load.push_back(key);
            Cover(key, resident, plan.draw);
        }
    }
    std::unordered_set<Fs2024TileKey> hidden;
    for (const Fs2024TileKey& key : plan.draw) {
        if (HasAncestorIn(key, plan.draw)) hidden.insert(key);
    }
    for (const Fs2024TileKey& key : hidden) plan.draw.erase(key);

    const std::unordered_set<Fs2024TileKey> keep(wanted.begin(), wanted.end());
    for (const Fs2024TileKey& key : resident) {
        if (!keep.count(key) && !plan.draw.count(key)) {
            plan.evict.push_back(key);
        }
    }
    return plan;
}

}  // namespace sdl3cpp::services::impl
