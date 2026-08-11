#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <optional>

namespace sdl3cpp::core {

/**
 * @brief Represents a single pixel with RGBA channels.
 */
struct Pixel {
    uint8_t r, g, b, a;

    Pixel() : r(0), g(0), b(0), a(255) {}
    Pixel(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}

    bool operator==(const Pixel& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    bool EqualsWithTolerance(const Pixel& other, uint8_t tolerance) const {
        auto diff = [](uint8_t a, uint8_t b) -> int { return static_cast<int>(a) - static_cast<int>(b); };
        return std::abs(diff(r, other.r)) <= tolerance &&
               std::abs(diff(g, other.g)) <= tolerance &&
               std::abs(diff(b, other.b)) <= tolerance &&
               std::abs(diff(a, other.a)) <= tolerance;
    }

    std::string ToHex() const {
        char buf[7];
        snprintf(buf, sizeof(buf), "%02x%02x%02x", r, g, b);
        return std::string(buf);
    }

    uint32_t ToARGB() const {
        return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
    }
};

/**
 * @brief Helper for reading and verifying pixel data from CSV files.
 *
 * Supports multiple CSV formats:
 * - Standard (x,y,r,g,b,a): Individual pixel data per row
 * - Row-major (RGB values as comma-separated on each line): Compact format
 */
class PixelDataCSV {
public:
    PixelDataCSV() = default;

    static constexpr const char* FORMAT_STANDARD = "standard";
    static constexpr const char* FORMAT_ROW_MAJOR = "row_major";

    uint32_t width = 0;
    uint32_t height = 0;
    std::string format;
    std::vector<std::vector<Pixel>> pixelData;  // [y][x]

    /// Load CSV file with pixel data
    bool LoadFromFile(const std::string& filePath);

    /// Load from standard format (x,y,r,g,b,a)
    bool LoadStandardFormat(const std::vector<std::string>& lines);

    /// Load from row-major format (RGB values per row)
    bool LoadRowMajorFormat(const std::vector<std::string>& lines);

    /// Get pixel at (x, y)
    std::optional<Pixel> GetPixel(uint32_t x, uint32_t y) const;

    /// Get multiple pixels in a region
    std::vector<Pixel> GetPixelRegion(uint32_t x0, uint32_t y0, uint32_t width, uint32_t height) const;

    /// Check if pixel at (x, y) matches expected color
    bool PixelMatchesExact(uint32_t x, uint32_t y, const Pixel& expected) const;

    /// Check if pixel at (x, y) matches with tolerance
    bool PixelMatchesTolerance(uint32_t x, uint32_t y, const Pixel& expected,
                               uint8_t tolerance = 5) const;

    /// Compute average color in region
    Pixel GetAverageColor(uint32_t x0, uint32_t y0, uint32_t regionWidth,
                          uint32_t regionHeight) const;

    /// Count pixels matching a color (exact match)
    uint32_t CountPixels(const Pixel& color) const;

    /// Count pixels matching a color (with tolerance)
    uint32_t CountPixelsWithTolerance(const Pixel& color, uint8_t tolerance = 5) const;

    /// Compute histogram of unique colors
    std::map<uint32_t, uint32_t> GetColorHistogram() const;

    /// Compute brightness statistics (0-255)
    struct BrightnessStats {
        uint8_t min, max;
        double average;
    };

    BrightnessStats GetBrightnessStats() const;

    /// Check if image is mostly empty (all black or mostly transparent)
    bool IsMostlyEmpty(uint8_t brightnessThreshold = 30) const;

    /// Check if image has significant variation
    bool HasSignificantVariation(uint8_t minBrightnessDiff = 50) const;

    /// Get percentage of opaque pixels
    double GetOpacityPercentage() const;

    /// Verify image dimensions
    bool VerifyDimensions(uint32_t expectedWidth, uint32_t expectedHeight) const;

    /// Validate that CSV has valid structure
    bool Validate() const;

    /// Get detailed statistics string
    std::string GetStatisticsString() const;
};

}  // namespace sdl3cpp::core
