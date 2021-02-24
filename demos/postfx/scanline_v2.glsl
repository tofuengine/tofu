const float linecount = 240.0;
const vec4 gradA = vec4(0.0, 0.1, 0.0, 1.0);
const vec4 gradB = vec4(0.2, 0.5, 0.1, 1.0);
const vec4 gradC = vec4(0.9, 1.0, 0.6, 1.0);

// 2D Random
float random (vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

// 2D Noise based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise2D(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    // Smooth Interpolation

    // Cubic Hermine Curve.  Same as SmoothStep()
    vec2 u = f*f*(3.0-2.0*f);
    // u = smoothstep(0.,1.,f);

    // Mix 4 corners percentages
    return mix(a, b, u.x) +
            (c - a)* u.y * (1.0 - u.x) +
            (d - b) * u.x * u.y;
}

float noise(vec2 uv, float factor) {
    vec4 v = vec4(vec3(noise2D(uv + u_time * 0.1 * vec2(9.0, 7.0))), 1.0);
    return factor * v.x + (1.0 - factor);
}

vec4 base(sampler2D texture, vec2 uv) {
    return texture2D(texture, uv + .1 * noise(uv, 1.0) * vec2(0.1, 0.0));
}

float triangle(float phase) {
    phase *= 2.0;
    return 1.0 - abs(mod(phase, 2.0) - 1.0);
}

float scanline(sampler2D texture, vec2 uv, float factor, float contrast) {
    float lum = dot(base(texture, uv).rgb, vec3(0.2, 0.5, 0.3));
    lum *= noise(uv, factor);
    float tri = triangle(uv.y * linecount);
    tri = pow(tri, contrast * (1.0 - lum) + .5);
    return tri * lum;
}

vec4 gradient(float i) {
    i = clamp(i, 0.0, 1.0) * 2.0;
    if (i < 1.0) {
        return (1.0 - i) * gradA + i * gradB;
    } else {
        i -= 1.0;
        return (1.0 - i) * gradB + i * gradC;
    }
}

vec4 vignette(vec2 uv, vec4 at) {
    float dx = 1.3 * abs(uv.x - .5);
    float dy = 1.3 * abs(uv.y - .5);
    return at * (1.0 - dx * dx - dy * dy);
}

vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
    vec2 uv = texture_coords;
    uv.y = floor(uv.y * linecount) / linecount;
    return vignette(uv, gradient(scanline(texture, uv, 0.3, .1)));
}
