#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdbool.h>

extern void resolve_path(const char *path, char *resolved);

extern char *load_file_as_string(const char *filename, const char *mode);

#endif/* __UTILS_H__ */
