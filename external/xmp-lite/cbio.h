#ifndef LIBXMP_CBIO_H
#define LIBXMP_CBIO_H

#include <stdio.h>

typedef struct {
	size_t (*read)(void *, void *, size_t);
	int (*seek)(void *, long, int);
	long (*tell)(void *);
	int (*eof)(void *);
} CBFUNC;

typedef struct {
	CBFUNC func;
	void *ud;
} CBFILE;

#ifdef __cplusplus
extern "C" {
#endif

CBFILE  *cbopen(CBFUNC func, void *ud);
int     cbgetc(CBFILE *stream);
size_t  cbread(void *, size_t, size_t, CBFILE *);
int     cbseek(CBFILE *, long, int);
long    cbtell(CBFILE *);
int     cbclose(CBFILE *);
int	cbeof(CBFILE *);
#ifndef LIBXMP_CORE_PLAYER
int	cbstat(CBFILE *, struct stat *);
#endif

#ifdef __cplusplus
}
#endif

#endif
