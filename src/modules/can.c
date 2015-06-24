// Module for interfacing Lua code with a Controller Area Network (CAN)
// Modified to include support for Hempl.

#include "platform.h"
#include "pico.h"

// ****************************************************************************
// CAN for picoLisp.

// (can-setup id clock) -> clock
any plisp_can_setup(any ex) {
  unsigned id;
  u32 clock;
  any x, y;

  // get id.
  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y);
  MOD_CHECK_ID(ex, can, id);
  
  // get clock value.
  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  clock = unBox(y);

  return box(platform_can_setup(id, clock));
}

// (can-send id canid canidtype 'message) -> message
any plisp_can_send(any ex) {
  size_t len;
  int id, canid, idtype;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // id.
  MOD_CHECK_ID(ex, can, id);

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  canid = unBox(y); // can id.

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  idtype = unBox(y); // id type.

  x = cdr(x);
  len = bufSize(y = EVAL(car(x)));
  char data[len];
  NeedSym(ex, y);
  bufString(y, data); // can data.
  
  if (len > PLATFORM_CAN_MAXLEN)
    err(ex, y, "message exceeds max length");

  platform_can_send(id, canid, idtype, len, (const u8 *)data);
  return mkStr(data);
}

// (can-recv id) -> str
any plisp_can_recv(any ex) {
  u8 len, idtype, data[8];
  int id;
  u32 canid;
  any x, y;
  cell c1;

  // get id.
  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y);
  MOD_CHECK_ID(ex, can, id);

  if (platform_can_recv(id,
			&canid,
			&idtype,
			&len,
			data) == PLATFORM_OK) {
    Push(c1, y = cons(mkStr((char *)data), Nil));
    Push(c1, y = cons(box(idtype), y));
    Push(c1, y = cons(box(canid), y));
    return Pop(c1);
  } else {
    return Nil;
  }
}
