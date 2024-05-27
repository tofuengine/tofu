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
/*
 * Scanlines Sine Absolute Value
 * An ultra light scanline shader
 * by RiskyJumps
 * license: public domain
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

vec4 effect(sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
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
