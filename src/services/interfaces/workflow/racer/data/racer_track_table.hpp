#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// One circuit: its name and where its geometry and route live.
struct RacerTrackInfo {
    int id = 0;
    std::string name;
    std::string planet;
    int model = -1;    ///< model-block index of the 'Trak' model
    int spline = -1;   ///< spline-block index of its route
};

/// One racer and the model-block index of their pod ('Podd').
struct RacerPodInfo {
    std::string name;
    int podd = -1;
    /// The game's handling record (see RacerHandling); zeros if absent.
    std::array<float, 15> handling{};
};

struct RacerTrackTable {
    std::vector<RacerTrackInfo> tracks;
    /// Fog colour per planet name, rgb 0..1.
    std::map<std::string, std::array<float, 3>> fog;
    std::vector<RacerPodInfo> racers;
    bool loaded = false;
};

/// Reads packages/racer/assets/racer_tracks.json (or another path).
RacerTrackTable LoadRacerTrackTable(const std::string& path);

/// A track by id, or by a case-insensitive substring of its name.
const RacerTrackInfo* FindRacerTrack(const RacerTrackTable& table,
                                     const std::string& key);

/// A racer by a case-insensitive substring of their name.
const RacerPodInfo* FindRacerPod(const RacerTrackTable& table,
                                 const std::string& key);

}  // namespace sdl3cpp::services::impl
