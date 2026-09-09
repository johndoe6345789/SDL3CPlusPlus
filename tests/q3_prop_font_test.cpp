// Proportional font measurement. The skip test used to compare a
// glyph's width against the space width, which silently dropped every
// glyph that happened to be that wide: ' ', '(', 'I' and 'i'.

#include "services/interfaces/workflow/quake3/q3_overlay_utils.hpp"

#include <gtest/gtest.h>

using sdl3cpp::q3overlay::PropStringWidth;
using sdl3cpp::q3overlay::kPropMap;
using sdl3cpp::q3overlay::kPropGap;
using sdl3cpp::q3overlay::kPropSpace;

TEST(PropFont, TheNarrowGlyphsHaveRealWidths) {
    for (const char ch : {'(', 'I', 'i'}) {
        EXPECT_GT(kPropMap[static_cast<int>(ch)][2], 0)
            << "glyph '" << ch << "' should be drawable";
    }
}

TEST(PropFont, EveryLetterContributesWidth) {
    // "EXIT" must be wider than "EXT" by the I plus a gap. It was not,
    // because I measured as a space.
    const float withI = PropStringWidth("EXIT");
    const float withoutI = PropStringWidth("EXT");
    EXPECT_GT(withI, withoutI);
    EXPECT_NEAR(withI - withoutI,
                kPropMap[static_cast<int>('I')][2] + kPropGap, 0.01f);
}

TEST(PropFont, SpaceUsesTheSpaceWidth) {
    const float twoWords = PropStringWidth("A A");
    const float joined = PropStringWidth("AA");
    EXPECT_NEAR(twoWords - joined, kPropSpace, 0.01f);
}

TEST(PropFont, EmptyAndNullMeasureZero) {
    EXPECT_FLOAT_EQ(PropStringWidth(""), 0.0f);
    EXPECT_FLOAT_EQ(PropStringWidth(nullptr), 0.0f);
}

TEST(PropFont, WidthGrowsWithLength) {
    EXPECT_LT(PropStringWidth("I"), PropStringWidth("II"));
}
