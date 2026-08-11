#include "core/pixel_data_csv.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iomanip>

namespace sdl3cpp::core {

bool PixelDataCSV::LoadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line[0] != '#') {
            lines.push_back(line);
        }
    }
    file.close();

    if (lines.empty()) {
        return false;
    }

    // Try to detect format from first line
    const auto& firstLine = lines[0];
    bool hasCommas = firstLine.find(',') != std::string::npos;

    if (hasCommas && firstLine.find("x") != std::string::npos) {
        // Standard format with x,y,r,g,b headers
        format = FORMAT_STANDARD;
        return LoadStandardFormat(lines);
    } else if (hasCommas) {
        // Row-major format with RGB values
        format = FORMAT_ROW_MAJOR;
        return LoadRowMajorFormat(lines);
    }

    return false;
}

bool PixelDataCSV::LoadStandardFormat(const std::vector<std::string>& lines) {
    pixelData.clear();

    if (lines.empty() || lines[0].find("x,y,r") == std::string::npos) {
        return false;
    }

    // Skip header line and parse data
    for (size_t i = 1; i < lines.size(); ++i) {
        const auto& line = lines[i];
        std::istringstream iss(line);
        std::string token;

        uint32_t x, y, r, g, b, a = 255;
        if (std::getline(iss, token, ',') && (x = std::stoul(token)) &&
            std::getline(iss, token, ',') && (y = std::stoul(token)) &&
            std::getline(iss, token, ',') && (r = std::stoul(token)) &&
            std::getline(iss, token, ',') && (g = std::stoul(token)) &&
            std::getline(iss, token, ',') && (b = std::stoul(token))) {

            std::getline(iss, token, ',');
            if (!token.empty()) {
                a = std::stoul(token);
            }

            // Update width/height
            width = std::max(width, x + 1);
            height = std::max(height, y + 1);

            // Ensure pixelData is large enough
            while (pixelData.size() <= y) {
                pixelData.push_back(std::vector<Pixel>());
            }
            while (pixelData[y].size() <= x) {
                pixelData[y].push_back(Pixel());
            }

            pixelData[y][x] = Pixel(static_cast<uint8_t>(r), static_cast<uint8_t>(g),
                                     static_cast<uint8_t>(b), static_cast<uint8_t>(a));
        }
    }

    return !pixelData.empty();
}

bool PixelDataCSV::LoadRowMajorFormat(const std::vector<std::string>& lines) {
    pixelData.clear();
    height = 0;
    width = 0;

    for (const auto& line : lines) {
        std::vector<Pixel> row;
        std::istringstream iss(line);
        std::string pixelStr;

        while (std::getline(iss, pixelStr, ',')) {
            // Parse format like "R031G031B031"
            Pixel pixel;
            bool parsed = false;

            if (pixelStr.find('R') != std::string::npos && pixelStr.find('G') != std::string::npos &&
                pixelStr.find('B') != std::string::npos) {
                // RGB format
                size_t rPos = pixelStr.find('R');
                size_t gPos = pixelStr.find('G');
                size_t bPos = pixelStr.find('B');

                if (rPos != std::string::npos && gPos != std::string::npos && bPos != std::string::npos) {
                    try {
                        pixel.r = static_cast<uint8_t>(std::stoul(pixelStr.substr(rPos + 1, gPos - rPos - 1)));
                        pixel.g = static_cast<uint8_t>(std::stoul(pixelStr.substr(gPos + 1, bPos - gPos - 1)));
                        pixel.b = static_cast<uint8_t>(std::stoul(pixelStr.substr(bPos + 1)));
                        parsed = true;
                    } catch (...) {
                    }
                }
            } else if (pixelStr.find("#") != std::string::npos) {
                // Hex format like #1F1F1F
                try {
                    std::string hexStr = pixelStr.substr(pixelStr.find('#') + 1);
                    uint32_t hexVal = std::stoul(hexStr, nullptr, 16);
                    pixel.r = (hexVal >> 16) & 0xFF;
                    pixel.g = (hexVal >> 8) & 0xFF;
                    pixel.b = hexVal & 0xFF;
                    parsed = true;
                } catch (...) {
                }
            }

            if (parsed) {
                row.push_back(pixel);
            }
        }

        if (!row.empty()) {
            pixelData.push_back(row);
            width = std::max(width, static_cast<uint32_t>(row.size()));
            height++;
        }
    }

    return !pixelData.empty() && width > 0 && height > 0;
}

std::optional<Pixel> PixelDataCSV::GetPixel(uint32_t x, uint32_t y) const {
    if (y >= pixelData.size() || x >= pixelData[y].size()) {
        return std::nullopt;
    }
    return pixelData[y][x];
}

std::vector<Pixel> PixelDataCSV::GetPixelRegion(uint32_t x0, uint32_t y0,
                                                  uint32_t regionWidth, uint32_t regionHeight) const {
    std::vector<Pixel> region;

    for (uint32_t y = y0; y < y0 + regionHeight && y < height; ++y) {
        for (uint32_t x = x0; x < x0 + regionWidth && x < width; ++x) {
            if (auto pixel = GetPixel(x, y)) {
                region.push_back(*pixel);
            }
        }
    }

    return region;
}

bool PixelDataCSV::PixelMatchesExact(uint32_t x, uint32_t y, const Pixel& expected) const {
    if (auto pixel = GetPixel(x, y)) {
        return *pixel == expected;
    }
    return false;
}

bool PixelDataCSV::PixelMatchesTolerance(uint32_t x, uint32_t y, const Pixel& expected,
                                          uint8_t tolerance) const {
    if (auto pixel = GetPixel(x, y)) {
        return pixel->EqualsWithTolerance(expected, tolerance);
    }
    return false;
}

Pixel PixelDataCSV::GetAverageColor(uint32_t x0, uint32_t y0, uint32_t regionWidth,
                                     uint32_t regionHeight) const {
    auto region = GetPixelRegion(x0, y0, regionWidth, regionHeight);
    if (region.empty()) {
        return Pixel();
    }

    uint64_t sumR = 0, sumG = 0, sumB = 0, sumA = 0;
    for (const auto& pixel : region) {
        sumR += pixel.r;
        sumG += pixel.g;
        sumB += pixel.b;
        sumA += pixel.a;
    }

    return Pixel(static_cast<uint8_t>(sumR / region.size()),
                 static_cast<uint8_t>(sumG / region.size()),
                 static_cast<uint8_t>(sumB / region.size()),
                 static_cast<uint8_t>(sumA / region.size()));
}

uint32_t PixelDataCSV::CountPixels(const Pixel& color) const {
    uint32_t count = 0;
    for (const auto& row : pixelData) {
        for (const auto& pixel : row) {
            if (pixel == color) {
                count++;
            }
        }
    }
    return count;
}

uint32_t PixelDataCSV::CountPixelsWithTolerance(const Pixel& color, uint8_t tolerance) const {
    uint32_t count = 0;
    for (const auto& row : pixelData) {
        for (const auto& pixel : row) {
            if (pixel.EqualsWithTolerance(color, tolerance)) {
                count++;
            }
        }
    }
    return count;
}

std::map<uint32_t, uint32_t> PixelDataCSV::GetColorHistogram() const {
    std::map<uint32_t, uint32_t> histogram;
    for (const auto& row : pixelData) {
        for (const auto& pixel : row) {
            histogram[pixel.ToARGB()]++;
        }
    }
    return histogram;
}

PixelDataCSV::BrightnessStats PixelDataCSV::GetBrightnessStats() const {
    BrightnessStats stats{255, 0, 0.0};

    if (pixelData.empty()) {
        return stats;
    }

    double sum = 0;
    uint32_t count = 0;

    for (const auto& row : pixelData) {
        for (const auto& pixel : row) {
            // Luminance formula (standard Y in YUV)
            uint8_t brightness =
                static_cast<uint8_t>(0.299 * pixel.r + 0.587 * pixel.g + 0.114 * pixel.b);

            stats.min = std::min(stats.min, brightness);
            stats.max = std::max(stats.max, brightness);
            sum += brightness;
            count++;
        }
    }

    stats.average = count > 0 ? sum / count : 0;
    return stats;
}

bool PixelDataCSV::IsMostlyEmpty(uint8_t brightnessThreshold) const {
    if (pixelData.empty()) {
        return true;
    }

    uint32_t darkPixels = 0;
    uint32_t totalPixels = 0;

    for (const auto& row : pixelData) {
        for (const auto& pixel : row) {
            uint8_t brightness =
                static_cast<uint8_t>(0.299 * pixel.r + 0.587 * pixel.g + 0.114 * pixel.b);
            if (brightness < brightnessThreshold) {
                darkPixels++;
            }
            totalPixels++;
        }
    }

    return darkPixels > totalPixels * 0.9;  // 90% dark pixels
}

bool PixelDataCSV::HasSignificantVariation(uint8_t minBrightnessDiff) const {
    auto stats = GetBrightnessStats();
    return (static_cast<uint32_t>(stats.max) - static_cast<uint32_t>(stats.min)) >=
           minBrightnessDiff;
}

double PixelDataCSV::GetOpacityPercentage() const {
    if (pixelData.empty()) {
        return 0.0;
    }

    uint32_t opaquePixels = 0;
    uint32_t totalPixels = 0;

    for (const auto& row : pixelData) {
        for (const auto& pixel : row) {
            if (pixel.a > 127) {
                opaquePixels++;
            }
            totalPixels++;
        }
    }

    return totalPixels > 0 ? (100.0 * opaquePixels) / totalPixels : 0.0;
}

bool PixelDataCSV::VerifyDimensions(uint32_t expectedWidth, uint32_t expectedHeight) const {
    return width == expectedWidth && height == expectedHeight;
}

bool PixelDataCSV::Validate() const {
    if (pixelData.empty()) {
        return false;
    }

    // Check that all rows have consistent width
    size_t expectedWidth = pixelData[0].size();
    for (const auto& row : pixelData) {
        if (row.size() != expectedWidth) {
            return false;
        }
    }

    return width > 0 && height > 0 && !format.empty();
}

std::string PixelDataCSV::GetStatisticsString() const {
    std::ostringstream oss;

    if (!Validate()) {
        oss << "Invalid CSV data";
        return oss.str();
    }

    auto histogram = GetColorHistogram();
    auto brightness = GetBrightnessStats();

    oss << "Dimensions: " << width << "x" << height << "\n";
    oss << "Total Pixels: " << (width * height) << "\n";
    oss << "Unique Colors: " << histogram.size() << "\n";
    oss << "Brightness - Min: " << static_cast<int>(brightness.min) << ", Max: "
        << static_cast<int>(brightness.max) << ", Avg: " << std::fixed << std::setprecision(1)
        << brightness.average << "\n";
    oss << "Opacity: " << std::fixed << std::setprecision(1) << GetOpacityPercentage() << "%\n";
    oss << "Format: " << (format.empty() ? "unknown" : format) << "\n";

    return oss.str();
}

}  // namespace sdl3cpp::core
