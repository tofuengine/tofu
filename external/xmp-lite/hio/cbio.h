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

extern CBFILE *cbopen(CBFUNC func, void *ud);
extern int     cbgetc(CBFILE *stream);
extern size_t  cbread(void *, size_t, size_t, CBFILE *);
extern int     cbseek(CBFILE *, long, int);
extern long    cbtell(CBFILE *);
extern int     cbclose(CBFILE *);
extern int     cbeof(CBFILE *);

#ifdef __cplusplus
}
#endif

#endif
