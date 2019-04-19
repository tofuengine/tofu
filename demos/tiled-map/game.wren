import "random" for Random

import "collections" for Grid
import "graphics" for Bank, Canvas
import "io" for File

class BankBatch {

    construct new(bank) {
        _bank = bank
        _batch = []
    }

    push(cellId, x, y) {
        push(cellId, x, y, 0.0, 1.0, 1.0)
    }

    push(cellId, x, y, r) {
        push(cellId, x, y, r, 1.0, 1.0)
    }

    push(cellId, x, y, r, sx, sy) {
        var item = {
                "cellId": cellId,
                "x": x,
                "y": y,
                "r": r,
                "sx": sx,
                "sy": sy
            }
        _batch.add(item)
    }

    clear() {
        _batch.clear()
    }

    flush() {
        for (item in _batch) {
            _bank.blit(item["cellId"], item["x"], item["y"], item["r"], item["sx"], item["sy"])
        }
        _batch.clear()
    }

}

class TilemapCamera {

    construct new(mapWidth, mapHeight, tileWidth, tileHeight, columns, rows) {
        _tileWidth = tileWidth
        _tileHeight = tileHeight
        _mapWidth = mapWidth * tileWidth
        _mapHeight = mapHeight * tileHeight
        _width = tileWidth * columns
        _height = tileHeight * rows
        _columns = columns
        _rows = rows

        moveTo(0, 0)
    }

    max(a, b) {
        if (a > b) {
            return a
        }
        return b
    }

    min(a, b) {
        if (a < b) {
            return a
        }
        return b
    }

    moveTo(x, y) {
        _x = min(max(x, 0), _mapWidth - _width) // TODO: precompute "maxX" e "maxY"
        _y = min(max(y, 0), _mapHeight - _height)

        _startCol = (_x / _tileWidth).floor
        _startRow = (_y / _tileHeight).floor
        _offsetX = -(_x % _tileWidth)
        _offsetY = -(_y % _tileHeight)
    }

    scrollBy(dx, dy) {
        moveTo(_x + dx, _y + dy)
    }

    draw(callback) {
        for (i in 0 .. _rows) { // Inclusive, we handle an additional row to enable scrolling offset.
            var y = i * _tileHeight + _offsetY
            var r = _startRow + i
            for (j in 0 .. _columns) {
                var x = j * _tileWidth + _offsetX
                var c = _startCol + j
                callback.call(x, y, c, r)
            }
        }
    }

}

// https://developer.mozilla.org/en-US/docs/Games/Techniques/Tilemaps/Square_tilemaps_implementation:_Scrolling_maps
class Tilemap {

    load_(file) {
        var content = File.read(file)
        var tokens = content.split("\n")

        var cells = []
        for (chunk in tokens[5 .. -1]) { // This is the slowest part while loading, but not dramatical.
            for (cell in chunk.split(" ")) {
                cells.add(Num.fromString(cell))
            }
        }

        var result = {
            "bank": {
                "atlas": tokens[0],
                "width": Num.fromString(tokens[1]),
                "height": Num.fromString(tokens[2])
            },
            "grid": {
                "width": Num.fromString(tokens[3]),
                "height": Num.fromString(tokens[4]),
                "cells": cells
            }
        }
        return result
    }

    construct new(file) {
        var map = load_(file)

        var bank = map["bank"]
        _bank = Bank.new(bank["atlas"], bank["width"], bank["height"])  // Bank loading *the* slowest part.

        var grid = map["grid"]
//        _grid = Grid.new(grid["width"], grid["height"], grid["cells"]) // Bugged!!!
        _grid = Grid.new(grid["width"], grid["height"], null)
        _grid.fill(grid["cells"])
//        _grid.stride(0, 0, grid["cells"], grid["cells"].length())

        _camera = TilemapCamera.new(_grid.width, _grid.height, _bank.cellWidth, _bank.cellHeight, 15, 10)
        _camera.moveTo(16, 0)

        _angle = 0
    }

    scrollBy(dx, dy) {
        _camera.scrollBy(dx, dy)
    }

    moveTo(x, y) {
        _camera.moveTo(dx, dy)
    }

    update(deltaTime) {
        _angle = _angle + (90.0 * deltaTime)
    }

    render() {
        _camera.draw(Fn.new{|x, y, c, r|
                var cellId = _grid.peek(c, r)
                _bank.blit(cellId, x, y, _angle)
            })
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