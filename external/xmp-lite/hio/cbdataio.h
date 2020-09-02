#ifndef LIBXMP_CBDATAIO_H
#define LIBXMP_CBDATAIO_H

#include "cbio.h"
#include "types.h"

uint8 cbread8(CBFILE *cb, int *err);
int8 cbread8s(CBFILE *cb, int *err);
uint16 cbread16l(CBFILE *cb, int *err);
uint16 cbread16b(CBFILE *cb, int *err);
uint32 cbread32l(CBFILE *cb, int *err);
uint32 cbread32b(CBFILE *cb, int *err);

#endif
