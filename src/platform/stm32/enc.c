// eLua Module for STM32 timer encoder mode support
// enc is a platform-dependent (STM32) module, that binds to Lua the basic API
// from ST
// Modified to include support for PicoC.

#include "pico.h"
#include "platform.h"
#include "platform_conf.h"
#include "elua_int.h"
#include "enc.h"

static elua_int_c_handler prev_handler;
static elua_int_resnum index_resnum;
static int index_tmr_id;
static u16 index_count;
static void index_handler( elua_int_resnum resnum );

// TODO: Add PicoLisp module for enc
