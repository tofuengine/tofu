import "random" for Random

import "util" for Timer
import "graphics" for Canvas

class Game {

    construct new() {
        Canvas.palette("pico-8")

        _random = Random.new()
        _timerA = Timer.new(0.5, 50, Fn.new {
                _x = _random.int(0, Canvas.width)
            })
        _timerB = Timer.new(0.25, -1, Fn.new {
                _y = _random.int(0, Canvas.height)
            })
        _timerC = Timer.new(15, 0, Fn.new {
                _timerA.cancel()
                _timerB = null
            })

        _x = _random.int(0, Canvas.width)
        _y = _random.int(0, Canvas.height)
    }

    input() {
    }

    update(deltaTime) {
    }

    render(ratio) {
        Canvas.circle("fill", _x, _y, 2, 1)
    }

}