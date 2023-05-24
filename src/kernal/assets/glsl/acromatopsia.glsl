/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
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
 */

// Acromatopsia (Rod Monochromacy)
//
//   - very rare, affects only 0.003% population;
//   - total color blindness;
//   - not sexually linked trait;
//   - can be acquired (due to diencephalon or cerebral cortex damage) or congenital.
const mat3 m = mat3(
        // http://mkweb.bcgsc.ca/colorblind/math.mhtml
        0.2126, 0.2126, 0.2126,
        0.7152, 0.7152, 0.7152,
        0.0722, 0.0722, 0.0722
    );

vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
    vec3 pixel = texture2D(texture, texture_coords).rgb;
    return vec4(m * pixel, 1.0);
}
