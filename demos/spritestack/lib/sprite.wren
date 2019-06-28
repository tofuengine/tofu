import "graphics" for Canvas

import "./lib/math" for Vector2D

class Sprite {

    construct new(bank, from, to, scale) {
        _bank = bank
        _from = from
        _to = to
        _scale = scale

        _mass = 1.0
        _inertia = 1.0

        _position = Vector2D.new(0, 0)
        _velocity = Vector2D.new(0, 0)
        _acceleration = 0
        _angularVelocity = 0
        _angle = 0
    }

    move(x, y) {
        _position = Vector2D.new(x, y)
    }

    rotate(torque) {
        // When updating the angular velocity we should also take into account
        // that steering is easier at higher speeds.
        _angularVelocity = _angularVelocity + torque / _inertia
    }

    accelerate(force) {
        _acceleration = _acceleration + force / _mass
    }

    update(deltaTime) {
        _angle = _angle + _angularVelocity * deltaTime
        _angularVelocity = _angularVelocity * 0.90

        _velocity = _velocity.add(Vector2D.fromPolar(_angle, _acceleration * deltaTime))
        _acceleration = _acceleration * 0.90

        _position = _position.add(_velocity.scale(deltaTime))
        _velocity = _velocity.scale(0.95)
    }

    render() {
        var x = _position.x
        var y = _position.y
        for (id in _from .. _to) {
            var i = (id - _from) * _scale
            for (j in i ... i + _scale) {
                _bank.blit(id, x, y - j, _angle - (Num.pi / 2), _scale, _scale) // Compensate for sprite alignment
            }
        }

//        var direction = Vector2D.fromPolar(_angle, 48)
//        Canvas.line(x, y, x + direction.x, y + direction.y, 32)
        var color = Canvas.colorFromArgb("FFFF4444")
        Canvas.line(x, y, x + _velocity.x, y + _velocity.y, color)
    }

}
