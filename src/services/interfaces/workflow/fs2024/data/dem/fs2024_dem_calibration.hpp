#pragma once

#include <cstdint>

namespace sdl3cpp::fs2024 {

/// Metres above sea level for one raw FS2024 DEM sample.
///
/// FS2024 does not store heights linearly: a count is worth ~0.02 m
/// near sea level and ~0.45 m at 7,500 m, and steepens again below sea
/// level -- a compander that spends its 16 bits where most of the
/// world's ground actually is. No closed form (square, power, sinh,
/// log) fits the whole range, so this is a measured calibration curve:
/// 65 knots, one every 1024 counts, built by isotonic regression over
/// 22,743 samples paired with Copernicus GLO-30 terrain in seven
/// regions from the Dead Sea (-430 m) to Everest, and checked against
/// surveyed lake surfaces it was not fitted to (all within ~65 m).
/// Interpolated linearly between knots; strictly increasing.
float Fs2024DemMetres(std::int16_t raw);

}  // namespace sdl3cpp::fs2024
