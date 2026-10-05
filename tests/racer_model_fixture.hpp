#pragma once

#include "services/interfaces/workflow/racer/data/racer_model.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace racer_test {

struct Builder {
    std::vector<std::uint8_t> bytes = std::vector<std::uint8_t>(0x1B0, 0);
    void U32(std::size_t at, std::uint32_t v) {
        for (int k = 0; k < 4; ++k) bytes[at + k] = v >> (24 - 8 * k);
    }
    void U16(std::size_t at, int v) {
        bytes[at] = static_cast<std::uint8_t>(v >> 8);
        bytes[at + 1] = static_cast<std::uint8_t>(v);
    }
    void F32(std::size_t at, float v) {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &v, 4);
        U32(at, bits);
    }
};

/// 'Trak' -> transformed node (+100 on x) -> mesh group -> one mesh
/// with a 4-bit 32x16 texture (index 7) and one triangle.
inline std::vector<std::uint8_t> OneTriangleModel() {
    Builder b;
    b.U32(0x00, 0x5472616B);  // 'Trak'
    b.U32(0x04, 0x10);        // node list: one node
    b.U32(0x08, 0xFFFFFFFF);
    b.U32(0x10, 0xD064);      // transformed node
    b.U32(0x10 + 0x14, 1);
    b.U32(0x10 + 0x18, 0x60);
    const float m[12] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 100, 0, 0};
    for (int i = 0; i < 12; ++i) b.F32(0x2C + 4 * i, m[i]);
    b.U32(0x60, 0x70);
    b.U32(0x70, 0x3064);      // mesh group
    b.U32(0x70 + 0x14, 1);
    b.U32(0x70 + 0x18, 0xB0);
    b.U32(0xB0, 0xC0);
    b.U32(0xC0, 0x100);       // mesh: material, commands, vertices
    b.U32(0xC0 + 0x30, 0x150);
    b.U32(0xC0 + 0x34, 0x180);
    b.U16(0xC0 + 0x3A, 3);
    b.U32(0x100 + 0x08, 0x110);
    b.U16(0x110 + 0x0C, 0x0200);
    b.U16(0x110 + 0x10, 32);
    b.U16(0x110 + 0x12, 16);
    b.U32(0x110 + 0x38, 0x0A000007);
    b.U32(0x150, 0x01003006);  // gSPVertex n=3 v0=0
    b.U32(0x154, 0x180);
    b.U32(0x158, 0x05000204);  // gSP1Triangle 0 1 2
    b.U32(0x160, 0xDF000000);
    const int v[3][5] = {{0, 0, 0, 0, 0}, {10, 0, 0, 4096, 0},
                         {0, 10, 0, 0, 4096}};
    for (int i = 0; i < 3; ++i) {
        const std::size_t at = 0x180 + 16 * i;
        for (int k = 0; k < 3; ++k) b.U16(at + 2 * k, v[i][k]);
        b.U16(at + 8, v[i][3]);
        b.U16(at + 10, v[i][4]);
        b.U32(at + 12, 0xFF8040FF);
    }
    return b.bytes;
}

}  // namespace racer_test
