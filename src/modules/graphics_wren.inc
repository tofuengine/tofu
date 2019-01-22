static const char graphics_wren[] = 
    "foreign class Canvas {\n"
    "\n"
    "    foreign static point(x, y, color)\n"
    "    foreign static palette(colors)\n"
    "    foreign static bank(id, file, width, height)\n"
    "    foreign static sprite(bank, id, x, y, r, sx, sy)\n"
    "\n"
    "}\n";
