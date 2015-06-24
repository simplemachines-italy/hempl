// Module for interfacing with Lua SPI code
// Modified to include support for Hempl.

#include "pico.h"
#include "platform.h"

// ****************************************************************************
// SPI module for picoLisp.

// (spi-sson 'num) -> Nil
any plisp_spi_sson(any ex) {
  unsigned id;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, spi, id);

  platform_spi_select(id, PLATFORM_SPI_SELECT_ON);
  return Nil;
}

// (spi-ssoff 'num) -> Nil
any plisp_spi_ssoff(any ex) {
  unsigned id;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, spi, id);

  platform_spi_select(id, PLATFORM_SPI_SELECT_OFF);
  return Nil;
}

// (spi-setup 'num 'num 'num 'num 'num 'num) -> num
any plisp_spi_setup(any ex) {
  unsigned id, cpol, cpha, is_master, databits;
  u32 clock, res;
  any x, y;
  
  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, spi, id);

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  is_master = unBox(y); // get type.
  if (!is_master)
    err(ex, y, "invalid type - only *spi-master* is supported");

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  clock = unBox(y); // get clock.
  
  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  cpol = unBox(y); // clock polarity.
  if ((cpol != 0) && (cpol != 1))
    err(ex, y, "invalid clock polarity.");

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  cpha = unBox(y); // clock phase.
  if ((cpha != 0) && (cpha != 1))
    err(ex, y, "invalid clock phase.");

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  databits = unBox(y); // get databits.
  
  res = platform_spi_setup(id,
			   is_master,
			   clock,
			   cpol,
			   cpha,
			   databits);
  return box(res);
}

// Helpers for picoLisp spi print function.

static void outString_spi(unsigned id, char *s) {
  while (*s)
    platform_spi_send_recv(id, *s++);
}
 
static void outNum_spi(unsigned id, long n) {
  char buf[BITS/2];

  bufNum(buf, n);
  outString_spi(id, buf);
}

static void plisp_spih_prin(unsigned id, any x) {
  if (!isNil(x)) {
    if (isNum(x))
      outNum_spi(id, unBox(x));
    else if (isSym(x)) {
      int i, c;
      word w;
      u8 byte;

      for (x = name(x), c = getByte1(&i, &w, &x); c; c = getByte(&i, &w, &x)) {
        if (c != '^') {
          byte = c;
          platform_spi_send_recv(id, byte);
        }
        else if (!(c = getByte(&i, &w, &x))) {
	  byte = '^';
          platform_spi_send_recv(id, byte);
        }
        else if (c == '?') {
          byte = 127;
          platform_spi_send_recv(id, byte);
        }
        else {
          c &= 0x1F;
          byte = (u8)c;
          platform_spi_send_recv(id, byte);
        }
      }
    }
    else {
      while (plisp_spih_prin(id, car(x)), !isNil(x = cdr(x))) {
        if (!isCell(x)) {
          plisp_spih_prin(id, x);
          break;
	}
      }
    }
  }
}

// (spi-write 'num 'any) -> any
any plisp_spi_write(any ex) {
  unsigned id;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, spi, id);

  x = cdr(x), y = EVAL(car(x));
  plisp_spih_prin(id, y);
  return y;
}

// TODO:
// plisp_spi_readwrite(any) {}

