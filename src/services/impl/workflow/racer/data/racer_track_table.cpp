#include "services/interfaces/workflow/racer/data/racer_track_table.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>

namespace sdl3cpp::services::impl {
namespace {

std::string Lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });
    return text;
}

bool NameMatches(const std::string& name, const std::string& key) {
    return !key.empty() && Lower(name).find(Lower(key)) != std::string::npos;
}

}  // namespace

RacerTrackTable LoadRacerTrackTable(const std::string& path) {
    RacerTrackTable table;
    std::ifstream in(path);
    if (!in) return table;
    const auto doc = nlohmann::json::parse(in, nullptr, false);
    if (doc.is_discarded()) return table;
    for (const auto& t : doc.value("tracks", nlohmann::json::array())) {
        table.tracks.push_back({t.value("id", 0), t.value("name", ""),
                                t.value("planet", ""), t.value("model", -1),
                                t.value("spline", -1)});
    }
    for (const auto& r : doc.value("racers", nlohmann::json::array())) {
        RacerPodInfo racer{r.value("name", ""), r.value("podd", -1), {},
                           r.value("voice", "")};
        const auto handling = r.value("handling", nlohmann::json::array());
        for (std::size_t i = 0;
             i < racer.handling.size() && i < handling.size(); ++i) {
            racer.handling[i] = handling[i].get<float>();
        }
        table.racers.push_back(racer);
    }
    // Held by name: iterating items() of the temporary value() returns
    // would read a destroyed object (and found no planets at all).
    const nlohmann::json planets =
        doc.value("planets", nlohmann::json::object());
    for (const auto& [planet, rgb] : planets.items()) {
        if (rgb.is_array() && rgb.size() == 3) {
            table.fog[planet] = {rgb[0].get<float>(), rgb[1].get<float>(),
                                 rgb[2].get<float>()};
        }
    }
    table.loaded = !table.tracks.empty();
    return table;
}

const RacerTrackInfo* FindRacerTrack(const RacerTrackTable& table,
                                     const std::string& key) {
    for (const auto& track : table.tracks) {
        if (std::to_string(track.id) == key) return &track;
    }
    for (const auto& track : table.tracks) {
        if (NameMatches(track.name, key)) return &track;
    }
    return nullptr;
}

const RacerPodInfo* FindRacerPod(const RacerTrackTable& table,
                                 const std::string& key) {
    for (const auto& racer : table.racers) {
        if (NameMatches(racer.name, key)) return &racer;
    }
    return nullptr;
}

}  // namespace sdl3cpp::services::impl
