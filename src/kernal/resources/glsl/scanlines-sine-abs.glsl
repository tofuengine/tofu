/*
    Scanlines Sine Absolute Value
    An ultra light scanline shader
    by RiskyJumps
	license: public domain
*/

//#pragma parameter amp          "Amplitude"      1.2500  0.000 2.000 0.05
//#pragma parameter phase        "Phase"          0.5000  0.000 2.000 0.05
//#pragma parameter lines_black  "Lines Blacks"   0.0000  0.000 1.000 0.05
//#pragma parameter lines_white  "Lines Whites"   1.0000  0.000 2.000 0.05
 
#define freq             0.500000
#define offset           0.000000
#define pi               3.141592654

#define amp              1.250000
#define phase            0.500000
#define lines_black      0.000000
#define lines_white      1.000000

vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
    vec3 pixel = texture2D(texture, texture_coords).rgb;

    float omega = 2.0 * pi * freq; // Angular frequency
    float angle = texture_coords.y * omega * u_texture_size.y + phase;

    float lines;
    lines = sin(angle);
    lines *= amp;
    lines += offset;
    lines = abs(lines);
    lines *= lines_white - lines_black;
    lines += lines_black;
    pixel *= lines;
 
    return vec4(pixel.rgb, 1.0);
}
