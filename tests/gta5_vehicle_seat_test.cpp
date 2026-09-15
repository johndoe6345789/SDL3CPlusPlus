// Which car the player is sitting in, across a removal.
//
// state.seated is a position in state.vehicles, and RemoveGta5Vehicle
// shortens that list under it. It used to clear the seat only when the
// car being taken away *was* the seated one, so taking away any car
// below it left the index naming whatever had slid down into the slot
// -- the player silently driving a different car, with nothing to see.
// Gta5SeatAfterRemoval is that reasoning on its own, and this is it
// pinned: only its own car going puts the player out.

#include "services/interfaces/workflow/gta5/vehicle/gta5_seat_index.hpp"

#include <gtest/gtest.h>

namespace sdl3cpp::services::impl {
namespace {

// The car the player is in is taken away: they are on foot.
TEST(Gta5SeatAfterRemoval, TheirOwnCarGoingPutsThemOut) {
    EXPECT_EQ(Gta5SeatAfterRemoval(1, 1u), -1);
    EXPECT_EQ(Gta5SeatAfterRemoval(0, 0u), -1);
}

// One below it goes: the seat follows its car down a place, so it is
// still the same car.
TEST(Gta5SeatAfterRemoval, OneBelowSlidesTheSeatDown) {
    EXPECT_EQ(Gta5SeatAfterRemoval(2, 0u), 1);
    EXPECT_EQ(Gta5SeatAfterRemoval(2, 1u), 1);
    EXPECT_EQ(Gta5SeatAfterRemoval(7, 3u), 6);
}

// One above it goes: the seat is where it was.
TEST(Gta5SeatAfterRemoval, OneAboveLeavesTheSeatAlone) {
    EXPECT_EQ(Gta5SeatAfterRemoval(0, 2u), 0);
    EXPECT_EQ(Gta5SeatAfterRemoval(3, 4u), 3);
}

// On foot stays on foot, whichever car goes.
TEST(Gta5SeatAfterRemoval, OnFootIsLeftOnFoot) {
    EXPECT_EQ(Gta5SeatAfterRemoval(-1, 0u), -1);
    EXPECT_EQ(Gta5SeatAfterRemoval(-1, 9u), -1);
}

// Emptying a list from the back -- what the shop does when it takes the
// old car away -- puts the player out wherever they were sitting, and
// never leaves the seat pointing past the end on the way there.
TEST(Gta5SeatAfterRemoval, ClearingFromTheBackAlwaysEndsOnFoot) {
    for (int sat = 0; sat < 4; ++sat) {
        int seat = sat;
        for (std::size_t left = 4u; left-- > 0u;) {
            seat = Gta5SeatAfterRemoval(seat, left);
            EXPECT_LT(seat, static_cast<int>(left));
        }
        EXPECT_EQ(seat, -1);
    }
}

}  // namespace
}  // namespace sdl3cpp::services::impl
