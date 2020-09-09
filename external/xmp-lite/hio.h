#ifndef XMP_HIO_H
#define XMP_HIO_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct HIO_FUNCS {
	size_t (*read)(void *, size_t, size_t, void *);
	int (*seek)(void *, long, int);
	long (*tell)(void *);
	int (*eof)(void *);
} HIO_FUNCS;

typedef struct HIO_HANDLE HIO_HANDLE;

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
extern HIO_HANDLE *hio_open_callbacks(HIO_FUNCS, void *);
extern int	hio_close	(HIO_HANDLE *);
extern long	hio_size	(HIO_HANDLE *);

#endif
