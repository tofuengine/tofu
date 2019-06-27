class Easing {

    static tween(time, duration, from, to, function) {
        var r = function.call(time / duration)
        return (1 - r) * from + r * to // Precise method, which guarantees v = v1 when t = 1.
    }
    static linear {
        return Fn.new {|r|
            return r
        }
    }
    static quad {
        return Fn.new {|r|
            return r * r
        }
    }
    static cubic {
        return Fn.new {|r|
            return r * r * r
        }
    }
    static quart {
        return Fn.new {|r|
            return r * r * r * r
        }
    }
    static quint {
        return Fn.new {|r|
            return r * r * r * r * r
        }
    }
    static expo {
        return Fn.new {|r|
            return 2.pow(10 * (r - 1))
        }
    }
    static sine {
        return Fn.new {|r|
            return -(r * (Num.pi * 0.5)).cos + 1
        }
    }
    static circ {
        return Fn.new {|r|
            return -((1 - (r * r)).sqrt - 1)
        }
    }
    static back {
        return Fn.new {|r|
            return r * r * (2.7 * r - 1.7)
        }
    }
    static elastic {
        return Fn.new {|r|
            return -(2.pow(10 * (r - 1)) * ((r - 1.075) * (Num.pi * 2) / 0.3).sin)
        }
    }

}
