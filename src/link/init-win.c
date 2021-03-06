/*****************************************************************************

This file is in the public domain and may be distributed and modified without
restriction.

******************************************************************************

This version of the initialisation process does not define a main() function.
Parasol must be initialised via the init_parasol() function and closed with
close_parasol().  Another difference vs startup-win.c is that error messages
are returned by the function and not automatically presented in a dialog box.

*****************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <parasol/main.h>
#include <parasol/system/errors.h>

#define DLLCALL // __declspec(dllimport)
#define WINAPI  __stdcall
#define HKEY_LOCAL_MACHINE 0x80000002
#define KEY_READ 0x20019
#define MAX_PATH 260
#define INVALID_HANDLE_VALUE (void *)(-1)

DLLCALL APTR WINAPI LoadLibraryA(STRING);
DLLCALL LONG WINAPI FreeLibrary(APTR);
DLLCALL LONG WINAPI FindClose(APTR);
DLLCALL APTR WINAPI FindFirstFileA(STRING, void *);
DLLCALL APTR WINAPI GetProcAddress(APTR, STRING);
DLLCALL LONG WINAPI RegOpenKeyExA(LONG,STRING,LONG,LONG,APTR *);
DLLCALL LONG WINAPI RegQueryValueExA(APTR,STRING,LONG *,LONG *,BYTE *,LONG *);
DLLCALL void WINAPI CloseHandle(APTR);
DLLCALL int  WINAPI MessageBoxA(LONG,STRING,STRING,LONG);
DLLCALL LONG WINAPI GetCurrentDirectoryA(LONG, STRING);
DLLCALL LONG WINAPI GetModuleFileNameA(APTR, STRING, LONG);
DLLCALL LONG WINAPI SetDllDirectoryA(STRING);
DLLCALL LONG WINAPI SetDefaultDllDirectories(LONG DirectoryFlags);
DLLCALL void * AddDllDirectory(STRING NewDirectory);

#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#define LOAD_LIBRARY_SEARCH_USER_DIRS 0x00000400
#define LOAD_LIBRARY_SEARCH_SYSTEM32 0x00000800

typedef struct _FILETIME {
	LONG dwLowDateTime;
	LONG dwHighDateTime;
} FILETIME,*PFILETIME,*LPFILETIME;

typedef struct _WIN32_FIND_DATAW {
	LONG dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	LONG nFileSizeHigh;
	LONG nFileSizeLow;
	LONG dwReserved0;
	LONG dwReserved1;
   UWORD cFileName[MAX_PATH];
	UWORD cAlternateFileName[14];
} WIN32_FIND_DATAW,*LPWIN32_FIND_DATAW;

typedef WIN32_FIND_DATAW WIN32_FIND_DATA,*LPWIN32_FIND_DATA;

#define MB_ICONERROR 16
#define MB_OK 0

extern void program(void);
extern STRING ProgCopyright;
extern STRING ProgAuthor;
extern STRING ProgName;
extern STRING ProgDate;
extern LONG ProgDebug;
extern FLOAT ProgCoreVersion;

struct CoreBase *CoreBase;

static ERROR PROGRAM_DataFeed(OBJECTPTR, struct acDataFeed *);
void close_parasol(void);
static APTR find_core(char *PathBuffer, int Size);

//****************************************************************************

APTR corehandle = 0;
void (*closecore)(void) = 0;
APTR keyhandle;
static char msgbuf[120];

const char * init_parasol(int argc, CSTRING *argv)
{
   #define MAX_ARGS 30
   struct OpenInfo info;
   struct CoreBase * (*opencore)(struct OpenInfo *);
   const char *msg = NULL;
   APTR *actions;
   char path_buffer[256];

   corehandle = NULL;
   opencore   = NULL;
   closecore  = NULL;
   keyhandle  = NULL;

   corehandle = find_core(path_buffer, sizeof(path_buffer));
   if (!corehandle) {
      msg = "Failed to open Parasol's core library.";
      goto exit;
   }

   if (!(opencore = GetProcAddress(corehandle, "OpenCore"))) {
      msg = "Could not find the OpenCore symbol in Parasol.";
      goto exit;
   }

   if (!(closecore = GetProcAddress(corehandle, "CloseCore"))) {
      msg = "Could not find the CloseCore symbol in Parasol.";
      goto exit;
   }

   info.Detail    = ProgDebug;
   info.MaxDepth  = 10;
   info.Name      = ProgName;
   info.Author    = ProgAuthor;
   info.Date      = ProgDate;
   info.Copyright = ProgCopyright;
   info.Args      = argv;
   info.ArgCount  = argc;
   info.CoreVersion = ProgCoreVersion; // Minimum required core version
   info.CompiledAgainst = VER_CORE; // The core that this code is compiled against
   info.Error = ERR_Okay;
   info.Flags = OPF_CORE_VERSION|OPF_COMPILED_AGAINST|OPF_NAME|OPF_AUTHOR|OPF_DATE|OPF_COPYRIGHT|OPF_ARGS|OPF_ERROR;
   if (ProgDebug > 0) info.Flags |= OPF_DETAIL|OPF_MAX_DEPTH;
   if (ProgDebug IS -1) {
      info.Detail = 0;
      info.MaxDepth = 0;
      info.Flags |= OPF_DETAIL|OPF_MAX_DEPTH;
   }

   if ((CoreBase = opencore(&info))) {
      OBJECTPTR task = CurrentTask();

      if (!GetPointer(task, FID_Actions, &actions)) {
         actions[AC_DataFeed] = PROGRAM_DataFeed;
      }
   }
   else if (info.Error IS ERR_CoreVersion) msg = "This program requires the latest version of the Parasol framework.\nPlease visit www.parasol.ws to upgrade.";
   else {
      snprintf(msgbuf, sizeof(msgbuf), "Failed to initialise Parasol, error code %d.", info.Error);
      return msgbuf;
   }

   return msg;

exit:
   close_parasol();
   return msg;
}

//****************************************************************************

void close_parasol(void)
{
   if (closecore) closecore();
   if (corehandle) FreeLibrary(corehandle);
   if (keyhandle) CloseHandle(keyhandle);
}

#include "common-win.c"
#include "startup-common.c"
