// Shell: 'lua' implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "shell.h"
#include "common.h"
#include "type.h"
#include "platform_conf.h"

// EOF is different in UART mode and TCP/IP mode
#ifdef BUILD_CON_GENERIC
  #define SHELL_EOF_STRING        "CTRL+Z"
#else
  #define SHELL_EOF_STRING        "CTRL+D"
#endif

const char shell_help_picolisp[] = "[-e <stat>] [-l <name>] [-i] [-v] [<script>]\n"
  "  [<script>]: execute the given script.\n"
  "  [-e <stat>]: execute string 'stat'.\n"
  "  [-l <name>]: require library 'name'.\n"
  "  [-i]: enter interactive mode after executing 'script'.\n"
  "  [-v]: show version information.\n"
  "Without arguments it executes the interactive Lua interpreter.\n";
const char shell_help_summary_picolisp[] = "start a PicoLisp session";

void shell_picolisp( int argc, char **argv )
{
  printf( "Press " SHELL_EOF_STRING " to exit PicoLisp\n" );
  picolisp_main( argc, argv );
  clearerr( stdin );
}
