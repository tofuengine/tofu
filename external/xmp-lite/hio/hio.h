#ifndef XMP_HIO_H
#define XMP_HIO_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "dataio.h"
#include "memio.h"
#include "cbio.h"

#define HIO_HANDLE_TYPE(x) ((x)->type)

typedef struct {
#define HIO_HANDLE_TYPE_FILE		0
#define HIO_HANDLE_TYPE_MEMORY		1
#define HIO_HANDLE_TYPE_CALLBACKS	2
	int type;
	long size;
	union {
		FILE *file;
		MFILE *mem;
		CBFILE *cb;
	} handle;
	int error;
} HIO_HANDLE;

extern int8_t	hio_read8s	(HIO_HANDLE *);
extern uint8_t	hio_read8	(HIO_HANDLE *);
extern uint16_t	hio_read16l	(HIO_HANDLE *);
extern uint16_t	hio_read16b	(HIO_HANDLE *);
extern uint32_t	hio_read32l	(HIO_HANDLE *);
extern uint32_t	hio_read32b	(HIO_HANDLE *);
extern size_t	hio_read	(void *, size_t, size_t, HIO_HANDLE *);	
extern int	hio_seek	(HIO_HANDLE *, long, int);
extern long	hio_tell	(HIO_HANDLE *);
extern int	hio_eof		(HIO_HANDLE *);
extern int	hio_error	(HIO_HANDLE *);
extern HIO_HANDLE *hio_open	(const void *, const char *);
extern HIO_HANDLE *hio_open_mem  (const void *, long);
extern HIO_HANDLE *hio_open_file (FILE *);
extern HIO_HANDLE *hio_open_callbacks(CBFUNC, void *);
extern int	hio_close	(HIO_HANDLE *);
extern long	hio_size	(HIO_HANDLE *);

#endif
