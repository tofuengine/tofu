#ifndef __FILE_H__
#define __FILE_H__

#include <limits.h>

#ifdef PATH_MAX
  #define PATH_FILE_MAX       PATH_MAX
#else
  #define PATH_FILE_MAX       1024
#endif

extern void file_resolve_path(char *resolved, const char *path);

extern char *file_load_as_string(const char *filename, const char *mode);

#endif/* __FILE_H__ */
