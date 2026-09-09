#include "services/interfaces/workflow/quake3/q3_anim_cfg.hpp"

#include <algorithm>
#include <sstream>

namespace sdl3cpp::q3 {

std::vector<AnimClip> ParseAnimCfg(const std::string& text) {
    std::vector<AnimClip> clips;
    std::istringstream lines(text);
    std::string line;

    while (std::getline(lines, line)) {
        const auto comment = line.find("//");
        if (comment != std::string::npos) {
            line = line.substr(0, comment);
        }
        std::istringstream fields(line);
        AnimClip clip;
        if (fields >> clip.firstFrame >> clip.numFrames >>
            clip.loopingFrames >> clip.fps) {
            clips.push_back(clip);
        }
    }

    // Leg animations live in their own model, which does not contain the
    // torso-only frames the config counted before them.
    if (static_cast<int>(clips.size()) > kLegsWalkCr) {
        const int skip = clips[kLegsWalkCr].firstFrame -
                         clips[kTorsoGesture].firstFrame;
        const int last = std::min(static_cast<int>(clips.size()),
                                  kTorsoGetFlag);
        for (int i = kLegsWalkCr; i < last; ++i) {
            clips[i].firstFrame -= skip;
        }
    }
    return clips;
}

}  // namespace sdl3cpp::q3
