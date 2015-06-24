// Module for interfacing with PWM
// Modified to include support for Hempl.

#include "platform.h"

// ****************************************************************************
// Pulse width modulation module for picoLisp.

#include "pico.h"

// (pwm-setup 'num 'num 'num) -> num
any plisp_pwm_setup(any ex) {
  u32 freq;
  unsigned duty, id;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, pwm, id);

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  freq = unBox(y); // get frequency.
  if (freq <= 0)
    err(ex, y, "frequency must be > 0");

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  duty = unBox(y); // get duty cycle.
  if (duty > 100)
    err(ex, y, "duty cycle must be from 0 to 100");

  freq = platform_pwm_setup(id, freq, duty);
  return box(freq);
}

// (pwm-start 'num) -> Nil
any plisp_pwm_start(any ex) {
  unsigned id;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, pwm, id);
  platform_pwm_start(id);

  return Nil;
}

// (pwm-stop 'num) -> Nil
any plisp_pwm_stop(any ex) {
  unsigned id;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, pwm, id);
  platform_pwm_stop(id);

  return Nil;
}

// (pwm-setclock 'num 'num) -> num
any plisp_pwm_setclock(any ex) {
  unsigned id;
  u32 clk;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, pwm, id);

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  clk = unBox(y); // get clock.
  if (clk <= 0)
    err(ex, y, "frequency must be > 0");

  clk = platform_pwm_set_clock(id, (u32)clk);
  return box(clk);
}

// (pwm-getclock 'num) -> num
any plisp_pwm_getclock(any ex) {
  unsigned id;
  u32 clk;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, pwm, id);

  clk = platform_pwm_get_clock(id);
  return box(clk);
}

