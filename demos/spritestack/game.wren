import "random" for Random

import "graphics" for Bank, Canvas, Font
import "events" for Environment, Input

import "./lib/sprite" for Sprite

var CHUNK_SIZE = 4

class Game {

    construct new() {
        _random = Random.new()

        _sprites = []

        Canvas.palette = "6-bit-rgb"
        Canvas.background = 0

        _bank = Bank.new("./assets/images/slices.png", 15, 32)
        _font = Font.default

        _angle = 0
    }

    input() {
        if (Input.isKeyPressed(Input.start)) {
            for (i in 1 .. CHUNK_SIZE) {
//                var sprite = Sprite.new(_bank, 0, 13, _random.int(1, 4))
//                sprite.move(_random.int(0, Canvas.width), _random.int(0, Canvas.height))
                var sprite = Sprite.new(_bank, 0, 13, 3)
                sprite.move(Canvas.width / 2, Canvas.height / 2)
                _sprites.insert(-1, sprite)
            }
        } else if (Input.isKeyDown(Input.left)) {
            for (sprite in _sprites) {
                sprite.rotate(-ANGLE_STEP)
            }
        } else if (Input.isKeyDown(Input.right)) {
            for (sprite in _sprites) {
                sprite.rotate(ANGLE_STEP)
            }
        } else if (Input.isKeyDown(Input.up)) {
            for (sprite in _sprites) {
                sprite.accelerate(1.0)
            }
        } else if (Input.isKeyDown(Input.down)) {
            for (sprite in _sprites) {
                sprite.accelerate(-1.0)
            }
        } else if (Input.isKeyPressed(Input.select)) {
        } else if (Input.isKeyPressed(Input.y)) {
            _running = !_running
        }
    }

    update(deltaTime) {
        for (sprite in _sprites) {
            sprite.update(deltaTime)
        }
    }

    render(ratio) {
        for (sprite in _sprites) {
            sprite.render()
        }
        _font.write("FPS: %(Environment.fps.round)", 0, 0, 63, 1.0, "left")
        _font.write("#%(_sprites.count) sprites", Canvas.width, 0, 63, 1.0, "right")
    }

}