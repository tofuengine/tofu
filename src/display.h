#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <stdbool.h>

#include <raylib/raylib.h>

typedef struct _Display_t {
    bool display_fps;

    int window_width, window_height, window_scale;

    int width, height;
    RenderTexture2D offscreen;
} Display_t;

extern void Display_initialize(Display_t *display, const int width, const int height, const char *title, bool hide_cursor);
extern bool Display_shouldClose(Display_t *display);
extern void Display_renderBegin(Display_t *display, void callback(void));
extern void Display_renderEnd(Display_t *display, void callback(void), const double fps, const double delta_time);
extern void Display_terminate(Display_t *display);

#endif  /* __DISPLAY_H__ */