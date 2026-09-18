#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>

namespace sdl3cpp::fs2024 {

constexpr std::size_t kDx10HeaderSize = 148;  // 128 + DDS_HEADER_DXT10
constexpr std::size_t kBc7BlockBytes = 16;

constexpr std::uint32_t kDxgiBc1Unorm = 71;
constexpr std::uint32_t kDxgiBc1UnormSrgb = 72;
constexpr std::uint32_t kDxgiBc7Unorm = 98;
constexpr std::uint32_t kDxgiBc7UnormSrgb = 99;

/// The shape of a DX10 block-compressed texture array -- BC7, as the
/// building generator ships its facades, or BC1, as the ground material
/// array is -- and how many bytes one layer's whole mip chain takes:
/// layer N starts that far in.
struct DdsArrayInfo {
    std::uint32_t width = 0, height = 0, mips = 1, layers = 1;
    std::uint32_t dxgiFormat = 0;
    std::uint32_t blockBytes = kBc7BlockBytes;  ///< 8 for BC1, 16 for BC7
    std::size_t layerBytes = 0;

    bool IsBc7() const {
        return dxgiFormat == kDxgiBc7Unorm || dxgiFormat == kDxgiBc7UnormSrgb;
    }
};

DdsArrayInfo ReadDdsArrayInfo(std::ifstream& in, const std::string& path);

}  // namespace sdl3cpp::fs2024
