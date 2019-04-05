import "random" for Random

import "collections" for Grid
import "graphics" for Bank, Canvas
import "io" for File

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

        _angle = 0
    }

    update(deltaTime) {
        _angle = _angle + (45.0 * deltaTime)
    }

    render() {
        for (i in 0 ... 16) {
            var y = i * 32 // TODO: Access bank cell size.
            for (j in 0 ... 16) {
                var x = j * 32 // TODO: ditto
                var cell = _grid.peek(i, j)
                _bank.sprite(cell, x, y, _angle) // TODO: rename `sprite()` to `draw()` or `blit()`
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