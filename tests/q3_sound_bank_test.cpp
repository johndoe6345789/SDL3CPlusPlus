// Decoding Quake's WAVs straight out of the pk3 into memory.

#include "services/interfaces/workflow/quake3/q3_sound_bank.hpp"

#include <gtest/gtest.h>
#include <cstring>
#include <vector>

namespace q3 = sdl3cpp::q3;

namespace {

// Smallest valid 8-bit mono RIFF/WAVE: header plus four samples.
std::vector<uint8_t> MakeWav() {
    const uint8_t data[] = {
        'R','I','F','F', 40,0,0,0, 'W','A','V','E',
        'f','m','t',' ', 16,0,0,0, 1,0, 1,0,
        0x40,0x1f,0,0, 0x40,0x1f,0,0, 1,0, 8,0,
        'd','a','t','a', 4,0,0,0, 0x80,0x90,0x70,0x80};
    return std::vector<uint8_t>(std::begin(data), std::end(data));
}

}  // namespace

TEST(DecodeWav, ReadsAWavHeldInMemory) {
    const auto wav = MakeWav();
    q3::Sound sound;
    ASSERT_TRUE(q3::DecodeWav(wav.data(), wav.size(), sound));
    EXPECT_EQ(sound.pcm.size(), 4u);
    EXPECT_EQ(sound.spec.channels, 1);
    EXPECT_EQ(sound.spec.freq, 8000);
}

TEST(DecodeWav, RejectsNullAndEmpty) {
    q3::Sound sound;
    EXPECT_FALSE(q3::DecodeWav(nullptr, 0, sound));
    const uint8_t byte = 0;
    EXPECT_FALSE(q3::DecodeWav(&byte, 0, sound));
}

TEST(DecodeWav, RejectsBytesThatAreNotAWav) {
    const std::vector<uint8_t> junk(64, 0xAB);
    q3::Sound sound;
    EXPECT_FALSE(q3::DecodeWav(junk.data(), junk.size(), sound));
}

TEST(DecodeWav, LeavesTheSoundUntouchedOnFailure) {
    q3::Sound sound;
    sound.pcm = {1, 2, 3};
    const std::vector<uint8_t> junk(64, 0xAB);
    EXPECT_FALSE(q3::DecodeWav(junk.data(), junk.size(), sound));
    EXPECT_EQ(sound.pcm.size(), 3u) << "a bad entry must not poison the bank";
}

TEST(CountPlayable, IgnoresEntriesThatFailedToDecode) {
    q3::SoundBank bank;
    const auto wav = MakeWav();
    ASSERT_TRUE(q3::DecodeWav(wav.data(), wav.size(), bank["good.wav"]));
    bank["bad.wav"] = q3::Sound{};
    EXPECT_EQ(q3::CountPlayable(bank), 1u);
    EXPECT_EQ(bank.size(), 2u);
}

TEST(CountPlayable, EmptyBankCountsZero) {
    const q3::SoundBank bank;
    EXPECT_EQ(q3::CountPlayable(bank), 0u);
}
