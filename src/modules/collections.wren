foreign class Grid {

    construct new(width, height, content) {}

    foreign width
    foreign height
    foreign fill(content)
    foreign row(x, y, count, value)
    foreign peek(x, y)
    foreign poke(x, y, value)

}
