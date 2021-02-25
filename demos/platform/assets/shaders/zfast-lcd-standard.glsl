/*
    zfast_lcd_standard - A very simple LCD shader meant to be used at 1080p
		on the raspberry pi 3.
		
    Copyright (C) 2017 Greg Hogan (SoltanGris42)

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.


Notes:  This shader just does nearest neighbor scaling of the game and then
		darkens the border pixels to imitate an LCD screen. You can change the
		amount of darkening and the thickness of the borders.  You can also 
		do basic gamma adjustment.
		
*/

//#pragma parameter BORDERMULT "Border Multiplier" 14.0 -40.0 40.0 1.0
//#pragma parameter GBAGAMMA "GBA Gamma Hack" 1.0 0.0 1.0 1.0
#define BORDERMULT 14.0
#define GBAGAMMA 1.0

vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
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
