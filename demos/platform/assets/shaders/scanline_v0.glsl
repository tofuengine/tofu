vec4 effect(sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
    return pow(sin(screen_coords.y), 4.) * texture2D(texture, texture_coords);
}
