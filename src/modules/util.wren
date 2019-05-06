foreign class Timer {

    construct new(period, repeats, callback) {}

    foreign reset()
    foreign cancel()

}

foreign class Math {

    static max(a, b) {
        if (a > b) {
            return a
        }
        return b
    }

    static min(a, b) {
        if (a < b) {
            return a
        }
        return b
    }

}