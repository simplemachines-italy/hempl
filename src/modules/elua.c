// Interface with core services
// Modified to include support for Hempl.

#include "pico.h"

#include "platform.h"
#include "platform_conf.h"
#include "linenoise.h"
#include "shell.h"
#include <string.h>
#include <stdlib.h>

#if defined( USE_GIT_REVISION )
#include "git_version.h"
#else
#include "version.h"
#endif

// ****************************************************************************
// eLua core module for picoLisp.

// (elua-version) -> sym
any plisp_elua_version(any x) {
  return mkStr(ELUA_STR_VERSION);
}

// (elua-save-history 'sym) -> Nil
any plisp_elua_save_history(any x) {
#ifdef BUILD_LINENOISE
  int res; // holds result.
  any y;   // cdr(x)
  
  y = cdr(x);
  y = EVAL(car(y));
  // holds file name.
  char fname[bufSize(y)];
  NeedSym(x, y);
  bufString(y, fname);
  res = linenoise_savehistory(LINENOISE_ID_LUA, fname);
  if (res == 0)
    printf("History saved to %s.\n", fname);
  else if (res == LINENOISE_HISTORY_NOT_ENABLED)
    outString("linenoise not enabled for picoLisp.\n");
  else if (res = LINENOISE_HISTORY_EMPTY)
    outString("History empty, nothing to save.\n");
  else
    printf("Unable to save history to %s.\n", fname);
  return Nil;
#else
  err(NULL, NULL, "linenoise support not enabled.");
#endif
}

// (elua-shell 'sym) -> Nil
any plisp_elua_shell(any x) {
  any y = cdr(x);
  char *cmdcpy;

  y = EVAL(car(y));
  char pcmd[bufSize(y)];
  NeedSym(x, y);
  bufString(y, pcmd);
  // "+2" below comes from the string terminator (+1) and the '\n'
  // that will be added by the shell code (+1)
  if ((cmdcpy = (char *)malloc(strlen(pcmd) + 2)) == NULL)
    err(NULL, NULL, "not enough memory for elua-shell");
  strcpy(cmdcpy, pcmd);
  shellh_execute_command(cmdcpy, 0);

  free(cmdcpy);
  return Nil;
}

