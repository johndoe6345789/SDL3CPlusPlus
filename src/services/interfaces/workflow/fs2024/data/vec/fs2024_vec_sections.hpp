#pragma once

#include "services/interfaces/workflow/fs2024/data/vec/fs2024_vec_cursor.hpp"

#include <vector>

namespace sdl3cpp::fs2024 {

/// How a section states its feature count.
enum class VecHeader { Mask, Count32, Count16 };

/// What each feature record holds before its cumulative point count:
/// roads a u32 id, flags, a type and (on a bridge) a level; rail a u24
/// id, flags, (a level) and a type; water perhaps a flags byte; the
/// rest nothing.
enum class VecRecord { Road, Rail, Water, Bare };

/// One section: its header, its records, then all of its points.
std::vector<VecFeature> ReadVecSection(VecCursor& in, VecHeader header,
                                       VecRecord record);

/// The small table between the water and rail sections: u16 rows, and
/// when there are any a u16, three bytes a row and a u16 zero.
void SkipVecExtra(VecCursor& in);

}  // namespace sdl3cpp::fs2024
