#ifndef LIBXMP_HIO_TYPES_H
#define LIBXMP_HIO_TYPES_H

#ifdef __AROS__
#define __AMIGA__
#endif

/* AmigaOS fixes by Chris Young <cdyoung@ntlworld.com>, Nov 25, 2007
 */
#if defined B_BEOS_VERSION
#  include <SupportDefs.h>
#elif defined __amigaos4__
#  include <exec/types.h>
#else
typedef signed char int8;
typedef signed short int int16;
typedef signed int int32;
typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
#endif

#ifdef _MSC_VER				/* MSVC++6.0 has no long long */
typedef signed __int64 int64;
typedef unsigned __int64 uint64;
#elif !defined B_BEOS_VERSION		/* BeOS has its own int64 definition */
typedef unsigned long long uint64;
typedef signed long long int64;
#endif


#endif /* LIBXMP_HIO_TYPES_H */
