#include "services/interfaces/workflow/fs2024/prepare/fs2024_tiff_predictor.hpp"

#include <vector>

namespace sdl3cpp::tools::fs2024 {

void UndoFloatPredictorRow(std::uint8_t* row, std::uint32_t samplesPerRow,
                           std::uint32_t bytesPerSample) {
    const std::uint32_t rowBytes = samplesPerRow * bytesPerSample;

    // The encoder's delta was over the whole row's raw bytes, not per
    // sample -- undo with one running sum, wrapping mod 256 same as it
    // wrapped going the other way.
    for (std::uint32_t i = 1; i < rowBytes; ++i) {
        row[i] = static_cast<std::uint8_t>(row[i] + row[i - 1]);
    }

    // De-planarize: row currently holds byte-plane 0 (every sample's
    // most-significant byte) for all samples, then plane 1, and so on.
    // Reassemble each sample's bytes, big-endian in, little-endian out
    // (this tool only ever runs on little-endian hosts).
    std::vector<std::uint8_t> planar(row, row + rowBytes);
    for (std::uint32_t sample = 0; sample < samplesPerRow; ++sample) {
        for (std::uint32_t k = 0; k < bytesPerSample; ++k) {
            const std::uint32_t plane = bytesPerSample - 1 - k;
            row[sample * bytesPerSample + k] =
                planar[plane * samplesPerRow + sample];
        }
    }
}

}  // namespace sdl3cpp::tools::fs2024
