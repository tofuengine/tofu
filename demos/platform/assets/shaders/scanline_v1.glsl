const float amount = 0.5;
const float thickness = 1.0;
const float spacing = 1.0;

float round(float x) {
    return floor(x + 0.5);
}

vec4 effect(sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
    vec4 texel = texture2D(texture, texture_coords) * color;
    if (mod(screen_coords.y, round(thickness + spacing)) < round(spacing)) {
        return vec4(texel.rgb * (1.0 - amount), texel.a);
    }
    return texel;
}
