#pragma once

#include <cstddef>

namespace sdl3cpp::services::impl {

/// Where the seat points after the car at `index` is taken out of the
/// list it indexes into.
///
/// The seat is a position, not a handle, so shortening the list under
/// it moves whatever it names. Its own car going means the player is
/// out; a car below it going slides their car down one; a car above it
/// changes nothing. Getting this wrong is silent -- the player simply
/// finds themselves driving a different car -- which is why it is its
/// own function with its own tests.
inline int Gta5SeatAfterRemoval(int seated, std::size_t index) {
    const int gone = static_cast<int>(index);
    if (seated == gone) return -1;
    return seated > gone ? seated - 1 : seated;
}

}  // namespace sdl3cpp::services::impl
