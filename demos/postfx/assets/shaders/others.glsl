var SCALINES_v0 = "\n" +
    "vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {\n" +
    "    return pow(sin(screen_coords.y), 4.) * texture2D(texture, texture_coords);\n" +
    "}\n"

var SCALINES_v1 = "\n" +
    "const float amount = 0.5;\n" +
    "const float thickness = 1.0;\n" +
    "const float spacing = 1.0;\n" +
    "\n" +
    "vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {\n" +
    "    vec4 texel = texture2D(texture, texture_coords) * color;\n" +
    "    if (mod(screen_coords.y, round(thickness + spacing)) < round(spacing)) {\n" +
	"        return vec4(texel.rgb * (1.0 - amount), texel.a);\n" +
    "    }\n" +
    "    return texel;\n" +
    "}\n"

var LCD = "\n" +
    "vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {\n" +
    "    vec4 texel = texture2D(texture, texture_coords);\n" +
    "\n" +
    "    // Default lcd colour (affects brightness)\n" +
    "    float pb = 0.8;\n" +
    "    vec4 lcd = vec4(pb, pb, pb, 1.0);\n" +
    "\n" +
    "    // Change every 1st, 2nd, and 3rd vertical strip to RGB respectively\n" +
    "    int px = int(mod(screen_coords.x, 3.0));\n" +
    "    if (px == 1) lcd.r = 1.0;\n" +
    "    else if (px == 2) lcd.g = 1.0;\n" +
    "    else lcd.b = 1.0;\n" +
    "\n" +
    "    // Darken every 3rd horizontal strip for scanline\n" +
    "    float sclV = 0.25;\n" +
    "    if (int(mod(screen_coords.y, 3.0)) == 0) {\n" +
    "        lcd.rgb = vec3(sclV, sclV, sclV);\n" +
    "    }\n" +
    "\n" +
    "    return texel * lcd;\n" +
    "}\n"

var LCD_v1 = "\n" +
    "vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {\n" +
    "    vec4 texel = texture2D(texture, texture_coords);\n" +
    "\n" +
    "    // Default lcd colour (affects brightness)\n" +
    "    float pb = 0.8;\n" +
    "    vec4 lcd = vec4(pb, pb, pb, 1.0);\n" +
    "\n" +
    "    // Change every 1st, 2nd, and 3rd vertical strip to RGB respectively\n" +
    "    int px = int(mod(screen_coords.x, 3.0));\n" +
    "    if (px == 1) lcd.rgb *= vec3(1.0, 0.0, 0.0);\n" +
    "    else if (px == 2) lcd.rgb *= vec3(0.0, 1.0, 0.0);\n" +
    "    else lcd.rgb *= vec3(0.0, 0.0, 1.0);\n" +
    "\n" +
    "    // Darken every 3rd horizontal strip for scanline\n" +
    "    float sclV = 0.25;\n" +
    "    if (int(mod(screen_coords.y, 3.0)) == 0) {\n" +
    "        lcd.rgb *= sclV;\n" +
    "    }\n" +
    "\n" +
    "    return texel * lcd;\n" +
    "}\n"

var BARREL = "\n" +
    "const float thickness = 3.0;\n" +
    "\n" +
    "vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {\n" +
    "    vec2 delta = texture_coords - vec2(0.5, 0.5);\n" +
    "    vec2 uv_r = delta * 0.0250 + texture_coords;\n" +
    "    vec2 uv_g = delta * 0.0075 + texture_coords;\n" +
    "    vec2 uv_b = delta * 0.0150 + texture_coords;\n" +
    "    vec4 r = texture2D(texture, uv_r);\n" +
    "    vec4 g = texture2D(texture, uv_g);\n" +
    "    vec4 b = texture2D(texture, uv_b);\n" +
    "    vec4 texel = vec4(r.r, g.g, b.b, 1.0)\n;" +
    "    float y = (cos(u_time * 1.0) + 1) * 0.5 * u_resolution.y;\n" +
    "    float d = abs(y - screen_coords.y);\n" +
    "    if (d > thickness) {\n" +
    "        return texel;\n" +
    "    } else {\n" +
    "        return mix(texel, vec4(1.0, 1.0, 1.0, 1.0), 1.0 - d / thickness);\n" +
    "    }\n" +
    "}\n"

var SCALINES = "\n" +
"    const float linecount = 240.0;\n" +
"    const vec4 gradA = vec4(0.0, 0.1, 0.0, 1.0);\n" +
"    const vec4 gradB = vec4(0.2, 0.5, 0.1, 1.0);\n" +
"    const vec4 gradC = vec4(0.9, 1.0, 0.6, 1.0);\n" +
"\n" +
"    // 2D Random\n" +
"    float random (vec2 st) {\n" +
"        return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);\n" +
"    }\n" +
"\n" +
"    // 2D Noise based on Morgan McGuire @morgan3d\n" +
"    // https://www.shadertoy.com/view/4dS3Wd\n" +
"    float noise2D(vec2 st) {\n" +
"        vec2 i = floor(st);\n" +
"        vec2 f = fract(st);\n" +
"\n" +
"        // Four corners in 2D of a tile\n" +
"        float a = random(i);\n" +
"        float b = random(i + vec2(1.0, 0.0));\n" +
"        float c = random(i + vec2(0.0, 1.0));\n" +
"        float d = random(i + vec2(1.0, 1.0));\n" +
"\n" +
"        // Smooth Interpolation\n" +
"\n" +
"        // Cubic Hermine Curve.  Same as SmoothStep()\n" +
"        vec2 u = f*f*(3.0-2.0*f);\n" +
"        // u = smoothstep(0.,1.,f);\n" +
"\n" +
"        // Mix 4 coorners percentages\n" +
"        return mix(a, b, u.x) +\n" +
"                (c - a)* u.y * (1.0 - u.x) +\n" +
"                (d - b) * u.x * u.y;\n" +
"    }\n" +
"\n" +
"    float noise(vec2 uv, float factor) {\n" +
"        vec4 v = vec4(vec3(noise2D(uv + u_time * 0.1 * vec2(9.0, 7.0))), 1.0);\n" +
"        return factor * v.x + (1.0 - factor);\n" +
"    }\n" +
"\n" +
"    vec4 base(sampler2D texture, vec2 uv) {\n" +
"        return texture2D(texture, uv + .1 * noise(uv, 1.0) * vec2(0.1, 0.0));\n" +
"    }\n" +
"\n" +
"    float triangle(float phase) {\n" +
"        phase *= 2.0;\n" +
"        return 1.0 - abs(mod(phase, 2.0) - 1.0);\n" +
"    }\n" +
"\n" +
"    float scanline(sampler2D texture, vec2 uv, float factor, float contrast) {\n" +
"        float lum = dot(base(texture, uv).rgb, vec3(0.2, 0.5, 0.3));\n" +
"        lum *= noise(uv, factor);\n" +
"        float tri = triangle(uv.y * linecount);\n" +
"        tri = pow(tri, contrast * (1.0 - lum) + .5);\n" +
"        return tri * lum;\n" +
"    }\n" +
"\n" +
"    vec4 gradient(float i) {\n" +
"        i = clamp(i, 0.0, 1.0) * 2.0;\n" +
"        if (i < 1.0) {\n" +
"            return (1.0 - i) * gradA + i * gradB;\n" +
"        } else {\n" +
"            i -= 1.0;\n" +
"            return (1.0 - i) * gradB + i * gradC;\n" +
"        }\n" +
"    }\n" +
"\n" +
"    vec4 vignette(vec2 uv, vec4 at) {\n" +
"        float dx = 1.3 * abs(uv.x - .5);\n" +
"        float dy = 1.3 * abs(uv.y - .5);\n" +
"        return at * (1.0 - dx * dx - dy * dy);\n" +
"    }\n" +
"\n" +
"    vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {\n" +
"        vec2 uv = texture_coords;\n" +
"        uv.y = floor(uv.y * linecount) / linecount;\n" +
"        return vignette(uv, gradient(scanline(texture, uv, 0.3, .1)));\n" +
"    }\n"
