import "graphics" for Canvas

var MIN_FREQUENCY = 0.25
var MAX_FREQUENCY = 2.50
var MAX_ANGLE = Num.pi * 2
var CENTER_X = Canvas.width / 2
var CENTER_Y = (Canvas.height - 64) / 2
var PADDING = 16
var X_AMPLITUDE = (Canvas.width / 2) - PADDING
var Y_AMPLITUDE = ((Canvas.height - 64) / 2) - PADDING
var STEP = (2.0 * Num.pi) / 32.0

class Sprite {

    construct new(random, bank, index) {
        _bank = bank

        _angle = STEP * index // (2.0 * Num.pi) random.float() * MAX_ANGLE
         _frequency_x = 0.75 //(random.float() * (MAX_FREQUENCY - MIN_FREQUENCY)) + MIN_FREQUENCY
         _frequency_y = 0.50//(random.float() * (MAX_FREQUENCY - MIN_FREQUENCY)) + MIN_FREQUENCY
         _frequency_s = 3.00 //(random.float() * (MAX_FREQUENCY - MIN_FREQUENCY)) + MIN_FREQUENCY

        _id = random.int(0, 11)
    }

    update(deltaTime) {
        _angle = _angle + deltaTime
    }

    render() {
        var x = CENTER_X + (_angle * _frequency_x).cos * X_AMPLITUDE
        var y = CENTER_Y + (_angle * _frequency_y).sin * Y_AMPLITUDE

        var s = (((_angle * _frequency_s).sin + 1.0) / 2.0) * 1.5 + 0.5
        _bank.blit(_id, x, y, s, s)
    }

}
