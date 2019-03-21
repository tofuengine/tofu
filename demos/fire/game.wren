import "random" for Random

import "collections" for Grid
import "graphics" for Canvas
import "events" for Input

var STEPS = 64
var PALETTE = [
        "FF000000", "FF240000", "FF480000", "FF6D0000",
        "FF910000", "FFB60000", "FFDA0000", "FFFF0000",
        "FFFF3F00", "FFFF7F00", "FFFFBF00", "FFFFFF00",
        "FFFFFF3F", "FFFFFF7F", "FFFFFFBF", "FFFFFFFF"
    ]

class Grid2 {

    construct new(width, height) {
        _width = width
        _height = height
        _data = List.filled(width * height, 0)
    }

    fill(value) {
        for (i in 0 ... _width * _height) {
            _data[i] = value
        }
    }

    peek(x, y) {
        return _data[(y * _width) + x]
    }

    poke(x, y, value) {
        _data[(y * _width) + x] = value
    }

}

class Game {

    construct new() {
        _random = Random.new()

        _xSize = Canvas.width / STEPS
        _ySize = Canvas.height / STEPS

        _windy = false

        _grid = Grid.new(STEPS, STEPS)
        reset()

        Canvas.palette(PALETTE)
    }

    reset() {
var s = System.clock
for (x in 0 ... 100000) {
        _grid.fill(0)
        for (j in 0 ... STEPS) {
            _grid.poke(j, STEPS - 1, PALETTE.count - 1)
        }
}
var e = System.clock
var delta = (e - s) * 1000
System.print("Took %(delta)ms")
    }

    input() {
        if (Input.isKeyPressed(Input.q)) {
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
    }

}