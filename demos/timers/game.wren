import "random" for Random

import "util" for Timer
import "graphics" for Canvas

class Game {

    construct new() {
        Canvas.palette("pico-8")

        _random = Random.new()
        _timer = Timer.new(0.5, -1, Fn.new {
                System.write("Tick")
                _x = _random.int(0, Canvas.width)
                _y = _random.int(0, Canvas.height)
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