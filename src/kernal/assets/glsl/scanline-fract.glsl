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
 * Super-basic scanline shader
 * (looks bad at non-integer scales)
 * by hunterk
 * license: public domain
 */

//#pragma parameter THICKNESS "Scanline Thickness" 2.0 1.0 12.0 1.0
//#pragma parameter DARKNESS "Scanline Darkness" 0.50 0.0 1.0 0.05
//#pragma parameter BRIGHTBOOST "Scanline Boost Bright" 1.1 1.0 1.2 0.1

#define THICKNESS 1.0
#define DARKNESS 0.5
#define BRIGHTBOOST 1.1

vec3 RGBtoYIQ(vec3 RGB){
	const mat3 yiqmat = mat3(
		0.2989, 0.5870, 0.1140,
		0.5959, -0.2744, -0.3216,
		0.2115, -0.5229, 0.3114);
	return RGB * yiqmat;
}

vec3 YIQtoRGB(vec3 YIQ){
	const mat3 rgbmat = mat3(
		1.0, 0.956, 0.6210,
		1.0, -0.2720, -0.6474,
		1.0, -1.1060, 1.7046);
	return YIQ * rgbmat;
}

vec4 effect(sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
	float lines = fract(texture_coords.y * u_texture_size.y);
	float scale_factor = floor((u_screen_size.y / u_texture_size.y) + 0.4999);
	vec4 screen = texture2D(texture, texture_coords * 1.0004);
	screen.rgb = RGBtoYIQ(screen.rgb);
	screen.r *= BRIGHTBOOST;
	screen.rgb = YIQtoRGB(screen.rgb);
    return (lines > (1.0 / scale_factor * THICKNESS)) ? screen : screen * vec4(1.0 - DARKNESS);
} 
