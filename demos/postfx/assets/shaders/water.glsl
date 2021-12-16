//
//  Value Noise 2D
//  Return value range of 0.0->1.0
//  https://github.com/BrianSharpe/Wombat/blob/master/Value2D.glsl
//
//  Credits to Brian Sharpe for this function.
//  https://github.com/BrianSharpe/Wombat
//
float value2D( vec2 P ) {
    //	establish our grid cell and unit position
    vec2 Pi = floor(P);
    vec2 Pf = P - Pi;

    //	calculate the hash.
    vec4 Pt = vec4( Pi.xy, Pi.xy + 1.0 );
    Pt = Pt - floor(Pt * ( 1.0 / 71.0 )) * 71.0;
    Pt += vec2( 26.0, 161.0 ).xyxy;
    Pt *= Pt;
    Pt = Pt.xzxz * Pt.yyww;
    vec4 hash = fract( Pt * ( 1.0 / 951.135664 ) );

    //	blend the results and return
    vec2 blend = Pf * Pf * Pf * (Pf * (Pf * 6.0 - 15.0) + 10.0);
    vec4 blend2 = vec4( blend, vec2( 1.0 - blend ) );
    return dot( hash, blend2.zxzx * blend2.wwyy );
}

const float MAGNIFICATION = 8.0;
float height(vec2 uv) {
    return value2D(uv * MAGNIFICATION);
}

vec4 bumpFromDepth(vec2 uv, vec2 resolution, float scale) {
    vec2 step = 1. / resolution;

    float h = height(uv);

    vec2 dxy = h - vec2(
        height(uv + vec2(step.x, 0.)),
        height(uv + vec2(0., step.y))
    );

  return vec4(normalize(vec3(dxy * scale / step, 1.)), h);
}

vec4 getAsBump(vec2 coord, float time, vec2 resolution) {
    // this is the waves moving under the texture
    vec2 offset = vec2(time / 12.0, time / 9.0);

    vec2 waveOne = coord + offset;
    vec4 sampleOne = vec4(bumpFromDepth(waveOne, resolution, .1).rgb * .5 + .2, 1.);

    offset += vec2(time / 14.0, time / 21.0);
    offset *= vec2(1.0, -1.0);

    vec2 waveTwo = coord - offset;
    vec4 sampleTwo = vec4(bumpFromDepth(waveTwo, resolution, .1).rgb * .5 + .2, 1.);

    return sampleOne + sampleTwo;
}

const vec4 WATER_TINT = vec4(0.0, 0.1, 0.2, 1.0);
vec4 getDistortedSample(sampler2D image, vec2 baseLoc, float distanceFromShore, vec2 offset, bool rough) {
    float cleanRange = 0.1;

    //offset -= 0.5;

    // if it's too close to shore, reduce distortion
    if (rough || distanceFromShore < cleanRange) {
        //offset *= distanceFromShore / cleanRange;
    }

    if (offset.x > 1.0) {
        //offset.x = 1.0;
    }

    // this is our distortion from the wave bumps
    baseLoc += offset * 0.02;

    return texture2D(image, baseLoc) + WATER_TINT;
}

const vec4 EDGE_COLOR = vec4(0,0,0,0);
//const float EDGE_OF_WATER = 0.25;
const float EDGE_OF_WATER = 256.0;
const float ANGLE_RATE = 3.0;

vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {
    vec2 uv = texture_coords;

    float edge = EDGE_OF_WATER / u_screen_size.y;

    if (abs(uv.y - edge) < 0.003) {
        return EDGE_COLOR;
    }

    vec4 normal = texture2D(texture, uv);

    if (uv.y < edge) {
        return normal;
    }

    float distFromEdge = uv.y - edge;
    vec2 mirrorCoords = vec2(uv.x, edge - distFromEdge * ANGLE_RATE);

    vec2 bumpNormal =  getAsBump(uv, u_time, u_screen_size).xy;

    vec4 distorted = getDistortedSample(texture, mirrorCoords, distFromEdge, bumpNormal, false);

    // Water is more trasparent near the wall.
    float ratio = pow(distFromEdge / edge, 0.75);
    return mix(normal, distorted, ratio);
}
