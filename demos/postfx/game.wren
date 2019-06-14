import "random" for Random

import "graphics" for Bank, Canvas, Font
import "events" for Environment, Input
import "io" for File

import "./lib/sprite" for Sprite

var LITTER_SIZE = 64

class Game {

    construct new() {
        _random = Random.new()

        _sprites = []

        Canvas.palette = "nes" // "pico-8"
        Canvas.background = 63
        Canvas.shader = File.read("./assets/shaders/water.glsl")
        //Canvas.send("u_strength", 100)

        _bank = Bank.new("./assets/images/diamonds.png", 16, 16)
        _font = Font.default

        _speed = 1.0
        _running = true
    }

    input() {
        if (Input.isKeyPressed(Input.start)) {
            for (i in 1 .. LITTER_SIZE) {
                _sprites.insert(-1, Sprite.new(_random, _bank, _sprites.count))
            }
        } else if (Input.isKeyPressed(Input.left)) {
            _speed = _speed * 0.5
        } else if (Input.isKeyPressed(Input.right)) {
            _speed = _speed * 2.0
        } else if (Input.isKeyPressed(Input.down)) {
            _speed = 1.0
        } else if (Input.isKeyPressed(Input.select)) {
            _sprites.clear()
        } else if (Input.isKeyPressed(Input.y)) {
            _running = !_running
        }
    }

    update(deltaTime) {
        if (!_running) {
            return
        }
        for (sprite in _sprites) {
            sprite.update(deltaTime * _speed)
        }
    }

    render(ratio) {
        for (sprite in _sprites) {
            sprite.render()
        }
        _font.write("FPS: %(Environment.fps.round)", 0, 0, 0, 1.0, "left")
        _font.write("#%(_sprites.count) sprites", Canvas.width, 0, 0, 1.0, "right")
    }

}