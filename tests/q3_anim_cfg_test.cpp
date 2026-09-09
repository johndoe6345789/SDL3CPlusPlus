// animation.cfg numbers frames across the whole character, but the legs
// live in their own model that lacks the torso-only frames. ioq3
// subtracts that gap; without it the legs animate from the wrong frames.

#include "services/interfaces/workflow/quake3/q3_anim_cfg.hpp"

#include <gtest/gtest.h>

namespace q3 = sdl3cpp::q3;

namespace {

// The first fourteen rows of models/players/keel/animation.cfg, which is
// enough to reach LEGS_WALKCR at index 13.
constexpr const char* kKeel =
    "0 30 0 25\n"       // BOTH_DEATH1
    "29 1 0 25\n"       // BOTH_DEAD1
    "30 30 0 25\n"      // BOTH_DEATH2
    "59 1 0 25\n"       // BOTH_DEAD2
    "60 30 0 25\n"      // BOTH_DEATH3
    "89 1 0 25\n"       // BOTH_DEAD3
    "90 40 0 20\n"      // TORSO_GESTURE
    "130 6 0 15\n"      // TORSO_ATTACK
    "136 6 0 15\n"      // TORSO_ATTACK2
    "142 5 0 20\n"      // TORSO_DROP
    "147 4 0 20\n"      // TORSO_RAISE
    "151 1 0 15\n"      // TORSO_STAND
    "152 1 0 15\n"      // TORSO_STAND2
    "153 9 9 20\n";     // LEGS_WALKCR

}  // namespace

TEST(ParseAnimCfg, ReadsEveryClip) {
    const auto clips = q3::ParseAnimCfg(kKeel);
    ASSERT_EQ(clips.size(), 14u);
    EXPECT_EQ(clips[0].numFrames, 30);
    EXPECT_EQ(clips[q3::kTorsoGesture].firstFrame, 90);
}

TEST(ParseAnimCfg, ShiftsLegFramesDownByTheTorsoOnlyGap) {
    const auto clips = q3::ParseAnimCfg(kKeel);
    // skip = LEGS_WALKCR(153) - TORSO_GESTURE(90) = 63
    EXPECT_EQ(clips[q3::kLegsWalkCr].firstFrame, 153 - 63);
}

TEST(ParseAnimCfg, LeavesTorsoAndDeathFramesAlone) {
    const auto clips = q3::ParseAnimCfg(kKeel);
    EXPECT_EQ(clips[0].firstFrame, 0);
    EXPECT_EQ(clips[q3::kTorsoGesture].firstFrame, 90);
    EXPECT_EQ(clips[12].firstFrame, 152);
}

TEST(ParseAnimCfg, KeepsFramesInsideTheLegModel) {
    // lower.md3 for keel has 191 frames; an unadjusted leg index would
    // point at frames belonging to the torso model.
    const auto clips = q3::ParseAnimCfg(kKeel);
    EXPECT_LT(clips[q3::kLegsWalkCr].firstFrame, 191);
    EXPECT_GE(clips[q3::kLegsWalkCr].firstFrame, 0);
}

TEST(ParseAnimCfg, IgnoresCommentsAndBlankLines) {
    const auto clips = q3::ParseAnimCfg("// header\n\n0 30 0 25 // death\n");
    ASSERT_EQ(clips.size(), 1u);
    EXPECT_EQ(clips[0].fps, 25);
}

TEST(ParseAnimCfg, SkipsTextOnlyDirectives) {
    const auto clips = q3::ParseAnimCfg("sex m\nfootsteps normal\n0 1 0 25\n");
    ASSERT_EQ(clips.size(), 1u);
}

TEST(ParseAnimCfg, ShortFileNeedsNoAdjustment) {
    const auto clips = q3::ParseAnimCfg("0 30 0 25\n29 1 0 25\n");
    ASSERT_EQ(clips.size(), 2u);
    EXPECT_EQ(clips[1].firstFrame, 29);
}

TEST(ParseAnimCfg, EmptyInputGivesNoClips) {
    EXPECT_TRUE(q3::ParseAnimCfg("").empty());
}
