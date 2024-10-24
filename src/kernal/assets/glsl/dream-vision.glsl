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

vec4 effect(sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
    vec4 pixel = texture2D(texture, texture_coords);

    pixel += texture2D(texture, texture_coords + 0.001);
    pixel += texture2D(texture, texture_coords + 0.003);
    pixel += texture2D(texture, texture_coords + 0.005);
    pixel += texture2D(texture, texture_coords + 0.007);
    pixel += texture2D(texture, texture_coords + 0.009);
    pixel += texture2D(texture, texture_coords + 0.011);

    pixel += texture2D(texture, texture_coords - 0.001);
    pixel += texture2D(texture, texture_coords - 0.003);
    pixel += texture2D(texture, texture_coords - 0.005);
    pixel += texture2D(texture, texture_coords - 0.007);
    pixel += texture2D(texture, texture_coords - 0.009);
    pixel += texture2D(texture, texture_coords - 0.011);

    pixel.rgb = vec3((pixel.r + pixel.g + pixel.b)/3.0);
    pixel = pixel/9.5;

    return pixel;
} 
