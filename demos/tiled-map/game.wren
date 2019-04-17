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

    construct new(grid, tileWidth, tileHeight, columns, rows) {
        _grid = grid
        _tileWidth = tileWidth
        _tileHeight = tileHeight
        _width = tileWidth * columns
        _height = tileHeight * rows
        _columns = columns
        _rows = rows

        moveTo(0, 0)
    }

    moveTo(x, y) {
        if (x < 0) {
            x = 0
        }
        if (y < 0) {
            y = 0
        }
        if (x > (_grid.width - _width)) {
            x = _grid.width - _width
        }
        if (y > (_grid.height - _height)) {
            y = _grid.height - _height
        }
        _x = x
        _y = y

        _startCol = (_x / _tileWidth).floor;
        _endCol = _startCol + (_width / _tileWidth);
        _startRow = (_y / _tileHeight).floor;
        _endRow = _startRow + (_height / _tileHeight);

        _offsetX = -_x + _startCol * _tileWidth;
        _offsetY = -_y + _startRow * _tileHeight;
    }

    scrollBy(dx, dy) {
        moveTo(_x + dx, _y + dy)
    }

    draw(callback) {
        for (var c = startCol; c <= endCol; c++) {
            for (var r = startRow; r <= endRow; r++) {
                var tile = map.getTile(c, r);
                var x = (c - startCol) * map.tsize + offsetX;
                var y = (r - startRow) * map.tsize + offsetY;
                if (tile !== 0) { // 0 => empty tile
                    _bank.blit(tile, x.round, y.round)
                }
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

        _camera = TilemapCamera.new(_grid, _bank.cellWidth, _bank.cellHeight, 15, 10)

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
        for (i in _camera["y"] ... _camera["y"] + _camera["height"]) {
            var y = i * _bank.cellHeight
            for (j in _camera["x"] ... _camera["x"] + _camera["width"]) {
                var x = j * _bank.cellWidth
                var cell = _grid.peek(i, j)
                _bank.blit(cell, x, y, _angle)
            }
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