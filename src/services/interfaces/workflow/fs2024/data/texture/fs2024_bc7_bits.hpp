#pragma once

#include <cstdint>

namespace sdl3cpp::fs2024 {

/// One BC7 mode's layout (BPTC spec table: NS, PB, RB, ISB, CB, AB,
/// EPB, SPB, IB, IB2).
struct Bc7Mode {
    int subsets, partitionBits, rotationBits, selectorBits;
    int colourBits, alphaBits;
    bool endpointPBits, sharedPBits;
    int indexBits, index2Bits;
};
extern const Bc7Mode kBc7Modes[8];

/// Reads a block's 128 bits LSB-first, as BC7 packs them.
class Bc7Bits {
public:
    explicit Bc7Bits(const std::uint8_t* block) : block_(block) {}
    int Read(int count);

private:
    const std::uint8_t* block_;
    int at_ = 0;
};

/// Up to six endpoints (two per subset), RGBA, fully unquantised to
/// 8 bits; alpha is 255 in modes without alpha endpoints.
using Bc7Endpoints = int[6][4];
void ReadBc7Endpoints(Bc7Bits& bits, const Bc7Mode& mode,
                      Bc7Endpoints& ends);

std::uint8_t Bc7Interpolate(int e0, int e1, int index, int indexBits);

}  // namespace sdl3cpp::fs2024
