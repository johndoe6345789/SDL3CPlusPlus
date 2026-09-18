#include "services/interfaces/workflow/fs2024/data/texture/fs2024_bc7_decode.hpp"

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_bc7_bits.hpp"
#include "services/interfaces/workflow/fs2024/data/texture/fs2024_bc7_tables.hpp"

#include <cstring>
#include <utility>

namespace sdl3cpp::fs2024 {

void DecodeBc7Block(const std::uint8_t* block, std::uint8_t* out,
                    int outStride) {
    Bc7Bits bits(block);
    int mode = 0;
    while (mode < 8 && bits.Read(1) == 0) ++mode;
    if (mode == 8) {
        for (int y = 0; y < 4; ++y) std::memset(out + y * outStride, 0, 16);
        return;
    }
    const Bc7Mode& m = kBc7Modes[mode];
    const int partition = bits.Read(m.partitionBits);
    const int rotation = bits.Read(m.rotationBits);
    const int swapIndices = bits.Read(m.selectorBits);

    Bc7Endpoints ends{};
    ReadBc7Endpoints(bits, m, ends);

    std::uint32_t shape = 0;
    int anchors[3] = {0, 0, 0};
    if (m.subsets == 2) {
        shape = kBc7Partition2[partition];
        anchors[1] = kBc7Anchor2[partition];
    } else if (m.subsets == 3) {
        shape = kBc7Partition3[partition];
        anchors[1] = kBc7Anchor3a[partition];
        anchors[2] = kBc7Anchor3b[partition];
    }
    int subset[16], index[16], index2[16];
    for (int i = 0; i < 16; ++i) {
        subset[i] = (shape >> (2 * i)) & 3;
        const bool anchor = anchors[subset[i]] == i;
        index[i] = bits.Read(m.indexBits - (anchor ? 1 : 0));
    }
    for (int i = 0; i < 16; ++i) {
        index2[i] = m.index2Bits ? bits.Read(m.index2Bits - (i ? 0 : 1)) : 0;
    }

    for (int i = 0; i < 16; ++i) {
        const int* e0 = ends[subset[i] * 2];
        const int* e1 = ends[subset[i] * 2 + 1];
        int colourIdx = index[i], colourBits = m.indexBits;
        int alphaIdx = index[i], alphaBits = m.indexBits;
        if (m.index2Bits) {
            alphaIdx = index2[i];
            alphaBits = m.index2Bits;
            if (swapIndices) {
                std::swap(colourIdx, alphaIdx);
                std::swap(colourBits, alphaBits);
            }
        }
        std::uint8_t* px = out + (i / 4) * outStride + (i % 4) * 4;
        for (int c = 0; c < 3; ++c) {
            px[c] = Bc7Interpolate(e0[c], e1[c], colourIdx, colourBits);
        }
        px[3] = Bc7Interpolate(e0[3], e1[3], alphaIdx, alphaBits);
        if (rotation) std::swap(px[3], px[rotation - 1]);
    }
}

}  // namespace sdl3cpp::fs2024
