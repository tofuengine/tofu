/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
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
 */

#if defined(DEBUG) && !defined(SANITIZE)
    #define STB_LEAKCHECK_IMPLEMENTATION
    #include <stb/stb_leakcheck.h>
#endif

#if defined(DEBUG) && !defined(SANITIZE)
    #define STBDS_REALLOC(c,p,s) stb_leakcheck_realloc((p), (s), __FILE__, __LINE__)
    #define STBDS_FREE(c,p)      stb_leakcheck_free((p))
#endif
#define STB_DS_IMPLEMENTATION
#include <stb/stb_ds.h>

#if defined(DEBUG) && !defined(SANITIZE)
    #define STBI_MALLOC(s)    stb_leakcheck_malloc((s), __FILE__, __LINE__)
    #define STBI_REALLOC(p,s) stb_leakcheck_realloc((p), (s), __FILE__, __LINE__)
    #define STBI_FREE(p)      stb_leakcheck_free((p))
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#if defined(DEBUG) && !defined(SANITIZE)
    #define STBIW_MALLOC(s)    stb_leakcheck_malloc((s), __FILE__, __LINE__)
    #define STBIW_REALLOC(p,s) stb_leakcheck_realloc((p), (s), __FILE__, __LINE__)
    #define STBIW_FREE(p)      stb_leakcheck_free((p))
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

void *stb_memdup(const void *ptr, size_t size)
{
    void *copy = malloc(size);
    if (copy) {
        memcpy(copy, ptr, size);
    }
    return copy;
}
