#include <math.h>

#ifndef M_PI
  #define M_PI      3.14159265358979323846f
#endif
#ifndef M_PI_2
  #define M_PI_2    1.57079632679489661923f
#endif

/*
-6dB linear
-4.5dB = 10^(-4.5/20) = 0.595662144 (power taper)
-3dB = 10^(-3/20) = 0.707945784 (constant power taper)
-1.5dB = 10^(-1.5/20) = 0.841395142 (power taper)

-3dB constant power sin/cos

with these two the automated panning of a mono tracks is weird, as the power changes and is too high at the center.
-0dB balance sin/cos
-0dB balance sqrt
*/
// formula to get dB value -> log(level) * 20
static float _level_to_db(float level)
{
    return logf(level) * 20.0f;
}

typedef struct _Gain_t {
    float left, right;
} Gain_t;

typedef Gain_t (*Pan_Function_t)(float value);

/*
https://www.kvraudio.com/forum/viewtopic.php?t=299032
https://www.kvraudio.com/forum/viewtopic.php?t=148865
https://forum.cockos.com/showthread.php?t=49809
https://www.cs.cmu.edu/~music/icm-online/readings/panlaws/panlaws.pdf
http://rs-met.com/documents/tutorials/PanRules.pdf
*/

// Helps to understand the various pan levels.
// http://prorec.com/2013/05/the-pan-law-of-the-land/

// http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIxLSgoeCsxKS8yKSIsImNvbG9yIjoiI0ZGMDAwMCJ9LHsidHlwZSI6MCwiZXEiOiIoKHgrMSkvMikiLCJjb2xvciI6IiMwMDAwRkYifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyItMSIsIjEiLCIwIiwiMSJdfV0-
Gain_t _power_6db_linear_pan(float value) // AKA linear pan
{
    const float theta = (value + 1.0f) * 0.5f; // [-1, 1] -> [0 , 1]
    return (Gain_t){ .left = 1.0f - theta, .right = theta }; // powf(theta, 1)
}

// http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIoMS0oeCsxKS8yKV4wLjc1IiwiY29sb3IiOiIjRkYwMDAwIn0seyJ0eXBlIjowLCJlcSI6IigoeCsxKS8yKV4wLjc1IiwiY29sb3IiOiIjMDAwMEZGIn0seyJ0eXBlIjoxMDAwLCJ3aW5kb3ciOlsiLTEiLCIxIiwiMCIsIjEiXX1d
Gain_t _power_45db_linear_pan(float value)
{
    const float theta = (value + 1.0f) * 0.5f; // [-1, 1] -> [0 , 1]
    return (Gain_t){ .left = powf(1.0f - theta, 0.75f), .right = powf(theta, 0.75f) };
}

// http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiJzcXJ0KDEtKHgrMSkvMikiLCJjb2xvciI6IiNGRjAwMDAifSx7InR5cGUiOjAsImVxIjoic3FydCgoeCsxKS8yKSIsImNvbG9yIjoiIzAwMDBGRiJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0xIiwiMSIsIjAiLCIxIl19XQ--
Gain_t _constant_power_3db_sqrt_pan(float value)
{
    const float theta = (value + 1.0f) * 0.5f; // [-1, 1] -> [0 , 1]
    return (Gain_t){ .left = sqrtf(1.0f - theta), .right = sqrtf(theta) }; // powf(theta, 0.5)
}

// http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIoMS0oeCsxKS8yKV4wLjI1IiwiY29sb3IiOiIjRkYwMDAwIn0seyJ0eXBlIjowLCJlcSI6IigoeCsxKS8yKV4wLjI1IiwiY29sb3IiOiIjMDAwMEZGIn0seyJ0eXBlIjoxMDAwLCJ3aW5kb3ciOlsiLTEiLCIxIiwiMCIsIjEiXX1d
Gain_t _power_15db_linear_pan(float value)
{
    const float theta = (value + 1.0f) * 0.5f; // [-1, 1] -> [0 , 1]
    return (Gain_t){ .left = powf(1.0f - theta, 0.25f), .right = powf(theta, 0.25f) };
}

// http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiJjb3MoKCh4KzEpLzIpKnBpLzIpXjIiLCJjb2xvciI6IiNGRjAwMDAifSx7InR5cGUiOjAsImVxIjoic2luKCgoeCsxKS8yKSpwaS8yKV4yIiwiY29sb3IiOiIjMDAwMEZGIn0seyJ0eXBlIjoxMDAwLCJ3aW5kb3ciOlsiLTEiLCIxIiwiMCIsIjEiXX1d
Gain_t _power_6db_sincos_pan(float value)
{
    const float theta = (value + 1.0f) * 0.5f * M_PI_2; // [-1, 1] -> [0 , 1] -> [0, pi/2]
    return (Gain_t){ .left = powf(cosf(theta), 2.0f), .right = powf(sinf(theta), 2.0f) };
}

// http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiJjb3MoKCh4KzEpLzIpKnBpLzIpXjEuNSIsImNvbG9yIjoiI0ZGMDAwMCJ9LHsidHlwZSI6MCwiZXEiOiJzaW4oKCh4KzEpLzIpKnBpLzIpXjEuNSIsImNvbG9yIjoiIzAwMDBGRiJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0xIiwiMSIsIjAiLCIxIl19XQ--
Gain_t _power_45db_sincos_pan(float value)
{
    const float theta = (value + 1.0f) * 0.5f * M_PI_2; // [-1, 1] -> [0 , 1] -> [0, pi/2]
    return (Gain_t){ .left = powf(cosf(theta), 1.5f), .right = powf(sinf(theta), 1.5f) };
}

// http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiJjb3MoKCh4KzEpLzIpKnBpLzIpIiwiY29sb3IiOiIjRkYwMDAwIn0seyJ0eXBlIjowLCJlcSI6InNpbigoKHgrMSkvMikqcGkvMikiLCJjb2xvciI6IiMwMDAwRkYifSx7InR5cGUiOjEwMDAsIndpbmRvdyI6WyItMSIsIjEiLCIwIiwiMSJdfV0-
Gain_t _constant_power_3db_sincos_pan(float value)
{
    const float theta = (value + 1.0f) * 0.5f * M_PI_2; // [-1, 1] -> [0 , 1] -> [0, pi/2]
    return (Gain_t){ .left = cosf(theta), .right = sinf(theta) };
}

// http://fooplot.com/#W3sidHlwZSI6MCwiZXEiOiJjb3MoKCh4KzEpLzIpKnBpLzIpXjAuNSIsImNvbG9yIjoiI0ZGMDAwMCJ9LHsidHlwZSI6MCwiZXEiOiJzaW4oKCh4KzEpLzIpKnBpLzIpXjAuNSIsImNvbG9yIjoiIzAwMDBGRiJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0xIiwiMSIsIjAiLCIxIl19XQ--
Gain_t _power_15db_sincos_pan(float value)
{
    const float theta = (value + 1.0f) * 0.5f * M_PI_2; // [-1, 1] -> [0 , 1] -> [0, pi/2]
    return (Gain_t){ .left = powf(cosf(theta), 0.5f), .right = powf(sinf(theta), 0.5f) };
}

Gain_t _0db_linear_balance(float value)
{
    if (value < 0.0f) {
        return (Gain_t){ .left = 1.0, .right = 1.0f + value };
    } else
    if (value > 0.0f) {
        return (Gain_t){ .left = 1.0f - value, .right = 1.0f };
    } else {
        return (Gain_t){ .left = 1.0, .right = 1.0f };
    }
}

Gain_t _0db_sqrt_balance(float value)
{
    if (value < 0.0f) {
        return (Gain_t){ .left = 1.0, .right = sqrt(1.0f + value) };
    } else
    if (value > 0.0f) {
        return (Gain_t){ .left = sqrt(1.0f - value), .right = 1.0f };
    } else {
        return (Gain_t){ .left = 1.0, .right = 1.0f };
    }
}

Gain_t _0db_sincos_balance(float value)
{
    if (value < 0.0f) {
        return (Gain_t){ .left = 1.0, .right = sinf((1.0f + value) * M_PI_2) };
    } else
    if (value > 0.0f) {
        return (Gain_t){ .left = sinf((1.0f - value) * M_PI_2), .right = 1.0f }; // equal to negative
    } else {
        return (Gain_t){ .left = 1.0, .right = 1.0f };
    }
}
