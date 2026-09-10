#include "services/interfaces/workflow/quake3/q3_md3_tags.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_format.hpp"

#include <cstring>
#include <string>

namespace sdl3cpp::q3 {
namespace {

constexpr float kTagScale = kMd3XyzScale * kMd3WorldScale;

nlohmann::json TagToJson(const Md3Tag& tag) {
    nlohmann::json out;
    out["origin"] = nlohmann::json::array({tag.origin[0] * kTagScale,
                                           tag.origin[2] * kTagScale,
                                           -tag.origin[1] * kTagScale});

    nlohmann::json axis = nlohmann::json::array();
    for (int i = 0; i < 3; ++i) {
        axis.push_back(nlohmann::json::array(
            {tag.axis[i][0], tag.axis[i][2], -tag.axis[i][1]}));
    }
    out["axis"] = axis;
    return out;
}

}  // namespace

glm::mat4 TagMatrix(const nlohmann::json& tag) {
    const auto& ax = tag["axis"];  // 3 rows in engine space
    const auto& o  = tag["origin"];
    // Transpose: row i of the tag's axis -> column i of the glm matrix.
    return glm::mat4(glm::vec4(ax[0][0].get<float>(), ax[1][0].get<float>(),
                               ax[2][0].get<float>(), 0.0f),
                     glm::vec4(ax[0][1].get<float>(), ax[1][1].get<float>(),
                               ax[2][1].get<float>(), 0.0f),
                     glm::vec4(ax[0][2].get<float>(), ax[1][2].get<float>(),
                               ax[2][2].get<float>(), 0.0f),
                     glm::vec4(o[0].get<float>(), o[1].get<float>(),
                               o[2].get<float>(), 1.0f));
}

nlohmann::json BuildMd3TagsJson(const std::vector<uint8_t>& md3Bytes) {
    nlohmann::json frames = nlohmann::json::array();
    if (md3Bytes.size() < sizeof(Md3Header)) {
        return frames;
    }

    const uint8_t* base  = md3Bytes.data();
    const auto& header   = *reinterpret_cast<const Md3Header*>(base);
    const int frameCount = header.numFrames;
    const int tagCount   = header.numTags;
    if (frameCount <= 0 || tagCount <= 0 || header.ofsTags <= 0) {
        return frames;
    }

    const size_t needed = static_cast<size_t>(header.ofsTags) +
                          static_cast<size_t>(frameCount) *
                              static_cast<size_t>(tagCount) * sizeof(Md3Tag);
    if (md3Bytes.size() < needed) {
        return frames;
    }

    const auto* tags = reinterpret_cast<const Md3Tag*>(base + header.ofsTags);
    for (int f = 0; f < frameCount; ++f) {
        nlohmann::json frame = nlohmann::json::object();
        for (int t = 0; t < tagCount; ++t) {
            const auto& tag = tags[f * tagCount + t];
            frame[std::string(tag.name, strnlen(tag.name, 64))] =
                TagToJson(tag);
        }
        frames.push_back(frame);
    }
    return frames;
}

}  // namespace sdl3cpp::q3
