#include "services/impl/workflow/graphics/video_encoder_impl.hpp"

extern "C" {
#include <libavutil/error.h>
}

namespace sdl3cpp::services::impl {

std::string AvErrorText(int code) {
    char text[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(code, text, sizeof(text));
    return text;
}

}  // namespace sdl3cpp::services::impl
