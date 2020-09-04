#ifndef LIBXMP_CBDATAIO_H
#define LIBXMP_CBDATAIO_H

#include <stdint.h>
#include "cbio.h"

extern uint8_t cbread8(CBFILE *cb, int *err);
extern int8_t cbread8s(CBFILE *cb, int *err);
extern uint16_t cbread16l(CBFILE *cb, int *err);
extern uint16_t cbread16b(CBFILE *cb, int *err);
extern uint32_t cbread32l(CBFILE *cb, int *err);
extern uint32_t cbread32b(CBFILE *cb, int *err);

#endif
