foreign class Grid {

    construct new(width, height, content) {}

    foreign width
    foreign height
    foreign fill(content)
    foreign stride(column, row, count, value)
    foreign peek(column, row)
    foreign poke(column, row, value)

}
