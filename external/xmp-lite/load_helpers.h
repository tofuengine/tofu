#ifndef XMP_LOAD_HELPERS_H
#define XMP_LOAD_HELPERS_H

#include "common.h"

char *libxmp_adjust_string(char *s);
void libxmp_load_prologue(struct context_data *);
void libxmp_load_epilogue(struct context_data *);
int  libxmp_prepare_scan(struct context_data *);
void libxmp_free_scan(struct context_data *ctx);
int libxmp_set_player_mode(struct context_data *ctx);

#endif
