#include "services/interfaces/workflow/gta5/hud/gta5_settings.hpp"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace sdl3cpp::services::impl {
namespace {

std::string Environment(const char* name) {
    const char* value = std::getenv(name);
    return value ? value : std::string();
}

}  // namespace

std::string Gta5SettingsPath() {
    // Windows keeps such things in APPDATA; the rest follow the XDG
    // base directory, which falls back to ~/.config.
    std::string base = Environment("APPDATA");
    if (base.empty()) base = Environment("XDG_CONFIG_HOME");
    if (base.empty()) {
        const std::string home = Environment("HOME");
        if (!home.empty()) base = home + "/.config";
    }
    if (base.empty()) return {};
    return (std::filesystem::path(base) / "SDL3CPlusPlus" / "gta5" /
            "settings.json")
        .string();
}

bool LoadGta5Settings(Gta5Settings& settings) {
    const std::string path = Gta5SettingsPath();
    if (path.empty()) return false;
    std::ifstream in(path);
    const auto doc = nlohmann::json::parse(in, nullptr, false);
    if (!doc.is_object()) return false;
    settings.car = doc.value("car", settings.car);
    settings.colour = doc.value("colour", settings.colour);
    settings.health = doc.value("health", settings.health);
    settings.armour = doc.value("armour", settings.armour);
    settings.station = doc.value("station", settings.station);
    settings.inventory.clip =
        doc.value("clip", std::vector<int>{});
    settings.inventory.reserve =
        doc.value("reserve", std::vector<int>{});
    settings.inventory.current = doc.value("weapon", -1);
    return true;
}

bool SaveGta5Settings(const Gta5Settings& settings) {
    const std::string path = Gta5SettingsPath();
    if (path.empty()) return false;
    std::error_code error;
    std::filesystem::create_directories(
        std::filesystem::path(path).parent_path(), error);
    nlohmann::json doc;
    doc["comment"] = "GTA V package: the car, guns and armour kept "
                     "between sessions.";
    doc["car"] = settings.car;
    doc["colour"] = settings.colour;
    doc["health"] = settings.health;
    doc["armour"] = settings.armour;
    doc["station"] = settings.station;
    doc["clip"] = settings.inventory.clip;
    doc["reserve"] = settings.inventory.reserve;
    doc["weapon"] = settings.inventory.current;
    std::ofstream out(path);
    if (!out) return false;
    out << doc.dump(2) << "\n";
    return true;
}

}  // namespace sdl3cpp::services::impl
