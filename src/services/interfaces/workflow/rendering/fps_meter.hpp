#pragma once

#include <cstdint>

namespace sdl3cpp::services::impl {

/**
 * @brief Exponential moving average of the instantaneous frame rate.
 *
 * The first sample only seeds the timestamp, so Update() returns 0 until two
 * frames have been observed.  Non-advancing timestamps are ignored rather than
 * producing an infinite rate.
 */
class FpsMeter {
public:
    /// @param smoothing Weight kept from the previous average, in [0, 1].
    explicit FpsMeter(float smoothing = 0.9f) : smoothing_(smoothing) {}

    /// Feeds a monotonic timestamp in nanoseconds; returns the smoothed rate.
    float Update(uint64_t nowNs);

    float Value() const {
        return smoothed_;
    }

private:
    float smoothing_;
    float smoothed_  = 0.0f;
    uint64_t lastNs_ = 0;
};

}  // namespace sdl3cpp::services::impl
