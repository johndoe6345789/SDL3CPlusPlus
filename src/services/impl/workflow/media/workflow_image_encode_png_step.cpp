#include "services/interfaces/workflow/media/workflow_image_encode_png_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL.h>
#include <stdexcept>
#include <filesystem>
#include <cstdlib>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowImageEncodePngStep::WorkflowImageEncodePngStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowImageEncodePngStep::GetPluginId() const {
    return "image.encode.png";
}

void WorkflowImageEncodePngStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;

    // --- Resolve I/O keys ---
    const std::string pixelDataKeyKey = resolver.GetRequiredInputKey(step, "pixel_data_key");
    const std::string widthKey        = resolver.GetRequiredInputKey(step, "width");
    const std::string heightKey       = resolver.GetRequiredInputKey(step, "height");
    const std::string outputPathKey   = resolver.GetRequiredInputKey(step, "output_path");
    const std::string outputSuccessKey = resolver.GetRequiredOutputKey(step, "success");

    // --- Read pixel data context key name, then fetch pixel data ---
    const auto* dataKeyPtr = context.TryGet<std::string>(pixelDataKeyKey);
    if (!dataKeyPtr || dataKeyPtr->empty()) {
        throw std::runtime_error(
            "image.encode.png: pixel_data_key input is missing or empty");
    }

    const auto* pixel_data = context.TryGet<std::vector<uint8_t>>(*dataKeyPtr);
    if (!pixel_data || pixel_data->empty()) {
        throw std::runtime_error(
            "image.encode.png: no pixel data found at context key '" + *dataKeyPtr + "'");
    }

    // --- Read width and height ---
    const auto* w_ptr = context.TryGet<uint32_t>(widthKey);
    const auto* h_ptr = context.TryGet<uint32_t>(heightKey);
    if (!w_ptr || !h_ptr || *w_ptr == 0 || *h_ptr == 0) {
        throw std::runtime_error(
            "image.encode.png: width/height not found or zero in context");
    }
    const uint32_t w = *w_ptr;
    const uint32_t h = *h_ptr;

    // Sanity check: pixel data size must match dimensions
    const uint32_t pixel_size = 4;  // ABGR8888
    const uint32_t row_pitch  = w * pixel_size;
    const uint32_t expected   = row_pitch * h;
    if (pixel_data->size() < expected) {
        throw std::runtime_error(
            "image.encode.png: pixel data size (" + std::to_string(pixel_data->size()) +
            ") < expected (" + std::to_string(expected) + ") for " +
            std::to_string(w) + "x" + std::to_string(h));
    }

    // --- Read and resolve output path ---
    const auto* output_path = context.TryGet<std::string>(outputPathKey);
    if (!output_path || output_path->empty()) {
        throw std::runtime_error("image.encode.png: output_path input is missing or empty");
    }

    std::string resolved_path = *output_path;

    // Resolve ~ to HOME
    if (!resolved_path.empty() && resolved_path[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) {
            resolved_path = std::string(home) + resolved_path.substr(1);
        }
    }

    // Create output directory if needed
    std::filesystem::path path(resolved_path);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    // Replace .png extension with .bmp (SDL only supports BMP natively)
    std::string save_path = resolved_path;
    if (save_path.size() > 4 &&
        save_path.substr(save_path.size() - 4) == ".png") {
        save_path = save_path.substr(0, save_path.size() - 4) + ".bmp";
    }

    // --- Create SDL surface from raw pixel data and save ---
    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        static_cast<int>(w), static_cast<int>(h),
        SDL_PIXELFORMAT_ABGR8888,
        const_cast<void*>(static_cast<const void*>(pixel_data->data())),
        static_cast<int>(row_pitch));

    if (!surface) {
        if (logger_) {
            logger_->Error("image.encode.png: SDL_CreateSurfaceFrom failed: " +
                           std::string(SDL_GetError()));
        }
        context.Set(outputSuccessKey, false);
        return;
    }

    if (!SDL_SaveBMP(surface, save_path.c_str())) {
        if (logger_) {
            logger_->Error("image.encode.png: SDL_SaveBMP failed: " +
                           std::string(SDL_GetError()));
        }
        SDL_DestroySurface(surface);
        context.Set(outputSuccessKey, false);
        return;
    }

    SDL_DestroySurface(surface);
    context.Set(outputSuccessKey, true);

    if (logger_) {
        logger_->Info("image.encode.png: Saved " +
                      std::to_string(w) + "x" + std::to_string(h) +
                      " image to " + save_path);
    }
}

}  // namespace sdl3cpp::services::impl
