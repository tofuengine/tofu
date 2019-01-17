import "random" for Random

import "lib/draw" for Draw
import "lib/other" for Other

class Game {

    construct new() {
        _random = Random.new()
        _time = 0
        _x = 0
        _speedX = 32.0
    }

    handle(inputs) {
    }

    update(deltaTime) {
        _time = _time + deltaTime
        _x = _x + _speedX * deltaTime
        if (_x > 320) {
            _x = 320
            _speedX = _speedX * -1
        }
        if (_x < 0) {
            _x = 0
            _speedX = _speedX * -1
        }
    }

    render() {
        var x = _x //__random.int() % 320
        var y = _time.sin * 64 + 120 //__random.int() % 240
        Draw.point(x, y, 255)
    }

}