#include "services/interfaces/workflow/gta5/gta5_radio.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kRate = 44100;
constexpr float kTau = 6.2831853f;
// I, V, vi, IV: each chord's three notes, in semitones above the key.
constexpr int kChords[4][3] = {{0, 4, 7}, {7, 11, 14}, {9, 12, 16},
                               {5, 9, 12}};

float Note(float key, int semitones) {
    return key * std::pow(2.f, static_cast<float>(semitones) / 12.f);
}

}  // namespace

Gta5Clip SynthGta5Music(int variant) {
    const float beat = 60.f / (92.f + 16.f * static_cast<float>(variant % 3));
    const float key = Note(110.f, (variant * 5) % 12);  // A, then fourths
    std::mt19937 rng(variant + 1);
    std::uniform_real_distribution<float> noise(-1.f, 1.f);
    std::vector<float> wave(std::size_t(32.f * beat * kRate));  // 8 bars
    float peak = 1e-6f;
    for (std::size_t i = 0; i < wave.size(); ++i) {
        const float t = float(i) / kRate, beats = t / beat;
        const int* chord = kChords[int(beats / 4.f) % 4];
        const float in = std::fmod(beats, 1.f) * beat;  // s into the beat
        const float off = std::fmod(beats + 0.5f, 1.f) * beat;
        float pad = 0.f;
        for (int n = 0; n < 3; ++n) {
            pad += std::sin(kTau * Note(key * 2.f, chord[n]) * t);
        }
        const float bass = std::sin(kTau * Note(key * 0.5f, chord[0]) * t) *
                           std::exp(-in * 4.f);
        const float kick = std::sin(kTau * 55.f * in) * std::exp(-in * 18.f);
        const float hat = noise(rng) * std::exp(-off * 60.f);
        wave[i] = 0.1f * pad + 0.35f * bass + 0.6f * kick + 0.08f * hat;
        peak = std::max(peak, std::abs(wave[i]));
    }
    for (float& s : wave) s *= 0.8f / peak;
    Gta5Clip clip;
    clip.spec = {SDL_AUDIO_F32, 1, kRate};
    clip.pcm.resize(wave.size() * sizeof(float));
    std::memcpy(clip.pcm.data(), wave.data(), clip.pcm.size());
    return clip;
}

}  // namespace sdl3cpp::services::impl
