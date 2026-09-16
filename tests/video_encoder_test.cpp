#include "services/interfaces/workflow/graphics/video_encoder.hpp"
#include "video_test_media.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace {

using sdl3cpp::services::impl::VideoEncoder;
using video_test::ReadBack;
using video_test::Scrolling;
using video_test::Summary;

std::string TempPath(const std::string& extension) {
    const auto dir = std::filesystem::temp_directory_path();
    return (dir / ("video_encoder_test" + extension)).string();
}

/// 0..59 less a hitch at 20-21, with 30 sent twice. 161 wide, which
/// 4:2:0 rounds down to 160.
int WriteClip(VideoEncoder& encoder) {
    int written = 0;
    for (std::int64_t pts = 0; pts < 60; pts += pts == 19 ? 3 : 1) {
        written += encoder.Write(Scrolling(161, 120, pts));
        if (pts == 30) {
            written += encoder.Write(Scrolling(161, 120, pts));
        }
    }
    return written;
}

TEST(VideoEncoderTest, EveryFrameReachesTheFile) {
    for (const std::string extension : {".mp4", ".mkv"}) {
        SCOPED_TRACE(extension);
        const std::string path = TempPath(extension);
        VideoEncoder encoder;
        ASSERT_EQ(encoder.Open({path, 161, 120, 30, 23}), "");
        EXPECT_EQ(WriteClip(encoder), 58);
        encoder.Close();
        EXPECT_EQ(ReadBack(path), (Summary{58, 160, 60}));
        std::filesystem::remove(path);
    }
}

TEST(VideoEncoderTest, UnknownContainerIsAnError) {
    VideoEncoder encoder;
    const std::string path = "clip.notavideo";
    EXPECT_NE(encoder.Open({path, 64, 48, 30, 23}), "");
    EXPECT_FALSE(encoder.Write(Scrolling(64, 48, 0)));
}

}  // namespace
