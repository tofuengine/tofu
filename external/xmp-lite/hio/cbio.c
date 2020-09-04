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
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include "cbio.h"

CBFILE *cbopen(CBFUNC func, void *ud)
{
	CBFILE *cb;

	cb = (CBFILE *)malloc(sizeof (CBFILE));
	if (cb == NULL)
		return NULL;
	
	cb->func = func;
	cb->ud = ud;

	return cb;
}

int cbgetc(CBFILE *cb)
{
	uint8_t c;
	size_t l = cb->func.read(cb->ud, &c, sizeof(uint8_t));
	if (l == sizeof(uint8_t))
		return c;
	else
		return EOF;
}

size_t cbread(void *buf, size_t size, size_t num, CBFILE *cb)
{
	size_t l = cb->func.read(cb->ud, buf, size * num);
	return l / size;
}

int cbseek(CBFILE *cb, long offset, int whence)
{
	return cb->func.seek(cb->ud, offset, whence);
}

long cbtell(CBFILE *cb)
{
	return cb->func.tell(cb->ud);
}

int cbclose(CBFILE *cb)
{
	free(cb);
	return 0;
}

int	cbeof(CBFILE *cb)
{
	return cb->func.eof(cb->ud);
}
