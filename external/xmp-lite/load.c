/* Extended Module Player
 * Copyright (C) 1996-2021 Claudio Matsuoka and Hipolito Carraro Jr
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
#include <string.h>
#include <errno.h>
#ifdef __native_client__
#include <sys/syslimits.h>
#else
#include <limits.h>
#endif

#include "format.h"
#include "list.h"
#include "hio.h"
#include "loader.h"
#include "load_helpers.h"
#include "scan.h"

static int test_module(struct xmp_test_info *info, HIO_HANDLE *h)
{
	char buf[XMP_NAME_SIZE];
	int i;

	if (info != NULL) {
		*info->name = 0;	/* reset name prior to testing */
		*info->type = 0;	/* reset type prior to testing */
	}

	for (i = 0; format_loaders[i] != NULL; i++) {
		hio_seek(h, 0, SEEK_SET);
		if (format_loaders[i]->test(h, buf, 0) == 0) {
			if (info != NULL) {
				strncpy(info->name, buf, XMP_NAME_SIZE - 1);
				info->name[XMP_NAME_SIZE - 1] = '\0';

				strncpy(info->type, format_loaders[i]->name,
							XMP_NAME_SIZE - 1);
				info->type[XMP_NAME_SIZE - 1] = '\0';
			}
			return 0;
		}
	}
	return -XMP_ERROR_FORMAT;
}

LIBXMP_EXPORT int xmp_test_module(const char *path, struct xmp_test_info *info)
{
	HIO_HANDLE *h;
	int ret = libxmp_get_filetype(path);

	if (ret == XMP_FILETYPE_NONE) {
		return -XMP_ERROR_SYSTEM;
	}
	if (ret & XMP_FILETYPE_DIR) {
		errno = EISDIR;
		return -XMP_ERROR_SYSTEM;
	}

	if ((h = hio_open(path, "rb")) == NULL)
		return -XMP_ERROR_SYSTEM;

	ret = test_module(info, h);

	hio_close(h);
	return ret;
}

LIBXMP_EXPORT int xmp_test_module_from_memory(const void *mem, long size, struct xmp_test_info *info)
{
	HIO_HANDLE *h;
	int ret;

	if (size <= 0) {
		return -XMP_ERROR_INVALID;
	}

	if ((h = hio_open_mem(mem, size)) == NULL)
		return -XMP_ERROR_SYSTEM;

	ret = test_module(info, h);

	hio_close(h);
	return ret;
}

LIBXMP_EXPORT int xmp_test_module_from_file(void *file, struct xmp_test_info *info)
{
	HIO_HANDLE *h;
	int ret;

	if ((h = hio_open_file((FILE *)file)) == NULL)
		return -XMP_ERROR_SYSTEM;

	ret = test_module(info, h);

	hio_close(h);
	return ret;
}

LIBXMP_EXPORT int xmp_test_module_from_callbacks(size_t (*read)(void *, size_t, size_t, void *), int (*seek)(void *, long, int), long (*tell)(void *), int (*eof)(void *), void *userdata,
									struct xmp_test_info *info)
{
	HIO_HANDLE *h;
	int ret;

	if ((h = hio_open_callbacks((HIO_FUNCS){ .read = read, .seek = seek, .tell = tell, .eof = eof }, userdata)) == NULL)
		return -XMP_ERROR_SYSTEM;

	ret = test_module(info, h);

	hio_close(h);
	return ret;
}

static int load_module(xmp_context opaque, HIO_HANDLE *h)
{
	struct context_data *ctx = (struct context_data *)opaque;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	int i, j, ret;
	int test_result, load_result;

	libxmp_load_prologue(ctx);

	D_(D_WARN "loading");
	test_result = load_result = -1;
	for (i = 0; format_loaders[i] != NULL; i++) {
		hio_seek(h, 0, SEEK_SET);
		hio_error(h); /* reset error flag */

		D_(D_WARN "testing format: %s", format_loaders[i]->name);
		test_result = format_loaders[i]->test(h, NULL, 0);
		if (test_result == 0) {
			hio_seek(h, 0, SEEK_SET);
			D_(D_WARN "loading w/ format: %s", format_loaders[i]->name);
			load_result = format_loaders[i]->loader(m, h, 0);
			break;
		}
	}

	if (test_result < 0) {
		xmp_release_module(opaque);
		return -XMP_ERROR_FORMAT;
	}

	if (load_result < 0) {
		goto err_load;
	}

	/* Sanity check: number of channels, module length */
	if (mod->chn > XMP_MAX_CHANNELS || mod->len > XMP_MAX_MOD_LENGTH) {
		goto err_load;
	}

	/* Sanity check: channel pan */
	for (i = 0; i < mod->chn; i++) {
		if (mod->xxc[i].vol < 0 || mod->xxc[i].vol > 0xff) {
			goto err_load;
		}
		if (mod->xxc[i].pan < 0 || mod->xxc[i].pan > 0xff) {
			goto err_load;
		}
	}

	/* Sanity check: patterns */
	if (mod->xxp == NULL) {
		goto err_load;
	}
	for (i = 0; i < mod->pat; i++) {
		if (mod->xxp[i] == NULL) {
			goto err_load;
		}
		for (j = 0; j < mod->chn; j++) {
			int t = mod->xxp[i]->index[j];
			if (t < 0 || t >= mod->trk || mod->xxt[t] == NULL) {
				goto err_load;
			}
		}
	}

	libxmp_adjust_string(mod->name);
	for (i = 0; i < mod->ins; i++) {
		libxmp_adjust_string(mod->xxi[i].name);
	}
	for (i = 0; i < mod->smp; i++) {
		libxmp_adjust_string(mod->xxs[i].name);
	}

	libxmp_load_epilogue(ctx);

	ret = libxmp_prepare_scan(ctx);
	if (ret < 0) {
		xmp_release_module(opaque);
		return ret;
	}

	ret = libxmp_scan_sequences(ctx);
	if (ret < 0) {
		xmp_release_module(opaque);
		return -XMP_ERROR_LOAD;
	}

	ctx->state = XMP_STATE_LOADED;

	return 0;

    err_load:
	xmp_release_module(opaque);
	return -XMP_ERROR_LOAD;
}

LIBXMP_EXPORT int xmp_load_module(xmp_context opaque, const char *path)
{
	struct context_data *ctx = (struct context_data *)opaque;
	HIO_HANDLE *h;
	int ret;

	D_(D_WARN "path: %s", path);

	ret = libxmp_get_filetype(path);

	if (ret == XMP_FILETYPE_NONE) {
		return -XMP_ERROR_SYSTEM;
	}
	if (ret & XMP_FILETYPE_DIR) {
		errno = EISDIR;
		return -XMP_ERROR_SYSTEM;
	}

	if ((h = hio_open(path, "rb")) == NULL) {
		return -XMP_ERROR_SYSTEM;
	}

	if (ctx->state > XMP_STATE_UNLOADED)
		xmp_release_module(opaque);

	ctx->m.filename = NULL;
	ctx->m.dirname = NULL;
	ctx->m.basename = NULL;

	ret = load_module(opaque, h);
	hio_close(h);

	return ret;
}

LIBXMP_EXPORT int xmp_load_module_from_memory(xmp_context opaque, const void *mem, long size)
{
	struct context_data *ctx = (struct context_data *)opaque;
	struct module_data *m = &ctx->m;
	HIO_HANDLE *h;
	int ret;

	if (size <= 0) {
		return -XMP_ERROR_INVALID;
	}

	if ((h = hio_open_mem(mem, size)) == NULL)
		return -XMP_ERROR_SYSTEM;

	if (ctx->state > XMP_STATE_UNLOADED)
		xmp_release_module(opaque);

	m->filename = NULL;
	m->basename = NULL;
	m->dirname = NULL;
	m->size = size;

	ret = load_module(opaque, h);

	hio_close(h);

	return ret;
}

LIBXMP_EXPORT int xmp_load_module_from_file(xmp_context opaque, void *file)
{
	struct context_data *ctx = (struct context_data *)opaque;
	struct module_data *m = &ctx->m;
	HIO_HANDLE *h;
	int ret;

	if ((h = hio_open_file((FILE *)file)) == NULL)
		return -XMP_ERROR_SYSTEM;

	if (ctx->state > XMP_STATE_UNLOADED)
		xmp_release_module(opaque);

	m->filename = NULL;
	m->basename = NULL;
	m->dirname = NULL;
	m->size = hio_size(h);

	ret = load_module(opaque, h);

	hio_close(h);

	return ret;
}

LIBXMP_EXPORT int xmp_load_module_from_callbacks(xmp_context opaque, size_t (*read)(void *, size_t, size_t, void *), int (*seek)(void *, long, int), long (*tell)(void *), int (*eof)(void *), void *userdata)
{
	struct context_data *ctx = (struct context_data *)opaque;
	struct module_data *m = &ctx->m;
	HIO_HANDLE *h;
	int ret;

	if ((h = hio_open_callbacks((HIO_FUNCS){ .read = read, .seek = seek, .tell = tell, .eof = eof }, userdata)) == NULL)
		return -XMP_ERROR_SYSTEM;

	if (ctx->state > XMP_STATE_UNLOADED)
		xmp_release_module(opaque);

	m->filename = NULL;
	m->basename = NULL;
	m->dirname = NULL;
	m->size = hio_size(h);

	ret = load_module(opaque, h);

	hio_close(h);

	return ret;
}

LIBXMP_EXPORT void xmp_release_module(xmp_context opaque)
{
	struct context_data *ctx = (struct context_data *)opaque;
	struct module_data *m = &ctx->m;
	struct xmp_module *mod = &m->mod;
	int i;

	/* can't test this here, we must call release_module to clean up
	 * load errors
	if (ctx->state < XMP_STATE_LOADED)
		return;
	 */

	if (ctx->state > XMP_STATE_LOADED)
		xmp_end_player(opaque);

	ctx->state = XMP_STATE_UNLOADED;

	D_(D_INFO "freeing memory");

	if (mod->xxt != NULL) {
		for (i = 0; i < mod->trk; i++) {
			free(mod->xxt[i]);
		}
		free(mod->xxt);
		mod->xxt = NULL;
	}

	if (mod->xxp != NULL) {
		for (i = 0; i < mod->pat; i++) {
			free(mod->xxp[i]);
		}
		free(mod->xxp);
		mod->xxp = NULL;
	}

	if (mod->xxi != NULL) {
		for (i = 0; i < mod->ins; i++) {
			free(mod->xxi[i].sub);
			free(mod->xxi[i].extra);
		}
		free(mod->xxi);
		mod->xxi = NULL;
	}

	if (mod->xxs != NULL) {
		for (i = 0; i < mod->smp; i++) {
			libxmp_free_sample(&mod->xxs[i]);
		}
		free(mod->xxs);
		mod->xxs = NULL;
	}

	free(m->xtra);
	m->xtra = NULL;

#ifndef LIBXMP_CORE_DISABLE_IT
	if (m->xsmp != NULL) {
		for (i = 0; i < mod->smp; i++) {
			libxmp_free_sample(&m->xsmp[i]);
		}
		free(m->xsmp);
		m->xsmp = NULL;
	}
#endif

	libxmp_free_scan(ctx);

	free(m->comment);
	m->comment = NULL;

	D_(D_INFO "free dirname/basename");
	free(m->dirname);
	free(m->basename);
	m->basename = NULL;
	m->dirname = NULL;
}

LIBXMP_EXPORT void xmp_scan_module(xmp_context opaque)
{
	struct context_data *ctx = (struct context_data *)opaque;

	if (ctx->state < XMP_STATE_LOADED)
		return;

	libxmp_scan_sequences(ctx);
}
