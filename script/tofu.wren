import "game" for Game

class Tofu {

    static initialize() {
        System.write(">>> Tofu.initialize")
        __game = Game.new()
    }

    static handle(inputs) {
        __game.handle(inputs)
    }

    static update(deltaTime) {
        __game.update(deltaTime)
    }

    static render(ratio) {
        __game.render()
    }

    static terminate() {
        System.write("<<< Tofu.terminate")
    }

}
/*
import "game" for Game

class XTofu {

    construct new() {
        _game = Game.new()
    }

    initialize() {
        System.write(">>> Tofu.initialize")
    }

    handle(inputs) {
        _game.handle(inputs)
    }

    update(deltaTime) {
        _game.update(deltaTime)
    }

    render(ratio) {
        _game.render()
    }

    terminate() {
        System.write("<<< Tofu.terminate")
    }

}

var Tofu = XTofu.new()
*/