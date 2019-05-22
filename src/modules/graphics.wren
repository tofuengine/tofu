foreign class Bank {

    construct new(file, cellWidth, cellWeight) {}

    foreign cellWidth
    foreign cellHeight

    blit(cellId, x, y) {
        blit(cellId, x, y, 1.0, 1.0, 1.0)
    }
    blit(cellId, x, y, r) {
        blit(cellId, x, y, r, 1.0, 1.0)
    }
    foreign blit(cellId, x, y, r, sx, sy)

}

foreign class Font {

    construct new(file, glyphWidth, glyphHeight) {}

    static default { Font.new("default") }

    foreign write(text, x, y, color, scale, align)

}

foreign class Canvas {

    foreign static width
    foreign static height
    foreign static palette(colors)

    foreign static point(x, y, color)
    foreign static line(x0, y0, x1, y1, color)
    foreign static polygon(mode, vertices, color)
    foreign static circle(mode, x, y, radius, color)

    static triangle(mode, x0, y0, x1, y1, x2, y2, color) {
        polygon(mode, [ x0, y0, x1, y1, x2, y2, x0, y0 ], color)
    }
    static rectangle(mode, x, y, width, height, color) {
        var offset = mode == "line" ? 1 : 0
        var left = x
        var top = y
        var right = left + width - offset
        var bottom = top + height - offset
        polygon(mode, [ left, top, left, bottom, right, top, right, bottom ], color) // CCW strip
    }
    static square(mode, x, y, size, color) {
        rectangle(mode, x, y, size, size, color)
    }

}

class BankBatch {

    construct new(bank) {
        _bank = bank
        _batch = []
    }

    push(cellId, x, y) {
        _batch.add(BankBatchEntry.new(cellId, x, y, 0.0, 1.0, 1.0, 0))
    }

    push(cellId, x, y, r) {
        _batch.add(BankBatchEntry.new(cellId, x, y, r, 1.0, 1.0, 0))
    }

    push(cellId, x, y, r, sx, sy) {
        _batch.add(BankBatchEntry.new(cellId, x, y, r, sx, sy, 0))
    }

    push(cellId, x, y, r, sx, sy, priority) {
        var item = BankBatchEntry.new(cellId, x, y, r, sx, sy, priority)

        var index = _batch.count
        for (i in 0 ... _batch.count) {
            var other = _batch[i]
            if (item.priority <= other.priority) {
                index = i
                break
            }
        }
        _batch.insert(index, item)
    }

    clear() {
        _batch.clear()
    }

    blit() {
        for (item in _batch) {
            item.blit(_bank)
        }
    }

}

class BankBatchEntry {

    construct new(cellId, x, y, r, sx, sy, priority) {
        _cellId = cellId
        _cameraX = x
        _cameraY = y
        _r = r
        _sx = sx
        _sy = sy
        _priority = priority
    }

    priority {
        return _priority
    }

    blit(bank) {
        bank.blit(_cellId, _cameraX, _cameraY, _r, _sx, _sy)
    }

}