/*
 * Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/

static const char graphics_wren[] = 
    "foreign class Canvas {\n"
    "\n"
    "    foreign static width\n"
    "    foreign static height\n"
    "    foreign static point(x, y, color)\n"
    "    foreign static palette(colors)\n"
    "    foreign static bank(bank_id, file, width, height)\n"
    "    foreign static sprite(bank_id, sprite_id, x, y, r, sx, sy)\n"
    "    foreign static text(font_id, text, x, y, color, size, align)\n"
    "\n"
    "    static sprite(bank_id, sprite_id, x, y) {\n"
    "        sprite(bank_id, sprite_id, x, y, 0.0, 1.0, 1.0)\n"
    "    }\n"
    "    static sprite(bank_id, sprite_id, x, y, r) {\n"
    "        sprite(bank_id, sprite_id, x, y, r, 1.0, 1.0)\n"
    "    }\n"
    "\n"
    "}\n";
