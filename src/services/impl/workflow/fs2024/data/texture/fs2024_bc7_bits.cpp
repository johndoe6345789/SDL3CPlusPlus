#include "services/interfaces/workflow/fs2024/data/texture/fs2024_bc7_bits.hpp"

namespace sdl3cpp::fs2024 {
namespace {

const int kWeights2[] = {0, 21, 43, 64};
const int kWeights3[] = {0, 9, 18, 27, 37, 46, 55, 64};
const int kWeights4[] = {0,  4,  9,  13, 17, 21, 26, 30,
                         34, 38, 43, 47, 51, 55, 60, 64};

int Unquantise(int value, int bits) {
    value <<= 8 - bits;
    return value | (value >> bits);
}

}  // namespace

const Bc7Mode kBc7Modes[8] = {
    {3, 4, 0, 0, 4, 0, true, false, 3, 0},
    {2, 6, 0, 0, 6, 0, false, true, 3, 0},
    {3, 6, 0, 0, 5, 0, false, false, 2, 0},
    {2, 6, 0, 0, 7, 0, true, false, 2, 0},
    {1, 0, 2, 1, 5, 6, false, false, 2, 3},
    {1, 0, 2, 0, 7, 8, false, false, 2, 2},
    {1, 0, 0, 0, 7, 7, true, false, 4, 0},
    {2, 6, 0, 0, 5, 5, true, false, 2, 0},
};

int Bc7Bits::Read(int count) {
    int value = 0;
    for (int i = 0; i < count; ++i, ++at_) {
        value |= ((block_[at_ >> 3] >> (at_ & 7)) & 1) << i;
    }
    return value;
}

void ReadBc7Endpoints(Bc7Bits& bits, const Bc7Mode& mode,
                      Bc7Endpoints& ends) {
    const int count = mode.subsets * 2;
    for (int c = 0; c < 3; ++c) {
        for (int e = 0; e < count; ++e) ends[e][c] = bits.Read(mode.colourBits);
    }
    for (int e = 0; e < count; ++e) {
        ends[e][3] = mode.alphaBits ? bits.Read(mode.alphaBits) : 255;
    }
    int pbits[6] = {0, 0, 0, 0, 0, 0};
    if (mode.endpointPBits) {
        for (int e = 0; e < count; ++e) pbits[e] = bits.Read(1);
    } else if (mode.sharedPBits) {
        for (int s = 0; s < mode.subsets; ++s) {
            pbits[s * 2] = pbits[s * 2 + 1] = bits.Read(1);
        }
    }
    const bool hasP = mode.endpointPBits || mode.sharedPBits;
    for (int e = 0; e < count; ++e) {
        for (int c = 0; c < 4; ++c) {
            const int width = c < 3 ? mode.colourBits : mode.alphaBits;
            if (width == 0) continue;
            ends[e][c] = hasP
                ? Unquantise((ends[e][c] << 1) | pbits[e], width + 1)
                : Unquantise(ends[e][c], width);
        }
    }
}

std::uint8_t Bc7Interpolate(int e0, int e1, int index, int indexBits) {
    const int w = indexBits == 2   ? kWeights2[index]
                  : indexBits == 3 ? kWeights3[index]
                                   : kWeights4[index];
    return static_cast<std::uint8_t>(((64 - w) * e0 + w * e1 + 32) >> 6);
}

}  // namespace sdl3cpp::fs2024
