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

#include "hio.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "fdataio.h"
#include "mdataio.h"
#include "cbdataio.h"

static long get_size_cb(CBFUNC func, void *ud)
{
	long size, pos;

	pos = func.tell(ud);
	if (pos >= 0) {
		if (func.seek(ud, 0, SEEK_END) < 0) {
			return -1;
		}
		size = func.tell(ud);
		if (func.seek(ud, pos, SEEK_SET) < 0) {
			return -1;
		}
		return size;
	} else {
		return pos;
	}
}

static long get_size(FILE *f)
{
	long size, pos;

	pos = ftell(f);
	if (pos >= 0) {
		if (fseek(f, 0, SEEK_END) < 0) {
			return -1;
		}
		size = ftell(f);
		if (fseek(f, pos, SEEK_SET) < 0) {
			return -1;
		}
		return size;
	} else {
		return pos;
	}
}

int8_t hio_read8s(HIO_HANDLE *h)
{
	int err;
	int8_t ret = 0;

	switch (HIO_HANDLE_TYPE(h)) {
	case HIO_HANDLE_TYPE_FILE:
		ret = fread8s(h->handle.file, &err);
		if (err != 0) {
			h->error = err;
		}
		break;
	case HIO_HANDLE_TYPE_MEMORY:
		ret = mread8s(h->handle.mem, &err);
		if (err != 0) {
			h->error = err;
		}
		break;
	case HIO_HANDLE_TYPE_CALLBACKS:
		ret = cbread8s(h->handle.cb, &err);
		if (err != 0) {
			h->error = err;
		}
	}

	return ret;
}

uint8_t hio_read8(HIO_HANDLE *h)
{
	int err;
	uint8_t ret = 0;

	switch (HIO_HANDLE_TYPE(h)) {
	case HIO_HANDLE_TYPE_FILE:
		ret = fread8(h->handle.file, &err);
		if (err != 0) {
			h->error = err;
		}
		break;
	case HIO_HANDLE_TYPE_MEMORY:
		ret = mread8(h->handle.mem, &err);
		if (err != 0) {
			h->error = err;
		}
		break;
	case HIO_HANDLE_TYPE_CALLBACKS:
		ret = cbread8(h->handle.cb, &err);
		if (err != 0) {
			h->error = err;
		}
	}

	return ret;
}

uint16_t hio_read16l(HIO_HANDLE *h)
{
	int err;
	uint16_t ret = 0;

	switch (HIO_HANDLE_TYPE(h)) {
	case HIO_HANDLE_TYPE_FILE:
		ret = fread16l(h->handle.file, &err);
		if (err != 0) {
			h->error = err;
		}
		break;
	case HIO_HANDLE_TYPE_MEMORY:
		ret = mread16l(h->handle.mem, &err);
		if (err != 0) {
			h->error = err;
		}
		break;
	case HIO_HANDLE_TYPE_CALLBACKS:
		ret = cbread16l(h->handle.cb, &err);
		if (err != 0) {
			h->error = err;
		}
	}

	return ret;
}

uint16_t hio_read16b(HIO_HANDLE *h)
{
	int err;
	uint16_t ret = 0;

	switch (HIO_HANDLE_TYPE(h)) {
	case HIO_HANDLE_TYPE_FILE:
		ret = fread16b(h->handle.file, &err);
		if (err != 0) {
			h->error = err;
		}
		break;
	case HIO_HANDLE_TYPE_MEMORY:
		ret = mread16b(h->handle.mem, &err);
		if (err != 0) {
			h->error = err;
		}
		break;
	case HIO_HANDLE_TYPE_CALLBACKS:
		ret = cbread16b(h->handle.cb, &err);
		if (err != 0) {
			h->error = err;
		}
	}

	return ret;
}

uint32_t hio_read32l(HIO_HANDLE *h)
{
	int err;
	uint32_t ret = 0;

	switch (HIO_HANDLE_TYPE(h)) {
	case HIO_HANDLE_TYPE_FILE:
		ret = fread32l(h->handle.file, &err);
		if (err != 0) {
			h->error = err;
		}
		break;
	case HIO_HANDLE_TYPE_MEMORY:
		ret = mread32l(h->handle.mem, &err);
		if (err != 0) {
			h->error = err;
		}
		break;
	case HIO_HANDLE_TYPE_CALLBACKS:
		ret = cbread32l(h->handle.cb, &err);
		if (err != 0) {
			h->error = err;
		}
	}

	return ret;
}

uint32_t hio_read32b(HIO_HANDLE *h)
{
	int err;
	uint32_t ret = 0;

	switch (HIO_HANDLE_TYPE(h)) {
	case HIO_HANDLE_TYPE_FILE:
		ret = fread32b(h->handle.file, &err);
		if (err != 0) {
			h->error = err;
		}
		break;
	case HIO_HANDLE_TYPE_MEMORY:
		ret = mread32b(h->handle.mem, &err);
		if (err != 0) {
			h->error = err;
		}
		break;
	case HIO_HANDLE_TYPE_CALLBACKS:
		ret = cbread32b(h->handle.cb, &err);
		if (err != 0) {
			h->error = err;
		}
	}

	return ret;
}

size_t hio_read(void *buf, size_t size, size_t num, HIO_HANDLE *h)
{
	size_t ret = 0;

	switch (HIO_HANDLE_TYPE(h)) {
	case HIO_HANDLE_TYPE_FILE:
		ret = fread(buf, size, num, h->handle.file);
		if (ret != num) {
			if (ferror(h->handle.file)) {
				h->error = errno;
			} else {
				h->error = feof(h->handle.file) ? EOF : -2;
			}
		}
		break;
	case HIO_HANDLE_TYPE_MEMORY:
		ret = mread(buf, size, num, h->handle.mem);
		if (ret != num) {
			h->error = errno;
		}
		break;
	case HIO_HANDLE_TYPE_CALLBACKS:
		ret = cbread(buf, size, num, h->handle.cb);
		if (ret != num) {
			h->error = errno;
		}
		break;
	}

	return ret;
}

int hio_seek(HIO_HANDLE *h, long offset, int whence)
{
	int ret = -1;

	switch (HIO_HANDLE_TYPE(h)) {
	case HIO_HANDLE_TYPE_FILE:
		ret = fseek(h->handle.file, offset, whence);
		if (ret < 0) {
			h->error = errno;
		}
		break;
	case HIO_HANDLE_TYPE_MEMORY:
		ret = mseek(h->handle.mem, offset, whence);
		if (ret < 0) {
			h->error = errno;
		}
		break;
	case HIO_HANDLE_TYPE_CALLBACKS:
		ret = cbseek(h->handle.cb, offset, whence);
		if (ret < 0) {
			h->error = errno;
		}
		break;
	}

	return ret;
}

long hio_tell(HIO_HANDLE *h)
{
	long ret = -1;

	switch (HIO_HANDLE_TYPE(h)) {
	case HIO_HANDLE_TYPE_FILE:
		ret = ftell(h->handle.file);
		if (ret < 0) {
			h->error = errno;
		}
		break;
	case HIO_HANDLE_TYPE_MEMORY:
		ret = mtell(h->handle.mem);
		if (ret < 0) {
			h->error = errno;
		}
		break;
	case HIO_HANDLE_TYPE_CALLBACKS:
		ret = cbtell(h->handle.cb);
		if (ret < 0) {
			h->error = errno;
		}
		break;
	}

	return ret;
}

int hio_eof(HIO_HANDLE *h)
{
	switch (HIO_HANDLE_TYPE(h)) {
	case HIO_HANDLE_TYPE_FILE:
		return feof(h->handle.file);
	case HIO_HANDLE_TYPE_MEMORY:
		return meof(h->handle.mem);
	case HIO_HANDLE_TYPE_CALLBACKS:
		return cbeof(h->handle.cb);
	default:
		return EOF;
	}
}

int hio_error(HIO_HANDLE *h)
{
	int error = h->error;
	h->error = 0;
	return error;
}

HIO_HANDLE *hio_open(const void *path, const char *mode)
{
	HIO_HANDLE *h;

	h = (HIO_HANDLE *)malloc(sizeof (HIO_HANDLE));
	if (h == NULL)
		goto err;
	
	h->error = 0;
	h->type = HIO_HANDLE_TYPE_FILE;
	h->handle.file = fopen(path, mode);
	if (h->handle.file == NULL)
		goto err2;

	h->size = get_size(h->handle.file);
	if (h->size < 0)
		goto err3;

	return h;

    err3:
	fclose(h->handle.file);
    err2:
	free(h);
    err:
	return NULL;
}

HIO_HANDLE *hio_open_mem(const void *ptr, long size)
{
	HIO_HANDLE *h;

	h = (HIO_HANDLE *)malloc(sizeof (HIO_HANDLE));
	if (h == NULL)
		return NULL;
	
	h->error = 0;
	h->type = HIO_HANDLE_TYPE_MEMORY;
	h->handle.mem = mopen(ptr, size);
	h->size = size;

	return h;
}

HIO_HANDLE *hio_open_file(FILE *f)
{
	HIO_HANDLE *h;

	h = (HIO_HANDLE *)malloc(sizeof (HIO_HANDLE));
	if (h == NULL)
		return NULL;
	
	h->error = 0;
	h->type = HIO_HANDLE_TYPE_FILE;
	h->handle.file = f /*fdopen(fileno(f), "rb")*/;
	h->size = get_size(f);

	return h;
}

HIO_HANDLE *hio_open_callbacks(CBFUNC func, void *ud)
{
	HIO_HANDLE *h;

	h = (HIO_HANDLE *)malloc(sizeof (HIO_HANDLE));
	if (h == NULL)
		return NULL;
	
	h->error = 0;
	h->type = HIO_HANDLE_TYPE_CALLBACKS;
	h->handle.cb = cbopen(func, ud);
	h->size = get_size_cb(func, ud);

	return h;
}

int hio_close(HIO_HANDLE *h)
{
	int ret;

	switch (HIO_HANDLE_TYPE(h)) {
	case HIO_HANDLE_TYPE_FILE:
		ret = fclose(h->handle.file);
		break;
	case HIO_HANDLE_TYPE_MEMORY:
		ret = mclose(h->handle.mem);
		break;
	case HIO_HANDLE_TYPE_CALLBACKS:
		ret = cbclose(h->handle.cb);
		break;
	default:
		ret = -1;
	}

	free(h);
	return ret;
}

long hio_size(HIO_HANDLE *h)
{
	return h->size;
}
