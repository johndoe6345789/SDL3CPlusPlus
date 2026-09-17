#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::tools::fs2024 {

/// One entry of a BGL "ModelData" (section type 0x2B) directory: a
/// RIFF/GLTF-wrapped model, named by its own embedded `GXML` chunk.
/// Real files (FS2024's own `Asobo_POI.BGL`, the library every
/// worldwide named landmark lives in) run over a gigabyte and hold
/// hundreds of these, so only `name`/`guid`/the blob's file position
/// are read up front -- the model itself is read on demand by
/// `ReadModelRiff`.
struct ModelLibraryEntry {
    std::string guid;  ///< 16 raw bytes, hex-encoded
    std::string name;  ///< from GXML's `<ModelInfo name="...">`
    std::uint64_t fileOffset = 0;  ///< absolute offset of its RIFF blob
    std::uint64_t size = 0;
};

/// Every model in `path`'s ModelData section. Reads only the
/// directory table and each entry's small `GXML` chunk, not the
/// (often multi-megabyte) geometry itself.
std::vector<ModelLibraryEntry> ListModelLibrary(const std::string& path);

/// Reads one entry's full `RIFF ... GLTF` blob, as found by
/// `ListModelLibrary` -- ready for `ParseModelRiff`
/// (fs2024_gltf_model.hpp).
std::vector<std::uint8_t> ReadModelRiff(const std::string& path,
                                       const ModelLibraryEntry& entry);

}  // namespace sdl3cpp::tools::fs2024
