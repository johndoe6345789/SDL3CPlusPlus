#include "services/interfaces/workflow/racer/audio/racer_voice_lines.hpp"
#include "services/interfaces/workflow/racer/data/racer_wav.hpp"

#include <gtest/gtest.h>

using sdl3cpp::services::impl::ReadRacerWav;
using sdl3cpp::services::impl::RacerPcm;
using sdl3cpp::services::impl::ResampleRacerPcm;
using sdl3cpp::services::impl::WriteRacerWav;

TEST(RacerWav, RoundTripsPcm) {
    RacerPcm pcm;
    pcm.channels = 1;
    pcm.sampleRate = 22050;
    pcm.samples = {0, 1000, -2000, 3000};
    const auto file = WriteRacerWav(pcm);
    EXPECT_EQ(file.size(), 44u + 8u);
    RacerPcm back;
    ASSERT_TRUE(ReadRacerWav(file, back));
    EXPECT_EQ(back.channels, 1);
    EXPECT_EQ(back.sampleRate, 22050u);
    EXPECT_EQ(back.samples, pcm.samples);
}

TEST(RacerWav, RejectsNonRiffInput) {
    const std::vector<std::uint8_t> junk(64, 'x');
    RacerPcm out;
    EXPECT_FALSE(ReadRacerWav(junk, out));
}

TEST(RacerWav, DoublesRateWithInterpolation) {
    RacerPcm pcm;
    pcm.sampleRate = 22050;
    pcm.samples = {0, 1000, 2000, 3000};
    const auto out = ResampleRacerPcm(pcm, 44100);
    EXPECT_EQ(out.sampleRate, 44100u);
    ASSERT_EQ(out.samples.size(), 8u);
    EXPECT_EQ(out.samples[0], 0);
    EXPECT_EQ(out.samples[1], 500);   // halfway between 0 and 1000
    EXPECT_EQ(out.samples[2], 1000);
}

TEST(RacerVoice, LinesFollowEachRacersNumbering) {
    using sdl3cpp::services::impl::RacerVoiceEvent;
    using sdl3cpp::services::impl::RacerVoiceLineFile;
    EXPECT_EQ(RacerVoiceLineFile("sb", RacerVoiceEvent::Win, 0),
              "sbsp014.wav");
    EXPECT_EQ(RacerVoiceLineFile("as", RacerVoiceEvent::Win, 0),
              "assp015.wav");  // Anakin's sit one later
    EXPECT_EQ(RacerVoiceLineFile("as", RacerVoiceEvent::Taunt, 0),
              "assp017.wav");
    EXPECT_EQ(RacerVoiceLineFile("bq", RacerVoiceEvent::Taunt, 9),
              "bqsp025.wav");
    EXPECT_EQ(RacerVoiceLineFile("as", RacerVoiceEvent::Taunt, 8),
              "assp025.wav");
}
