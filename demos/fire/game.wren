import "random" for Random

import "collections" for Grid
import "graphics" for Canvas, Font
import "events" for Environment, Input

var STEPS = 64
var PALETTE = [
        "FF000000", "FF240000", "FF480000", "FF6D0000",
        "FF910000", "FFB60000", "FFDA0000", "FFFF0000",
        "FFFF3F00", "FFFF7F00", "FFFFBF00", "FFFFFF00",
        "FFFFFF3F", "FFFFFF7F", "FFFFFFBF", "FFFFFFFF"
    ]

class Game {

    construct new() {
        _random = Random.new()

        _font = Font.default

        _xSize = Canvas.width / STEPS
        _ySize = Canvas.height / STEPS

        _windy = false

        _grid = Grid.new(STEPS, STEPS, null)
        reset()

        Canvas.palette = PALETTE
    }

    reset() {
        _grid.fill(0)
        _grid.stride(0, STEPS - 1, PALETTE.count - 1, STEPS)
    }

    input() {
        if (Input.isKeyPressed(Input.select)) {
            _windy = !_windy
        }
    }

    update(deltaTime) {
        for (i in 0 ... (STEPS - 1)) {
            for (j in 0 ... STEPS) {
                var value = _grid.peek(j, i + 1)
                if (value > 0) {
                    var delta = _random.int(0, 2)
                    value = value - delta
                    if (value < 0) {
                        value = 0
                    }
                }

                var x = j
                if (_windy) {
                    x = x + _random.int(0, 3) - 1
                    if (x < 0) {
                        x = 0
                    }
                    if (x >= STEPS) {
                        x = STEPS - 1
                    }
                }
                _grid.poke(x, i, value)
            }
        }
    }

    render(ratio) {
        for (i in 0 ... STEPS) {
            var y = i * _ySize
            for (j in 0 ... STEPS) {
                var x = j * _xSize
                var value = _grid.peek(j, i)
                if (value > 0) {
                    Canvas.rectangle("fill", x, y, _xSize, _ySize, value)
                }
            }
        }

        _font.write("FPS: %(Environment.fps.round)", 0, 0, 15, 1.0, "left")
    }

}