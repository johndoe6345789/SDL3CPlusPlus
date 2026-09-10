#include "services/interfaces/workflow/rendering/fps_meter.hpp"

namespace sdl3cpp::services::impl {

float FpsMeter::Update(uint64_t nowNs) {
    if (lastNs_ != 0 && nowNs > lastNs_) {
        const float deltaSeconds =
            static_cast<float>(static_cast<double>(nowNs - lastNs_) / 1e9);
        if (deltaSeconds > 0.0f) {
            const float instantaneous = 1.0f / deltaSeconds;
            smoothed_ =
                smoothed_ * smoothing_ + instantaneous * (1.0f - smoothing_);
        }
    }
    lastNs_ = nowNs;
    return smoothed_;
}

}  // namespace sdl3cpp::services::impl
