// Module for interfacing with timers
// Modified to include support for Hempl.

#include "pico.h"

// Generic includes.
#include "platform.h"
#include "platform_conf.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VTIMER_NAME_LEN     6
#define MIN_VTIMER_NAME_LEN     5

// ****************************************************************************
// Timer module for picoLisp.

// (tmr-delay ['num] 'num) -> Nil
any tmr_delay(any ex) {
  timer_data_type period;
  unsigned id = PLATFORM_TIMER_SYS_ID;
  any x, y;

  x = cdr(ex), y = EVAL(car(x));
  if (plen(ex) == 1) {
    // We only have 1 parameter. Assume
    // *tmr-sys-timer* and get the time
    // period.
    NeedNum(ex, y);
    period = (timer_data_type)unBox(y);
  } else {
    // Minimum 2 args required here - the
    // id and the period. Ignore the others.
    NeedNum(ex, y);
    id = unBox(y);
    MOD_CHECK_TIMER(ex, id);
    x = cdr(x), y = EVAL(car(x));
    NeedNum(ex, y);
    period = unBox(y);
  }
  platform_timer_delay(id, period);
  return Nil;
}

// (tmr-read ['num]) -> num
any tmr_read(any ex) {
  unsigned id = PLATFORM_TIMER_SYS_ID;
  timer_data_type res;
  any x, y;

  x = cdr(ex), y = EVAL(car(x));
  if (plen(ex) > 0) {
    NeedNum(ex, y);
    id = unBox(y);
    MOD_CHECK_TIMER(ex, id);      
  }

  res = platform_timer_op(id, PLATFORM_TIMER_OP_READ, 0);
  return box(res);
}

// (tmr-start ['num]) -> num
any tmr_start(any ex) {
  unsigned id = PLATFORM_TIMER_SYS_ID;
  timer_data_type res;
  any x, y;

  x = cdr(ex), y = EVAL(car(x));
  if (plen(ex) > 0) {
    NeedNum(ex, y);
    id = unBox(y);
    MOD_CHECK_TIMER(ex, id);
  }

  res = platform_timer_op(id, PLATFORM_TIMER_OP_START, 0);
  return box(res);
}

// (tmr-gettimediff 'num 'num 'num) -> num
any tmr_gettimediff(any ex) {
  timer_data_type start, end, res;
  unsigned id = PLATFORM_TIMER_SYS_ID;
  any x, y;

  x = cdr(ex), y = EVAL(car(x));
  NeedNum(ex, y);
  id = unBox(y); // get id.
  MOD_CHECK_TIMER(ex, id);

  x = cdr(x), y = EVAL(car(x));
  NeedNum(ex, y);
  start = unBox(y); // get start.

  x = cdr(x), y = EVAL(car(x));
  NeedNum(ex, y);
  end = unBox(y); // get end.

  res = platform_timer_get_diff_us(id, start, end);
  return box(res);
}

// (tmr-getdiffnow 'num 'num) -> num
any tmr_getdiffnow(any ex) {
  timer_data_type start, res;
  unsigned id;
  any x, y;

  x = cdr(ex), y = EVAL(car(x));
  NeedNum(ex, y);
  id = unBox(y); // get id.
  MOD_CHECK_TIMER(ex, id);

  x = cdr(x), y = EVAL(car(x));
  NeedNum(ex, y);
  start = unBox(y); // get start.
  res = platform_timer_get_diff_crt(id, start);
  return box(res);
}

// (tmr-getmindelay ['num]) -> num
any tmr_getmindelay(any ex) {
  timer_data_type res;
  unsigned id = PLATFORM_TIMER_SYS_ID;
  any x, y;

  x = cdr(ex), y = EVAL(car(x));
  if (plen(ex) > 0) {
    NeedNum(ex, y);
    id = unBox(y);
    MOD_CHECK_TIMER(ex, id);
  }

  res = platform_timer_op(id, PLATFORM_TIMER_OP_GET_MIN_DELAY, 0);
  return box(res);
}

// (tmr-getmaxdelay ['num]) -> num
any tmr_getmaxdelay(any ex) {
  timer_data_type res;
  unsigned id = PLATFORM_TIMER_SYS_ID;
  any x, y;

  x = cdr(ex), y = EVAL(car(x));
  if (plen(ex) > 0) {
    NeedNum(ex, y);
    id = unBox(y);
    MOD_CHECK_TIMER(ex, id);
  }

  res = platform_timer_op(id, PLATFORM_TIMER_OP_GET_MAX_DELAY, 0);
  return box(res);
}

// (tmr-setclock 'num 'num) -> num
any tmr_setclock(any ex) {
  u32 clock;
  unsigned id;
  any x, y;

  x = cdr(ex), y = EVAL(car(x));
  NeedNum(ex, y);
  id = unBox(y); // get id.
  MOD_CHECK_TIMER(ex, id);

  x = cdr(x), y = EVAL(car(x));
  NeedNum(ex, y);
  clock = unBox(y); // get clock.

  clock = platform_timer_op(id, PLATFORM_TIMER_OP_SET_CLOCK, clock);
  return box(clock);
}

// (tmr-getclock ['num]) -> num
any tmr_getclock(any ex) {
  timer_data_type res;
  unsigned id = PLATFORM_TIMER_SYS_ID;
  any x, y;

  x = cdr(ex), y = EVAL(car(x));
  if (plen(ex) > 0) {
    NeedNum(ex, y);
    id = unBox(y);
    MOD_CHECK_TIMER(ex, id);
  }
  res = platform_timer_op(id, PLATFORM_TIMER_OP_GET_CLOCK, 0);
  return box(res);
}

#ifdef HAS_TMR_MATCH_INT_PICOLISP
// TODO: For now, Hempl doesn't support interrupts.
// any tmr_set_match_int(any ex)
// {}
#endif // HAS_TMR_MATCH_INT_PICOLISP

// Look for all VIRTx timer identifiers.
#if VTMR_NUM_TIMERS > 0
// (tmr-decode 'sym) -> num
any tmr_decode(any ex) {
  char* pend;
  long res;
  any x, y;

  x = cdr(ex), y = EVAL(car(x));
  NeedSym(ex, y);
  char key[bufSize(y)];
  bufString(y, key);
  if (strlen(key) > MAX_VTIMER_NAME_LEN ||
      strlen(key) < MIN_VTIMER_NAME_LEN)
    return box(0);
  if (strncmp(key, "VIRT", 4))
    return box(0);
  res = strtol(key + 4, &pend, 10);
  if (*pend != '\0')
    return box(0);
  if (res >= VTMR_NUM_TIMERS)
    return box(0);

  return box(res + VTMR_FIRST_ID);
}

#else

// Just in case VTMR_NUM_TIMERS not > 0
//
// This case has to be defined here since
// there is no other validation mechanism
// in modules/picolisp_mod.h
//
any tmr_decode(any ex) {
  err(ex, NULL, "VTMR_NUM_TIMERS is not > 0");
  return Nil;
}

#endif // #if VTMR_NUM_TIMERS > 0

