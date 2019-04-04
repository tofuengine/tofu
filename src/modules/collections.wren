foreign class Grid {

    construct new(width, height) {}

    foreign width
    foreign height
    foreign fill(valueOrList, offset, length)
    foreign row(x, y, count, value)
    foreign peek(x, y)
    foreign poke(x, y, value)

    fill(valueOrList) {
        fill(valueOrList, 0, -1)
    }

}
