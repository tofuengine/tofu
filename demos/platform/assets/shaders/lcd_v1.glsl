vec4 effect(sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
    vec4 texel = texture2D(texture, texture_coords);

    // Default lcd colour (affects brightness)
    float pb = 0.8;
    vec4 lcd = vec4(pb, pb, pb, 1.0);

    // Change every 1st, 2nd, and 3rd vertical strip to RGB respectively
    int px = int(mod(screen_coords.x, 3.0));
    if (px == 1) lcd.r = 1.0;
    else if (px == 2) lcd.g = 1.0;
    else lcd.b = 1.0;

    // Darken every 3rd horizontal strip for scanline
    float sclV = 0.25;
    if (int(mod(screen_coords.y, 3.0)) == 0) {
        lcd.rgb = vec3(sclV, sclV, sclV);
    }

    return texel * lcd;
}
