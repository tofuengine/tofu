#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <stdbool.h>

#include <raylib/raylib.h>

typedef struct _Display_Configuration_t {
    int width, height;
    int colors;
    bool fullscreen;
    bool autofit;
    bool hide_cursor;
    bool display_fps;
} Display_Configuration_t;

typedef struct _Display_t {
    Display_Configuration_t configuration;

    int window_width, window_height, window_scale;

    RenderTexture2D offscreen;
    Rectangle offscreen_source, offscreen_destination;
    Vector2 offscreen_origin;
} Display_t;

extern bool Display_initialize(Display_t *display, const Display_Configuration_t *configuration, const char *title);
extern bool Display_shouldClose(Display_t *display);
extern void Display_renderBegin(Display_t *display, void callback(void));
extern void Display_renderEnd(Display_t *display, void callback(void), const double fps, const double delta_time);
extern void Display_terminate(Display_t *display);

#endif  /* __DISPLAY_H__ */