import "random" for Random

import "graphics" for Canvas
import "events" for Input

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

        _xSize = Canvas.width / STEPS
        _ySize = Canvas.height / STEPS

        _windy = false

        reset()

        Canvas.palette(PALETTE)
    }

    reset() {
        _grid = List.filled(STEPS * STEPS, 0)
        var offset = (STEPS - 1) * STEPS
        for (j in 0 ... STEPS) {
            _grid[offset + j] = PALETTE.count - 1
        }
    }

    input() {
        if (Input.isKeyPressed(Input.q)) {
            _windy = !_windy
        }
    }

    update(deltaTime) {
        for (i in 0 ... (STEPS - 1)) {
            var offset = i * STEPS
            for (j in 0 ... STEPS) {
                var value = _grid[offset + STEPS + j]
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
                _grid[offset + x] = value
            }
        }
    }

    render(ratio) {
        for (i in 0 ... STEPS) {
            var offset = i * STEPS
            var y = i * _ySize
            for (j in 0 ... STEPS) {
                var x = j * _xSize
                var value = _grid[offset + j]
                if (value > 0) {
                    Canvas.rectangle("fill", x, y, _xSize, _ySize, value)
                }
            }
        }
    }

}