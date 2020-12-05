#ifndef LIBXMP_SMIX_H
#define LIBXMP_SMIX_H

#include "common.h"

extern struct xmp_instrument *libxmp_get_instrument(struct context_data *ctx, int ins);
extern struct xmp_sample *libxmp_get_sample(struct context_data *ctx, int smp);

#endif
