#pragma once

#include <cstdint>

namespace sdl3cpp::fs2024 {

/// Undoes TIFF's Predictor=3 (floating-point horizontal predictor) on
/// one decompressed row, in place. `samplesPerRow` is pixels times
/// samples-per-pixel (1 for a single-band DEM); `bytesPerSample` is 4
/// for float32.
///
/// The encoder both delta-codes the row's raw bytes and reorders them
/// into byte planes (all MSBs, then all next-most-significant, ...),
/// always big-endian regardless of the file's own byte order -- so
/// undoing it takes both a cumulative sum and a de-planarize, not just
/// one or the other.
void UndoFloatPredictorRow(std::uint8_t* row, std::uint32_t samplesPerRow,
                           std::uint32_t bytesPerSample);

}  // namespace sdl3cpp::fs2024
