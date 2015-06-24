// Module for interfacing with platform data
// Modified to include support for Hempl.

#include "platform.h"

#define MACRO_NAME( x ) MACRO_AGAIN( x )
#define MACRO_AGAIN( x ) #x

// ****************************************************************************
// Platform module for picoLisp.

#include "pico.h"

// (pd-platform) -> sym
any pd_platform(any x) {
   return mkStr(MACRO_NAME(ALCOR_PLATFORM));
}

// (pd-cpu) -> sym
any pd_cpu(any x) {
   return mkStr(MACRO_NAME(ALCOR_CPU));
}

// (pd-board) -> sym
any pd_board(any x) {
   return mkStr(MACRO_NAME(ALCOR_BOARD));
}
