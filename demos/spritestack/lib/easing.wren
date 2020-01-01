/*
 * Copyright (c) 2019-2020 by Marco Lizza (marco.lizza@gmail.com)
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
 **/

class Easing {

    // Inspired by the following:
    //
    // https://github.com/sole/tween.js/blob/master/src/Tween.js
    // https://github.com/kikito/tween.lua/blob/master/tween.lua
    // https://gist.github.com/Fonserbc/3d31a25e87fdaa541ddf
    // https://github.com/mobius3/tweeny/blob/master/include/easing.h

    // Returns a function object bound to a specific easing function, duration and range of values.
    // The function object applies a single tweening evaluation, by normalizing the `time` argument
    // over the `duration`.
    static tweener(function, duration, from, to) {
        return Fn.new {|time|
            var r = function.call(time / duration)
            return (1 - r) * from + r * to // Precise method, which guarantees correct result `r = 1`.
        }
    }
    static tweener(function, duration) {
        return Fn.new {|time|
            return function.call(time / duration)
        }
    }
    static tweener(function) {
        return function // Calling the specific easing function directly.
    }

    // TODO: implement easing functions in C?

    static linear {
        return Fn.new {|r|
            return r
        }
    }
    static sineIn {
        return Fn.new {|r|
            return 1 - (r * Num.pi * 0.5).cos
        }
    }
    static sineOut {
        return Fn.new {|r|
            return (r * Num.pi * 0.5).sin
        }
    }
    static sineInOut {
        return Fn.new {|r|
            return 0.5 * (1 - (r * Num.pi).cos)
        }
    }
    static quadIn {
        return Fn.new {|r|
            return r * r
        }
    }
    static quadOut {
        return Fn.new {|r|
            return r * (2 - r)
            //return 1 - (r - 1).pow(2)
        }
    }
    static quadInOut {
        return Fn.new {|r|
            r = r * 2
            if (r < 1) {
                r = r * 0.5
                return r * r
            }
            r = (r - 1) * 0.5
            return r * (2 - r)
        }
    }
    static cubicIn {
        return Fn.new {|r|
            return r.pow(3)
        }
    }
    static cubicOut {
        return Fn.new {|r|
            return 1 + (r - 1).pow(3)
        }
    }
    static cubicInOut {
        return Fn.new {|r|
            r = r * 2
            if (r < 1) {
                r = r * 0.5
                return r.pow(3)
            }
            r = (r - 1) * 0.5
            return 1 + (r - 1).pow(3)
        }
    }
    static quartIn {
        return Fn.new {|r|
            return r.pow(4)
        }
    }
    static quartOut {
        return Fn.new {|r|
            return 1 - (r - 1).pow(4)
        }
    }
    static quartInOut {
        return Fn.new {|r|
            r = r * 2
            if (r < 1) {
                r = r * 0.5
                return r.pow(4)
            }
            r = (r - 1) * 0.5
            return 1 - (r - 1).pow(4)
        }
    }
    static quintIn {
        return Fn.new {|r|
            return r.pow(5)
        }
    }
    static quintOut {
        return Fn.new {|r|
            return 1 + (r - 1).pow(5)
        }
    }
    static quintInOut {
        return Fn.new {|r|
            r = r * 2
            if (r < 1) {
                r = r * 0.5
                return r.pow(5)
            }
            r = (r - 1) * 0.5
            return 1 + (r - 1).pow(5)
        }
    }
    static expoIn {
        return Fn.new {|r|
            if (r == 0) {
                return 0
            }
            return 1024.pow(r - 1)
        }
    }
    static expoOut {
        return Fn.new {|r|
            if (r == 1) {
                return 1
            }
            return 1 - 2.pow(-10 * r)
        }
    }
    static expoInOut {
        return Fn.new {|r|
            if (r == 0) {
                return 0
            }
            if (r == 1) {
                return 1
            }
            r = r * 2
            if (r < 1) {
                return 0.5 * 1024.pow(r - 1)
            }
            return 0.5 * (2 - 2.pow(-10 * (r - 1)))
        }
    }
    static circIn {
        return Fn.new {|r|
            return 1 - (1 - r.pow(2)).sqrt
        }
    }
    static circOut {
        return Fn.new {|r|
            return (1 - (r - 1).pow(2)).sqrt
        }
    }
    static circInOut {
        return Fn.new {|r|
            r = r * 2
            if (r < 1) {
                return 0.5 * (1 - (1 - r.pow(2)).sqrt)
            }
            return 0.5 * (1 + (1 - (r - 2).pow(2)).sqrt)
        }
    }
    static backIn {
        return Fn.new {|r|
            // var s = 1.70158
            return r * r * (2.70158 * r - 1.70158)
        }
    }
    static backOut {
        return Fn.new {|r|
            r = r - 1
            return r * r * (2.70158 * r + 1.70158) + 1
        }
    }
    static backInOut {
        return Fn.new {|r|
            // var s = 1.70158 * 1.525 = 2.59491
            r = r * 2
            if (r < 1) {
                return 0.5 * r * r * (3.59491 * r - 2.59491)
            }
            r = r - 2
            return 0.5 * r * r * (3.59491 * r + 2.59491) + 2
        }
    }
    static elasticIn {
        return Fn.new {|r|
            if (r == 0) {
                return 0
            }
            if (r == 1) {
                return 1
            }
            return -2.pow(10 * (r - 1)) * ((r - 1.1) * 5 * Num.pi).sin
        }
    }
    static elasticOut {
        return Fn.new {|r|
            if (r == 0) {
                return 0
            }
            if (r == 1) {
                return 1
            }
            return 2.pow(-10 * r) * ((r - 0.1) * 5 * Num.pi).sin
        }
    }
    static elasticInOut {
        return Fn.new {|r|
            if (r == 0) {
                return 0
            }
            if (r == 1) {
                return 1
            }
            r = r * 2
            if (r < 1) {
                return -0.5 * 2.pow(10 * (r - 1)) * ((r - 1.1) * 5 * Num.pi).sin
            }
            return 0.5 * 2.pow(-10 * (r - 1)) * ((r - 1.1) * 5 * Num.pi).sin + 1
        }
    }
    static bounceIn {
        var o = bounceOut
        return Fn.new {|r|
            return 1 - o.call(1 - r)
        }
    }
    static bounceOut {
        return Fn.new {|r|
            if (r < (1 / 2.75)) {
                return 7.5625 * r * r
            } else if (r < (2 / 2.75)) {
                r = r - (1.5 / 2.75)
                return 7.5625 * r * r + 0.75
            } else if (r < (2.5 / 2.75)) {
                r = r - (2.25 / 2.75)
                return 7.5625 * r * r + 0.9375
            } else {
                r = r - (2.625 / 2.75)
                return 7.5625 * r * r + 0.984375
            }
        }
    }
    static bounceInOut {
        var i = bounceIn
        var o = bounceOut
        return Fn.new {|r|
            if (r < 0.5) {
                return i.call(r * 2) * 0.5
            }
            return o.call(r * 2 - 1) * 0.5 + 0.5
        }
    }

}
