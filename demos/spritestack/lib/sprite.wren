import "graphics" for Canvas, Font

class Sprite {

    construct new(bank, from, to, scale) {
        _font = Font.default

        _bank = bank
        _from = from
        _to = to
        _scale = scale

        _speed = 0
        _angle = 0
        _x = 0
        _y = 0
    }

    move(x, y) {
        _x = x
        _y = y
    }

    rotate(angle) {
        _angle = _angle + angle
    }

    accelerate(speed) {
        _speed = _speed + speed
    }

    update(deltaTime) {
        _x = _x + _angle.cos * _speed * deltaTime
        _y = _y + _angle.sin * _speed * deltaTime
    }

    render() {
        for (id in _from .. _to) {
            var i = (id - _from) * _scale
            for (j in i ... i + _scale) {
                _bank.blit(id, _x, _y - j, _angle - (Num.pi / 2), _scale, _scale) // Compensate for sprite alignment
            }
        }
        //var x = _x + _angle.cos * 32
        //var y = _y + _angle.sin * 32
        //Canvas.line(_x, _y, x, y, 32)
    }

}
