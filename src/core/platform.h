/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TOFU_CORE_PLATFORM_H
#define TOFU_CORE_PLATFORM_H

#define PLATFORM_H_INCLUDED

#define PLATFORM_UNKNOWN 0
#define PLATFORM_WINDOWS 1
#define PLATFORM_ANDROID 2
#define PLATFORM_LINUX   3
#define PLATFORM_BSD     4
#define PLATFORM_HPUX    5
#define PLATFORM_AIX     6
#define PLATFORM_IOS     7
#define PLATFORM_OSX     8
#define PLATFORM_SOLARIS 9

#if (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || defined(__BIG_ENDIAN__)
    #define PLATFORM_BIG_ENDIAN
#else
    #define PLATFORM_LITTLE_ENDIAN
#endif

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_ID PLATFORM_WINDOWS // Windows
    #define PLATFORM_NAME "Windows"
#elif defined(__CYGWIN__) && !defined(_WIN32)
    #define PLATFORM_ID PLATFORM_WINDOWS // Windows (Cygwin POSIX under Microsoft Window)
    #define PLATFORM_NAME "Cygwin"
#elif defined(__ANDROID__)
    #define PLATFORM_ID PLATFORM_ANDROID // Android (implies Linux, so it must come first)
    #define PLATFORM_NAME "Android"
#elif defined(__linux__)
    #define PLATFORM_ID PLATFORM_LINUX // Debian, Ubuntu, Gentoo, Fedora, openSUSE, RedHat, CentOS and other
    #define PLATFORM_NAME "Linux"
#elif defined(__unix__) || defined(__APPLE__) && defined(__MACH__)
    #include <sys/param.h>
    #if defined(BSD)
        #define PLATFORM_ID PLATFORM_BSD // FreeBSD, NetBSD, OpenBSD, DragonFly BSD
        #define PLATFORM_NAME "BSD"
    #endif
#elif defined(__hpux)
    #define PLATFORM_ID PLATFORM_HP_UX // HP-UX
    #define PLATFORM_NAME "HP-UX"
#elif defined(_AIX)
    #define PLATFORM_ID PLATFORM_AIX // IBM AIX
    #define PLATFORM_NAME "AIX"
#elif defined(__APPLE__) && defined(__MACH__) // Apple OSX and iOS (Darwin)
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR == 1
        #define PLATFORM_ID PLATFORM_IOS // Apple iOS
        #define PLATFORM_NAME "iOS"
    #elif TARGET_OS_IPHONE == 1
        #define PLATFORM_ID PLATFORM_IOS // Apple iOS
        #define PLATFORM_NAME "iOS"
    #elif TARGET_OS_MAC == 1
        #define PLATFORM_ID PLATFORM_OSX // Apple OSX
        #define PLATFORM_NAME "iOS"
    #endif
#elif defined(__sun) && defined(__SVR4)
    #define PLATFORM_ID PLATFORM_SOLARIS // Oracle Solaris, Open Indiana
    #define PLATFORM_NAME "Solaris"
#else
    #define PLATFORM_ID PLATFORM_UNKNOWN
    #define PLATFORM_NAME "Unknown"
#endif

#if PLATFORM_ID == PLATFORM_LINUX
    #include <linux/limits.h>

    #define PLATFORM_PATH_SEPARATOR    '/'
    #define PLATFORM_PATH_SEPARATOR_SZ "/"
    #define PLATFORM_PATH_CURRENT_SZ   "./"
    #define PLATFORM_PATH_PARENT_SZ    "../"
    #define PLATFORM_PATH_MAX          PATH_MAX
    #define PLATFORM_PATH_USER         "~/.local/share/tofuengine"
#elif PLATFORM_ID == PLATFORM_WINDOWS
    #include <limits.h>

    #define PLATFORM_PATH_SEPARATOR    '\\'
    #define PLATFORM_PATH_SEPARATOR_SZ "\\"
    #define PLATFORM_PATH_CURRENT_SZ   ".\\"
    #define PLATFORM_PATH_PARENT_SZ    "..\\"
    #define PLATFORM_PATH_MAX          PATH_MAX
    #define PLATFORM_PATH_USER         "%AppData%\\tofuengine"
#elif PLATFORM_ID == PLATFORM_OSX
    #define PLATFORM_PATH_SEPARATOR    '/'
    #define PLATFORM_PATH_SEPARATOR_SZ "/"
    #define PLATFORM_PATH_CURRENT_SZ    "./"
    #define PLATFORM_PATH_PARENT_SZ    "../"
    #define PLATFORM_PATH_MAX          1024
    #define PLATFORM_PATH_USER         "~/Library/Application Support/tofuengine"
#endif

#endif  /* TOFU_CORE_PLATFORM_H */
