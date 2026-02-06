#pragma once

#include <core/config.h>
#include <core/platform.h>
//#define _LOG_TAG "display"
//#include <libs/log.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <string.h>
#include <stdio.h>

#if TOFU_GRAPHICS_PIXEL_FORMAT == PIXEL_FORMAT_RGBA8888
#if PLATFORM_ID == PLATFORM_WINDOWS
    #define _PIXEL_FORMAT GL_BGRA
#else
    #define _PIXEL_FORMAT GL_RGBA
#endif
    #define _PIXEL_TYPE   GL_UNSIGNED_BYTE
    #define _PIXEL_BPP    4
#elif TOFU_GRAPHICS_PIXEL_FORMAT == PIXEL_FORMAT_RGB565
    #define _PIXEL_FORMAT GL_RGB
    #define _PIXEL_TYPE   GL_UNSIGNED_SHORT_5_6_5
    #define _PIXEL_BPP    2
#else
    #error "unsupported TOFU_GRAPHICS_PIXEL_FORMAT"
#endif

#ifndef PBO_RING_SIZE
    #define PBO_RING_SIZE 3
#endif

typedef struct PboRingGL33 {
    GLuint pbo[PBO_RING_SIZE];
    int    idx;
    int    w, h;
    size_t bytes;
} PboRingGL33;

static inline void pbo_ring_gl33_init(PboRingGL33* r, size_t w, size_t h) {
    *r = (PboRingGL33){
            .w = w,
            .h = h,
            .bytes = w * h * _PIXEL_BPP,
            .idx = 0
        };

    glGenBuffers(PBO_RING_SIZE, r->pbo);
    for (int i = 0; i < PBO_RING_SIZE; ++i) {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, r->pbo[i]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, r->bytes, NULL, GL_STREAM_DRAW);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Tight packing (no padding), we are only transferring data (one time set)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

static inline void pbo_ring_gl33_destroy(PboRingGL33* r) {
    glDeleteBuffers(PBO_RING_SIZE, r->pbo);
    *r = (PboRingGL33){ 0 };
}

static inline void pbo_ring_gl33_upload_tex2d_copy_only_no_orphan(PboRingGL33* r, const void* pixels) {
    const GLuint pbo = r->pbo[r->idx];
    r->idx = (r->idx + 1) % PBO_RING_SIZE;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);

    void* dst = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, r->bytes,
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    if (!dst) {
        // fallback: orphan + remap
        glBufferData(GL_PIXEL_UNPACK_BUFFER, r->bytes, NULL, GL_STREAM_DRAW);
        dst = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, r->bytes,
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    }

    if (dst) {
        memcpy(dst, pixels, r->bytes);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, r->w, r->h,
                    _PIXEL_FORMAT, _PIXEL_TYPE, (const void*)0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

// Upload full frame from CPU buffer (tight) into currently bound GL_TEXTURE_2D.
// - cpu_pixels must be tight packed in the correct format (w*h*bpp bytes).
static inline void pbo_ring_gl33_upload_tex2d_copy_only(PboRingGL33* r, const void* cpu_pixels) {
    const GLuint pbo = r->pbo[r->idx];
    r->idx = (r->idx + 1) % PBO_RING_SIZE;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);

    // Orphan every time: for tiny buffers this is usually fine and avoids any in-flight hazard.
    // If you want, we can make it "on demand", but this is simplest + predictable.
    glBufferData(GL_PIXEL_UNPACK_BUFFER, r->bytes, NULL, GL_STREAM_DRAW);

    void* dst = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, r->bytes,
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    if (dst) {
        memcpy(dst, cpu_pixels, r->bytes);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    0, 0, r->w, r->h,
                    _PIXEL_FORMAT, _PIXEL_TYPE,
                    NULL);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}
