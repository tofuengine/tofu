class Sprite {

    construct new(bank, from, to, scale) {
        _bank = bank
        _from = from
        _to = to
        _scale = scale

        _angle = 0
        _x = 0
        _y = 0
    }

    move(x, y) {
        _x = x
        _y = y
    }

    rotate(angle) {
        _angle = angle
    }

    render() {
        for (id in _from .. _to) {
            var i = (id - _from) * _scale
            for (j in i ... i + _scale) {
                _bank.blit(id, _x, _y - j, _angle, _scale, _scale)
            }
        }
    }

}
