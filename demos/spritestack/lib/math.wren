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

    isZero {
        return _x == 0 && _y == 0
    }

    isEqual(v) {
        return _x == v.x && _y == v.y
    }

    clone {
        return Vector2D.new(_x, _y)
    }

    assign(v) {
        _x = v.x
        _y== v.y
    }

    add(v) {
        return Vector2D.new(_x + v.x, _y + v.y)
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
        var s = dot(v) / v.dot(v)
        return Vector2D.new(s * v.x, s * v.y)
    }

    //       a dot b
    // a - 2 ------- b
    //       b dot b
    //
    // https://math.stackexchange.com/questions/2239169/reflecting-a-vector-over-another-line
    mirror(v) {
        var s = 2 * dot(v) / v.dot(v)
        return Vector2D.new(_x - s * v.x, _y - s * v.y)
    }

    dot(v) {
        return (_x * v.x) + (_y * v.y)
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

    distanceToSquared(v) {
        var dx = v.x - _x 
        var dy = v.y - _y
        return (dx * dx) + (dy * dy)
    }

    distanceTo(v) {
        return distanceToSquared(v).sqrt
    }

    angleTo(v) {
        return (v.y - _y).atan(v.x - _x)
    }

    angleBetween(v) {
        return _y.atan(self.x) - v.y.atan(v.x)
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

}
