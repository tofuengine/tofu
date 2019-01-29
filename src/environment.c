#include "environment.h"

#include <stdlib.h>
#include <string.h>

void Environment_initialize(Environment_t *environment, const char *base_path, int width, int height)
{
    strcpy(environment->base_path, base_path);
    environment->should_close = false;

    Graphics_t *graphics = &environment->graphics;
    *graphics = (Graphics_t){
            .width = width,
            .height = height
        };

    for (size_t i = 0; i < MAX_GRAPHIC_BANKS; ++i) {
        Bank_t *bank = &graphics->banks[i];
        *bank = (Bank_t){};
    }
}

void Environment_terminate(Environment_t *environment)
{
    Graphics_t *graphics = &environment->graphics;

    for (size_t i = 0; i < MAX_GRAPHIC_BANKS; ++i) {
        Bank_t *bank = &graphics->banks[i];
        if (bank->atlas.id > 0) {
            UnloadTexture(bank->atlas);
            bank->atlas.id = 0;
        }
    }
}
