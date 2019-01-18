import "random" for Random

import "./lib/draw" for Draw
import "./lib/other" for Other

class Game {

    construct new() {
        _random = Random.new()
        _time = 0
        _x = 0
        _speedX = 16.0
        _color = 0
        _colorSpeed = 64.0
    }

    handle(inputs) {
    }

    update(deltaTime) {
        _color = _color + _colorSpeed * deltaTime
        if (_color > 255) {
            _color = 255
            _colorSpeed = _colorSpeed * -1
        }
        if (_color < 0) {
            _color = 0
            _colorSpeed = _colorSpeed * -1
        }

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

    render(ratio) {
        var x = _x //__random.int() % 320
        var y = _time.sin * 64 + 120 //__random.int() % 240
        Draw.point(x, y, _color.floor)
    }

}