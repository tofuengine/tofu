foreign class Environment {

    foreign static quit()

}

foreign class Input {

    static up { 265 }
    static down { 264 }
    static left { 263 }
    static right { 262 }
    static space { 32 }
    static enter { 257 }
    static escape { 256 }
    static z { 90 }
    static x { 88 }
    static q { 81 }

    foreign static isKeyDown(key)
    foreign static isKeyUp(key)
    foreign static isKeyPressed(key)
    foreign static isKeyReleased(key)

}
