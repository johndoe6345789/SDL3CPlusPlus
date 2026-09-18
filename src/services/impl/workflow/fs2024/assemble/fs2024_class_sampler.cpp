#include "services/interfaces/workflow/fs2024/assemble/fs2024_class_sampler.hpp"

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_lcg_tile.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kFinest = 12, kCoarsest = 8, kCore = 257;
constexpr std::size_t kMaxImages = 256;

std::uint64_t ImageKey(int level, int x, int y) {
    return (static_cast<std::uint64_t>(level) << 56) |
           (static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) << 28) |
           static_cast<std::uint32_t>(y);
}

/// The sample nearest `fraction` (0..1 across the tile), allowing for a
/// one-sample skirt around the 257-sample core when the image has one.
int Nearest(double fraction, int size) {
    const int skirt = (size - kCore) / 2;
    const int at =
        static_cast<int>(std::lround(fraction * (kCore - 1))) + skirt;
    return std::clamp(at, 0, size - 1);
}

}  // namespace

Fs2024ClassSampler::Fs2024ClassSampler(const std::string& cglRoot)
    : pyramid_(cglRoot, "lcg"), images_(kMaxImages) {}

std::shared_ptr<const Fs2024ClassSampler::Image> Fs2024ClassSampler::ImageAt(
    int level, int x, int y) {
    return images_.Get(ImageKey(level, x, y), [&] {
        std::shared_ptr<Image> image;
        const auto blob = pyramid_.Read(level, x, y);
        if (blob.empty()) return image;
        const auto lcg = sdl3cpp::fs2024::DecodeFs2024LcgTile(blob);
        image = std::make_shared<Image>();
        image->width = lcg.width;
        image->height = lcg.height;
        image->classes.resize(static_cast<std::size_t>(lcg.width) *
                              lcg.height);
        for (std::size_t i = 0; i < image->classes.size(); ++i) {
            image->classes[i] =
                static_cast<std::uint8_t>(lcg.rgba[i * 4 + 1] / 10);
        }
        return image;
    });
}

std::uint8_t Fs2024ClassSampler::ClassAt(double u, double v) {
    for (int level = kFinest; level >= kCoarsest; --level) {
        const double side = static_cast<double>(1 << level);
        const int x = static_cast<int>(std::floor(u * side));
        const int y = static_cast<int>(std::floor(v * side));
        const auto image = ImageAt(level, x, y);
        if (!image) continue;
        const int c = Nearest(u * side - x, image->width);
        const int r = Nearest(v * side - y, image->height);
        return image->classes[static_cast<std::size_t>(r) * image->width + c];
    }
    return 0;
}

}  // namespace sdl3cpp::services::impl
