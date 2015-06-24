// Module for interfacing with PIO
// Modified to include support for Hempl.

#include "pico.h"
#include "platform.h"
#include "platform_conf.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// PIO public constants
#define PIO_DIR_OUTPUT      0
#define PIO_DIR_INPUT       1

// PIO private constants
#define PIO_PORT_OP         0
#define PIO_PIN_OP          1

// Local operation masks for all the ports
static pio_type pio_masks[ PLATFORM_IO_PORTS ];

// ****************************************************************************
// Generic helper functions

// Helper function: clear all masks
//
static void pioh_clear_masks(void) {
  int i;
  
  for (i = 0; i < PLATFORM_IO_PORTS; i++)
    pio_masks[i] = 0;
}

#define emit_hfunc_set_pin()\
  plisp_pioh_set_pin(any ex, any y, int v, int op)

#define emit_hfunc_set_port()\
  plisp_pioh_set_port(any ex, any y, int v, int op, pio_type mask)

#define emit_hfunc_setdir()\
  plisp_pio_gen_setdir(any ex, any y, int v, int optype, int op)

#define emit_hfunc_setpull()\
  plisp_pio_gen_setpull(any ex, any y, int v, int optype, int op)

# define emit_hfunc_setval()\
  plisp_pio_gen_setval(any ex, any y, int v, int optype, pio_type val)

#define emit_hfunc_error(msg)\
  err(ex, y, msg);

#define emit_static
#define emit_ret_type any
#define emit_pio_decode()\
  plisp_pio_decode(any ex)

#define emit_pio_numeric_ident()\
  pio_value_parse(char *key)

// helper macro.
#define PIO_CHECK(x)\
  if (!x)\
    emit_hfunc_error("Invalid port/pin");

static int emit_hfunc_set_pin() {
  pio_type pio_mask = 0;
  int port, pin;

  pioh_clear_masks();
  port = PLATFORM_IO_GET_PORT(v);
  pin = PLATFORM_IO_GET_PIN(v);

  if (PLATFORM_IO_IS_PORT(v) ||
      !platform_pio_has_port(port) ||
      !platform_pio_has_pin(port, pin))
    emit_hfunc_error("invalid pin");

  pio_mask |= 1 << pin;
  if (pio_mask)
    if (!platform_pio_op(port, pio_mask, op))
      emit_hfunc_error("invalid PIO operation");
  return 0;
}

static int emit_hfunc_set_port() {
  int port;
  u32 port_mask = 0;

  port = PLATFORM_IO_GET_PORT(v);
  if (!PLATFORM_IO_IS_PORT(v) ||
      !platform_pio_has_port(port))
    emit_hfunc_error("invalid port");

  port_mask |= (1ULL << port);
  if (port_mask & (1ULL << port))
    if (!platform_pio_op(port, mask, op))
      emit_hfunc_error("invalid PIO operation");
  return 0;
}

static int emit_hfunc_setdir() {
  if (op == PIO_DIR_INPUT)
    op = optype == PIO_PIN_OP ? PLATFORM_IO_PIN_DIR_INPUT : PLATFORM_IO_PORT_DIR_INPUT;
  else if (op == PIO_DIR_OUTPUT)
    op = optype == PIO_PIN_OP ? PLATFORM_IO_PIN_DIR_OUTPUT : PLATFORM_IO_PORT_DIR_OUTPUT;
  else
    emit_hfunc_error("invalid direction");

  // Check if pin operation.
  if (optype == PIO_PIN_OP) {
    plisp_pioh_set_pin(ex, y, v, op);
  // else, we have a port operation.
  } else {
    plisp_pioh_set_port(ex, y, v, op, PLATFORM_IO_ALL_PINS);
  }
  return 0;
}

static int emit_hfunc_setpull() {
  if ((op != PLATFORM_IO_PIN_PULLUP) &&
      (op != PLATFORM_IO_PIN_PULLDOWN) &&
      (op != PLATFORM_IO_PIN_NOPULL))
    emit_hfunc_error("invalid pull type");

  // Check if pin operation.
  if (optype == PIO_PIN_OP) {
    plisp_pioh_set_pin(ex, y, v, op);
  // else, we have a port operation.
  } else {
    plisp_pioh_set_port(ex, y, v, op, PLATFORM_IO_ALL_PINS);
  }

  return 0;
}

static int emit_hfunc_setval() {
  if ((optype == PIO_PIN_OP) &&
      (val != 1) &&
      (val != 0))
    emit_hfunc_error("invalid pin value");
 
  // Check if a pin operation.
  if (optype == PIO_PIN_OP) {
    plisp_pioh_set_pin(ex, y, v, val == 1 ? PLATFORM_IO_PIN_SET : PLATFORM_IO_PIN_CLEAR);
  // else, we have a port operation.
  } else {
    plisp_pioh_set_port(ex, y, v, val == 1 ? PLATFORM_IO_PIN_SET : PLATFORM_IO_PIN_CLEAR, val);
  }

  return 0;
}

// Helper function.
// returns pin/port numeric identifiers.
int emit_pio_numeric_ident() {
  int port = 0xFFFF, pin = 0xFFFF, isport = 0, sz;

  if (!key || *key != 'P')
    return 0;
  if (isupper(key[1])) { // PA, PB, ...
    if (PIO_PREFIX != 'A')
      return 0;
    port = key[1] - 'A';
    if (key[2] == '\0')
      isport = 1;
    else if (key[2] == '_') {
      if (sscanf(key + 3, "%d%n", &pin, &sz) != 1 || sz != strlen(key) - 3)
        return 0;

#ifdef ALCOR_PLATFORM_AVR32
      /* AVR32UC3A0 has a bizarre "port" called "PX" with 40 pins which map to
       * random areas of hardware ports 2 and 3:
       * PX00-PX10 = GPIO100-GPIO90     //Port 3 pins 04-00; port 2 pins 31-26
       * PX11-PX14 = GPIO109-GPIO106    //Port 3 pins 13-10
       * PX15-PX34 = GPIO89-GPIO70      //Port 2 pins 25-06
       * PX35-PX39 = GPIO105-GPIO101    //Port 3 pins 09-05
       * Then port = trunc(GPIO/32) and pin = GPIO % 32
       *
       * This "Port X" exists in EVK1100 and MIZAR32 but not on EVK1101, which
       * only has ports A and B. On EXK1101, the PC and PX syntax will still be
       * accepted but will return nil thanks to the checks against NUM_PIO. 
       */

      // Disallow "PC_06-PC_31" as aliases for PX pins
      if (key[1] == 'C' && pin > 5)
        return 0;

      // Disallow "PD_nn" as aliases for PX pins
      if (key[1] == 'D')
        return 0;

      // Map PX pins 00-39 to their ports/pins in the hardware register layout.
      if (key[1] == 'X') {
        unsigned gpio;

        // You cannot perform port operations on port X because it
        // doesn't exist in hardware.
        if (pin == 0xFFFF)
          return 0;

        // Map PX pin numbers to GPIO pin numbers
        if (pin < 0) return 0;
        if (pin <= 10) gpio = 100 - pin;
        else if (pin <= 14) gpio = 109 - (pin - 11);
        else if (pin <= 34) gpio = 89 - (pin - 15);
        else if (pin <= 39) gpio = 105 - (pin - 35);
        else return 0;

        port = gpio >> 5;
        pin = gpio & 0x1F;
      }
#endif
    }
  } else { // P0, P1, ...
    if (PIO_PREFIX != '0')
      return 0;
    if (!strchr(key, '_')) {  // parse port
      if (sscanf(key + 1, "%d%n", &port, &sz) != 1  || sz != strlen(key) - 1)
        return 0;
      isport = 1;
    } else {   // parse port_pin
      if (sscanf(key + 1, "%d_%d%n", &port, &pin, &sz) != 2 || sz != strlen(key) - 1)
        return 0;
    }
  }
  sz = -1;
  if (isport) {
    if (platform_pio_has_port(port))
      sz = PLATFORM_IO_ENCODE(port, 0, 1);
  } else {
    if (platform_pio_has_port(port) && platform_pio_has_pin(port, pin))
      sz = PLATFORM_IO_ENCODE(port, pin, 0);
  }
  if (sz == -1) {
    return 0;
  } else {
    return sz;
  }
}

// The 'decode' function returns a port/pin pair
// from a platform code.
//
emit_static emit_ret_type emit_pio_decode() {
  int code, port, pin;
  any x, y;
  cell c1;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  code = unBox(y); // get code.

  // get pin and port values.
  port = PLATFORM_IO_GET_PORT(code);
  pin = PLATFORM_IO_GET_PIN(code);

#ifdef ALCOR_PLATFORM_AVR32
  /* AVR32UC3A0 has a bizarre "port" called "PX" with 40 pins which map to
   * random areas of hardware ports 2 and 3:
   * PX00-PX04 = GPIO100-GPIO96     //Port 3 pins 04-00
   * PX05-PX10 = GPIO95-GPIO90      //Port 2 pins 31-26
   * PX11-PX14 = GPIO109-GPIO106    //Port 3 pins 13-10                       
   * PX15-PX34 = GPIO89-GPIO70      //Port 2 pins 25-06
   * PX35-PX39 = GPIO105-GPIO101    //Port 3 pins 09-05
   *
   * Here, we reverse the decode the hardware port/pins to the PX pin names.
   * This is the inverse of the code above in pio_mt_index().
   */
  if ((port == 2 && pin >= 6) ||
      (port == 3 && pin <= 13)) {
    switch (port) {
    case 2:
      if (pin >= 26)
        pin = (26 + 10) - pin;  // PX05-PX10
      else
        pin = (25 + 15) - pin;  // PX15-PX34
      break;
    case 3:
      if (pin <= 4)
        pin = 4 - pin;          // PX00-PX04
      else if (pin <= 9)
        pin = (35 + 9) - pin;   // PX35-PX39
      else /* 10-13 */
        pin = (13 + 11) - pin;  // PX11-PX14
      break;
    }
    port = 'X' - 'A';   // 'A','B','C' are returned as 0,1,2 so 'X' is 23
  }
#endif

  Push(c1, y = cons(box(port), Nil));
  Push(c1, y = cons(box(pin), y));

  return Pop(c1);
}

// ****************************************************************************
// PIO module for picoLisp.
//
// Pin operations.

// (pio-pin-setdir 'sym 'num) -> Nil
any plisp_pio_pin_setdir(any ex) {
  any x, y;
  int ret, dir;

  // get dir.
  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  dir = unBox(y);

  // get symbol.
  x = cdr(x);
  NeedSym(ex, y = EVAL(car(x)));
  char s[bufSize(y)];
  bufString(y, s);
  ret = pio_value_parse(s);
  PIO_CHECK(ret);

  plisp_pio_gen_setdir(ex, NULL, ret, PIO_PIN_OP, dir);
  return Nil;
}

// (pio-pin-setpull 'sym 'num) -> Nil
any plisp_pio_pin_setpull(any ex) {
  any x, y;
  int ret, dir;

  // get dir.
  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  dir = unBox(y);

  // get symbol.
  x = cdr(x);
  NeedSym(ex, y = EVAL(car(x)));
  char s[bufSize(y)];
  bufString(y, s);
  ret = pio_value_parse(s);
  PIO_CHECK(ret);
  
  plisp_pio_gen_setpull(ex, NULL, ret, PIO_PIN_OP, dir);
  return Nil;
}

// (pio-pin-setval 'sym 'num) -> Nil
any plisp_pio_pin_setval(any ex) {
  any x, y;
  int ret; pio_type val;

  // get value to set first.
  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  val = unBox(y);
  
  // get symbol. 
  x = cdr(x);
  NeedSym(ex, y = EVAL(car(x)));
  char s[bufSize(y)];
  bufString(y, s);
  ret = pio_value_parse(s);
  PIO_CHECK(ret);

  plisp_pio_gen_setval(ex, NULL, ret, PIO_PIN_OP, val);
  return Nil;
}

// (pio-pin-sethigh 'sym) -> Nil
any plisp_pio_pin_sethigh(any ex) {
  any x, y;
  int ret;

  // get symbol.
  x = cdr(ex);
  NeedSym(ex, y = EVAL(car(x)));
  char s[bufSize(y)];
  bufString(y, s);
  ret = pio_value_parse(s);
  PIO_CHECK(ret);

  plisp_pio_gen_setval(ex, NULL, ret, PIO_PIN_OP, 1);
  return Nil;
}

// (pio-pin-setlow 'sym) -> Nil
any plisp_pio_pin_setlow(any ex) {
  any x, y;
  int ret;

  // get symbol.
  x = cdr(ex);
  NeedSym(ex, y = EVAL(car(x)));
  char s[bufSize(y)];
  bufString(y, s);
  ret = pio_value_parse(s);
  PIO_CHECK(ret);

  plisp_pio_gen_setval(ex, NULL, ret, PIO_PIN_OP, 0);
  return Nil;
}

// (pio-pin-getval 'sym) -> num
any plisp_pio_pin_getval(any ex) {
  pio_type value;
  int v, port, pin;
  any x, y;

  // get symbol.
  x = cdr(ex);
  NeedSym(ex, y = EVAL(car(x)));
  char s[bufSize(y)];
  bufString(y, s);
  v = pio_value_parse(s);
  PIO_CHECK(v);

  port = PLATFORM_IO_GET_PORT(v);
  pin = PLATFORM_IO_GET_PIN(v);

  if (PLATFORM_IO_IS_PORT(v) ||
      !platform_pio_has_port(port) ||
      !platform_pio_has_pin(port, pin)) {
    emit_hfunc_error("invalid pin");
  } else {
    value = platform_pio_op(port, 1 << pin, PLATFORM_IO_PIN_GET);
    return box(value);
  }
}

// ****************************************************************************
// Port operations

// (pio-port-setdir 'sym 'num) -> Nil
any plisp_pio_port_setdir(any ex) {
  any x, y;
  int ret, dir;

  // get direction first.
  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  dir = unBox(y);

  // get symbol.
  x = cdr(x);
  NeedSym(ex, y = EVAL(car(x)));
  char s[bufSize(y)];
  bufString(y, s);
  ret = pio_value_parse(s);
  PIO_CHECK(ret);

  plisp_pio_gen_setdir(ex, NULL, ret, PIO_PORT_OP, dir);
  return Nil;
}

// (pio-port-setpull 'sym 'num) -> Nil 
any plisp_pio_port_setpull(any ex) {
  any x, y;
  int ret, dir;

  // get pull value first.
  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  dir = unBox(y);

  // get symbol.
  x = cdr(x);
  NeedSym(ex, y = EVAL(car(x)));
  char s[bufSize(y)];
  bufString(y, s);
  ret = pio_value_parse(s);
  PIO_CHECK(ret);

  plisp_pio_gen_setpull(ex, NULL, ret, PIO_PORT_OP, dir);
  return Nil;
}

// (pio-port-setval 'sym) -> Nil
any plisp_pio_port_setval(any ex) {
  any x, y;
  int ret; pio_type val;

  // get set value first.
  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  val = (pio_type)unBox(y);

  // get symbol.
  x = cdr(x);
  NeedSym(ex, y = EVAL(car(x)));
  char s[bufSize(y)];
  bufString(y, s);
  ret = pio_value_parse(s);
  PIO_CHECK(ret);

  plisp_pio_gen_setval(ex, NULL, ret, PIO_PORT_OP, val);
  return Nil;
}

// (pio-port-sethigh 'sym) -> Nil
any plisp_pio_port_sethigh(any ex) {
  any x, y;
  int ret;

  // get symbol.
  x = cdr(ex);
  NeedSym(ex, y = EVAL(car(x)));
  char s[bufSize(y)];
  bufString(y, s);
  ret = pio_value_parse(s);
  PIO_CHECK(ret);

  plisp_pio_gen_setval(ex, NULL, ret, PIO_PORT_OP, 1);
  return Nil;
}

// (pio-port-setlow 'sym) -> Nil
any plisp_pio_port_setlow(any ex) {
  any x, y;
  int ret;

  // get symbol.
  x = cdr(ex);
  NeedSym(ex, y = EVAL(car(x)));
  char s[bufSize(y)];
  bufString(y, s);
  ret = pio_value_parse(s);
  PIO_CHECK(ret);

  plisp_pio_gen_setval(ex, NULL, ret, PIO_PORT_OP, 0);
  return Nil;
}

// (pio-port-getval 'sym) -> Nil
any plisp_pio_port_getval(any ex) {
  pio_type value;
  int v, port;
  any x, y;

  // get symbol.
  x = cdr(ex);
  NeedSym(ex, y = EVAL(car(x)));
  char s[bufSize(y)];
  bufString(y, s);
  v = pio_value_parse(s);
  PIO_CHECK(v);
  port = PLATFORM_IO_GET_PORT(v);

  if (!PLATFORM_IO_IS_PORT(v) ||
      !platform_pio_has_port(port)) {
    emit_hfunc_error("invalid port");
  } else {
    value = platform_pio_op(port,
			    PLATFORM_IO_ALL_PINS,
			    PLATFORM_IO_PORT_GET_VALUE);
  }
  return box(value);
}

