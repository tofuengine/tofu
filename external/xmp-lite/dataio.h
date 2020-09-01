#ifndef LIBXMP_IO_H
#define LIBXMP_IO_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int8	read8s			(FILE *, int *err);
uint8	read8			(FILE *, int *err);
uint16	read16l			(FILE *, int *err);
uint16	read16b			(FILE *, int *err);
uint32	read32l			(FILE *, int *err);
uint32	read32b			(FILE *, int *err);

uint16	readmem16l		(const uint8 *);
uint16	readmem16b		(const uint8 *);
uint32	readmem32l		(const uint8 *);
uint32	readmem32b		(const uint8 *);

#endif /* LIBXMP_IO_H */
