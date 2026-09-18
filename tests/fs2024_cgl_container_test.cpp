// A synthetic FBsA container, built with liblzma's own raw encoder
// the way FS2024 packs its CGLs (props 0x6c: lc0 lp2 pb2, no end
// marker, no .lzma header), read back with ReadCglContainer.

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_cgl_container.hpp"

#include <gtest/gtest.h>
#include <lzma.h>

#include <cstring>
#include <filesystem>
#include <fstream>

namespace tools = sdl3cpp::fs2024;

namespace {

std::vector<std::uint8_t> Pack(const std::vector<std::uint8_t>& raw) {
    lzma_options_lzma options{};
    lzma_lzma_preset(&options, 6);
    options.lc = 0; options.lp = 2; options.pb = 2;
    lzma_filter filters[] = {{LZMA_FILTER_LZMA1, &options},
                             {LZMA_VLI_UNKNOWN, nullptr}};
    std::vector<std::uint8_t> out(raw.size() * 2 + 256);
    std::size_t used = 0;
    EXPECT_EQ(lzma_raw_buffer_encode(filters, nullptr, raw.data(),
                                     raw.size(), out.data(), &used,
                                     out.size()), LZMA_OK);
    out.resize(used);
    return out;
}

void Put32(std::vector<std::uint8_t>& b, std::size_t at, std::uint32_t v) {
    std::memcpy(b.data() + at, &v, 4);
}

}  // namespace

TEST(Fs2024CglContainer, ReadsTableAndBothTileKinds) {
    std::vector<std::uint8_t> tile0(3000);
    for (std::size_t i = 0; i < tile0.size(); ++i) tile0[i] = i % 7;
    const std::vector<std::uint8_t> tile1 = {9, 8, 7};  // stored raw
    const auto packed0 = Pack(tile0);
    const std::vector<std::uint16_t> words = {
        2, 3,
        static_cast<std::uint16_t>(packed0.size()),
        static_cast<std::uint16_t>(0x8000 - (packed0.size() - 3)),
        static_cast<std::uint16_t>(tile0.size() - packed0.size()), 0};
    std::vector<std::uint8_t> rawTable(words.size() * 2);
    std::memcpy(rawTable.data(), words.data(), rawTable.size());
    const auto table = Pack(rawTable);

    std::vector<std::uint8_t> file(0x34, 0);
    std::memcpy(file.data(), "FBsA", 4);
    Put32(file, 32, 2);
    Put32(file, 36, 0x34);
    Put32(file, 40, 0x80000000u | static_cast<std::uint32_t>(table.size()));
    file[48] = file[49] = 0x6c;
    file.insert(file.end(), table.begin(), table.end());
    file.insert(file.end(), packed0.begin(), packed0.end());
    file.insert(file.end(), tile1.begin(), tile1.end());
    const auto path = std::filesystem::temp_directory_path() / "cgl.cgl";
    std::ofstream(path, std::ios::binary)
        .write(reinterpret_cast<const char*>(file.data()), file.size());

    const auto cgl = tools::ReadCglContainer(path.string());
    ASSERT_EQ(cgl.tiles.size(), 2u);
    ASSERT_NE(tools::FindCglTile(cgl, 5), nullptr);
    EXPECT_EQ(tools::FindCglTile(cgl, 4), nullptr);
    EXPECT_EQ(tools::ReadCglTile(cgl, cgl.tiles[0]), tile0);
    EXPECT_EQ(tools::ReadCglTile(cgl, *tools::FindCglTile(cgl, 5)), tile1);
}
