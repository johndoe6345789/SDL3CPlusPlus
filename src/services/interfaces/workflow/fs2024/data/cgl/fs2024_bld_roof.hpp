#pragma once

#include "services/interfaces/workflow/fs2024/building/fs2024_roof_mesh.hpp"

#include <cstdint>

namespace sdl3cpp::fs2024 {

/// FS2024's own roof shapes, in the order its generator's name table
/// lists them (inside grammar.pggmod), with the stored code one above
/// the name's own position -- code 0 means the roof was never
/// surveyed. The offset is what the data itself says: across whole
/// suburban tiles (Harrow, Bromley) code 3 is two thirds of every
/// roof and code 4 most of the rest, which is British semi-detached
/// hips and gables; in Westminster code 1 is three quarters, which is
/// central London's flat roofs. Reading the codes an entry lower
/// would make Westminster a city of lean-tos.
enum class BldRoofType : std::uint8_t {
    Unsurveyed = 0, Flat, Pent, Hipped, Gabled, Individual, Mansard,
    Pyramidal, Gambrel, Dome, Onion, Round, Saltbox, Halfhipped,
    Doublesaltbox, Quadruplesaltbox, Sidehipped, Dutchgable, Cone,
};

/// The nearest shape this engine builds. Everything domed, conical or
/// onion-topped becomes a pyramid; everything gambrel/saltbox/dutch
/// becomes a gable; mansards and half-hips become hips.
sdl3cpp::services::impl::RoofShape RoofShapeOfBld(std::uint8_t roofType);

/// How far a roof of that shape rises above the eaves for a building
/// of half-width `halfWidth` metres: a 35 degree pitch, the usual
/// British roof, capped so a wide warehouse does not grow a spire.
float BldRoofRise(sdl3cpp::services::impl::RoofShape shape,
                  float halfWidth);

}  // namespace sdl3cpp::fs2024
