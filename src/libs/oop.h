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
 * Copyright (c) 2019-2026 Marco Lizza
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

#ifndef TOFU_LIBS_OOP_H
#define TOFU_LIBS_OOP_H

#define OOP_CLASS(type) \
    typedef struct type##_s type##_t;

#define OOP_INTERFACE_BEGIN(type) \
    typedef struct type##_vtable_s {

#define OOP_INTERFACE_METHOD(name, rvalue, ...) \
    (rvalue) (*name)(##__VA_ARGS__);

#define OOP_INTERFACE_END(type) \
    } type##_vtable_t;

#define OOP_CLASS_BEGIN(name, type) \
    typedef struct name##_s { \
        type##_vtable_t _vtable;

#define OOP_CLASS_END(name) \
    } name##_t;

// GCC/Clang statement expression
#define OOP_NEW(type, ...) \
    ({ \
        (type##_t *) _self = malloc(sizeof((type##_t))); \
        _self ? _self->_vtable.ctor(__VA_ARGS__), _self : NULL; \
    })

#define OOP_DELETE(self) \
    do { \
        if (self) { \
            self->_vtable.dtor(self); \
            free(self); \
        } \
    } while (0)

#define OOP_CALL(self, method, ...) \
    ((self)->_vtable.(method)((self), __VA_ARGS__))

#endif  /* TOFU_LIBS_OOP_H */
