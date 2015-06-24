// eLua interrupt support
// Modified to include support for Hempl.

#include "elua_int.h"
#include "pico.h"
#include "platform.h"
#include "platform_conf.h"
#include "type.h"
#include <stdio.h>
#include <string.h>

// ****************************************************************************
// Interrupt handlers for PicoLisp

#ifdef ALCOR_BOARD_MIZAR32

// Just to keep common.c happy for Mizar32. We'll get rid of this later.
int elua_int_add( elua_int_id inttype, elua_int_resnum resnum )
{
  return PLATFORM_ERR;
}

#endif

#if defined (BUILD_PICOLISP_INT_HANDLERS)

/*
 * TODO: Construct code for handling interrupts
 */

#else // #if defined (BUILD_PICOLISP_INT_HANDLERS)

// This is needed by lua_close (lstate.c)
// Raman: I'm just leaving it at this for now. I'll get back
// to it soon.
void elua_int_disable_all()
{
}

// This too
void elua_int_cleanup()
{
}

void elua_int_enable( elua_int_id inttype )
{
}

void elua_int_disable( elua_int_id inttype )
{
}

int elua_int_add( elua_int_id inttype, elua_int_resnum resnum )
{
  return PLATFORM_ERR;
}

#endif // #if defined(BUILD_PICOLISP_INT_HANDLERS)

// ****************************************************************************
// C handlers

#ifdef BUILD_C_INT_HANDLERS

static elua_int_c_handler elua_int_c_handler_list[ INT_ELUA_LAST ];

elua_int_c_handler elua_int_set_c_handler( elua_int_id inttype, elua_int_c_handler phandler )
{
  elua_int_c_handler crthandler;

  if( inttype < ELUA_INT_FIRST_ID || inttype > INT_ELUA_LAST )
    return NULL;
  inttype -= ELUA_INT_FIRST_ID;
  crthandler = elua_int_c_handler_list[ inttype ];
  elua_int_c_handler_list[ inttype ] = phandler;
  return crthandler;
}

elua_int_c_handler elua_int_get_c_handler( elua_int_id inttype )
{
  if( inttype < ELUA_INT_FIRST_ID || inttype > INT_ELUA_LAST )
    return NULL;
  return elua_int_c_handler_list[ inttype - ELUA_INT_FIRST_ID ];
}

#else // #ifdef BUILD_C_INT_HANDLERS

elua_int_c_handler elua_int_set_c_handler( elua_int_id inttype, elua_int_c_handler phandler )
{
  return NULL;
}

elua_int_c_handler elua_int_get_c_handler( elua_int_id inttype )
{
  return NULL;
}

#endif // #ifdef BUILD_C_INT_HANDLERS
