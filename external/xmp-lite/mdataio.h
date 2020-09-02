#ifndef LIBXMP_MDATAIO_H
#define LIBXMP_MDATAIO_H

#include "common.h"
#include "memio.h"

extern uint8 mread8(MFILE *m, int *err);
extern int8 mread8s(MFILE *m, int *err);
extern uint16 mread16l(MFILE *m, int *err);
extern uint16 mread16b(MFILE *m, int *err);
extern uint32 mread32l(MFILE *m, int *err);
extern uint32 mread32b(MFILE *m, int *err);

#endif
