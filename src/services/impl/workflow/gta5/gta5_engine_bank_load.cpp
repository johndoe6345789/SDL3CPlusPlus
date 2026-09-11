#include "services/interfaces/workflow/gta5/gta5_engine_bank.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

namespace sdl3cpp::services::impl {
namespace {

/// A WAV as mono float at its own rate.
bool ReadMono(const std::filesystem::path& path, Gta5EngineSweep& sweep) {
    SDL_AudioSpec spec{};
    Uint8* data = nullptr;
    Uint32 length = 0;
    if (!SDL_LoadWAV(path.string().c_str(), &spec, &data, &length)) {
        return false;
    }
    const SDL_AudioSpec mono{SDL_AUDIO_F32, 1, spec.freq};
    Uint8* converted = nullptr;
    int size = 0;
    const bool ok = SDL_ConvertAudioSamples(&spec, data, int(length), &mono,
                                            &converted, &size);
    SDL_free(data);
    if (!ok) return false;
    const auto* samples = reinterpret_cast<const float*>(converted);
    sweep.samples.assign(samples, samples + size / int(sizeof(float)));
    sweep.rate = spec.freq;
    SDL_free(converted);
    return true;
}

bool ReadSweep(const std::filesystem::path& folder,
               const nlohmann::json& entry, Gta5EngineSweep& sweep) {
    if (!entry.is_object() ||
        !ReadMono(folder / entry.value("file", std::string()), sweep)) {
        return false;
    }
    const nlohmann::json grains =
        entry.value("grains", nlohmann::json::array());
    for (const auto& grain : grains) {
        if (!grain.is_array() || grain.size() < 2) continue;
        sweep.starts.push_back(grain[0].get<std::uint32_t>());
        sweep.revs.push_back(grain[1].get<float>());
    }
    // The last grain ends where the recording does.
    sweep.starts.push_back(static_cast<std::uint32_t>(sweep.samples.size()));
    return sweep.revs.size() >= 2;
}

}  // namespace

bool LoadGta5EngineBank(const std::filesystem::path& folder,
                        Gta5EngineBank& bank) {
    std::ifstream in(folder / "grains.json");
    const nlohmann::json root = nlohmann::json::parse(in, nullptr, false);
    if (!root.is_object()) return false;
    const auto part = [&](const char* name, Gta5EngineSweep& sweep) {
        return ReadSweep(folder, root.value(name, nlohmann::json()), sweep);
    };
    bank.loaded = part("engine_accel", bank.accel) &&
                  part("engine_decel", bank.decel) && part("idle", bank.idle);
    return bank.loaded;
}

}  // namespace sdl3cpp::services::impl
