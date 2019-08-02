local Bunny = require("lib.bunny")

local LITTER_SIZE = 250
local MAX_BUNNIES = 32768

local Game = {}

Game.__index = Game

function Game.new(position, angle, fov, radius)
  return setmetatable({}, Game)
end

function Game:input()
  if Input.isKeyPressed(Input.start) then
    for (i in 1 .. LITTER_SIZE) {
        _bunnies.insert(-1, Bunny.new(_random, _bank))
    }
    if (_bunnies.count >= MAX_BUNNIES) {
        Environment.quit()
    }
  elseif Input.is_key_pressed(Input.left) {
    _speed = _speed * 0.5
} else if (Input.is_key_pressed(Input.right)) {
    _speed = _speed * 2.0
} else if (Input.is_key_pressed(Input.down)) {
    _speed = 1.0
} else if (Input.is_key_pressed(Input.select)) {
    _bunnies.clear()
} else if (Input.is_key_pressed(Input.y)) {
    _running = !_running
}
end

function Game:update(delta_time)
end

function Game:render(ratio)
end

class Game {

    construct new() {
        _random = Random.new()

        _bunnies = []

        Canvas.set_palette("gameboy")
        Canvas.set_background(1)

        _bank = Bank.new("./assets/sheet.png", 26, 37)
        _font = Font.default

        _speed = 1.0
        _running = true
    }

    input() {
    }

    update(deltaTime) {
        if (!_running) {
            return
        }
        for (bunny in _bunnies) {
            bunny.update(deltaTime * _speed)
        }
    }

    render(ratio) {
        for (bunny in _bunnies) {
            bunny.render()
        }
        _font:write("FPS: %(Environment.fps.round)", 0, 0, 0, 1.0, "left")
        _font:write("#%(_bunnies.count) bunnies", Canvas.get_width(), 0, 3, 1.0, "right")
    }

}

return Game
