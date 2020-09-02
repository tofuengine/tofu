#ifndef LIBXMP_DATAIO_H
#define LIBXMP_DATAIO_H

#include <stdint.h>

extern uint16_t readmem16l(const uint8_t *);
extern uint16_t readmem16b(const uint8_t *);
extern uint32_t readmem32l(const uint8_t *);
extern uint32_t readmem32b(const uint8_t *);

#endif /* LIBXMP_DATAIO_H */
