import "random" for Random

import "graphics" for Bank, Canvas, Font
import "events" for Environment, Input
import "util" for Math

import "./lib/algorithms" for Algorithms
import "./lib/easing" for Easing
import "./lib/sprite" for Sprite

var CHUNK_SIZE = 1

var TORQUE = 0.25
var THROTTLE = 50.0
var BRAKE = -25.0

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
        _forceLife = 0
        _torqueLife = 0

        _tweener = Easing.tweener(Easing.sineOut)
    }

    // http://www.iforce2d.net/b2dtut/top-down-car
    // file:///C:/Users/mlizza/Downloads/[Andrew_Kirmse]_Game_Programming_Gems_4(z-lib.org).pdf
    input() {
        if (Input.isKeyPressed(Input.start)) {
            for (i in 1 .. CHUNK_SIZE) {
                var sprite = Sprite.new(_bank, 0, 13, 1)
                sprite.move(Canvas.width / 2, Canvas.height / 2)
                _sprites.insert(-1, sprite)
            }
        }
        if (Input.isKeyDown(Input.left)) {
            _torque = -TORQUE
        }
        if (Input.isKeyDown(Input.right)) {
            _torque = TORQUE
        }
        if (Input.isKeyDown(Input.up)) {
            _force = THROTTLE
        }
        if (Input.isKeyDown(Input.down)) {
            _force = BRAKE
        }
        if (Input.isKeyPressed(Input.select)) {
        }
        if (Input.isKeyPressed(Input.y)) {
            _running = !_running
        }
    }

    update(deltaTime) {
        if (_force != 0) {
            _forceLife = Math.min(_forceLife + deltaTime, 1)
        } else {
            _forceLife = 0
        }
        if (_torque != 0) {
            _torqueLife = Math.min(_torqueLife + deltaTime, 1)
        } else {
            _torqueLife = 0
        }
        for (sprite in _sprites) {
            sprite.accelerate(_tweener.call(_forceLife) * _force)
            sprite.rotate(_tweener.call(_torqueLife) * _torque)
            sprite.update(deltaTime)
        }
        _force = 0
        _torque = 0
    }

    render(ratio) {
        for (sprite in _sprites) {
            sprite.render()
        }
        _font.write("FPS: %(Environment.fps.round)", 0, 0, 63, 1.0)
        _font.write("#%(_sprites.count) sprites", Canvas.width, 0, 63, 1.0, "right")
    }

}