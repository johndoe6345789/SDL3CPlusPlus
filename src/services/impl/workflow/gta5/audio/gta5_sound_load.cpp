#include "services/interfaces/workflow/gta5/audio/gta5_sound.hpp"

#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

Gta5Sounds LoadGta5Sounds(const std::filesystem::path& dir,
                          const std::shared_ptr<ILogger>& logger) {
    Gta5Sounds sounds;
    const std::pair<const char*, std::vector<Gta5Clip>*> kinds[] = {
        {"steps", &sounds.steps},      {"strokes", &sounds.strokes},
        {"splash", &sounds.splash},    {"engine", &sounds.engine},
        {"water", &sounds.water},      {"weapons", &sounds.shots},
        {"explosions", &sounds.blasts}};
    std::string report;
    for (const auto& [name, set] : kinds) {
        *set = LoadGta5SoundSet(dir / name);
        report += std::string(" ") + name + " ";
        if (!set->empty()) {
            report += std::to_string(set->size());
            continue;
        }
        // Stand-ins until GTA's are exported: four of a one-shot, so
        // they vary; one of a loop.
        const bool loop = set == &sounds.engine || set == &sounds.water;
        for (int v = 0; v < (loop ? 1 : 4); ++v) {
            set->push_back(SynthGta5Sound(name, v));
        }
        report += "synth";
    }
    // GTA's wet_feet_heel layer: a splash on every step, so only ever
    // in shallow water. Without it, wading sounds like walking.
    sounds.wetSteps = LoadGta5SoundSet(dir / "wet_steps");
    report += " wet_steps " + std::to_string(sounds.wetSteps.size());
    if (sounds.wetSteps.empty()) sounds.wetSteps = sounds.steps;
    if (logger) {
        logger->Info("gta5.sound: from " + dir.string() + ":" + report);
    }
    return sounds;
}

}  // namespace sdl3cpp::services::impl
