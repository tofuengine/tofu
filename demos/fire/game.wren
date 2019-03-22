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
        var count = _width * _height
        for (i in 0 ... count) {
            _data[i] = value
        }
    }

    row(x, y, count, value) {
        var offset = (y * _width) + x
        for (j in 0 ... count) {
            _data[offset + j] = value
        }
    }

    peek(x, y) {
        return _data[(y * _width) + x]
    }

    poke(x, y, value) {
        _data[(y * _width) + x] = value
    }

}

class Grid3 {

    construct new(width, height) {
        _width = width
        _height = height
        _data = List.filled(width * height, 0)
        _offsets = List.filled(height, 0)
        for (i in 0 ... _height) {
            _offsets[i] = i * _width
        }
    }

    fill(value) {
        var count = _width * _height
        for (i in 0 ... count) {
            _data[i] = value
        }
    }

    row(x, y, count, value) {
        var offset = _offsets[y] + x
        for (j in 0 ... count) {
            _data[offset + j] = value
        }
    }

    peek(x, y) {
        return _data[_offsets[y] + x]
    }

    poke(x, y, value) {
        _data[_offsets[y] + x] = value
    }

}

class Grid4 {

    construct new(width, height) {
        _width = width
        _height = height
        _data = List.filled(width * height, 0)
        _offsets = List.filled(height, 0)
        for (i in 0 ... _height) {
            _offsets[i] = i * _width
        }
    }

    fill(value) {
        var count = _width * _height
        var i = 0
        while (i < count) {
            _data[i] = value
            i = i + 1
        }
    }

    row(x, y, count, value) {
        var offset = _offsets[y] + x
        for (j in 0 ... count) {
            _data[offset + j] = value
        }
    }

    peek(x, y) {
        return _data[_offsets[y] + x]
    }

    poke(x, y, value) {
        _data[_offsets[y] + x] = value
    }

}

class Game {

    construct new() {
        _random = Random.new()

        _xSize = Canvas.width / STEPS
        _ySize = Canvas.height / STEPS

        _windy = false

        test(Grid.new(STEPS, STEPS))
        test(Grid2.new(STEPS, STEPS))
        test(Grid3.new(STEPS, STEPS))
        test(Grid4.new(STEPS, STEPS))

        _grid = Grid.new(STEPS, STEPS)
        reset()

        Canvas.palette(PALETTE)
    }

    test(grid) {
        var s = System.clock
        _grid = Grid.new(STEPS, STEPS)
        for (x in 0 ... 10000) {
//            grid.fill(0)
//            for (j in 0 ... STEPS) {
//                grid.poke(j, STEPS - 1, PALETTE.count - 1)
//            }
            grid.row(0, STEPS - 1, STEPS - 1, PALETTE.count - 1)
        }
        var e = System.clock
        var delta = (e - s) * 1000
        System.print("Grid took %(delta)ms")
    }

    reset() {
        _grid.fill(0)
        for (j in 0 ... STEPS) {
            _grid.poke(j, STEPS - 1, PALETTE.count - 1)
        }
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