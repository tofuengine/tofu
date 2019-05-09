#ifndef __GRAPHICS_PALETTES_H__
#define __GRAPHICS_PALETTES_H__

#include "../../display.h"

typedef struct _Predefined_Palette_t {
    const char *id;
    int count;
    const Color colors[MAX_PALETTE_COLORS];
} Predefined_Palette_t;

extern const Predefined_Palette_t *graphics_palettes_find(const char *id);

#endif  /* __GRAPHICS_PALETTES_H__ */
