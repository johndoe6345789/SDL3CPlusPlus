#include "services/interfaces/workflow/graphics/video_encoder.hpp"
#include "video_test_audio_media.hpp"
#include "video_test_media.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace {

using sdl3cpp::services::impl::VideoEncoder;

std::string TempPath(const std::string& extension) {
    const auto dir = std::filesystem::temp_directory_path();
    return (dir / ("video_encoder_audio_test" + extension)).string();
}

// A second of picture and half a second of 44.1 kHz mono, which the
// file holds as 48 kHz stereo, padded with silence to the picture's end.
TEST(VideoEncoderAudioTest, TrackRunsAsLongAsThePicture) {
    for (const std::string extension : {".mp4", ".mkv"}) {
        SCOPED_TRACE(extension);
        const std::string path = TempPath(extension);
        VideoEncoder encoder;
        ASSERT_EQ(encoder.Open({path, 160, 120, 30, 23, 48000, 2}), "");
        const auto tone = video_test::Tone(44100, 1, 0.5);
        for (int pts = 0; pts < 30; ++pts) {
            encoder.Write(video_test::Scrolling(160, 120, pts));
            // Half the tone, in ten pieces, over the first ten frames.
            if (pts < 10) {
                const int piece = int(tone.size()) / 10;
                EXPECT_TRUE(encoder.WriteAudio(tone.data() + pts * piece,
                                               piece, 44100, 1));
            }
        }
        encoder.Close();
        const auto got = video_test::ReadAudioBack(path);
        EXPECT_EQ(got.streams, 2);
        EXPECT_EQ(got.audioChannels, 2);
        EXPECT_NEAR(got.audioEnd, 1.0, 0.05);
        std::filesystem::remove(path);
    }
}

TEST(VideoEncoderAudioTest, NoTrackUnlessAsked) {
    const std::string path = TempPath(".mkv");
    VideoEncoder encoder;
    ASSERT_EQ(encoder.Open({path, 160, 120, 30, 23}), "");
    encoder.Write(video_test::Scrolling(160, 120, 0));
    const auto tone = video_test::Tone(48000, 2, 0.1);
    EXPECT_FALSE(encoder.WriteAudio(tone.data(), 4800, 48000, 2));
    encoder.Close();
    EXPECT_EQ(video_test::ReadAudioBack(path).streams, 1);
    std::filesystem::remove(path);
}

}  // namespace
