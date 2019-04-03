import "random" for Random

import "collections" for Grid
import "graphics" for Bank

class Map {

    construct new(file) {
        var map = Wren.compile(file) // Read the content of the file and call interpreter.

        var bank = map["bank"]
        _bank = Bank.new(bank["file"], bank["width"], bank["height"])

        _grid = Grid.new(data["width"], data["height"])
        for (value in map["data"]) {
            _grid.poke(i % _grid.width, i / _grid.width, value)
        }
    }

    update(deltaTime) {

    }

    render() {
        
    }

}

class Game {

    construct new() {
        Canvas.palette("arne-16")

        _map = Map.new("./assets/simple.map")
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