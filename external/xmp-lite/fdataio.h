#ifndef LIBXMP_FDATAIO_H
#define LIBXMP_FDATAIO_H

#include "common.h"
#include <stdio.h>

int8	fread8s			(FILE *, int *err);
uint8	fread8			(FILE *, int *err);
uint16	fread16l		(FILE *, int *err);
uint16	fread16b		(FILE *, int *err);
uint32	fread32l		(FILE *, int *err);
uint32	fread32b		(FILE *, int *err);

#endif /* LIBXMP_FDATAIO_H */
