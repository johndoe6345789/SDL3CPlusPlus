#include "services/interfaces/workflow/gta5/resource/gta5_resource.hpp"

#include <zlib.h>

#include <array>
#include <fstream>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint32_t kRsc7Magic = 0x37435352u;  // "RSC7"

/// A page-size flag word as bytes. Each bit group counts pages of one
/// size class, and the low nibble selects the base page size. The two
/// words' totals must equal the inflated payload exactly.
std::uint64_t PageBytes(std::uint32_t f) {
    const std::uint64_t pages =
        (((f >> 27) & 0x1u) << 0) + (((f >> 26) & 0x1u) << 1) +
        (((f >> 25) & 0x1u) << 2) + (((f >> 24) & 0x1u) << 3) +
        (((f >> 17) & 0x7Fu) << 4) + (((f >> 11) & 0x3Fu) << 5) +
        (((f >> 7) & 0xFu) << 6) + (((f >> 5) & 0x3u) << 7) +
        (((f >> 4) & 0x1u) << 8);
    return (std::uint64_t{0x200} << (f & 0xFu)) * pages;
}

/// Raw deflate (no zlib header) into `dst`, reading the file in chunks
/// only until `dst` is full -- which is what lets systemOnly skip the
/// graphics pages without reading them off disk.
bool InflateInto(std::ifstream& in, std::vector<std::uint8_t>& dst) {
    z_stream z{};
    if (inflateInit2(&z, -15) != Z_OK) return false;
    z.next_out = dst.data();
    z.avail_out = static_cast<uInt>(dst.size());
    std::vector<char> chunk(1u << 16);
    int rc = Z_OK;
    while (z.avail_out > 0 && rc == Z_OK) {
        if (z.avail_in == 0) {
            in.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
            z.avail_in = static_cast<uInt>(in.gcount());
            z.next_in = reinterpret_cast<Bytef*>(chunk.data());
            if (z.avail_in == 0) break;
        }
        rc = inflate(&z, Z_NO_FLUSH);
    }
    const bool complete = z.avail_out == 0;
    inflateEnd(&z);
    return complete;
}

}  // namespace

bool LoadGta5Resource(const std::string& path, Gta5Resource& out,
                      bool systemOnly) {
    std::ifstream in(path, std::ios::binary);
    std::array<std::uint32_t, 4> header{};
    in.read(reinterpret_cast<char*>(header.data()), sizeof(header));
    if (!in || header[0] != kRsc7Magic) return false;

    out.version = header[1];
    const std::uint64_t system = PageBytes(header[2]);
    const std::uint64_t total = system + PageBytes(header[3]);
    const std::uint64_t wanted = systemOnly ? system : total;
    if (wanted == 0 || total > (std::uint64_t{1} << 31)) return false;

    out.systemSize = static_cast<std::uint32_t>(system);
    out.data.assign(static_cast<std::size_t>(wanted), 0);
    return InflateInto(in, out.data);
}

}  // namespace sdl3cpp::services::impl
