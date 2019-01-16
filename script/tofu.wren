import "main" for Main

class Tofu {

    static initialize() {
        System.write("Tofu.initialize")
        Main.initialize()
    }

    static handle(inputs) {
        //System.write("B")
    }

    static update(deltaTime) {
        //System.write("C " + deltaTime.toString)
    }

    static render(ratio) {
        //System.write("D  " + ratio.toString)
    }

    static terminate() {
        Main.terminate()
        System.write("Tofu.terminate")
    }

}