import "random" for Random

import "graphics" for Canvas
import "events" for Input

import "./lib/bunny" for Bunny

var LITTER_SIZE = 500

class Game {

    construct new() {
        _random = Random.new()

        _bunnies = []

        Canvas.bank(0, "./assets/sheet.png", 26, 37)
    }

    input() {
        if (Input.isKeyPressed(32)) {
            for (i in 1 .. LITTER_SIZE) {
                _bunnies.insert(-1, Bunny.new())
            }
        } else if (Input.isKeyPressed(81)) {
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