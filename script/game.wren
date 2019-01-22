import "random" for Random

import "graphics" for Canvas
import "./lib/other" for Other

class Game {

    construct new() {
        _random = Random.new()
        _time = 0
        _x = 0
        _speedX = 16.0
        _color = 0
        _colorSpeed = 64.0
        _angle = 0.0

        Canvas.palette([ 0, 1, 2, 3, 4, 5, 6, 7 ])
        Canvas.bank(0, "./assets/sheet.png", 16, 16)
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

        _angle = _angle + 180.0 * deltaTime
    }

    render(ratio) {
        var x = _x //__random.int() % 320
        var y = _time.sin * 64 + 120 //__random.int() % 240
        Canvas.point(x, y, _color.floor)

        Canvas.sprite(0, 0, x, y, _angle, 1.0, 1.0)
    }

}