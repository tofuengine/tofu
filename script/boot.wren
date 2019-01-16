import "main" for Main

class Tofu {

    static initialize() {
        Main.initialize()
    }

    static run(fps) {
        var secondsPerUpdate = 1.0 / fps

        var previous = System.clock()
        var lag = 0.0

        while (Engine.running) {
            var current = System.clock()
            var elapsed = current - previous
            previous = current
            lag += elapsed

            Main.input(Engine.inputs)

            while (lag >= secondsPerUpdate) {
                Main.update(secondsPerUpdate)
                lag -= secondsPerUpdate
            }

            Main.render( / secondsPerUpdate)
        }
    }

    static terminate() {
        Main.terminate()
    }

}