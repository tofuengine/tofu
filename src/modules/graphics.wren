foreign class Bank {

    construct new(file, cell_width, cell_height) {}

    foreign cellWidth
    foreign cellHeight

    draw(cell_id, x, y) {
        draw(id, x, y, 0.0)
    }
    draw(cell_id, x, y, r) {
        draw(id, x, y, r, 1.0, 1.0)
    }
    foreign draw(cell_id, x, y, r, sx, sy)

}

foreign class Font {

    construct new(file) {}

    static default { Font.new("default") }

    foreign text(text, x, y, color, size, align)

}

foreign class Canvas {

    foreign static width
    foreign static height
    foreign static palette(colors)

    foreign static point(x, y, color)
    foreign static polygon(mode, vertices, color)
    foreign static circle(mode, x, y, radius, color)

    static line(x0, y0, x1, y1, color) {
        polygon("line", [ x0, y0, x1, y1 ], color)
    }
    static triangle(mode, x0, y0, x1, y1, x2, y2, color) {
        polygon(mode, [ x0, y0, x1, y1, x2, y2, x0, y0 ], color)
    }
    static rectangle(mode, x, y, width, height, color) {
        var offset = mode == "line" ? 1 : 0
        var left = x
        var top = y
        var right = left + width - offset
        var bottom = top + height - offset
        polygon(mode, [ left, top, left, bottom, right, bottom, right, top, left, top ], color)
    }
    static square(mode, x, y, size, color) {
        rectangle(mode, x, y, size, size, color)
    }

}