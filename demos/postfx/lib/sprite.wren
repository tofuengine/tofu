import "graphics" for Canvas

var MAX_SPEED = 300
var GRAVITY = 981
var DAMPENING = 0.9
var ROTATION_SPEED = Num.pi * 32.0
var MIN_X = 8
var MAX_X = Canvas.width - 8
var MIN_Y = 0
var MAX_Y = Canvas.height - 64

class Sprite {

    construct new(random, bank) {
        _x = random.int(MIN_X, MAX_X)
        _y = (MAX_Y - MIN_Y) / 8
        _vx = (random.float() * MAX_SPEED) - (MAX_SPEED / 2.0)
        _vy = (random.float() * MAX_SPEED) - (MAX_SPEED / 2.0)

        _rotation = 0.0

        _random = random
        _bank = bank
        _id = random.int(0, 7)
    }

    update(deltaTime) {
        _x = _x + _vx * deltaTime
        _y = _y + _vy * deltaTime

        _vy = _vy + GRAVITY * deltaTime

        if (_x > MAX_X) {
            _vx = _vx * DAMPENING * -1.0
            _x = MAX_X
        } else if (_x < MIN_X) {
            _vx = _vx * DAMPENING * -1.0
            _x = MIN_X
        }

        if (_y > MAX_Y) {
            _vy = _vy * DAMPENING * -1.0
            _y = MAX_Y

            if (_vy.abs <= 400.0 && _random.float() <= 0.10) { // Higher bounce occasionally.
                _vy = _vy - ((_random.float() * 150.0) + 100.0)
            }
        } else if (_y < MIN_Y) {
            _vy = _vy * DAMPENING * -1.0
            _y = MIN_Y
        }

        _rotation = _rotation + deltaTime * ROTATION_SPEED
    }

    render() {
        //var angle = _vx.sign * _rotation
        //_bank.blit(_id, _x, _y, angle)
        _bank.blit(_id, _x, _y)
    }

}
