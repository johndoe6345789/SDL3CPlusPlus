#include "services/interfaces/workflow/fs2024/data/dem/fs2024_dem_cgl_tile.hpp"

#include <JXRGlue.h>

#include <cstring>
#include <stdexcept>

namespace sdl3cpp::fs2024 {
namespace {

constexpr std::size_t kPrefixBytes = 13;
constexpr std::uint8_t kPrefixMark = 0x04;

/// Owns jxrlib's stream and decoder, so every exit path releases both.
struct JxrDecode {
    WMPStream* stream = nullptr;
    PKImageDecode* decoder = nullptr;
    ~JxrDecode() {
        if (decoder) decoder->Release(&decoder);
        if (stream) CloseWS_Memory(&stream);
    }
};

void Check(ERR err, const char* what) {
    if (Failed(err)) {
        throw std::runtime_error(std::string("DEM JPEG XR: ") + what);
    }
}

}  // namespace

Fs2024DemSamples DecodeFs2024DemTile(const std::vector<std::uint8_t>& payload) {
    if (payload.size() < kPrefixBytes + 4 || payload[0] != kPrefixMark) {
        throw std::runtime_error("DEM tile: unknown framing");
    }
    std::uint32_t length = 0;
    std::memcpy(&length, payload.data() + kPrefixBytes, 4);
    if (length > payload.size() - kPrefixBytes - 4) {
        throw std::runtime_error("DEM tile: image runs past the tile");
    }
    // jxrlib reads through a non-const pointer but never writes.
    std::vector<std::uint8_t> image(
        payload.begin() + kPrefixBytes + 4,
        payload.begin() + kPrefixBytes + 4 + length);

    JxrDecode jxr;
    Check(CreateWS_Memory(&jxr.stream, image.data(), image.size()), "stream");
    Check(PKImageDecode_Create_WMP(&jxr.decoder), "decoder");
    Check(jxr.decoder->Initialize(jxr.decoder, jxr.stream), "header");

    Fs2024DemSamples samples;
    Check(jxr.decoder->GetSize(jxr.decoder, &samples.width, &samples.height),
          "size");
    samples.raw.resize(static_cast<std::size_t>(samples.width) *
                       samples.height);
    const PKRect whole{0, 0, samples.width, samples.height};
    Check(jxr.decoder->Copy(jxr.decoder, &whole,
                            reinterpret_cast<U8*>(samples.raw.data()),
                            static_cast<U32>(samples.width * 2)),
          "pixels");
    return samples;
}

}  // namespace sdl3cpp::fs2024
