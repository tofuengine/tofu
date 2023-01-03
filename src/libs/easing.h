/*
 * MIT License
 *
 * Copyright (c) 2019-2022 Marco Lizza
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

// http://robertpenner.com/easing/

#ifndef TOFU_LIBS_EASING_H
#define TOFU_LIBS_EASING_H

typedef float (*Easing_Function_t)(float);

extern float easing_linear(float p);
extern float easing_quadratic_in(float p);
extern float easing_quadratic_out(float p);
extern float easing_quadratic_in_out(float p);
extern float easing_cubic_in(float p);
extern float easing_cubic_out(float p);
extern float easing_cubic_in_out(float p);
extern float easing_quartic_in(float p);
extern float easing_quartic_out(float p);
extern float easing_quartic_in_out(float p);
extern float easing_quintic_in(float p);
extern float easing_quintic_out(float p);
extern float easing_quintic_in_out(float p);
extern float easing_sine_in(float p);
extern float easing_sine_out(float p);
extern float easing_sine_in_out(float p);
extern float easing_circular_in(float p);
extern float easing_circular_out(float p);
extern float easing_circular_in_out(float p);
extern float easing_exponential_in(float p);
extern float easing_exponential_out(float p);
extern float easing_exponential_in_out(float p);
extern float easing_elastic_in(float p);
extern float easing_elastic_out(float p);
extern float easing_elastic_in_out(float p);
extern float easing_back_in(float p);
extern float easing_back_out(float p);
extern float easing_back_in_out(float p);
extern float easing_bounce_out(float p);
extern float easing_bounce_in(float p);
extern float easing_bounce_in_out(float p);

#endif  /* TOFU_LIBS_EASING_H */
