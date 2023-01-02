/*
 * MIT License
 * 
 * Copyright (c) 2019-2022 Marco Lizza
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

#include "sysinfo.h"

#include <core/platform.h>
#include <libs/log.h>

#include <string.h>

/* This file provides an implementation only for the native Windows API.  */
#if PLATFORM_ID == PLATFORM_WINDOWS
  #include <windows.h>
  #ifndef VER_PLATFORM_WIN32_CE
    #define VER_PLATFORM_WIN32_CE 3
  #endif
#elif PLATFORM_ID == PLATFORM_LINUX
  #include <sys/utsname.h>
#endif

#define LOG_CONTEXT "sysinfo"

bool SysInfo_inspect(SysInfo_Data_t *si)
{
#if PLATFORM_ID == PLATFORM_WINDOWS
  OSVERSIONINFO version;
  OSVERSIONINFOEX versionex;
  BOOL have_versionex; /* indicates whether versionex is filled */
  const char *super_version;

  /* Preparation: Fill version and, if possible, also versionex.
     But try to call GetVersionEx only once in the common case.  */
  versionex.dwOSVersionInfoSize = sizeof (OSVERSIONINFOEX);
  have_versionex = GetVersionEx ((OSVERSIONINFO *) &versionex);
  if (have_versionex)
    {
      /* We know that OSVERSIONINFO is a subset of OSVERSIONINFOEX.  */
      memcpy (&version, &versionex, sizeof (OSVERSIONINFO));
    }
  else
    {
      version.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
      if (!GetVersionEx (&version))
        abort();
    }

  /* Determine major-major Windows version.  */
  if (version.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
      /* Windows NT or newer.  */
      super_version = "NT";
    }
  else if (version.dwPlatformId == VER_PLATFORM_WIN32_CE)
    {
      /* Windows CE or Embedded CE.  */
      super_version = "CE";
    }
  else if (version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
      /* Windows 95/98/ME.  */
      switch (version.dwMinorVersion)
        {
        case 0:
          super_version = "95";
          break;
        case 10:
          super_version = "98";
          break;
        case 90:
          super_version = "ME";
          break;
        default:
          super_version = "";
          break;
        }
    }
  else
    super_version = "";

  /* Fill in sysname.  */
  sprintf (si->system, "Windows%s", super_version);

  /* Fill in release, version.  */
  /* The MSYS uname.exe programs uses strings from a modified Cygwin runtime:
       $ ./uname.exe -r      => 1.0.11(0.46/3/2)
       $ ./uname.exe -v      => 2008-08-25 23:40
     There is no point in imitating this behaviour.  */
  if (version.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
      /* Windows NT or newer.  */
      struct windows_version
        {
          int major;
          int minor;
          int build;
          unsigned int server_offset;
          const char *name;
        };

      /* Storing the workstation and server version names in a single
         stream does not waste memory when they are the same.  These
         macros abstract the representation.  VERSION1 is used if
         version.wProductType does not matter, VERSION2 if it does.  */
      #define VERSION1(major, minor, build, name) \
        { major, minor, build, 0, name }
      #define VERSION2(major, minor, build, workstation, server) \
        { major, minor, build, sizeof workstation, workstation "\0" server }
      static const struct windows_version versions[] =
        {
          VERSION2 (3, -1, -1, "Windows NT Workstation", "Windows NT Server"),
          VERSION2 (4, -1, -1, "Windows NT Workstation", "Windows NT Server"),
          VERSION1 (5, 0, -1, "Windows 2000"),
          VERSION1 (5, 1, -1, "Windows XP"),
          VERSION1 (5, 2, -1, "Windows Server 2003"),
          VERSION2 (6, 0, -1, "Windows Vista", "Windows Server 2008"),
          VERSION2 (6, 1, -1, "Windows 7", "Windows Server 2008 R2"),
          VERSION2 (6, 2, -1, "Windows 8", "Windows Server 2012"),
          VERSION2 (6, 3, -1, "Windows 8.1", "Windows Server 2012 R2"),
          VERSION2 (10, 0, -1, "Windows 10", "Windows Server 2016"),
          VERSION1 (10, 0, 22000, "Windows 11"),
          VERSION2 (-1, -1, -1, "Windows", "Windows Server")
        };
      const char *base;
      const struct windows_version *v = versions;

      /* Find a version that matches ours.  The last element is a
         wildcard that always ends the loop.  */
      while ((v->major != (int)version.dwMajorVersion && v->major != -1)
             || (v->minor != (int)version.dwMinorVersion && v->minor != -1)
             || (v->build < (int)version.dwBuildNumber && v->build != -1))
        v++;

      if (have_versionex && versionex.wProductType != VER_NT_WORKSTATION)
        base = v->name + v->server_offset;
      else
        base = v->name;
      if (v->major == -1 || v->minor == -1)
        sprintf (si->release, "%s %u.%u",
                 base,
                 (unsigned int) version.dwMajorVersion,
                 (unsigned int) version.dwMinorVersion);
      else
        strcpy (si->release, base);
    }
  else if (version.dwPlatformId == VER_PLATFORM_WIN32_CE)
    {
      /* Windows CE or Embedded CE.  */
      sprintf (si->release, "Windows CE %u.%u",
               (unsigned int) version.dwMajorVersion,
               (unsigned int) version.dwMinorVersion);
    }
  else
    {
      /* Windows 95/98/ME.  */
      sprintf (si->release, "Windows %s", super_version);
    }
  strcpy (si->version, version.szCSDVersion[0] != '\0' ? version.szCSDVersion : "vanilla");

  /* Fill in machine.  */
  {
    SYSTEM_INFO info;

    GetSystemInfo (&info);
    /* Check for Windows NT or CE, since the info.wProcessorLevel is
       garbage on Windows 95. */
    if (version.dwPlatformId == VER_PLATFORM_WIN32_NT
        || version.dwPlatformId == VER_PLATFORM_WIN32_CE)
      {
        /* Windows NT or newer, or Windows CE or Embedded CE.  */
        switch (info.wProcessorArchitecture)
          {
          case PROCESSOR_ARCHITECTURE_AMD64:
            strcpy (si->architecture, "x86_64");
            break;
          case PROCESSOR_ARCHITECTURE_IA64:
            strcpy (si->architecture, "ia64");
            break;
          case PROCESSOR_ARCHITECTURE_INTEL:
            strcpy (si->architecture, "i386");
            if (info.wProcessorLevel >= 3)
              si->architecture[1] =
                '0' + (info.wProcessorLevel <= 6 ? info.wProcessorLevel : 6);
            break;
          case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
            strcpy (si->architecture, "i686");
            break;
          case PROCESSOR_ARCHITECTURE_MIPS:
            strcpy (si->architecture, "mips");
            break;
          case PROCESSOR_ARCHITECTURE_ALPHA:
          case PROCESSOR_ARCHITECTURE_ALPHA64:
            strcpy (si->architecture, "alpha");
            break;
          case PROCESSOR_ARCHITECTURE_PPC:
            strcpy (si->architecture, "powerpc");
            break;
          case PROCESSOR_ARCHITECTURE_SHX:
            strcpy (si->architecture, "sh");
            break;
          case PROCESSOR_ARCHITECTURE_ARM:
            strcpy (si->architecture, "arm");
            break;
          default:
            strcpy (si->architecture, "unknown");
            break;
          }
      }
    else
      {
        /* Windows 95/98/ME.  */
        switch (info.dwProcessorType)
          {
          case PROCESSOR_AMD_X8664:
            strcpy (si->architecture, "x86_64");
            break;
          case PROCESSOR_INTEL_IA64:
            strcpy (si->architecture, "ia64");
            break;
          default:
            if (info.dwProcessorType % 100 == 86)
              sprintf (si->architecture, "i%u",
                       (unsigned int) info.dwProcessorType);
            else
              strcpy (si->architecture, "unknown");
            break;
          }
      }
  }
#elif PLATFORM_ID == PLATFORM_LINUX
    struct utsname uts;
    int result = uname(&uts);
    if (result == -1) {
        Log_write(LOG_LEVELS_ERROR, LOG_CONTEXT, "can't get system information");
        return false;
    }
    strncpy(si->system, uts.sysname, SYSINFO_NAME_LENGTH);
    strncpy(si->release, uts.release, SYSINFO_NAME_LENGTH);
    strncpy(si->version, uts.version, SYSINFO_NAME_LENGTH);
    strncpy(si->architecture, uts.machine, SYSINFO_NAME_LENGTH);
#endif
  return true;
}
