import "random" for Random

import "graphics" for Canvas
import "events" for Environment, Input

import "./lib/bunny" for Bunny

var LITTER_SIZE = 250
var MAX_BUNNIES = 32768

class Game {

    construct new() {
        _random = Random.new()

        _bunnies = []

        // Canvas.palette([ // Load the PICO-8 palette.
        //         "FF000000", "FF5F574F", "FFC2C3C7", "FFFFF1E8",
        //         "FFFFEC27", "FFFFA300", "FFFFCCAA", "FFAB5236",
        //         "FFFF77A8", "FFFF004D", "FF83769C", "FF7E2553",
        //         "FF29ADFF", "FF1D2B53", "FF008751", "FF00E436"
        //     ])
        Canvas.palette([ // Load the ARNE-16 palette.
                "FF000000", "FF493C2B", "FFBE2633", "FFE06F8B",
                "FF9D9D9D", "FFA46422", "FFEB8931", "FFF7E26B",
                "FFFFFFFF", "FF1B2632", "FF2F484E", "FF44891A",
                "FFA3CE27", "FF005784", "FF31A2F2", "FFB2DCEF"
            ])
        Canvas.bank(0, "./assets/sheet.png", 26, 37)
    }

    input() {
        if (Input.isKeyPressed(Input.space)) {
            for (i in 1 .. LITTER_SIZE) {
                _bunnies.insert(-1, Bunny.new(_random))
            }
            if (_bunnies.count >= MAX_BUNNIES) {
                Environment.quit()
            }
        } else if (Input.isKeyPressed(Input.q)) {
            _bunnies.clear()
        }
    }

    update(deltaTime) {
        for (bunny in _bunnies) {
            bunny.update(deltaTime)
        }
    }

    render(ratio) {
        for (bunny in _bunnies) {
            bunny.render()
        }

        Canvas.text(0, "#%(_bunnies.count) bunnies", Canvas.width, 0, 15, 10, "right")
    }

}