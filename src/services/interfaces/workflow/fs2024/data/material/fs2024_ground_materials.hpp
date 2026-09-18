#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace sdl3cpp::fs2024 {

/// A run of layers in FS2024's ground material array
/// (`bf-texture-synth-lib/.../autogen/array_low.dds`: 653 layers of
/// 256 x 256 BC1): `layers` interchangeable variations of one surface,
/// starting at `firstLayer`.
struct Fs2024GroundMaterial {
    int firstLayer = 0;
    int layers = 1;
};

/// Which array layers draw which kind of ground, read from the array's
/// own index (`autogen/arrays.xml`). Its keys are
/// `[density * 1e9 +] climate * 1000 + landClass`: land class is FS2024's
/// biome code (1 cultivated, 2 forest, 3 grassland, 8 artificial, ...,
/// see biomes_globeland30.xml); climate 1-4 picks dry / temperate /
/// dry / arid variants; density, for built-up ground, 1 urban,
/// 2 suburban, 3 industrial.
class Fs2024GroundMaterials {
public:
    static Fs2024GroundMaterials Read(const std::string& arraysXmlPath);

    /// The material for `landClass` in `climate`, falling back to the
    /// class in any climate, then to temperate grassland. Cultivated
    /// land and forest floor are not in the runtime array at all --
    /// FS2024 synthesises fields and woodland from separate texture
    /// sets -- so they take the array's own meadow and support-forest
    /// ground until that synthesis is reproduced.
    Fs2024GroundMaterial For(int landClass, int climate,
                             int density = 1) const;

    std::size_t Count() const { return byKey_.size(); }

private:
    std::map<std::int64_t, Fs2024GroundMaterial> byKey_;
};

}  // namespace sdl3cpp::fs2024
