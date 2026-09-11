#pragma once

#include <string>

namespace sdl3cpp::services::impl {

/// What a proxy model stands in for. GTA renders these into one special
/// pass each -- reflections, the sun's shadow map, water, night lights,
/// weather -- never into the view itself: drawn as scenery,
/// dt1_05_reflproxy smeared a district over its own streets.
enum class Gta5ProxyKind {
    None,  // ordinary scenery, whatever its name: land and LOD proxies
    Reflection,
    Shadow,
    Water,
    Light,
    Rain,
    Smoke,
};

/// The kind of an archetype, from its name: the only thing the extract
/// says about it. "cs2_05_land01_proxy" and "prop_proxy_hat_01" are
/// scenery; "dt1_02_rfl_proxy" and "id1_23_shadproxy" are not.
Gta5ProxyKind ClassifyGta5Proxy(const std::string& archetype);

const char* Gta5ProxyKindName(Gta5ProxyKind kind);

}  // namespace sdl3cpp::services::impl
