import "game" for Game

class Tofu {

    construct new() {
        _game = Game.new()
    }

    handle(inputs) {
        _game.handle(inputs)
    }

    update(deltaTime) {
        _game.update(deltaTime)
    }

    render(ratio) {
        _game.render(ratio)
    }

}
