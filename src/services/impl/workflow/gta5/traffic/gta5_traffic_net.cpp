#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_ai.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint32_t kMagic = 0x314E4E54;  // "TNN1"
constexpr int kMaxWidth = 128;

template <typename T>
bool Read(std::ifstream& file, T* into, std::size_t count) {
    file.read(reinterpret_cast<char*>(into),
              static_cast<std::streamsize>(count * sizeof(T)));
    return static_cast<bool>(file);
}

float Sigmoid(float x) { return 1.f / (1.f + std::exp(-x)); }

}  // namespace

bool Gta5DriverNet::Load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::uint32_t head[2] = {0, 0};
    if (!Read(file, head, 2) || head[0] != kMagic || head[1] == 0) {
        return false;
    }
    std::vector<Layer> layers(head[1]);
    int width = kGta5DriverInputs;
    for (Layer& layer : layers) {
        std::int32_t shape[2] = {0, 0};
        if (!Read(file, shape, 2) || shape[0] != width || shape[1] < 1 ||
            shape[1] > kMaxWidth) {
            return false;
        }
        layer.in = shape[0];
        layer.out = shape[1];
        layer.weights.resize(std::size_t(layer.in) * layer.out);
        layer.bias.resize(layer.out);
        if (!Read(file, layer.weights.data(), layer.weights.size()) ||
            !Read(file, layer.bias.data(), layer.bias.size())) {
            return false;
        }
        width = layer.out;
    }
    if (width != kGta5DriverOutputs) return false;
    layers_ = std::move(layers);
    return true;
}

Gta5DriverAction Gta5DriverNet::Run(const Gta5DriverSense& sense) const {
    float a[kMaxWidth] = {0.f}, b[kMaxWidth] = {0.f};
    Gta5DriverInputs(sense, a);
    float* now = a;
    float* next = b;
    for (std::size_t i = 0; i < layers_.size(); ++i) {
        const Layer& layer = layers_[i];
        const bool last = i + 1 == layers_.size();
        for (int row = 0; row < layer.out; ++row) {
            float sum = layer.bias[row];
            const float* w = &layer.weights[std::size_t(row) * layer.in];
            for (int col = 0; col < layer.in; ++col) sum += w[col] * now[col];
            next[row] = last ? sum : std::max(0.f, sum);
        }
        std::swap(now, next);
    }
    return {std::tanh(now[0]), Sigmoid(now[1]), Sigmoid(now[2])};
}

}  // namespace sdl3cpp::services::impl
