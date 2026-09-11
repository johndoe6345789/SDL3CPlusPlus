#pragma once

#include "services/interfaces/workflow/gta5/gta5_sound.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// A radio station: its name and its tracks, or with none, a stand-in
/// playing SynthGta5Music(variant).
struct Gta5Station {
    std::string name;
    std::vector<std::filesystem::path> tracks;
    int variant{0};
};

/// Every folder under dir holding WAVs, at any depth, is a station,
/// named by namesFile -- a json object from folder name, in upper case,
/// to station name -- or else by its folder.
std::vector<Gta5Station> ListGta5Stations(const std::filesystem::path& dir,
                                          const std::string& namesFile);

/// A whole track read from disk: a few minutes of PCM, tens of MB. Empty
/// when the file is not a WAV SDL can read.
Gta5Clip LoadGta5Track(const std::filesystem::path& path);

/// A stand-in song: eight bars of I-V-vi-IV over a kick, bass and hats,
/// each variant in its own key and tempo.
Gta5Clip SynthGta5Music(int variant);

}  // namespace sdl3cpp::services::impl
