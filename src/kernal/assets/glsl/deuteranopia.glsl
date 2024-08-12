/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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

// Deuteranopia (Red-Green Color Blindness)
// (https://www.color-blindness.com/deuteranopia-red-green-color-blindness/)
//
//   - similar to protanopia;
//   - most common form of color-blindness (affects ~6% of the male population);
//   - only GREEN and BLUE cones are present, *but* GREEN cones are malfunctioning;
//   - total absence (or heavily reduced) of RED retinal receptors;
//   - reds appears as yellowish-orange, purple appears as blue, green as light yellow;
//   - sexually linked trait, males are more affected (8% males, 0.6% females).

const mat3 m = mat3(
        // http://mkweb.bcgsc.ca/colorblind/math.mhtml
        0.33066007, 0.33066007, -0.02785538,
        0.66933993, 0.66933993,  0.02785538,
        0.00000000, 0.00000000,  1.00000000
    );

vec4 effect(sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
    vec3 pixel = texture2D(texture, texture_coords).rgb;
    return vec4(m * pixel, 1.0);
}
