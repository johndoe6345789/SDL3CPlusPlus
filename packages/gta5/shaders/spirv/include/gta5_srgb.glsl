// GTA's colour maps hold display values, as nearly every game's do: the
// lighting needs them linear, and the composite encodes once at the end.
// Lit as they were, every surface came out pale and flat.
vec3 SrgbToLinear(vec3 c) {
    vec3 lo = c / 12.92;
    vec3 hi = pow((c + 0.055) / 1.055, vec3(2.4));
    return mix(lo, hi, step(vec3(0.04045), c));
}
