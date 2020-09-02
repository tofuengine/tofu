#ifndef LIBXMP_MDATAIO_H
#define LIBXMP_MDATAIO_H

#include <stdint.h>
#include "memio.h"

extern uint8_t mread8(MFILE *m, int *err);
extern int8_t mread8s(MFILE *m, int *err);
extern uint16_t mread16l(MFILE *m, int *err);
extern uint16_t mread16b(MFILE *m, int *err);
extern uint32_t mread32l(MFILE *m, int *err);
extern uint32_t mread32b(MFILE *m, int *err);

#endif
