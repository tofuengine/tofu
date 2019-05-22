import "random" for Random

import "graphics" for Bank, Canvas, Font
import "events" for Environment, Input
import "util" for Timer

import "./lib/bunny" for Bunny

var LITTER_SIZE = 250
var MAX_BUNNIES = 32768

class Game {

    construct new() {
        _random = Random.new()

        _bunnies = []

        Canvas.palette("gameboy")

        _bank = Bank.new("./assets/sheet.png", 26, 37)
        _font = Font.default

        _running = true

        _tooggle = true
        _timer = Timer.new(5.0, 0, Fn.new {
                _tooggle = !_tooggle
                Canvas.palette(_tooggle ? "gameboy" : "gameboy-bw")
            })
    }

    input() {
        if (Input.isKeyPressed(Input.start)) {
            for (i in 1 .. LITTER_SIZE) {
                _bunnies.insert(-1, Bunny.new(_random, _bank))
            }
            if (_bunnies.count >= MAX_BUNNIES) {
                Environment.quit()
            }
        } else if (Input.isKeyPressed(Input.select)) {
            _bunnies.clear()
        } else if (Input.isKeyPressed(Input.y)) {
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

//        Canvas.point(50, 50, 3)
//        Canvas.line(150, 150, 250, 250, 3)
//        Canvas.triangle("fill", 150, 150, 50, 250, 250, 250, 3)
//        Canvas.rectangle("fill", 10, 10, 100, 100, 2)
//        Canvas.square("fill", 200, 10, 75, 2)

        _font.write("FPS: %(Environment.fps.round)", 0, 0, 1, 1.0, "left")
        _font.write("#%(_bunnies.count) bunnies", Canvas.width, 0, 3, 1.0, "right")
    }

}