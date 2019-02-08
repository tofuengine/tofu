foreign class Environment {

    foreign static quit()

}

foreign class Input {

    static space { 32 }
    static q { 81 }

    foreign static isKeyDown(key)
    foreign static isKeyUp(key)
    foreign static isKeyPressed(key)
    foreign static isKeyReleased(key)

}
