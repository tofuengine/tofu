vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
    return texture2D(texture, texture_coords) * color;
}
