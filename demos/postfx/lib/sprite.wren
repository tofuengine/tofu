import "graphics" for Canvas

var MIN_AMPLITUDE = 8
var MAX_AMPLITUDE = 64
var MIN_SPEED = 0.5
var MAX_SPEED = 4
var MAX_ANGLE = Num.pi
var MIN_X = 8
var MAX_X = Canvas.width - 8
var MIN_Y = 8
var MAX_Y = Canvas.height - 8

class Sprite {

    construct new(random, bank) {
        _bank = bank

        _cx = random.int(MIN_X, MAX_X)
        _cy = random.int(MIN_Y, MAX_Y)
        _ax = (random.float() * (MAX_AMPLITUDE - MIN_AMPLITUDE)) + MIN_AMPLITUDE
        _ay = (random.float() * (MAX_AMPLITUDE - MIN_AMPLITUDE)) + MIN_AMPLITUDE
        _angle_x = random.float() * MAX_ANGLE
        _angle_y = random.float() * MAX_ANGLE
        _speed_x = (random.float() * (MAX_SPEED - MIN_SPEED)) + MIN_SPEED
        _speed_y = (random.float() * (MAX_SPEED - MIN_SPEED)) + MIN_SPEED

        _id = random.int(0, 7)
    }

    update(deltaTime) {
        _angle_x = _angle_x + _speed_x * deltaTime
        _angle_y = _angle_y + _speed_y * deltaTime

        _x = _cx + _angle_x.cos * _ax
        _y = _cy + _angle_y.sin * _ay
    }

    render() {
        //var angle = _vx.sign * _rotation
        //_bank.blit(_id, _x, _y, angle)
        _bank.blit(_id, _x, _y)
    }

}
