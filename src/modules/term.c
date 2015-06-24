// Module for interfacing with terminal functions
// Modified to include support for Hempl.

#include "pico.h"

// Generic includes.
#include "platform.h"
#include "term.h"
#include <string.h>
#include "platform_conf.h"

// Common for all languages.
#undef _D
#define _D(x) #x
static const char* term_key_names[] = {TERM_KEYCODES};

#if defined BUILD_TERM

// ****************************************************************************
// Terminal module for picoLisp.

// (term-clrscr) -> Nil
any plisp_term_clrscr(any x) {
  term_clrscr();
  return Nil;
}

// (term-clreol) -> Nil
any plisp_term_clreol(any x) {
  term_clreol();
  return Nil;
}

// (term-moveto 'num 'num) -> Nil
any plisp_term_moveto(any ex) { 
  any x, y;
  long n1, n2;
  
  x = cdr(ex), y = EVAL(car(x));
  NeedNum(ex, y);
  n1 = unBox(y);
  x = cdr(x), y = EVAL(car(x));
  NeedNum(ex, y);
  n2 = unBox(y);
  term_gotoxy(n1, n2);
  return Nil;
}

// (term-moveup 'num) -> Nil
any plisp_term_moveup(any ex) {
  any x, y;
  long n;
  
  x = cdr(ex), y = EVAL(car(x));
  NeedNum(ex, y);
  n = unBox(y);
  term_up(n);
  return Nil;
}

// (term-movedown 'num) -> Nil
any plisp_term_movedown(any ex) {
  any x, y;
  long n;
  
  x = cdr(ex), y = EVAL(car(x));
  NeedNum(ex, y);
  n = unBox(y);
  term_down(n);
  return Nil;
}

// (term-moveleft 'num) -> Nil
any plisp_term_moveleft(any ex) {
  any x, y;
  long n;
  
  x = cdr(ex), y = EVAL(car(x));
  NeedNum(ex, y);
  n = unBox(y);
  term_left(n);
  return Nil;
}

// (term-moveright 'num) -> Nil
any plisp_term_moveright(any ex) {
  any x, y;
  long n;
  
  x = cdr(ex), y = EVAL(car(x));
  NeedNum(ex, y);
  n = unBox(y);
  term_right(n);
  return Nil;
}

// (term-getlines) -> num
any plisp_term_getlines(any x) {
  x = box(term_get_lines());
  return x;
}

// (term-getcols) -> num
any plisp_term_getcols(any x) {
  x = box(term_get_cols());
  return x;
}

// Helpers for picoLisp terminal print
// function.
static void outString_term(char *s) {
  while (*s)
    term_putch((u8)*s++);
}

static void outNum_term(long n) {
  char buf[BITS/2];

  bufNum(buf, n);
  outString_term(buf);
}

static void ptermh_prin(any x) {
  if (!isNil(x)) {
    if (isNum(x))
      outNum_term(unBox(x));
    else if (isSym(x)) {
      int i, c;
      word w;
      u8 byte;
      
      for (x = name(x), c = getByte1(&i, &w, &x); c; c = getByte(&i, &w, &x)) {
        if (c != '^') {
          byte = c;
	  term_putch(byte);
	}
        else if (!(c = getByte(&i, &w, &x))) {
	  byte = '^';
          term_putch(byte);
        }
        else if (c == '?') {
          byte = 127;
	  term_putch(byte);
        }
        else {
          c &= 0x1F;
          byte = (u8)c;
	  term_putch(byte);
	}
      }
    }
    else {
      while (ptermh_prin(car(x)), !isNil(x = cdr(x))) {
	if (!isCell(x)) {
	  ptermh_prin(x);
          break;
	}
      }
    }
  }
}

// (term-prinl ['num 'num] 'any ..) -> any
any plisp_term_prinl(any ex) {
  any x, y;
  long n1, n2;

  // if number of args > 1, we accept
  // a min of 3 args - x, y and the value
  // to print.
  if (plen(ex) > 1 && isNum(cadr(ex)) && isNum(caddr(ex))) {
    x = cdr(ex), y = EVAL(car(x));
    NeedNum(ex, y);
    n1 = unBox(y); // we get x here.
    x = cdr(x), y = EVAL(car(x));
    NeedNum(ex, y);
    n2 = unBox(y); // we get y here.
    term_gotoxy(n1, n2);
    // now, get the rest of the params
    // and prinl.
    while (isCell(x = cdr(x)))
      ptermh_prin(y = EVAL(car(x)));
  } else {
    // We don't have the coordinates.
    // we just print the first value
    // in the list (including NIL).
    x = cdr(ex), y = EVAL(car(x));
    ptermh_prin(y);
    while (isCell(x = cdr(x)))
      ptermh_prin(y = EVAL(car(x)));
  }

  newline();
  return y;
}

// (term-getcx) -> num
any plisp_term_getcx(any x) {
  x = box(term_get_cx());
  return x;
}

// (term-getcy) -> num
any plisp_term_getcy(any x) {
  x = box(term_get_cy());
  return x;
}

// (term-getchar ['sym]) -> num
any plisp_term_getchar(any ex) {
  any x, y;
  int temp = TERM_INPUT_WAIT, ret;

  // if number of args is > 0
  // get value; else getchar()
  // will wait.
  if (plen(ex) > 0) {
    x = cdr(ex);
    NeedNum(ex, y = EVAL(car(x)));
    return ((ret = term_getch(temp = unBox(y))) == -1?
	    Nil : box(ret));
  }
  return ((ret = term_getch(temp)) == -1?
	  Nil : box(ret));
}

// (term-decode 'sym) -> num | Nil
any plisp_term_decode(any ex) {
  any x, y;
  unsigned i, total = sizeof(term_key_names) / sizeof(char*);

  x = cdr(ex), y = EVAL(car(x));
  NeedSymb(ex, y);
  char key[bufSize(y)];
  bufString(y, key);

  if (*key != 'K')
    return Nil;

  for (i = 0; i < total; i++)
    if (!strcmp(key, term_key_names[i]))
      break;

  if (i == total)
    return Nil;
  else
    return box(i + TERM_FIRST_KEY);
}

#endif // #if defined BUILD_TERM
