const float thickness = 3.0;

vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
    vec2 delta = texture_coords - vec2(0.5, 0.5);
    vec2 uv_r = delta * 0.0250 + texture_coords;
    vec2 uv_g = delta * 0.0075 + texture_coords;
    vec2 uv_b = delta * 0.0150 + texture_coords;
    vec4 r = texture2D(texture, uv_r);
    vec4 g = texture2D(texture, uv_g);
    vec4 b = texture2D(texture, uv_b);
    vec4 texel = vec4(r.r, g.g, b.b, 1.0);
    float y = (cos(u_time * 1.0) + 1) * 0.5 * u_screen_size.y;
    float d = abs(y - screen_coords.y);
    if (d > thickness) {
        return texel;
    } else {
        return mix(texel, vec4(1.0, 1.0, 1.0, 1.0), 1.0 - d / thickness);
    }
}
