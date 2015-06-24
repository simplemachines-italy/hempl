// Module for interfacing with CPU
// Modified to include support for Hempl.

#include "pico.h"

// Generic includes.
#include <string.h> 
#include "platform.h"

#undef _C
#define _C(x) {#x, x}
#include "platform_conf.h"

// CPU constants list
typedef struct
{
  const char* name;
  u32 val;
} cpu_const_t;

#ifdef PLATFORM_CPU_CONSTANTS
static const cpu_const_t cpu_constants[] = 
{
  PLATFORM_CPU_CONSTANTS,
  { NULL, 0 }
};
#endif

// ****************************************************************************
// CPU module for picoLisp.

// (cpu-w32 'addr 'data) -> num
any plisp_cpu_w32(any ex) {
  u32 addr, data;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  addr = unBox(y); // get address.

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  data = unBox(y); // get data.

  *(u32 *)addr = data;
  return Nil;
}

// (cpu-r32 'addr) -> data
any plisp_cpu_r32(any ex) {
  u32 addr;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  addr = unBox(y); // get address.

  return box(*(u32 *)addr);
}

// (cpu-w16 'addr 'data) -> num
any plisp_cpu_w16(any ex) {
  u32 addr;
  u16 data;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  addr = unBox(y); // get address.

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  data = unBox(y); // get data.

  *(u16 *)addr = data;
  return Nil;
}

// (cpu-r16 'addr) -> data
any plisp_cpu_r16(any ex) {
  u32 addr;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  addr = unBox(y); // get address.

  return box(*(u16 *)addr);
}

// (cpu-w8 'addr 'data) -> num
any plisp_cpu_w8(any ex) {
  u32 addr;
  u8 data;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  addr = unBox(y); // get address.

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  data = unBox(y); // get data.

  *(u8 *)addr = data;
  return Nil;
}

// (cpu-r8 'addr) -> data
any plisp_cpu_r8(any ex) {
  u32 addr;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  addr = unBox(y); // get address.

  return box(*(u8 *)addr);
}

// (cpu-clock) -> num
any plisp_cpu_clock(any ex) {
  return box(platform_cpu_get_frequency());
}


