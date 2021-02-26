// Super-basic scanline shader
// (looks bad at non-integer scales)
// by hunterk
// license: public domain

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

vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
	float lines = fract(texture_coords.y * u_texture_size.y);
	float scale_factor = floor((u_screen_size.y / u_texture_size.y) + 0.4999);
	vec4 screen = texture2D(texture, texture_coords * 1.0004);
	screen.rgb = RGBtoYIQ(screen.rgb);
	screen.r *= BRIGHTBOOST;
	screen.rgb = YIQtoRGB(screen.rgb);
    return (lines > (1.0 / scale_factor * THICKNESS)) ? screen : screen * vec4(1.0 - DARKNESS);
} 
