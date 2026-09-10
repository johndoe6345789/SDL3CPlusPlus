#include "services/interfaces/workflow/quake3/q3_md3_tags.hpp"

#include "services/interfaces/workflow/quake3/q3_axes.hpp"

namespace sdl3cpp::q3 {

glm::mat4 TagMatrix(const nlohmann::json& tag) {
    const auto& ax = tag["axis"];
    const auto& o  = tag["origin"];
    auto vec       = [&](int i) {
        return glm::vec3(ax[i][0].get<float>(), ax[i][1].get<float>(),
                               ax[i][2].get<float>());
    };
    // A Quake tag names its own forward, left and up. BuildMd3TagsJson
    // has already rewritten each of those three vectors into engine
    // space, but that only converts where they point — the tag still
    // has to be assembled into the column order the decoded child model
    // expects, which is the same job ModelBasis does for a whole model.
    // Permuting the vectors and stopping there attaches the torso with
    // a rotation belonging to neither convention, which is what turned
    // the players into something dog-shaped.
    const glm::vec3 forward = vec(0);
    const glm::vec3 up      = vec(2);
    glm::mat4 out           = ModelBasis(forward, up);
    out[3] = glm::vec4(o[0].get<float>(), o[1].get<float>(), o[2].get<float>(),
                       1.0f);
    return out;
}

}  // namespace sdl3cpp::q3
