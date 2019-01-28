foreign class Canvas {

    foreign static width
    foreign static height
    foreign static point(x, y, color)
    foreign static palette(colors)
    foreign static bank(bank_id, file, width, height)
    foreign static sprite(bank_id, sprite_id, x, y, r, sx, sy)
    foreign static text(font_id, text, x, y, color, size)

    static sprite(bank_id, sprite_id, x, y) {
        sprite(bank_id, sprite_id, x, y, 0.0, 1.0, 1.0)
    }
    static sprite(bank_id, sprite_id, x, y, r) {
        sprite(bank_id, sprite_id, x, y, r, 1.0, 1.0)
    }

}