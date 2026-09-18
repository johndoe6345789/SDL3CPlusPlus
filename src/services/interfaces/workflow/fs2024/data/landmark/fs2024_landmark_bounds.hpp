#pragma once

#include <string>

namespace sdl3cpp::fs2024 {

/// How far an extracted landmark model reaches from its own origin,
/// horizontally, in metres: read back from the `.lmk` this bake just
/// wrote. The Palace of Westminster is nearly 300 m long, so nothing
/// but the model's own geometry can say how much ground it covers --
/// and the generated buildings standing on that ground (FS2024's
/// footprint library has the Palace in it too, as plain outlines)
/// have to give way to it.
float ReadLandmarkRadius(const std::string& outDir,
                        const std::string& model);

}  // namespace sdl3cpp::fs2024
