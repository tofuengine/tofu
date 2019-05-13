import "random" for Random

import "graphics" for Bank, Canvas, Font
import "events" for Environment, Input

import "./lib/bunny" for Bunny

var LITTER_SIZE = 250
var MAX_BUNNIES = 32768

class Game {

    construct new() {
        _random = Random.new()

        _bunnies = []

        Canvas.palette("arne-16")

        _bank = Bank.new("./assets/sheet.png", 26, 37)
        _font = Font.default

        _running = true
    }

    input() {
        if (Input.isKeyPressed(Input.space)) {
            for (i in 1 .. LITTER_SIZE) {
                _bunnies.insert(-1, Bunny.new(_random, _bank))
            }
            if (_bunnies.count >= MAX_BUNNIES) {
                Environment.quit()
            }
        } else if (Input.isKeyPressed(Input.q)) {
            _bunnies.clear()
        } else if (Input.isKeyPressed(Input.enter)) {
            _running = !_running
        }
    }

    update(deltaTime) {
        if (!_running) {
            return
        }
        for (bunny in _bunnies) {
            bunny.update(deltaTime)
        }
    }

    render(ratio) {
        for (bunny in _bunnies) {
            bunny.render()
        }

        _font.write("#%(_bunnies.count) bunnies", Canvas.width, 0, 15, 10, "right", 0.50)
    }

}