/* Extended Module Player
 * Copyright (C) 1996-2018 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "fdataio.h"
#include <byteswap.h>
#include <errno.h>
#include "dataio.h"

uint8 fread8(FILE *f, int *err)
{
	uint8 b;
	size_t read = fread(&b, sizeof(uint8), 1, f);
	if (read != 1) {
		*err = ferror(f) ? errno : EOF;
		return 0xFF;
	}
	*err = 0;
	return b;
}

int8 fread8s(FILE *f, int *err)
{
	int8 b;
	size_t read = fread(&b, sizeof(int8), 1, f);
	if (read != 1) {
		*err = ferror(f) ? errno : EOF;
		return 0;
	}
	*err = 0;
	return b;
}

uint16 fread16l(FILE *f, int *err)
{
	uint8 b[sizeof(uint16)];
	size_t read = fread(&b, sizeof(b), 1, f);
	if (read != 1) {
		*err = ferror(f) ? errno : EOF;
		return 0xffff;
	}
	*err = 0;
	return readmem16l(b);
}

uint16 fread16b(FILE *f, int *err)
{
	uint8 b[sizeof(uint16)];
	size_t read = fread(&b, sizeof(b), 1, f);
	if (read != 1) {
		*err = ferror(f) ? errno : EOF;
		return 0xffff;
	}
	*err = 0;
	return readmem16b(b);
}

uint32 fread32l(FILE *f, int *err)
{
	uint8 b[sizeof(uint32)];
	size_t read = fread(&b, sizeof(b), 1, f);
	if (read != 1) {
		*err = ferror(f) ? errno : EOF;
		return 0xffffffff;
	}
	*err = 0;
	return readmem32l(b);
}

uint32 fread32b(FILE *f, int *err)
{
	uint8 b[sizeof(uint32)];
	size_t read = fread(&b, sizeof(b), 1, f);
	if (read != 1) {
		*err = ferror(f) ? errno : EOF;
		return 0xffffffff;
	}
	*err = 0;
	return readmem32b(b);
}
