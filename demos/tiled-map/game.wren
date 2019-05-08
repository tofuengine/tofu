import "random" for Random

import "collections" for Grid
import "graphics" for Bank, Canvas, Font, BankBatch
import "events" for Input
import "io" for File
import "util" for Math // TODO: rename to `core` or `lang`?

class Constants {

    static cameraSpeed { 64.0 }

}

// https://developer.mozilla.org/en-US/docs/Games/Techniques/Tilemaps/Square_tilemaps_implementation:_Scrolling_maps
class Tilemap {

    construct new(file, cameraColumns, cameraRows, cameraAlignment) { // TODO: pass a camera easing function.
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

        _batch = BankBatch.new(_bank)

        camera(cameraColumns, cameraRows, cameraAlignment)
    }

    camera(cameraColumns, cameraRows, cameraAlignment) {
        var multipliers = { "left": 0.0, "top": 0.0, "center": 0.5, "middle": 0.5, "right": 1.0, "bottom": 1.0 }
        var alignments = cameraAlignment.split("-")

        _cameraColumns = cameraColumns
        _cameraRows = cameraRows
        _cameraWidth = cameraRows * _bank.cellWidth
        _cameraHeight = cameraColumns * _bank.cellHeight
        _cameraAlignmentX = multipliers[alignments[0]] * -_cameraWidth
        _cameraAlignmentY = multipliers[alignments[1]] * -_cameraHeight
        _cameraMinX = -_cameraAlignmentX
        _cameraMaxX = (_grid.width - cameraColumns) * _bank.cellWidth + _cameraAlignmentX
        _cameraMinY = -_cameraAlignmentY
        _cameraMaxY = (_grid.height - _cameraRows) * _bank.cellHeight + _cameraAlignmentY

        _modified = true
    }

    scrollBy(dx, dy) {
        moveTo(_cameraX + dx, _cameraY + dy)
    }

    moveTo(x, y) {
        _cameraX = Math.min(Math.max(x, _cameraMinX), _cameraMaxX)
        _cameraY = Math.min(Math.max(y, _cameraMinY), _cameraMaxY)

        var cameraX = (_cameraX - _cameraAlignmentX).floor // Discard non-integer part! Internally we track sub-pixel movements.
        var cameraY = (_cameraY - _cameraAlignmentY).floor

        _cameraStartColumn = (cameraX / _bank.cellWidth)
        _cameraStartRow = (cameraY / _bank.cellHeight)
        _cameraOffsetX = -(cameraX % _bank.cellWidth)
        _cameraOffsetY = -(cameraY % _bank.cellHeight)

        _modified = true
    }

    update(deltaTime) {
        // TODO: update the camera position in the case we are performing easings and/or following the user
        // (or some more advanced techniques).

        if (_modified) { // Check if we need to rebuild the bank batch.
            _modified = false

            _batch.clear()

            for (i in 0 .. _cameraRows) { // Inclusive, we handle an additional row to enable scrolling offset.
                var y = i * _bank.cellHeight + _cameraOffsetY
                var r = _cameraStartRow + i
                for (j in 0 .. _cameraColumns) {
                    var x = j * _bank.cellWidth + _cameraOffsetX
                    var c = _cameraStartColumn + j

                    var cellId = _grid.peek(c, r)

                    _batch.push(cellId, x, y)
                }
            }
        }
    }

    render() {
        _batch.blit()

        Font.default.write("%(_cameraX.floor) %(_cameraY.floor)", Canvas.width, 0, 15, 10, "right")
    }

}

class Game {

    construct new() {
        Canvas.palette("nes")

        _map = Tilemap.new("./assets/world.map", 15, 10, "left-top")
        _map.moveTo(0, 0)
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