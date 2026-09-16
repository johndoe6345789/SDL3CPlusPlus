// Runway markings, drawn analytically in runway metres.
//
// The ground map is 2 m a texel, and a 0.9 m centreline or a 1.8 m
// threshold stripe baked into it is a blurred blob up close and a
// checkerboard at a distance. Here each marking is a box in runway
// space, its edges smoothed over one pixel's footprint (fwidth), so it
// is sharp underfoot and still reads at a kilometre.
//
// Layout is ICAO Annex 14 for a 45 m precision runway: 0.9 m side and
// centre stripes (30 m dashes, 20 m gaps), twelve 30 m threshold
// stripes starting 6 m in, and aiming point blocks 400 m in.
// Needs u_runway and u_runwayAxis from FragmentUniforms.

const vec3 RUNWAY_ASPHALT = vec3(0.045, 0.045, 0.05);
const vec3 RUNWAY_PAINT = vec3(0.78, 0.78, 0.76);

// 1 inside [lo, hi], 0 outside, with edges one footprint `w` wide.
float band(float x, float lo, float hi, float w) {
    return clamp(min(x - lo, hi - x) / w + 0.5, 0.0, 1.0);
}

// Where marks are much finer than a pixel, fade them to half: what is
// left is shimmer, and their average brightness is about that anyway.
float settle(float mark, float w, float size) {
    return mix(mark, 0.5 * mark, smoothstep(0.5, 2.0, w / size));
}

// How much paint covers runway point (along, across); along is metres
// from the centre towards the heading, across is metres to its right.
float runwayPaint(float along, float across, float w) {
    vec2 half_ = u_runway.zw;
    float fromEnd = half_.x - abs(along);  // metres in from a threshold
    float side = abs(across);

    float edge = band(side, half_.y - 0.9, half_.y, w);

    float dashAt = mod(along + half_.x - 60.0, 50.0);
    float dashes = band(dashAt, 0.0, 30.0, w) *
                   band(fromEnd, 60.0, 1e6, w) * band(side, -0.45, 0.45, w);

    // Six stripes a side, 1.8 m wide on a 3.4 m pitch.
    float slot = side - 1.7;
    float stripe = band(mod(slot, 3.4), 0.0, 1.8, w) *
                   band(slot, 0.0, 5.0 * 3.4 + 1.8, w) *
                   band(fromEnd, 6.0, 36.0, w);

    float aiming = band(side, 9.5, 18.5, w) * band(fromEnd, 400.0, 445.0, w);

    float paint = max(edge, max(dashes, max(stripe, aiming)));
    return settle(paint, w, 0.9);
}

// The runway's colour here, and in `inside` how much of this pixel it
// covers (0 off the runway).
vec3 runwaySurface(vec3 worldPos, out float inside) {
    vec2 d = worldPos.xz - u_runway.xy;
    vec2 axis = u_runwayAxis.xy;
    float along = dot(d, axis);
    float across = dot(d, vec2(-axis.y, axis.x));
    // Derivatives before any branch, where they are well defined.
    float w = max(max(fwidth(along), fwidth(across)), 1e-4);

    inside = 0.0;
    if (u_runway.z <= 0.0) return RUNWAY_ASPHALT;
    inside = band(abs(along), -1.0, u_runway.z, w) *
             band(abs(across), -1.0, u_runway.w, w);
    float paint = runwayPaint(along, across, w);
    return mix(RUNWAY_ASPHALT, RUNWAY_PAINT, paint);
}
