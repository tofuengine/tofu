/* Copyright 1994 - 2013 by Steven Worley
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/* Worley()

   An implementation of the key cellular texturing basis
   function. This function is hardwired to return an average F_1 value
   of 1.0. It returns the <n> most closest feature point distances
   F_1, F_2, .. F_n the vector delta to those points, and a 32 bit
   seed for each of the feature points.  This function is not
   difficult to extend to compute alternative information such as
   higher order F values, to use the Manhattan distance metric, or
   other fun perversions.

   <at>    The input sample location. 
   <max_order>  Smaller values compute faster. < 5, read the book to extend it.
   <F>     The output values of F_1, F_2, ..F[n] in F[0], F[1], F[n-1]
   <delta> The output vector difference between the sample point and the n-th
            closest feature point. Thus, the feature point's location is the
            hit point minus this value. The DERIVATIVE of F is the unit
            normalized version of this vector.
   <ID>    The output 32 bit ID number which labels the feature point. This
            is useful for domain partitions, especially for coloring flagstone
            patterns.

   This implementation is tuned for speed in a way that any order > 5
   will likely have discontinuous artifacts in its computation of F5+.
   This can be fixed by increasing the internal points-per-cube
   density in the source code, at the expense of slower
   computation. The book lists the details of this tuning.  */

#ifndef __WORLEY__NOISE__
#define __WORLEY__NOISE__

#include <stdint.h>
#include <stddef.h>

void Worley(double at[3], size_t max_order,
	    double *F, double (*delta)[3], uint32_t *ID);

#endif /* __WORLEY__NOISE__ */