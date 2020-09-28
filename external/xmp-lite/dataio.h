#ifndef LIBXMP_DATAIO_H
#define LIBXMP_DATAIO_H

#include <stdint.h>

extern uint16_t readmem16l(const void *);
extern uint16_t readmem16b(const void *);
extern uint32_t readmem32l(const void *);
extern uint32_t readmem32b(const void *);

#endif /* LIBXMP_DATAIO_H */
