// https://github.com/henriquelalves/SimpleGodotCRTShader

// Curvature
#define BarrelPower 1.1
// Color bleeding
#define color_bleeding 1.2
#define bleeding_range vec2(3.0, 3.0)
// Scanline
#define lines_distance 4.0
#define scan_size 2.0
#define scanline_alpha 0.9
#define lines_velocity 30.0

vec2 distort(vec2 p) 
{
	float angle = p.y / p.x;
	float theta = atan(p.y,p.x);
	float radius = pow(length(p), BarrelPower);
	
	p.x = radius * cos(theta);
	p.y = radius * sin(theta);
	
	return 0.5 * (p + vec2(1.0,1.0));
}

void get_color_bleeding(inout vec4 current_color,inout vec4 color_left){
	current_color = current_color*vec4(color_bleeding,0.5,1.0-color_bleeding,1);
	color_left = color_left*vec4(1.0-color_bleeding,0.5,color_bleeding,1);
}

void get_color_scanline(vec2 uv,inout vec4 c,float time){
	float line_row = floor((uv.y * u_screen_size.y / scan_size) + mod(time * lines_velocity, lines_distance));
	float n = 1.0 - ceil((mod(line_row,lines_distance)/lines_distance));
	c = c - n*c*(1.0 - scanline_alpha);
	c.a = 1.0;
}

vec4 effect(sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
	vec2 xy = texture_coords.xy * 2.0 - vec2(1.0, 1.0);

	float d = length(xy);
	if(d < 1.5){
		xy = distort(xy);
	}
	else{
		xy = texture_coords.xy;
	}

	vec2 pixel_size = 1.0 / u_screen_size * bleeding_range;

	vec4 color_left = texture2D(texture, xy - pixel_size);
	vec4 current_color = texture2D(texture, xy);
	get_color_bleeding(current_color,color_left);
	vec4 c = current_color+color_left;
	get_color_scanline(xy,c,u_time);
	return c;
}
