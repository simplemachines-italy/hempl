// Module for interfacing with the I2C interface
// Modified to include support for Hempl.

#include "pico.h"

// Generic includes.
#include "platform.h"
#include <string.h>
#include <ctype.h>

// ****************************************************************************
// I2C module for picoLisp.

// (i2c-setup 'num 'num) -> num
any plisp_i2c_setup(any ex) {
  any x, y;
  unsigned id;
  s32 speed;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y);
  MOD_CHECK_ID(ex, i2c, id);

  x = cdr(x);
  NeedNum(x, y = EVAL(car(x)));
  speed = unBox(y);
  if (speed <= 0)
    err(ex, y, "frequency must be > 0");

  return box(platform_i2c_setup(id, (u32)speed));
}

// (i2c-start 'num) -> Nil
any plisp_i2c_start(any ex) {
  unsigned id;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y);
  MOD_CHECK_ID(ex, i2c, id);
  platform_i2c_send_start(id);

  return Nil;
}

// (i2c-stop 'num) -> Nil
any plisp_i2c_stop(any ex) {
  unsigned id;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y);
  MOD_CHECK_ID(ex, i2c, id);
  platform_i2c_send_stop(id);

  return Nil;
}

// (i2c-address 'num 'num 'num) -> num
any plisp_i2c_address(any ex) {
  unsigned id;
  int add, dir, ret;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.  
  MOD_CHECK_ID(ex, i2c, id);

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  add = unBox(y); // get address.

  // check address.
  if (add < 0 || add > 127)
    err(ex, y, "slave address must be from 0 to 127");

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  dir = unBox(y); // get direction.
  
  ret = platform_i2c_send_address(id, (u16)add, dir);

  return ret == 1 ? T : Nil;
}

// Helpers for picoLisp i2c 'write' function.

static void outString_i2c(unsigned id, char *s) {
  while (*s)
    platform_i2c_send_byte(id, *s++);
}

static void outNum_i2c(unsigned id, long n) {
  char buf[BITS/2];

  bufNum(buf, n);
  outString_i2c(id, buf);
}

static void i2ch_prin(unsigned id, any x) {
  if (!isNil(x)) {
    if (isNum(x))
      outNum_i2c(id, unBox(x));
    else if (isSym(x)) {
      int i, c;
      word w;
      u8 byte;
 
      for (x = name(x), c = getByte1(&i, &w, &x); c; c = getByte(&i, &w, &x)) {
        if (c != '^') {
          byte = c;
          platform_i2c_send_byte(id, byte);
        }
        else if (!(c = getByte(&i, &w, &x))) {
          byte = '^';
	  platform_i2c_send_byte(id, byte);
	}
        else if (c == '?') {
          byte = 127;
	  platform_i2c_send_byte(id, byte);
        }
        else {
          c &= 0x1F;
          byte = (u8)c;
	  platform_i2c_send_byte(id, byte);
        }
      }
    }
    else {
      while (i2ch_prin(id, car(x)), !isNil(x = cdr(x))) {
        if (!isCell(x)) {
	  i2ch_prin(id, x);
          break;
	}
      }
    }
  }
}

// (i2c-write 'num 'any ..) -> any
any plisp_i2c_write(any ex) {
  unsigned id;
  any x, y = Nil;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, i2c, id);

  while (isCell(x = cdr(x)))
    i2ch_prin(id, y = EVAL(car(x)));
  return y;
}

// (i2c-read 'num 'num) -> sym
any plisp_i2c_read(any ex) {
  unsigned id;
  u32 size, i, count = 0;
  int data;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, i2c, id);

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  size = unBox(y); // get size.
  char *b = malloc(size + 1);

  if (size == 0)
    return Nil;
  for (i = 0; i < size; i++) {
    if ((data = platform_i2c_recv_byte(id, i < size - 1)) == -1)
      break;
    else
      b[count++] = (char)data;
  }
  return mkStr(b);
}

