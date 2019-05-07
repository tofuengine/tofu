import "random" for Random

import "collections" for Grid
import "graphics" for Bank, Canvas, Font
import "events" for Input
import "io" for File
import "util" for Math // TODO: rename to `core` or `lang`?

class Constants {

    static cameraSpeed { 64.0 }

}

// https://developer.mozilla.org/en-US/docs/Games/Techniques/Tilemaps/Square_tilemaps_implementation:_Scrolling_maps
class Tilemap {

    load_(file) {
        var content = File.read(file)
        var tokens = content.split("\n")
        var cells = []
        for (chunk in tokens[5 .. -1]) { // This is slow, but not dramatical.
            for (cell in chunk.split(" ")) {
                cells.add(Num.fromString(cell))
            }
        }

        _bank = Bank.new(tokens[0], Num.fromString(tokens[1]), Num.fromString(tokens[2]))  // Bank loading is *the* slowest part.

        _grid = Grid.new(Num.fromString(tokens[3]), Num.fromString(tokens[4]), null)
//        _grid = Grid.new(Num.fromString(tokens[3]), Num.fromString(tokens[4]), cells) // Bugged!!!
        _grid.fill(cells)
//        _grid.stride(0, 0, cells, cells.length())
    }

    construct new(file, cameraColumns, cameraRows) {
        var map = load_(file)

        camera(cameraColumns, cameraRows)

        _batch = BankBatch.new(_bank)
    }

    camera(cameraColumns, cameraRows) {
        _cameraColumns = cameraColumns
        _cameraRows = cameraRows
        _cameraMaxX = (_grid.width - cameraColumns) * _bank.cellWidth
        _cameraMaxY = (_grid.height - _cameraRows) * _bank.cellHeight
    }

    scrollBy(dx, dy) {
        moveTo(_cameraX + dx, _cameraY + dy)
    }

    moveTo(x, y) {
        _cameraX = Math.min(Math.max(x, 0), _cameraMaxX)
        _cameraY = Math.min(Math.max(y, 0), _cameraMaxY)

        var cameraX = _cameraX.floor // Discard non-integer part! Internally we track sub-pixel movements.
        var cameraY = _cameraY.floor

        _cameraStartColumn = (cameraX / _bank.cellWidth)
        _cameraStartRow = (cameraY / _bank.cellHeight)
        _cameraOffsetX = -(cameraX % _bank.cellWidth)
        _cameraOffsetY = -(cameraY % _bank.cellHeight)
    }

    update(deltaTime) {
        // TODO: update the camera position in the case we are performing easings and/or following the user
        // (or some more advanced techniques).
    }

    render() {
        for (i in 0 .. _cameraRows) { // Inclusive, we handle an additional row to enable scrolling offset.
            var y = i * _bank.cellHeight + _cameraOffsetY
            var r = _cameraStartRow + i
            for (j in 0 .. _cameraColumns) {
                var x = j * _bank.cellWidth + _cameraOffsetX
                var c = _cameraStartColumn + j

                var cellId = _grid.peek(c, r)

                _bank.blit(cellId, x, y)
            }
        }

        Font.default.write("%(_cameraX.floor) %(_cameraY.floor)", Canvas.width, 0, 15, 10, "right")
    }

}

class Game {

    construct new() {
        Canvas.palette("arne-32")

        _map = Tilemap.new("./assets/world.map", 15, 10)
        _map.moveTo(16, 16)
    }

    input() {
        _dx = 0
        _dy = 0
        if (Input.isKeyDown(Input.left)) {
            _dx = _dx - 1
        }
        if (Input.isKeyDown(Input.right)) {
            _dx = _dx + 1
        }
        if (Input.isKeyDown(Input.up)) {
            _dy = _dy - 1
        }
        if (Input.isKeyDown(Input.down)) {
            _dy = _dy + 1
        }
    }

    update(deltaTime) {
        var dx = _dx * Constants.cameraSpeed * deltaTime
        var dy = _dy * Constants.cameraSpeed * deltaTime
        if (dx != 0.0 || dy != 0.0) {
            _map.scrollBy(dx, dy)
        }
        _map.update(deltaTime)
    }

    render(ratio) {
        _map.render()
    }

}