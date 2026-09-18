#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_lzma_raw.hpp"

#include <lzma.h>

#include <stdexcept>

namespace sdl3cpp::fs2024 {
namespace {

/// Runs a raw LZMA1 decoder over the whole input, growing `out` when
/// `grow` is set. A raw stream without an end marker never reports
/// LZMA_STREAM_END: running out of input (or filling a known-size
/// buffer) is the real "done", and whatever error the decoder raises
/// for its missing end marker afterwards is not a failure.
std::size_t Decode(const std::uint8_t* data, std::size_t size,
                   std::uint8_t props, std::vector<std::uint8_t>& out,
                   bool grow) {
    lzma_options_lzma options{};
    options.dict_size = LZMA_DICT_SIZE_DEFAULT;
    options.lc = props % 9;
    options.lp = (props / 9) % 5;
    options.pb = props / 45;
    lzma_filter filters[] = {{LZMA_FILTER_LZMA1, &options},
                             {LZMA_VLI_UNKNOWN, nullptr}};
    lzma_stream stream = LZMA_STREAM_INIT;
    if (lzma_raw_decoder(&stream, filters) != LZMA_OK) {
        throw std::runtime_error("LZMA: cannot start raw decoder");
    }
    stream.next_in = data;
    stream.avail_in = size;
    std::size_t produced = 0;
    lzma_ret ret = LZMA_OK;
    while (ret == LZMA_OK) {
        if (produced == out.size()) {
            if (!grow) break;
            out.resize(out.size() * 2 + 4096);
        }
        const std::size_t before = produced, inBefore = stream.avail_in;
        stream.next_out = out.data() + produced;
        stream.avail_out = out.size() - produced;
        ret = lzma_code(&stream, LZMA_RUN);
        produced = out.size() - stream.avail_out;
        if (produced == before && stream.avail_in == inBefore) break;
    }
    lzma_end(&stream);
    if (ret == LZMA_DATA_ERROR && produced == 0) {
        throw std::runtime_error("LZMA: corrupt stream");
    }
    return produced;
}

}  // namespace

std::vector<std::uint8_t> DecodeLzmaRaw(const std::uint8_t* data,
                                        std::size_t size,
                                        std::uint8_t props,
                                        std::size_t outputSize) {
    std::vector<std::uint8_t> out(outputSize);
    if (Decode(data, size, props, out, false) != outputSize) {
        throw std::runtime_error("LZMA: stream ended short");
    }
    return out;
}

std::vector<std::uint8_t> DecodeLzmaRawAll(const std::uint8_t* data,
                                           std::size_t size,
                                           std::uint8_t props) {
    std::vector<std::uint8_t> out;
    out.resize(Decode(data, size, props, out, true));
    return out;
}

}  // namespace sdl3cpp::fs2024
