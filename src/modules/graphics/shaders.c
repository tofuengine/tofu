/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/

#include "palettes.h"

#include <strings.h>

static const char *greyscale = 
    "#version 330\n"
    "\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "\n"
    "uniform sampler2D texture0;\n"
    "\n"
    "out vec4 finalColor;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec4 texel = texture(texture0, fragTexCoord)*fragColor;\n"
    "\n"
    "    float v = texel.r * 0.30 + texel.g * 0.59 + texel.b * 0.11;\n"
    "\n"
    "    finalColor = vec4(v, v, v, texel.a);\n"
    "}\n"
"";

static const char *wave = 
    "#version 330\n"
    "\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "\n"
    "uniform sampler2D texture0;\n"
    "uniform float time;\n"
    "\n"
    "out vec4 finalColor;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec2 uv = fragTexCoord.xy;\n"
    "    uv *= 2.0;\n"
    "    uv -= 1.0;\n"
    "    vec3 waveColor = vec3(1.0, 1.0, 1.0);\n"
    "    waveColor *= abs(0.2 / (sin(uv.x + sin(uv.y + time) * 0.1) * 20.0));\n"
    "    vec4 texel = texture(texture0, fragTexCoord) * fragColor;\n"
    "    finalColor = vec4(mix(waveColor.rgb, texel.rgb, 0.5), texel.a);\n"
    "}\n"
"";

const char *graphics_shaders_find(const char *id)
{
    if (strcasecmp(id, "greyscale") == 0) {
        return greyscale;
    }
    if (strcasecmp(id, "wave") == 0) {
        return wave;
    }
    return NULL;
}
