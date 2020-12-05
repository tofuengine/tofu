#ifndef LIBXMP_FORMAT_H
#define LIBXMP_FORMAT_H

#include "common.h"
#include "hio.h"

struct format_loader {
	const char *name;
	int (*const test)(HIO_HANDLE *, char *, const int);
	int (*const loader)(struct module_data *, HIO_HANDLE *, const int);
};

extern const struct format_loader *const format_loaders[];

#endif
