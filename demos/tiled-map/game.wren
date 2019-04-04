import "random" for Random

import "collections" for Grid
import "graphics" for Bank, Canvas
import "io" for File

class Tilemap {

    construct new(file) {
var marker = System.clock
        var content = File.read(file)
        var tokens = content.split("\n")

        _bank = Bank.new(tokens[0], Num.fromString(tokens[1]), Num.fromString(tokens[2]))

        _grid = Grid.new(Num.fromString(tokens[3]), Num.fromString(tokens[4]))

        var i = 0
        for (chunk in tokens[5 ... -1]) {
            for (cell in chunk.split(" ")) {
                var value = Num.fromString(cell)
                _grid.poke(i % _grid.width, i / _grid.width, value) // TODO: _grid.fill(data)
                i = i + 1
            }
        }
System.write("Timemap loading took %(System.clock - marker) second(s)")
    }

    update(deltaTime) {

    }

    render() {
        for (i in 0 ... 16) {
            var y = i * 32 // TODO: Access bank cell size.
            for (j in 0 ... 16) {
                var x = j * 32 // TODO: ditto
                var cell = _grid.peek(i, j)
                _bank.sprite(cell, x, y) // TODO: rename `sprite()` to `draw()` or `blit()`
            } // TODO: fix bank sprite "hot-spot", not in the center for tiling!
        }
    }

}

class Game {

    construct new() {
        Canvas.palette("arne-32")

        _map = Tilemap.new("./assets/world.map")
    }

    input() {
    }

    update(deltaTime) {
        _map.update(deltaTime)
    }

    render(ratio) {
        _map.render()
    }

}