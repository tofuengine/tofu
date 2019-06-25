import "graphics" for Canvas, Font

//import "./lib/math" for Vector2D

class Vector2D {

    construct new(x, y) {
        _x = x
        _y = y
    }

    static fromPolar(angle, magnitude) {
        return Vector2D.new(angle.cos * magnitude, angle.sin * magnitude)
    }

    static zero {
        return Vector2D.new(0, 0)
    }

    x {
        return _x
    }

    y {
        return _y
    }

    angle {
        return _y.atan(_x)
    }

    magnitudeSquared {
        return (_x * _x) + (_y * _y)
    }

    magnitude {
        return magnitudeSquared.sqrt
    }

    clone {
        return Vector2D.new(_x, _y)
    }

    isZero {
        return _x == 0 && _y == 0
    }

    isEqual(v) {
        return _x == v.x && _y == v.y
    }

    assign(v) {
        _x = v.x
        _y== v.y
    }

    add(other) {
        return Vector2D.new(_x + other.x, _y + other.y)
    }

    scale(s) {
        return Vector2D.new(_x * s, _y * s)
    }

    // | cos(a)  -sin(a) | | x |   | x' |
    // |                 | |   | = |    |
    // | sin(a)   cos(a) | | y |   | y' |
    rotate(a) {
        var cos = a.cos
        var sin = a.sin
        return Vector2D.new(cos * _x - sin * _y, sin * _x + cos * _y)
    }

    negate {
        return Vector2D.new(-_x, -_y)
    }

    // COUNTER-CLOCKWISE perpendicular vector (`perp` operator).
    perpendicular {
        return Vector2D.new(-_y, _x)
    }

    // a dot b
    // ------- b
    // b dot b
    //
    // https://en.wikipedia.org/wiki/Vector_projection
    project(v) {
        var s = this.dot(v) / v.dot(v)
        return Vector2D.new(s * v.x, s * v.y)
    }

    //       a dot b
    // a - 2 ------- b
    //       b dot b
    //
    // https://math.stackexchange.com/questions/2239169/reflecting-a-vector-over-another-line
    mirror(v) {
        var s = 2 * this.dot(v) / v.dot(v)
        return Vector2D.new(_x - s * v.x, _y - s * v.y)
    }

    dot(other) {
        return (_x * other.x) + (_y * other.y)
    }

    // Area of the parallelogram described by the vector, i.e. the DETERMINAND of
    // the matrix with the vectors as columns (or rows).
    //
    // It is also (if scaled) the sine of the angle between the vectors. That means
    // that if NEGATIVE the second vector is CLOCKWISE from the first one, if
    // POSITIVE the second vector is COUNTER-CLOCKWISE from the first one.
    //
    // It is also called "perp-dot", that is the dot product of the perpendicular
    // vector with another vector (i.e. `a.perpendicular.dot(b)`)
    //
    // NOTE: when on a 2D display, since the `y` component inverts it sign, also
    //       the rule inverts! That is if NEGATIVE then is COUNTER-CLOCKWISE.
    //
    // https://en.wikipedia.org/wiki/Exterior_algebra
    // http://geomalgorithms.com/vector_products.html#2D-Perp-Product
    perpDot(v) {
        return (_x * v.y) - (_y * v.x)
    }

    distanceFromSquared(v) {
        var dx = _x - v.x
        var dy = _y - v.y
        return (dx * dx) + (dy * dy)
    }

    distanceFrom(v) {
        return distanceFromSquared(v).sqrt
    }

    normalize() {
        return normalize(1)
    }

    normalize(l) {
        return scale(l / magnitude)
    }

    normalizeIfNotZero() {
        return normalizeIfNotZero(1)
    }

    normalizeIfNotZero(l) {
        if (isZero) {
            return this
        }
        return normalize(l)
    }

    // Normalize to the give `l` length only when greater than it.
    trim() {
        return trim(1)
    }

    trim(l) {
        var s = l * l / magnitudeSquared
        if (s >= 1) {
            return this
        }
        return scale(s.sqrt)
    }

    trimIfNotZero() {
        return trimIfNotZero(1)
    }

    trimIfNotZero(l) {
        if (isZero) {
            return this
        }
        return trim(l)
    }

    angleTo(v) {
        return (v.y - _y).atan(v.x - _x)
    }

    angleBetween(v) {
        return _y.atan(self.x) - v.y.atan(v.x)
    }

}

class RigidBody {

    construct new() {
        _position = Vector2D.new(0, 0)
        _velocity = Vector2D.new(0, 0)
        _forces = Vector2D.new(0, 0)
        _mass = 1.0

        _angle = 0.0
        _angularVelocity = 0.0
        _torque = 0.0
        _inertia = 1.0
    }

    moveTo(position) {
        _position = position
    }

    orient(angle) {
        _angle = angle
    }

    impulse(force, offset) {
        _forces = _forces.add(force)
        _torque = _torque + offset.cross(force)
    }

    position {
        return _position
    }

    angle {
        return _angle
    }

    update(deltaTime) {
        var acceleration = _forces.divide(_mass)
        _velocity = _velocity.add(acceleration.multiply(deltaTime))
        _position = _position.add(_velocity.multiply(deltaTime))
        _forces = Vector2D.new(0, 0)

        var angularAcceleration = _torque / _inertia
        _angularVelocity = _angularVelocity + angularAcceleration * deltaTime
        _angle = angle + _angularVelocity * deltaTime
        _torque = 0.0
    }

}

class SpriteV {

    construct new(bank, from, to, scale) {
        _font = Font.default

        _bank = bank
        _from = from
        _to = to
        _scale = scale

        _offset = Vector2D.new(0, 0)
        _body = RigidBody.new()
        _speed = 0
        _steering = 0
    }

    move(x, y) {
        _body.moveTo(Vector2D.new(x, y))
    }

    rotate(angle) {
        _steering = _steering + angle
    }

    accelerate(speed) {
        _speed = _speed + speed
    }

    update(deltaTime) {
        var offset = Vector2D.fromPolar(_steering, 1.0)
        var impulse = Vector2D.fromPolar(_body.angle, _speed)
        _body.impulse(impulse, offset)

        _body.update(deltaTime)

        _steering = _steering * 0.9
        _speed = _speed * 0.9
    }

    render() {
        var from = _body.position
        var to = Vector2D.fromPolar(_body.angle, 16).add(from)
        Canvas.line(from.x, from.y, to.x, to.y, 32)
/*
        for (id in _from .. _to) {
            var i = (id - _from) * _scale
            for (j in i ... i + _scale) {
                _bank.blit(id, _x, _y - j, _angle - (Num.pi / 2), _scale, _scale) // Compensate for sprite alignment
            }
        }
*/
        //var x = _x + _angle.cos * 32
        //var y = _y + _angle.sin * 32
        //Canvas.line(_x, _y, x, y, 32)
    }

}

class SpriteSSS {

    construct new(bank, from, to, scale) {
        _font = Font.default

        _bank = bank
        _from = from
        _to = to
        _scale = scale

        _speed = 0
        _angle = 0
        _steering = 0
        _x = 0
        _y = 0
    }

    move(x, y) {
        _x = x
        _y = y
    }

    rotate(angle) {
        _steering = _steering + angle
    }

    accelerate(speed) {
        _speed = _speed + speed
    }

    update(deltaTime) {
        _angle = _angle + _steering * 0.125
        _steering = _steering * 0.9
        _speed = _speed * 0.99

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

class Sprite {

    construct new(bank, from, to, scale) {
        _font = Font.default

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
        var speed = _velocity.magnitude
        if (speed == 0) {
            return
        }
        var inertia = _inertia / speed
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
        //var x = _x + _angle.cos * 32
        //var y = _y + _angle.sin * 32
        //Canvas.line(_x, _y, x, y, 32)
    }

}
