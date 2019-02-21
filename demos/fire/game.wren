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

        _grid = []
        for (i in 0 ... STEPS) {
            var row = []
            for (j in 0 ... STEPS) {
                row.insert(-1, 0)
            }
            _grid.insert(-1, row)
        }

        Canvas.palette(PALETTE)
    }

    input() {
        if (Input.isKeyPressed(Input.q)) {
            for (i in 0 ... STEPS) {
                for (j in 0 ... STEPS) {
                    _grid[i][j] = 0
                }
            }
            for (j in 0 ... STEPS) {
                _grid[STEPS - 1][j] = PALETTE.count - 1
            }
        }
    }

    update(deltaTime) {
        for (i in 0 ... (STEPS - 1)) {
            for (j in 0 ... STEPS) {
                var value = _grid[i + 1][j]
                if (value > 0) {
                    var delta = _random.int(0, 2)
                    _grid[i][j] = value - delta
                }
            }
        }
    }

    render(ratio) {
        for (i in 0 ... STEPS) {
            var y = i * _ySize
            for (j in 0 ... STEPS) {
                var x = j * _xSize
                var value = _grid[i][j]
                if (value > 0) {
                    Canvas.rectangle("fill", x, y, _xSize, _ySize, value)
                }
            }
        }
    }

}