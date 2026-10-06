#pragma once

#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include <SDL3/SDL_stdinc.h>

#include <filesystem>

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;

namespace racer_test {

/// Saves go to a scratch file, never the player's real profile.
class RacerFlowTest : public ::testing::Test {
protected:
    void SetUp() override {
        path_ = std::filesystem::temp_directory_path() / "racer_flow.json";
        SDL_setenv_unsafe("RACER_PROFILE", path_.string().c_str(), 1);
        table_.tracks.resize(25);
        table_.racers.resize(23);
    }
    void TearDown() override { std::filesystem::remove(path_); }

    RacerNav Press(bool RacerNav::*key) {
        RacerNav nav;
        nav.*key = true;
        return nav;
    }

    std::filesystem::path path_;
    RacerTrackTable table_;
    RacerFlow flow_;
};

}  // namespace racer_test
