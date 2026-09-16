#pragma once

#include <string>
#include <vector>

namespace video_test {

/// @p seconds of a 440 Hz tone: interleaved float, @p channels wide.
std::vector<float> Tone(int rate, int channels, double seconds);

/// A file's streams read back: how many, and where the audio one's
/// last shown packet ends, in seconds (-1 without audio).
struct AudioSummary {
    int streams       = 0;
    double audioEnd   = -1.0;
    int audioChannels = 0;
};
AudioSummary ReadAudioBack(const std::string& path);

}  // namespace video_test
