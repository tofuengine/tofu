import "random" for Random

import "graphics" for Bank, Canvas, Font
import "events" for Environment, Input

import "./lib/sprite" for Sprite
import "./lib/algorithms" for Algorithms

var CHUNK_SIZE = 1

var TORQUE = 0.02
var THROTTLE = 4.0
var BRAKE = -2.0

class Tween {

    static lerp(a, b, r) { // https://en.wikipedia.org/wiki/Linear_interpolation
        return (1 - r) * a + r * b // Precise method, which guarantees v = v1 when t = 1.
    }
    quad(a, b, r) {
        var t = r * r
        return lerp(a, b, t)
    }
    cubic(a, b, r) {
        var t = r * r * r
        return lerp(a, b, t)
    }
    quart(a, b, r) {
        var t = r * r * r * r
        return lerp(a, b, t)
    }
    quint(a, b, r) {
        var t = r * r * r * r * r
        return lerp(a, b, t)
    }
    expo(a, b, r) {
        var t = 2.pow(10 * (r - 1))
        return lerp(a, b, t)
    }
    sine(a, b, r) {
        var t = -(r * (Num.pi * 0.5)).cos + 1
        return lerp(a, b, t)
    }
    circ(a, b, r) {
        var t = -((1 - (r * r)).sqrt - 1)
        return lerp(a, b, t)
    }
    back(a, b, r) {
        var t = r * r * (2.7 * r - 1.7)
        return lerp(a, b, t)
    }
    elastic(a, b, r) {
        var t = -(2.pow(10 * (r - 1)) * ((r - 1.075) * (Num.pi * 2) / .3).sin)
        return lerp(a, b, t)
    }

}

class Game {

    test() {
        var L = []
        var f = Fn.new {|a, b|
                if(a == b) return 0
                if(a < b) return -1
                return 1
            }
        for (i in 0 ... 64) {
            L.insert(-1, _random.int(0, 16))
        }
        System.write(L)
        System.write(Algorithms.sorted(L, f))
//        Algorithms.sort(L, f)
//        Algorithms.msort(L, 0, L.count - 1)
        Algorithms.qsort(L, 0, L.count - 1)
        System.write(L)
        System.write(Algorithms.sorted(L, f))
    }

    construct new() {
        _random = Random.new()

        test()

        _sprites = []

        Canvas.palette = "6-bit-rgb"
        Canvas.background = 0

        _bank = Bank.new("./assets/images/slices.png", 15, 32)
        _font = Font.default

        _angle = 0

        _force = 0
        _torque = 0
        _pedalLife = 0
        _steerLife = 0
    }

    // http://www.iforce2d.net/b2dtut/top-down-car
    // file:///C:/Users/mlizza/Downloads/[Andrew_Kirmse]_Game_Programming_Gems_4(z-lib.org).pdf
    input() {
        if (Input.isKeyPressed(Input.start)) {
            for (i in 1 .. CHUNK_SIZE) {
//                var sprite = Sprite.new(_bank, 0, 13, _random.int(1, 4))
//                sprite.move(_random.int(0, Canvas.width), _random.int(0, Canvas.height))
                var sprite = Sprite.new(_bank, 0, 13, 1)
                sprite.move(Canvas.width / 2, Canvas.height / 2)
                _sprites.insert(-1, sprite)
            }
        }
        if (Input.isKeyDown(Input.left)) {
            for (sprite in _sprites) {
                _torque = _torque - TORQUE
            }
        }
        if (Input.isKeyDown(Input.right)) {
            for (sprite in _sprites) {
                _torque = _torque + TORQUE
            }
        }
        if (Input.isKeyDown(Input.up)) {
            for (sprite in _sprites) {
                _force = _force + THROTTLE
            }
        }
        if (Input.isKeyDown(Input.down)) {
            for (sprite in _sprites) {
                _force = _force + BRAKE
            }
        }
        if (Input.isKeyPressed(Input.select)) {
        }
        if (Input.isKeyPressed(Input.y)) {
            _running = !_running
        }
    }

    update(deltaTime) {
        if (_force > 0) {
            _forceLife = _forceLife + deltaTime
        }
        if (_torque > 0) {
            _forceLife = _forceLife + deltaTime
        }
        for (sprite in _sprites) {
            sprite.accelerate(_force)
            sprite.rotate(_torque)
            sprite.update(deltaTime)
        }
        _force = 0
        _torque = 0
    }

    render(ratio) {
        for (sprite in _sprites) {
            sprite.render()
        }
        _font.write("FPS: %(Environment.fps.round)", 0, 0, 63, 1.0, "left")
        _font.write("#%(_sprites.count) sprites", Canvas.width, 0, 63, 1.0, "right")
    }

}