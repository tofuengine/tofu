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
 * zfast_crt_standard - A simple, fast CRT shader.
 *
 * Copyright (C) 2017 Greg Hogan (SoltanGris42)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Notes: This shader does scaling with a weighted linear filter for adjustable
 * sharpness on the x and y axes based on the algorithm by Inigo Quilez here:
 * http://http://www.iquilezles.org/www/articles/texture/texture.htm
 * but modified to be somewhat sharper.  Then a scanline effect that varies
 * based on pixel brightness is applied along with a monochrome aperture mask.
 * This shader runs at 60fps on the Raspberry Pi 3 hardware at 2mpix/s
 * resolutions (1920x1080 or 1600x1200).
*/

//This can't be an option without slowing the shader down
//Comment this out for a coarser 3 pixel mask...which is currently broken
//on SNES Classic Edition due to Mali 400 gpu precision
#define FINEMASK

//Some drivers don't return black with texture coordinates out of bounds
//SNES Classic is too slow to black these areas out when using fullscreen
//overlays.  But you can uncomment the below to black them out if necessary
//#define BLACK_OUT_BORDER

// Parameter lines go here:
// #pragma parameter BLURSCALEX "Blur Amount X-Axis" 0.30 0.0 1.0 0.05
// #pragma parameter LOWLUMSCAN "Scanline Darkness - Low" 6.0 0.0 10.0 0.5
// #pragma parameter HILUMSCAN "Scanline Darkness - High" 8.0 0.0 50.0 1.0
// #pragma parameter BRIGHTBOOST "Dark Pixel Brightness Boost" 1.25 0.5 1.5 0.05
// #pragma parameter MASK_DARK "Mask Effect Amount" 0.25 0.0 1.0 0.05
// #pragma parameter MASK_FADE "Mask/Scanline Fade" 0.8 0.0 1.0 0.05

#define BLURSCALEX 0.45
#define LOWLUMSCAN 5.0
#define HILUMSCAN 10.0
#define BRIGHTBOOST 1.25
#define MASK_DARK 0.25
#define MASK_FADE 0.8

const float maskFade = 0.3333 * MASK_FADE;

vec4 effect(sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
	//This is just like "Quilez Scaling" but sharper
	vec2 p = texture_coords * u_texture_size;
	vec2 i = floor(p) + 0.50;
	vec2 f = p - i;
	p = (i + 4.0*f*f*f) / u_texture_size;
	p.x = mix(p.x , texture_coords.x, BLURSCALEX);
	float Y = f.y * f.y;
	float YY = Y * Y;
	float YYY = YY * Y;
	
#if defined(FINEMASK) 
	float whichmask = fract( screen_coords.x*-0.4999);
	float mask = 1.0 + float(whichmask < 0.5) * -MASK_DARK;
#else
	float whichmask = fract(screen_coords.x * -0.3333);
	float mask = 1.0 + float(whichmask <= 0.33333) * -MASK_DARK;
#endif
	vec3 colour = texture2D(texture, p).rgb;
	
	float scanLineWeight = (BRIGHTBOOST - LOWLUMSCAN*(Y - 2.05*YY));
	float scanLineWeightB = 1.0 - HILUMSCAN*(YY-2.8*YYY);	
	
#if defined(BLACK_OUT_BORDER)
	colour.rgb *= float(texture_coords.x > 0.0) * float(texture_coords.y > 0.0); // Why doesn't the driver do the right thing?
#endif

	return vec4(colour.rgb*mix(scanLineWeight*mask, scanLineWeightB, dot(colour.rgb,vec3(maskFade))), 1.0);
}
