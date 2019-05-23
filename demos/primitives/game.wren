import "random" for Random

import "graphics" for Canvas, Font
import "events" for Environment, Input
import "util" for Timer

class Game {

    construct new() {
        _random = Random.new()

        _bunnies = []

        Canvas.palette("arne-16")

        _font = Font.default

        _segments = 30
    }

    input() {
        if (Input.isKeyPressed(Input.start)) {
            Environment.quit()
        } else if (Input.isKeyPressed(Input.left)) {
            _segments = _segments - 1
        } else if (Input.isKeyPressed(Input.right)) {
            _segments = _segments + 1
        } else if (Input.isKeyPressed(Input.select)) {
        } else if (Input.isKeyPressed(Input.y)) {
        }
    }

    update(deltaTime) {
        for (bunny in _bunnies) {
            bunny.update(deltaTime)
        }
    }

    render(ratio) {
        Canvas.point(4, 4, 1)
        Canvas.line(4, 8, 7, 8, 2)
//        Canvas.triangle("fill", 150, 150, 50, 250, 250, 250, 3)
        Canvas.rectangle("fill", 4, 12, 8, 8, 3)
        Canvas.rectangle("line", 4, 23, 8, 8, 3)
//        Canvas.square("fill", 200, 10, 75, 2)
//        Canvas.circle("line", 100, 100, 50, 2)
//        Canvas.circle("fill", 200, 100, 50, 1)
//        Canvas.circle("fill", 300, 100, 50, 1, _segments)

        _font.write("FPS: %(Environment.fps.round)", Canvas.width, 0, 1, 1.0, "right")
    }

}