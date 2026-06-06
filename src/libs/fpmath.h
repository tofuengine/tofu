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

#ifndef TOFU_LIBS_FPMATH_H
#define TOFU_LIBS_FPMATH_H

#include <stdint.h>

#define FIXED32_SHIFT 16
#define FIXED32_ONE   (1 << FIXED32_SHIFT)

typedef int32_t fixed32_t;

#define FIXED32_FROM_FLOAT(v) ((fixed32_t)((v) * (float)FIXED32_ONE + ((v) >= 0 ? 0.5f : -0.5f)))
#define FIXED32_FROM_DOUBLE(v) ((fixed32_t)((v) * (double)FIXED32_ONE + ((v) >= 0 ? 0.5 : -0.5)))

#define FIXED32_TO_FLOAT(v) ((float)(v) / (float)FIXED32_ONE)
#define FIXED32_TO_DOUBLE(v) ((double)(v) / (double)FIXED32_ONE)
#define FIXED32_TO_INT(v) ((int)((v) >> FIXED32_SHIFT))

#endif  /* TOFU_LIBS_FPMATH_H */