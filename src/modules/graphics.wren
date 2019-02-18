foreign class Canvas {

    foreign static width
    foreign static height
    foreign static palette(colors)
//    foreign static font(font_id, family, size)
    foreign static bank(bank_id, file, width, height)

    foreign static text(font_id, text, x, y, color, size, align)

    foreign static point(x, y, color)
    foreign static polygon(mode, vertices, color)
    foreign static circle(mode, x, y, radius, color)

    foreign static sprite(bank_id, sprite_id, x, y, r, sx, sy)

    static line(x0, y0, x1, y1, color) {
        polygon("line", [ x0, y0, x1, y1 ], color)
    }
    static triangle(mode, x0, y0, x1, y1, x2, y2, color) {
        polygon(mode, [ x0, y0, x1, y1, x2, y2, x0, y0 ], color)
    }
    static rectangle(mode, x, y, width, height, color) {
        var left = x
        var top = y
        var right = left + width - 1
        var bottom = top + height - 1
        polygon(mode, [ left, top, left, bottom, right, bottom, right, top, left, top ], color)
    }
    static square(mode, x, y, size, color) {
        var left = x
        var top = y
        var right = left + size - 1
        var bottom = top + size - 1
        polygon(mode, [ left, top, left, bottom, right, bottom, right, top, left, top ], color)
    }

    static sprite(bank_id, sprite_id, x, y) {
        sprite(bank_id, sprite_id, x, y, 0.0, 1.0, 1.0)
    }
    static sprite(bank_id, sprite_id, x, y, r) {
        sprite(bank_id, sprite_id, x, y, r, 1.0, 1.0)
    }

}