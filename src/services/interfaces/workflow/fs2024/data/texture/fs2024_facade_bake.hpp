#pragma once

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_texture.hpp"
#include "services/interfaces/workflow/fs2024/data/texture/fs2024_pgg_assets.hpp"

namespace sdl3cpp::fs2024 {

/// One bay of facade: a wall the width of a window bay and the height
/// of one storey, with the window composited into it exactly where
/// FS2024's own generator puts it -- centred across the bay, its own
/// `offset` metres up from the floor, at its own real-world size.
/// Both source images tile at their asset's `dimensions_in_meters`,
/// so a 3.6 m brick texture repeats correctly inside a 4 m bay.
/// The result is `bayMetres` x `storeyMetres` at `pixelsPerMetre`.
DdsImage BakeFacadeBay(const PggAsset& wall, const DdsImage& wallImage,
                       const PggAsset& window, const DdsImage& windowImage,
                       float bayMetres, float storeyMetres,
                       int pixelsPerMetre);

}  // namespace sdl3cpp::fs2024
