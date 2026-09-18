// A roof FS2024 sampled from imagery keeps its own colour, the roof
// tile texture lending only its pattern. `tint` is the vertex's packed
// colour: x 5-bit r | g << 5 | b << 10, y 1 when there is one.
vec3 roofColour(vec3 albedo, vec2 tint) {
    if (tint.y < 0.5) return albedo;
    uint packed = uint(tint.x + 0.5);
    vec3 own = vec3(packed & 31u, (packed >> 5) & 31u,
                    (packed >> 10) & 31u) / 31.0;
    float pattern = dot(albedo, vec3(0.2126, 0.7152, 0.0722));
    return pow(own, vec3(2.2)) * clamp(pattern / 0.3, 0.6, 1.3);
}
