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
#include <memory.h>
#include <errno.h>

#include "dataio.h"

typedef struct {
	int (*close)(void *);
	size_t (*read)(void *, size_t, size_t, void *);
	int (*seek)(void *, long, int);
	long (*tell)(void *);
	int (*eof)(void *);
} HIO_VTABLE;

struct HIO_HANDLE {
	HIO_VTABLE vtable;
	void *user_data;
	int error;
};

static void *fio_open(const void *path, const char *mode)
{
	return fopen(path, mode);
}

static size_t fio_read(void *buffer, size_t size, size_t count, void *user_data)
{
	return fread(buffer, size, count, (FILE *)user_data);
}

static int fio_seek(void *user_data, long offset, int whence)
{
	return fseek((FILE *)user_data, offset, whence);
}

static long fio_tell(void *user_data)
{
	return ftell((FILE *)user_data);
}

static int fio_close(void *user_data)
{
	return fclose((FILE *)user_data);
}

static int fio_eof(void *user_data)
{
	return feof((FILE *)user_data);
}

typedef struct {
	const unsigned char *start;
	size_t pos;
	size_t size;
} MFILE;

static void *mio_open(const void *ptr, size_t size)
{
	MFILE *m = (MFILE *)malloc(sizeof(MFILE));
	if (!m)
		return NULL;
	
	m->start = ptr;
	m->pos = 0;
	m->size = size;

	return m;
}

static int mio_close(void *m)
{
	free(m);
	return 0;
}

static size_t mio_read(void *buf, size_t size, size_t num, void *handle)
{
	MFILE *m = (MFILE *)handle;

	size_t should_read = size * num;
	size_t can_read = m->size - m->pos;

	if (!size || !num || can_read == 0) {
		return 0;
	}

	if (should_read > can_read) {
		should_read = can_read;
	}

	memcpy(buf, m->start + m->pos, should_read);
	m->pos += should_read;

	return should_read / size;
}

static int mio_seek(void *handle, long offset, int whence)
{
	MFILE *m = (MFILE *)handle;

	long position;

	switch (whence) {
		default:
		case SEEK_SET:
			position = offset;
			break;
		case SEEK_CUR:
			position = (long)m->pos + offset;
			break;
		case SEEK_END:
			position = (long)(m->size - 1) + offset;
			break;
	}

	if (position < 0 || (size_t)position >= m->size)
		return -1;

	m->pos = (size_t)position;

	return 0;
}

static long mio_tell(void *handle)
{
	MFILE *m = (MFILE *)handle;
	return (long)m->pos;
}

static int mio_eof(void *handle)
{
	MFILE *m = (MFILE *)handle;
	return m->pos >= m->size ? 1 : 0;
}

typedef struct {
	HIO_FUNCS funcs;
	void *user_data;
} CBFILE;

static void *cbio_open(HIO_FUNCS funcs, void *user_data)
{
	CBFILE *cb = (CBFILE *)malloc(sizeof(CBFILE));
	if (!cb)
		return NULL;
	
	cb->funcs = funcs;
	cb->user_data = user_data;

	return cb;
}

static int cbio_close(void *cb)
{
	free(cb);
	return 0;
}

static size_t cbio_read(void *buf, size_t size, size_t num, void *handle)
{
	CBFILE *cb = (CBFILE *)handle;
	return cb->funcs.read(buf, size, num, cb->user_data);
}

static int cbio_seek(void *handle, long offset, int whence)
{
	CBFILE *cb = (CBFILE *)handle;
	return cb->funcs.seek(cb->user_data, offset, whence);
}

static long cbio_tell(void *handle)
{
	CBFILE *cb = (CBFILE *)handle;
	return cb->funcs.tell(cb->user_data);
}

static int cbio_eof(void *handle)
{
	CBFILE *cb = (CBFILE *)handle;
	return cb->funcs.eof(cb->user_data);
}

HIO_HANDLE *hio_open(const void *path, const char *mode)
{
	FILE *fp = fio_open(path, mode);
	if (!fp) {
		return NULL;
	}

	HIO_HANDLE *h = (HIO_HANDLE *)malloc(sizeof(HIO_HANDLE));
	if (!h) {
		fclose(fp);
		return NULL;
	}

	*h = (HIO_HANDLE){
			.vtable = (HIO_VTABLE){
					.close = fio_close,
					.read = fio_read,
					.seek = fio_seek,
					.tell = fio_tell,
					.eof = fio_eof,
				},
			.user_data = fp,
			.error = 0
		};

	return h;
}

HIO_HANDLE *hio_open_file(FILE *fp)
{
	HIO_HANDLE *h = (HIO_HANDLE *)malloc(sizeof(HIO_HANDLE));
	if (!h) {
		fclose(fp);
		return NULL;
	}

	*h = (HIO_HANDLE){
			.vtable = (HIO_VTABLE){
					.close = fio_close,
					.read = fio_read,
					.seek = fio_seek,
					.tell = fio_tell,
					.eof = fio_eof,
				},
			.user_data = fp,
			.error = 0
		};

	return h;
}

HIO_HANDLE *hio_open_mem(const void *ptr, long size)
{
	void *mp = mio_open(ptr, size);
	if (!mp) {
		return NULL;
	}

	HIO_HANDLE *h = (HIO_HANDLE *)malloc(sizeof(HIO_HANDLE));
	if (!h) {
		mio_close(mp);
		return NULL;
	}

	*h = (HIO_HANDLE){
			.vtable = (HIO_VTABLE){
					.close = mio_close,
					.read = mio_read,
					.seek = mio_seek,
					.tell = mio_tell,
					.eof = mio_eof,
				},
			.user_data = mp,
			.error = 0
		};

	return h;
}

HIO_HANDLE *hio_open_callbacks(HIO_FUNCS funcs, void *user_data)
{
	void *cbp = cbio_open(funcs, user_data);
	if (!cbp) {
		return NULL;
	}

	HIO_HANDLE *h = (HIO_HANDLE *)malloc(sizeof(HIO_HANDLE));
	if (!h) {
		cbio_close(cbp);
		return NULL;
	}

	*h = (HIO_HANDLE){
			.vtable = (HIO_VTABLE){
					.close = cbio_close,
					.read = cbio_read,
					.seek = cbio_seek,
					.tell = cbio_tell,
					.eof = cbio_eof,
				},
			.user_data = cbp,
			.error = 0
		};

	return h;

}

int hio_close(HIO_HANDLE *h)
{
	int result = h->vtable.close(h->user_data);
	free(h);
	return result;
}

long hio_size(HIO_HANDLE *h)
{
	long pos = h->vtable.tell(h->user_data);
	if (pos >= 0) {
		if (h->vtable.seek(h->user_data, 0, SEEK_END) < 0) {
			return -1;
		}
		long size = h->vtable.tell(h->user_data);
		if (h->vtable.seek(h->user_data, pos, SEEK_SET) < 0) {
			return -1;
		}
		return size;
	} else {
		return pos;
	}
}

int hio_seek(HIO_HANDLE *h, long offset, int whence)
{
	int result = h->vtable.seek(h->user_data, offset, whence);
	if (result < 0) {
		h->error = EOF;
	}
	return result;
}

long hio_tell(HIO_HANDLE *h)
{
	long result = h->vtable.tell(h->user_data);
	if (result < 0) {
		h->error = EOF;
	}
	return result;
}

int hio_eof(HIO_HANDLE *h)
{
	return h->vtable.eof(h->user_data);
}

int hio_error(HIO_HANDLE *h)
{
	int error = h->error;
	h->error = 0;
	return error;
}

int8_t hio_read8s(HIO_HANDLE *h)
{
	int8_t value;
	size_t result = h->vtable.read(&value, sizeof(int8_t), 1, h->user_data);
	if (result != 1) {
		h->error = EOF;
		return 0;
	}
	return value;
}

uint8_t hio_read8(HIO_HANDLE *h)
{
	uint8_t value;
	size_t result = h->vtable.read(&value, sizeof(uint8_t), 1, h->user_data);
	if (result != 1) {
		h->error = EOF;
		return 0;
	}
	return value;
}

uint16_t hio_read16l(HIO_HANDLE *h)
{
	uint8_t value[2];
	size_t result = h->vtable.read(&value, sizeof(uint8_t), 2, h->user_data);
	if (result != 2) {
		h->error = EOF;
		return 0;
	}
	return readmem16l(value);
}

uint16_t hio_read16b(HIO_HANDLE *h)
{
	uint8_t value[2];
	size_t result = h->vtable.read(&value, sizeof(uint8_t), 2, h->user_data);
	if (result != 2) {
		h->error = EOF;
		return 0;
	}
	return readmem16b(value);
}

uint32_t hio_read32l(HIO_HANDLE *h)
{
	uint8_t value[4];
	size_t result = h->vtable.read(&value, sizeof(uint8_t), 4, h->user_data);
	if (result != 4) {
		h->error = EOF;
		return 0;
	}
	return readmem32l(value);
}

uint32_t hio_read32b(HIO_HANDLE *h)
{
	uint8_t value[4];
	size_t result = h->vtable.read(&value, sizeof(uint8_t), 4, h->user_data);
	if (result != 4) {
		h->error = EOF;
		return 0;
	}
	return readmem32b(value);
}

size_t hio_read(void *buf, size_t size, size_t num, HIO_HANDLE *h)
{
	size_t result = h->vtable.read(buf, size, num, h->user_data);
	if (result != num) {
		h->error = EOF;
	}
	return result;
}
