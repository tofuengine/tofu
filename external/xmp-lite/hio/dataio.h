#ifndef LIBXMP_DATAIO_H
#define LIBXMP_DATAIO_H

#include "types.h"

uint16	readmem16l		(const uint8 *);
uint16	readmem16b		(const uint8 *);
uint32	readmem32l		(const uint8 *);
uint32	readmem32b		(const uint8 *);

#endif /* LIBXMP_DATAIO_H */
