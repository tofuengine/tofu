import "random" for Random

import "graphics" for Canvas
import "events" for Environment, Input

import "./lib/bunny" for Bunny

var KEY_SPACE = 32
var KEY_Q = 81
var LITTER_SIZE = 250
var MAX_BUNNIES = 32768

class Game {

    construct new() {
        _random = Random.new()

        _bunnies = []

        Canvas.bank(0, "./assets/sheet.png", 26, 37)
    }

    input() {
        if (Input.isKeyPressed(KEY_SPACE)) {
            for (i in 1 .. LITTER_SIZE) {
                _bunnies.insert(-1, Bunny.new(_random))
            }
            if (_bunnies.count >= MAX_BUNNIES) {
                Environment.quit()
            }
        } else if (Input.isKeyPressed(KEY_Q)) {
            _bunnies.clear()
        }
    }

    update(deltaTime) {
        for (bunny in _bunnies) {
            bunny.update(deltaTime)
        }
    }

    render(ratio) {
        for (bunny in _bunnies) {
            bunny.render()
        }

        Canvas.text(0, "#%(_bunnies.count) bunnies", 0, 10, 255, 10)
    }

}