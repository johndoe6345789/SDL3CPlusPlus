#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_gxml.hpp"

#include <cctype>

namespace sdl3cpp::fs2024 {
namespace {

/// `name="value"` from `tag`, matched case-insensitively (FS2024's
/// own GXML mixes `MinSize=`/`minSize=` across models).
std::string AttrValueCi(const std::string& tag, const std::string& name) {
    std::string lowerTag = tag, needle = name + "=\"";
    for (char& c : lowerTag) c = static_cast<char>(std::tolower(c));
    for (char& c : needle) c = static_cast<char>(std::tolower(c));
    const auto at = lowerTag.find(needle);
    if (at == std::string::npos) return {};
    const auto start = at + needle.size();
    const auto end = tag.find('"', start);
    return end == std::string::npos ? std::string()
                                    : tag.substr(start, end - start);
}

}  // namespace

std::vector<GltfLodHeader> ParseGxmlLods(const std::string& xml) {
    std::vector<GltfLodHeader> lods;
    std::size_t pos = 0;
    // The trailing space excludes the wrapping "<LODS>" element, which
    // "<LOD" alone would also match.
    while ((pos = xml.find("<LOD ", pos)) != std::string::npos) {
        const auto end = xml.find('>', pos);
        if (end == std::string::npos) break;
        const std::string tag = xml.substr(pos, end - pos);
        GltfLodHeader lod;
        lod.modelFile = AttrValueCi(tag, "ModelFile");
        const auto sizeStr = AttrValueCi(tag, "minsize");
        lod.minSize = sizeStr.empty() ? 0.f : std::stof(sizeStr);
        lods.push_back(std::move(lod));
        pos = end;
    }
    return lods;
}

}  // namespace sdl3cpp::fs2024
