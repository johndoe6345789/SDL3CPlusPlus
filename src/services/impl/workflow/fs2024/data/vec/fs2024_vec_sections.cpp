#include "services/interfaces/workflow/fs2024/data/vec/fs2024_vec_sections.hpp"

#include <stdexcept>

namespace sdl3cpp::fs2024 {
namespace {

void ReadRecord(VecCursor& in, VecRecord record, VecFeature& feature) {
    switch (record) {
        case VecRecord::Road:
            feature.uid = in.U32();
            feature.flags = in.U8();
            in.U8();  // type: unexplained
            if (feature.Bridge()) feature.level = in.U8();
            break;
        case VecRecord::Rail:
            feature.uid = in.U16();
            feature.uid |= static_cast<std::uint32_t>(in.U8()) << 16;
            feature.flags = in.U8();
            if (feature.Bridge()) feature.level = in.U8();
            in.U8();  // type
            break;
        case VecRecord::Water:
            if (in.Layout().flaggedWater) feature.flags = in.U8();
            break;
        case VecRecord::Bare:
            break;
    }
}

std::vector<int> Count(VecCursor& in, VecHeader header) {
    if (header == VecHeader::Mask) return in.ClassMask();
    const std::size_t n = header == VecHeader::Count32 ? in.U32() : in.U16();
    if (n > 60000) throw std::runtime_error("vec: feature count");
    return std::vector<int>(n, -1);
}

}  // namespace

std::vector<VecFeature> ReadVecSection(VecCursor& in, VecHeader header,
                                       VecRecord record) {
    const std::vector<int> classes = Count(in, header);
    std::vector<VecFeature> features(classes.size());
    std::vector<std::uint16_t> ends(classes.size());
    for (std::size_t i = 0; i < features.size(); ++i) {
        ReadRecord(in, record, features[i]);
        features[i].classBit = classes[i];
        ends[i] = in.U16();
        if (i > 0 && ends[i] < ends[i - 1]) {
            throw std::runtime_error("vec: point ends not monotonic");
        }
    }
    const std::vector<VecPoint> points =
        in.Points(features.empty() ? 0 : ends.back());
    std::size_t from = 0;
    for (std::size_t i = 0; i < features.size(); ++i) {
        features[i].points.assign(points.begin() + from,
                                  points.begin() + ends[i]);
        from = ends[i];
    }
    return features;
}

void SkipVecExtra(VecCursor& in) {
    const std::uint16_t rows = in.U16();
    if (rows == 0) return;
    in.U16();
    in.Skip(3u * rows);
    if (in.U16() != 0) throw std::runtime_error("vec: extra terminator");
}

}  // namespace sdl3cpp::fs2024
