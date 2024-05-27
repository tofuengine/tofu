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
 * zfast_lcd_standard - A very simple LCD shader meant to be used at 1080p
 *                      on the raspberry pi 3.
 *
 * Copyright (C) 2017 Greg Hogan (SoltanGris42)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Notes: This shader just does nearest neighbor scaling of the game and then
 * darkens the border pixels to imitate an LCD screen. You can change the
 * amount of darkening and the thickness of the borders.  You can also 
 * do basic gamma adjustment.
 */

//#pragma parameter BORDERMULT "Border Multiplier" 14.0 -40.0 40.0 1.0
//#pragma parameter GBAGAMMA "GBA Gamma Hack" 1.0 0.0 1.0 1.0
#define BORDERMULT 14.0
#define GBAGAMMA 1.0

vec4 effect(sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
	vec2 texcoordInPixels = texture_coords * u_texture_size;
	vec2 centerCoord = floor(texcoordInPixels.xy) + vec2(0.5, 0.5);
	vec2 distFromCenter = abs(centerCoord - texcoordInPixels);

	float Y = max(distFromCenter.x, distFromCenter.y);

	Y = Y * Y;
	float YY = Y*Y;
	float YYY = YY*Y;

	float LineWeight = YY - 2.7*YYY;
	LineWeight = 1.0 - BORDERMULT*LineWeight;

	vec3 colour = texture2D(texture, centerCoord / u_texture_size).rgb*LineWeight;

	if (GBAGAMMA > 0.5)
		colour.rgb*=0.6+0.4*(colour.rgb); //fake gamma because the pi is too slow!

	return vec4(colour.rgb , 1.0);
}
