// Module for interfacing with UART
// Modified to include support for Hempl.

#include "pico.h"

// Generic includes.
#include "platform.h"
#include "common.h"
#include "sermux.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "platform_conf.h"

// Modes for the UART read function
enum
{
  UART_READ_MODE_LINE,
  UART_READ_MODE_NUMBER,
  UART_READ_MODE_SPACE,
  UART_READ_MODE_MAXSIZE
};

#define UART_INFINITE_TIMEOUT PLATFORM_TIMER_INF_TIMEOUT

#if defined BUILD_SERMUX
# define MAX_VUART_NAME_LEN    6
# define MIN_VUART_NAME_LEN    6
#endif

// ****************************************************************************
// UART module for picoLisp.

// (uart-setup 'num 'num 'num 'num 'num) -> num
any plisp_uart_setup(any ex) {
  unsigned id, databits, parity, stopbits;
  u32 baud, res;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get uart id.

  MOD_CHECK_ID(ex, uart, id);
  if (id >= SERMUX_SERVICE_ID_FIRST)
    err(ex, y, "uart-setup can't be called on virtual UARTs");

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  baud = unBox(y); // get baud rate.

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  databits = unBox(y); // get num of bits.

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  parity = unBox(y); // get parity.

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  stopbits = unBox(y); // get stopbits.

  res = platform_uart_setup(id, baud, databits, parity, stopbits);
  return res == 0 ? Nil : box(res);
}

// Helpers for picoLisp uart 'write' function.
//
static void outString_uart(unsigned id, char *s) {
  while (*s)
    platform_uart_send(id, *s++);
}

static void outNum_uart(unsigned id, long n) {
  char buf[BITS/2];

  bufNum(buf, n);
  outString_uart(id, buf);
}

static void uarth_prin(unsigned id, any x) {
  if (!isNil(x)) {
    if (isNum(x))
      outNum_uart(id, unBox(x));
    else if (isSym(x)) {
      int i, c;
      word w;
      u8 byte;

      for (x = name(x), c = getByte1(&i, &w, &x); c; c = getByte(&i, &w, &x)) {
	if (c != '^') {
          byte = c;
          platform_uart_send(id, byte);
        }
        else if (!(c = getByte(&i, &w, &x))) {
          byte = '^';
          platform_uart_send(id, byte);
	}
        else if (c == '?') {
          byte = 127;
          platform_uart_send(id, byte);
        }
        else {
          c &= 0x1F;
          byte = (u8)c;
          platform_uart_send(id, byte);
	}
      }
    }
    else {
      while (uarth_prin(id, car(x)), !isNil(x = cdr(x))) {
	if (!isCell(x)) {
	  uarth_prin(id, x);
          break;
	}
      }
    }
  }
}

// (uart-write 'num 'any ..) -> any
any plisp_uart_write(any ex) {
  unsigned id;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.                                                                                               
  MOD_CHECK_ID(ex, uart, id);

  while (isCell(x = cdr(x)))
    uarth_prin(id, y = EVAL(car(x)));
  return y;
}

// (uart-read 'num 'sym ['num] ['num])
any plisp_uart_read(any ex) {
  int id, res, mode, issign;
  unsigned timer_id = PLATFORM_TIMER_SYS_ID;
  s32 maxsize = 0, count = 0;
  char *buffer = NULL;
  char cres;
  timer_data_type timeout = PLATFORM_TIMER_INF_TIMEOUT;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, uart, id);

  // Check format.
  x = cdr(x), y = EVAL(car(x));
  if (isNum(y)) {
    if ((maxsize = unBox(y)) < 0)
      err(ex, y, "invalid max size");
    mode = UART_READ_MODE_MAXSIZE;
  } else {
    if (equal(mkStr("*l"), y))
      mode = UART_READ_MODE_LINE;
    else if (equal(mkStr("*n"), y))
      mode = UART_READ_MODE_NUMBER;
    else if (equal(mkStr("*s"), y))
      mode = UART_READ_MODE_SPACE;
    else
      err(ex, y, "invalid format");
  }
  // Check if we have optional parameters.
  // Collect them if we do. Supplying optional
  // arguments means that we have atleast >
  // 2 parameters.
  if (plen(ex) > 2) {
    x = cdr(x);
    NeedNum(ex, y = EVAL(car(x)));
    timeout = unBox(y); // get timeout.
    x = cdr(x);
    NeedNum(ex, y = EVAL(car(x)));
    timer_id = unBox(y); // get timer id.
  }

  // Read data
  while (1) {
    buffer = realloc(buffer, 1);
    if ((res = platform_uart_recv(id, timer_id, timeout)) == -1)
      break; 
    cres = (char)res;
    count++;
    issign = (count == 1) && ((res == '-') || (res == '+'));
    // [TODO] this only works for lines that actually end with '\n',
    // other line endings are not supported.
    if ((cres == '\n') && (mode == UART_READ_MODE_LINE))
      break;
    if (!isdigit(cres) && !issign && (mode == UART_READ_MODE_NUMBER))
      break;
    if (isspace(cres) && (mode == UART_READ_MODE_SPACE))
      break;
    buffer[count - 1] = cres;
    if ((count == maxsize) && (mode == UART_READ_MODE_MAXSIZE))
      break;
  }
  buffer[count] = '\0';

  // Return an integer if needed
  if (mode == UART_READ_MODE_NUMBER)
    return box(res);
  else
    return mkStr(buffer);
}

// (uart-set-buffer 'num 'num) -> Nil
any plisp_uart_set_buffer(any ex) {
  int id; u32 size;  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, uart, id);

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  size = (u32)unBox(y); // get size.
  if (size && (size & (size - 1)))
    err(ex, y, "the buffer size must be a power of 2 or 0");
  if (size == 0 && id >= SERMUX_SERVICE_ID_FIRST)
    err(ex, y, "disabling buffers on virtual UARTs is not allowed");

  if (platform_uart_set_buffer(id, intlog2(size) == PLATFORM_ERR))
    err(ex, NULL, "unable to set UART buffer");

  return Nil;
}

// (uart-set-flow-control 'num 'num) -> Nil
any plisp_uart_set_flow_control(any ex) {
  any x, y;
  int id, type;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, uart, id);

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  type = unBox(y); // get type.

  if (platform_uart_set_flow_control(id, type) != PLATFORM_OK)
    err(ex, y, "unable to set the flow control on this interface.");

  return Nil;
}

// (uart-getchar 'num ['num] ['num]) -> Nil | sym
any plisp_uart_getchar(any ex) {
  int id, res;
  char cres[2];
  unsigned timer_id = PLATFORM_TIMER_SYS_ID;
  timer_data_type timeout = PLATFORM_TIMER_INF_TIMEOUT;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get uart id.
  MOD_CHECK_ID(ex, uart, id);

  if (plen(ex) > 1) {
    x = cdr(x);
    NeedNum(ex, y = EVAL(car(x)));
    timeout = unBox(y); // get timeout.

    x = cdr(x);
    NeedNum(ex, y = EVAL(car(x)));
    timer_id = unBox(y); // get timer id.
  }
  res = platform_uart_recv(id, timer_id, timeout);
  if (res) {
    cres[0] = (char)res;
    cres[1] = '\0';
    return mkStr(cres);
  }
  // In case -1 is returned, return Nil. If an empty
  // string is returned, picoLisp will throw an output
  // which might look a bit like this.
  // -> 
  // Nil provides more meaning.
  return Nil;
}

#if defined BUILD_SERMUX

// Look for all VUARTx timer identifiers.
//
// (uart-vuart-tmr-ident 'sym) -> Nil | num
any plisp_uart_vuart_tmr_ident(any ex) {
  char* pend;
  long res;
  any x, y;

  x = cdr(ex);
  NeedSym(ex, y = EVAL(car(x)));
  char key[bufSize(y)];
  bufString(y, key); // get key.

  if (strlen(key) > MAX_VUART_NAME_LEN ||
      strlen(key) < MIN_VUART_NAME_LEN)
    return Nil;

  if (strncmp(key, "VUART", 5))
    return Nil;

  res = strtol(key + 5, &pend, 10);
  if (*pend != '\0')
    return Nil;

  if (res >= SERMUX_NUM_VUART)
    return Nil;

  return box(SERMUX_SERVICE_ID_FIRST + res);
}

#else

// If SERMUX is not enabled, we have a dummy
// function which just reports the same.
any plisp_uart_vuart_tmr_ident(any ex) {
  err(NULL, NULL, "SERMUX support not enabled in this build.");
  return Nil;
}

#endif // #if defined BUILD_SERMUX

