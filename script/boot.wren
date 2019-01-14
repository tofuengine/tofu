import "main" for Main

class Tofu {

    static initialize() {
        Main.initialize()
    }

    static step() {
        var dt = 0.15
        Main.update(dt)
        Main.render()
    }

    static terminate() {
        Main.terminate()
    }

}