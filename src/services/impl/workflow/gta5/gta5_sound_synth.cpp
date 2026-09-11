#include "services/interfaces/workflow/gta5/gta5_sound.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kRate = 44100;
constexpr float kTau = 6.2831853f;

/// Noise through a one-pole low-pass that closes from `open` to `shut`
/// (fractions of the band) while it dies away `decay` per second: a
/// thud, a slap or a splash, by the numbers.
std::vector<float> Burst(float seconds, float open, float shut, float decay,
                         int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> noise(-1.f, 1.f);
    std::vector<float> wave(std::size_t(seconds * kRate));
    float low = 0.f;
    for (std::size_t i = 0; i < wave.size(); ++i) {
        const float t = float(i) / kRate;
        const float cut = shut + (open - shut) * std::exp(-t * 12.f);
        low += (noise(rng) - low) * cut;
        wave[i] = low * std::min(1.f, t * 400.f) * std::exp(-t * decay);
    }
    return wave;
}

/// A second of an engine turning at 50 Hz: a roughened firing pulse each
/// cycle, a whole number of cycles so that it loops without a click.
std::vector<float> Engine(int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> noise(-1.f, 1.f);
    std::vector<float> wave(kRate);
    float rough = 0.f;
    for (int i = 0; i < kRate; ++i) {
        const float cycle = std::fmod(i * 50.f / kRate, 1.f);
        rough += (noise(rng) - rough) * 0.1f;
        wave[i] = std::exp(-cycle * 5.f) * (0.8f + 0.6f * rough) +
                  0.3f * std::sin(kTau * 25.f * i / kRate);
    }
    return wave;
}

/// Three seconds of surf: deep noise swelling twice, its end faded into
/// its start so that it loops.
std::vector<float> Lapping(int seed) {
    std::vector<float> wave = Burst(3.5f, 0.03f, 0.03f, 0.f, seed);
    const std::size_t loop = 3 * kRate, fade = wave.size() - loop;
    for (std::size_t i = 0; i < wave.size(); ++i) {
        wave[i] *= 0.6f + 0.4f * std::sin(kTau * float(i) / (1.5f * kRate));
    }
    for (std::size_t i = 0; i < fade; ++i) {
        const float in = float(i) / fade;
        wave[i] = wave[i] * in + wave[loop + i] * (1.f - in);
    }
    wave.resize(loop);
    return wave;
}

}  // namespace

std::vector<float> SynthGta5Wave(const std::string& kind, int variant) {
    const int seed = variant * 7919 + int(kind.size()) * 104729;
    if (kind == "steps") return Burst(0.14f, 0.4f, 0.05f, 35.f, seed);
    if (kind == "strokes") return Burst(0.5f, 0.3f, 0.06f, 7.f, seed);
    if (kind == "splash") return Burst(1.2f, 0.7f, 0.04f, 3.5f, seed);
    if (kind == "engine") return Engine(seed);
    return Lapping(seed);
}

}  // namespace sdl3cpp::services::impl
