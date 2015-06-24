
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "type.h"
#include "devman.h"
#include "platform.h"
#include "romfs.h"
#include "xmodem.h"
#include "shell.h"
#include "pico.h"

// Generic includes.
#include "term.h"
#include "platform_conf.h"

// Validate configuratin options
#include "validate.h"

#include "mmcfs.h"
#include "romfs.h"

// Define here your autorun/boot files,
// in the order you want eLua to search for them
char *boot_order[] = {
#if defined BUILD_MMCFS
  "/mmc/autorun.l",
  "/mmc/autorun.lisp",
#endif // #if defined BUILD_MMCFS

#if defined BUILD_ROMFS
  "/rom/autorun.l",
  "/rom/autorun.lisp",
#endif // #if defined BUILD_ROMFS
};

extern char etext[];

// ****************************************************************************
//  Program entry point

int main( void )
{
  int i;
  FILE* fp;

  // Initialize platform first
  if( platform_init() != PLATFORM_OK )
  {
    // This should never happen
    while( 1 );
  }

  // Initialize device manager
  dm_init();

  // Register the ROM filesystem
  romfs_init();

  // Register the MMC filesystem
  mmcfs_init();

  // Search for autorun files in the defined order and execute the 1st if found
  for( i = 0; i < sizeof( boot_order ) / sizeof( *boot_order ); i++ )
  {
    if( ( fp = fopen( boot_order[ i ], "r" ) ) != NULL )
    {
      fclose( fp );
      // The entry point for PicoLisp.
      char* picolisp_argv[] = { "picolisp", boot_order[i], NULL };

      picolisp_main( 2, picolisp_argv );
      break; // autoruns only the first found
    }
  }
  
  // Run the shell
  if( shell_init() == 0 )
  {
    char* picolisp_argv[] = { "picolisp", NULL };
    picolisp_main( 1, picolisp_argv );
  }
  else
  {
    shell_start();
  }

  while( 1 );
}
