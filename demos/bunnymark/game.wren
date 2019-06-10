import "random" for Random

import "graphics" for Bank, Canvas, Font
import "events" for Environment, Input
import "util" for Timer

import "./lib/bunny" for Bunny

var LITTER_SIZE = 250
var MAX_BUNNIES = 32768

var SCALINES = "\n" +
    "const float amount = 0.5;\n" +
    "const float thickness = 1.0;\n" +
    "const float spacing = 1.0;\n" +
    "\n" +
    "vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {\n" +
    "    vec4 texel = texture2D(texture, texture_coords) * color;\n" +
    "    if (mod(screen_coords.y, round(thickness + spacing)) < round(spacing)) {\n" +
	"        return vec4(texel.rgb * (1.0 - amount), texel.a);\n" +
    "    }\n" +
    "    return texel;\n" +
    "}\n"

var BARREL = "\n" +
    "const float thickness = 3.0;\n" +
    "\n" +
    "vec4 effect(vec4 color, sampler2D texture, vec2 texture_coords, vec2 screen_coords) {\n" +
    "    vec2 delta = texture_coords - vec2(0.5, 0.5);\n" +
    "    vec2 uv_r = delta * 0.0250 + texture_coords;\n" +
    "    vec2 uv_g = delta * 0.0075 + texture_coords;\n" +
    "    vec2 uv_b = delta * 0.0150 + texture_coords;\n" +
    "    vec4 r = texture2D(texture, uv_r);\n" +
    "    vec4 g = texture2D(texture, uv_g);\n" +
    "    vec4 b = texture2D(texture, uv_b);\n" +
    "    vec4 texel = vec4(r.r, g.g, b.b, 1.0)\n;" +
    "    float y = (cos(u_time * 1.0) + 1) * 0.5 * u_resolution.y;\n" +
    "    float d = abs(y - screen_coords.y);\n" +
    "    if (d > thickness) {\n" +
    "        return texel;\n" +
    "    } else {\n" +
    "        return mix(texel, vec4(1.0, 1.0, 1.0, 1.0), 1.0 - d / thickness);\n" +
    "    }\n" +
    "}\n"

class Game {

    construct new() {
        _random = Random.new()

        _bunnies = []

        Canvas.palette = "gameboy"
        Canvas.background = 1
        Canvas.shader = BARREL
        //Canvas.shader = SCALINES
        //Canvas.send("u_strength", 100)

        var palette = Canvas.palette
        System.write(palette)

        _bank = Bank.new("./assets/sheet.png", 26, 37)
        _font = Font.default

        _speed = 1.0
        _running = true

        _tooggle = true
        _timer = Timer.new(5.0, 0, Fn.new {
                _tooggle = !_tooggle
                Canvas.palette = _tooggle ? "gameboy" : "gameboy-bw"
            })
    }

    input() {
        if (Input.isKeyPressed(Input.start)) {
            for (i in 1 .. LITTER_SIZE) {
                _bunnies.insert(-1, Bunny.new(_random, _bank))
            }
            if (_bunnies.count >= MAX_BUNNIES) {
                Environment.quit()
            }
        } else if (Input.isKeyPressed(Input.left)) {
            _speed = _speed * 0.5
        } else if (Input.isKeyPressed(Input.right)) {
            _speed = _speed * 2.0
        } else if (Input.isKeyPressed(Input.down)) {
            _speed = 1.0
        } else if (Input.isKeyPressed(Input.select)) {
            _bunnies.clear()
        } else if (Input.isKeyPressed(Input.y)) {
            _running = !_running
        }
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
        _font.write("FPS: %(Environment.fps.round)", 0, 0, 0, 1.0, "left")
        _font.write("#%(_bunnies.count) bunnies", Canvas.width, 0, 3, 1.0, "right")
    }

}