#ifndef __GRAPHICS_PALETTES_H__
#define __GRAPHICS_PALETTES_H__

#include "../../display.h"

typedef struct _Palette_t {
    const char *id;
    int count;
    const Color colors[MAX_PALETTE_COLORS];
} Palette_t;

extern const Palette_t *graphics_palettes_find(const char *id);

#endif  /* __GRAPHICS_PALETTES_H__ */
