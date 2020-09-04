#ifndef LIBXMP_MEMIO_H
#define LIBXMP_MEMIO_H

#include <limits.h>
#include <stddef.h>

typedef struct {
	const unsigned char *start;
	ptrdiff_t pos;
	ptrdiff_t size;
} MFILE;

#ifdef __cplusplus
extern "C" {
#endif

extern MFILE *mopen(const void *, long);
extern int    mgetc(MFILE *);
extern size_t mread(void *, size_t, size_t, MFILE *);
extern int    mseek(MFILE *, long, int);
extern long   mtell(MFILE *);
extern int    mclose(MFILE *);
extern int    meof(MFILE *);

#ifdef __cplusplus
}
#endif

#endif
