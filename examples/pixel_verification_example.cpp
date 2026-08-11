/**
 * @file pixel_verification_example.cpp
 * @brief Example: Using PixelDataCSV for screenshot verification in tests
 *
 * This example demonstrates how to use the PNG-to-CSV pixel verification
 * system to validate game screenshots in automated tests.
 */

#include "core/pixel_data_csv.hpp"

#include <iostream>
#include <iomanip>

using namespace sdl3cpp::core;

// ============================================================================
// EXAMPLE 1: Basic Image Loading and Inspection
// ============================================================================

void Example_BasicImageLoading() {
    std::cout << "\n=== Example 1: Basic Image Loading ===\n";

    // Load a CSV file with pixel data
    PixelDataCSV csv;
    if (!csv.LoadFromFile("test_outputs/standalone_cubes_frame.csv")) {
        std::cerr << "Failed to load CSV file\n";
        return;
    }

    // Print basic properties
    std::cout << "Image dimensions: " << csv.width << "x" << csv.height << "\n";
    std::cout << "Format: " << csv.format << "\n";
    std::cout << "Valid structure: " << (csv.Validate() ? "Yes" : "No") << "\n";
}

// ============================================================================
// EXAMPLE 2: Pixel-Level Inspection
// ============================================================================

void Example_PixelInspection() {
    std::cout << "\n=== Example 2: Pixel-Level Inspection ===\n";

    PixelDataCSV csv;
    if (!csv.LoadFromFile("test_outputs/standalone_cubes_frame.csv")) {
        return;
    }

    // Get pixel at specific location
    auto pixel = csv.GetPixel(512, 384);  // Center of 1024x768 image
    if (pixel.has_value()) {
        std::cout << "Center pixel: RGB("
                  << (int)pixel->r << ", " << (int)pixel->g << ", "
                  << (int)pixel->b << "), Alpha: " << (int)pixel->a << "\n";
        std::cout << "Hex color: #" << pixel->ToHex() << "\n";
    }

    // Sample multiple pixels
    std::cout << "\nCorner pixels:\n";
    auto corners = {
        std::make_pair(0, 0),
        std::make_pair(csv.width - 1, 0),
        std::make_pair(0, csv.height - 1),
        std::make_pair(csv.width - 1, csv.height - 1)
    };

    for (const auto& [x, y] : corners) {
        if (auto p = csv.GetPixel(x, y)) {
            std::cout << "(" << x << ", " << y << "): RGB("
                      << (int)p->r << ", " << (int)p->g << ", "
                      << (int)p->b << ")\n";
        }
    }
}

// ============================================================================
// EXAMPLE 3: Image Statistics and Analysis
// ============================================================================

void Example_ImageStatistics() {
    std::cout << "\n=== Example 3: Image Statistics ===\n";

    PixelDataCSV csv;
    if (!csv.LoadFromFile("test_outputs/standalone_cubes_frame.csv")) {
        return;
    }

    // Get brightness statistics
    auto brightness = csv.GetBrightnessStats();
    std::cout << "Brightness Range: " << (int)brightness.min
              << " - " << (int)brightness.max << "\n";
    std::cout << "Average brightness: " << std::fixed << std::setprecision(1)
              << brightness.average << "\n";

    // Get color distribution
    auto histogram = csv.GetColorHistogram();
    std::cout << "Unique colors: " << histogram.size() << "\n";

    // Print top 5 most common colors
    std::cout << "Most common colors:\n";
    std::vector<std::pair<uint32_t, uint32_t>> sorted(histogram.begin(),
                                                        histogram.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    for (size_t i = 0; i < 5 && i < sorted.size(); ++i) {
        uint32_t argb = sorted[i].first;
        uint32_t count = sorted[i].second;
        uint8_t r = (argb >> 16) & 0xFF;
        uint8_t g = (argb >> 8) & 0xFF;
        uint8_t b = argb & 0xFF;

        std::cout << "  #" << std::hex << std::setfill('0')
                  << std::setw(2) << (int)r
                  << std::setw(2) << (int)g
                  << std::setw(2) << (int)b << std::dec
                  << ": " << count << " pixels\n";
    }

    // Check if image is mostly empty
    std::cout << "Mostly empty (brightness < 30): "
              << (csv.IsMostlyEmpty(30) ? "Yes" : "No") << "\n";
    std::cout << "Has color variation: "
              << (csv.HasSignificantVariation(50) ? "Yes" : "No") << "\n";

    // Opacity information
    std::cout << "Opacity percentage: " << std::fixed << std::setprecision(1)
              << csv.GetOpacityPercentage() << "%\n";
}

// ============================================================================
// EXAMPLE 4: Region Analysis
// ============================================================================

void Example_RegionAnalysis() {
    std::cout << "\n=== Example 4: Region Analysis ===\n";

    PixelDataCSV csv;
    if (!csv.LoadFromFile("test_outputs/standalone_cubes_frame.csv")) {
        return;
    }

    // Analyze center region where game objects typically are
    uint32_t cx = csv.width / 2;
    uint32_t cy = csv.height / 2;
    uint32_t regionSize = 200;

    std::cout << "Analyzing " << regionSize << "x" << regionSize
              << " region at center (" << cx << ", " << cy << "):\n";

    auto avgColor = csv.GetAverageColor(cx - regionSize / 2, cy - regionSize / 2,
                                         regionSize, regionSize);
    std::cout << "Average color: RGB(" << (int)avgColor.r << ", "
              << (int)avgColor.g << ", " << (int)avgColor.b << ")\n";

    // Count specific colored pixels
    Pixel targetColor(31, 31, 31, 255);  // Dark background color
    uint32_t darkPixels = csv.CountPixelsWithTolerance(targetColor, 10);
    std::cout << "Dark background pixels in region: " << darkPixels << "\n";

    // Get all pixels in region
    auto region = csv.GetPixelRegion(cx - regionSize / 2, cy - regionSize / 2,
                                      regionSize, regionSize);
    std::cout << "Total pixels in region: " << region.size() << "\n";

    // Percentage of dark pixels
    double percentage = (100.0 * darkPixels) / region.size();
    std::cout << "Percentage dark: " << std::fixed << std::setprecision(1)
              << percentage << "%\n";
}

// ============================================================================
// EXAMPLE 5: Validation for Testing
// ============================================================================

void Example_TestValidation() {
    std::cout << "\n=== Example 5: Test Validation ===\n";

    PixelDataCSV csv;
    if (!csv.LoadFromFile("test_outputs/standalone_cubes_frame.csv")) {
        return;
    }

    // Perform validation checks typical in unit tests

    std::cout << "Validation Checks:\n";

    // Check 1: Image dimensions
    bool dimensionsValid = csv.VerifyDimensions(1024, 768);
    std::cout << "  Dimensions are 1024x768: " << (dimensionsValid ? "PASS" : "FAIL")
              << "\n";

    // Check 2: Image is not empty
    bool notEmpty = !csv.IsMostlyEmpty(30);
    std::cout << "  Image has content: " << (notEmpty ? "PASS" : "FAIL") << "\n";

    // Check 3: Image has color variation
    bool hasVariation = csv.HasSignificantVariation(50);
    std::cout << "  Image has variation: " << (hasVariation ? "PASS" : "FAIL")
              << "\n";

    // Check 4: CSV structure is valid
    bool isValid = csv.Validate();
    std::cout << "  CSV structure valid: " << (isValid ? "PASS" : "FAIL") << "\n";

    // Check 5: Expected color exists
    auto centerPixel = csv.GetPixel(512, 384);
    bool hasPixel = centerPixel.has_value();
    std::cout << "  Center pixel exists: " << (hasPixel ? "PASS" : "FAIL") << "\n";

    // Summary
    int passCount = (dimensionsValid ? 1 : 0) + (notEmpty ? 1 : 0) +
                    (hasVariation ? 1 : 0) + (isValid ? 1 : 0) + (hasPixel ? 1 : 0);
    std::cout << "\nOverall: " << passCount << "/5 checks passed\n";
}

// ============================================================================
// EXAMPLE 6: Comparing Regions
// ============================================================================

void Example_RegionComparison() {
    std::cout << "\n=== Example 6: Region Comparison ===\n";

    PixelDataCSV csv;
    if (!csv.LoadFromFile("test_outputs/standalone_cubes_frame.csv")) {
        return;
    }

    // Compare brightness of two regions
    auto topLeftBrightness =
        csv.GetAverageColor(0, 0, 200, 200);
    auto bottomRightBrightness =
        csv.GetAverageColor(csv.width - 200, csv.height - 200, 200, 200);

    std::cout << "Top-left region average brightness: "
              << std::fixed << std::setprecision(1)
              << (0.299 * topLeftBrightness.r + 0.587 * topLeftBrightness.g +
                  0.114 * topLeftBrightness.b)
              << "\n";

    std::cout << "Bottom-right region average brightness: "
              << (0.299 * bottomRightBrightness.r + 0.587 * bottomRightBrightness.g +
                  0.114 * bottomRightBrightness.b)
              << "\n";

    // Compare center regions
    auto centerLeft =
        csv.GetAverageColor(csv.width / 4 - 100, csv.height / 2 - 100, 200, 200);
    auto centerRight =
        csv.GetAverageColor(3 * csv.width / 4 - 100, csv.height / 2 - 100, 200, 200);

    std::cout << "Left-center color: RGB(" << (int)centerLeft.r << ", "
              << (int)centerLeft.g << ", " << (int)centerLeft.b << ")\n";
    std::cout << "Right-center color: RGB(" << (int)centerRight.r << ", "
              << (int)centerRight.g << ", " << (int)centerRight.b << ")\n";
}

// ============================================================================
// EXAMPLE 7: Detailed Statistics
// ============================================================================

void Example_DetailedStatistics() {
    std::cout << "\n=== Example 7: Detailed Statistics ===\n";

    PixelDataCSV csv;
    if (!csv.LoadFromFile("test_outputs/standalone_cubes_frame.csv")) {
        return;
    }

    // Print comprehensive statistics
    std::cout << csv.GetStatisticsString();
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "PNG to CSV Pixel Verification Examples\n";
    std::cout << "======================================\n";

    try {
        Example_BasicImageLoading();
        Example_PixelInspection();
        Example_ImageStatistics();
        Example_RegionAnalysis();
        Example_TestValidation();
        Example_RegionComparison();
        Example_DetailedStatistics();

        std::cout << "\n=== All Examples Complete ===\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
