#ifndef __TOFU_MEMORY_H__
#define __TOFU_MEMORY_H__

#include <stdlib.h>

extern void *Memory_alloc(size_t size);
extern void Memory_free(void *ptr);
extern void *Memory_realloc(void *ptr, size_t size);

extern void *Memory_clone(const void *ptr, size_t size);

#endif  /* __TOFU_MEMORY_H__ */
