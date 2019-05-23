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

    foreign static points(vertices, color)
    foreign static polyline(vertices, color)
    foreign static strip(vertices, color)
    foreign static fan(vertices, color)

    static point(x0, y0, color) {
        points([ x0, y0 ], color)
    }
    static line(x0, y0, x1, y1, color) {
        polyline([ x0, y0, x1, y1, x0, y0 ], color)
    }
    static triangle(mode, x0, y0, x1, y1, x2, y2, color) {
        if (mode == "line") {
            polyline([ x0, y0, x1, y1, x2, y2, x0, y0 ], color)
        } else {
            strip([ x0, y0, x1, y1, x2, y2 ], color)
        }
    }
    static rectangle(mode, x, y, width, height, color) {
        var offset = mode == "line" ? 1 : 0
        var x0 = x
        var y0 = y
        var x1 = x0 + width - offset
        var y1= y0 + height - offset
        if (mode == "line") {
            polyline([ x0, y0, x0, y1, x1, y1, x1, y0, x0, y0 ], color)
        } else {
            strip([ x0, y0, x0, y1, x1, y0, x1, y1 ], color)
        }
    }
    static square(mode, x, y, size, color) {
        rectangle(mode, x, y, size, size, color)
    }
    static circle(mode, x, y, radius, color) {
        circle(mode, x, y, radius, color, 30)
    }
    static circle(mode, x, y, radius, color, segments) {
        var step = (2 * Num.pi) / segments
        if (mode == "line") {
            var points = []
            for (i in 0 .. segments) {
                var angle = step * i
                points.insert(-1, x + angle.sin * radius)
                points.insert(-1, y + angle.cos * radius)
            }
            Canvas.polyline(points, color)
        } else {
            var points = []
            points.insert(-1, x)
            points.insert(-1, y)
            for (i in 0 .. segments) {
                var angle = step * i
                points.insert(-1, x + angle.sin * radius)
                points.insert(-1, y + angle.cos * radius)
            }
            Canvas.fan(points, color)
        }
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