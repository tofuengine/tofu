import "./game" for Game

class Tofu {

    construct new() {
        _game = Game.new()
    }

    input() {
        _game.input()
    }

    update(deltaTime) {
        _game.update(deltaTime)
    }

    render(ratio) {
        _game.render(ratio)
    }

}
