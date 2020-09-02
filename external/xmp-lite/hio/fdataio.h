#ifndef LIBXMP_FDATAIO_H
#define LIBXMP_FDATAIO_H

#include <stdint.h>
#include <stdio.h>

int8_t	fread8s			(FILE *, int *err);
uint8_t	fread8			(FILE *, int *err);
uint16_t	fread16l		(FILE *, int *err);
uint16_t	fread16b		(FILE *, int *err);
uint32_t	fread32l		(FILE *, int *err);
uint32_t	fread32b		(FILE *, int *err);

#endif /* LIBXMP_FDATAIO_H */
