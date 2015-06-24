// Module for interfacing with ADC
// Modified to include support for Hempl.


#include "pico.h"
#include "platform.h"
#include "common.h"
#include "platform_conf.h"
#include "elua_adc.h"

#ifdef BUILD_ADC

// ****************************************************************************
// ADC (Analog to digital converter) module for picoLisp.

// (adc-maxval 'num) -> num
any plisp_adc_maxval(any ex) {
  unsigned id;
  u32 res;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, adc, id);

  res = platform_adc_get_maxval(id);
  return box(res);
}

// (adc-setclock 'num 'num 'num) -> num
any plisp_adc_setclock(any ex) {
  s32 sfreq; // signed version for negative checking.
  u32 freq;
  unsigned id, timer_id = 0;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get adc id.
  MOD_CHECK_ID(ex, adc, id);

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  sfreq = unBox(y); // get frequency.
  if (sfreq < 0)
    err(ex, y, "frequency must be 0 or positive");

  freq = (u32) sfreq;
  if (freq > 0) {
    x = cdr(x);
    NeedNum(ex, y = EVAL(car(x)));
    timer_id = unBox(y); // get timer id.
    MOD_CHECK_ID(ex, timer, timer_id);
    MOD_CHECK_RES_ID(ex, adc, id, timer, timer_id);
  }

  platform_adc_set_timer(id, timer_id);
  freq = platform_adc_set_clock(id, freq);
  return box(freq);
}

// (adc-isdone) -> T | Nil
any plisp_adc_isdone(any ex) {
  unsigned id;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, adc, id);

  return platform_adc_is_done(id) == 0 ?
    T : Nil;
}

// (adc-setblocking 'num 'num) -> Nil
any plisp_adc_setblocking(any ex) {
  unsigned id, mode;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, adc, id);

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  mode = unBox(y); // get mode.

  platform_adc_set_blocking(id, mode);
  return Nil;
}

// (adc-setsmoothing 'num 'num) -> num
any plisp_adc_setsmoothing(any ex) {
  unsigned id, length, res;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, adc, id);

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  length = unBox(y); // get length.
  if (!(length & (length - 1))) {
    res = platform_adc_set_smoothing(id, length);
    if (res == PLATFORM_ERR)
      err(ex, NULL, "Buffer allocation failed.");
    else
      return box(res);
  } else {
    err(ex, y, "Length must be power of 2");
  }
}

// (adc-sample 'num 'num) -> Nil
any plisp_adc_sample(any ex) {
  unsigned id, count = 0, nchans = 1;
  int res, i;
  any x, y, s;

  // get count value, the second parameter
  // in the picoLisp function call.
  s = cdr(ex), y = EVAL(car(s)); s = cdr(s);
  NeedNum(ex, y = EVAL(car(s)));
  count = unBox(y);

  // validate count.
  if ((count == 0) || count & (count - 1))
    err(ex, y, "count must be power of 2 and > 0");

  // get first parameter in the function
  // call.
  x = cdr(ex), y = EVAL(car(x));
  // If first parameter is a table,
  // extract channel list.
  if (isCell(y)) {
    nchans = length(y);
    for (i = 0; i < nchans; i++) {
      NeedNum(y, car(y));
      id = unBox(car(y));
      MOD_CHECK_ID(ex, adc, id);
      res = adc_setup_channel(id, intlog2(count));
      if (res != PLATFORM_OK)
	err(ex, y, "sampling setup failed");
    }
    // initiate sampling.
    platform_adc_start_sequence();
  } else if (isNum(y)) {
    NeedNum(ex, y);
    id = unBox(y);
    MOD_CHECK_ID(ex, adc, id);
    res = adc_setup_channel(id, intlog2(count));
    if (res != PLATFORM_OK)
      err(ex, y, "sampling setup failed");
    platform_adc_start_sequence();
  } else {
    err(ex, y, "invalid channel selection");
  }
  return Nil;
}

// (adc-getsample 'num) -> num
any plisp_adc_getsample(any ex) {
  unsigned id;
  any x, y;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id.
  MOD_CHECK_ID(ex, adc, id);

  // If we have at least one sample, return it.
  if (adc_wait_samples(id, 1) >= 1)
    return box(adc_get_processed_sample(id));

  return Nil;
}

// (adc-getsamples 'num ['num]) -> lst
any plisp_adc_getsamples(any ex) {
#ifdef BUF_ENABLE_ADC
  unsigned id, i;
  u16 bcnt, count = 0;
  any x, y;
  cell c1;

  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id
  MOD_CHECK_ID(ex, adc, id);

  if (plen(ex) >= 2) {
    x = cdr(x);
    NeedNum(ex, y = EVAL(car(x)));
    count = (u16)unBox(y); // get count
  }

  bcnt = adc_wait_samples(id, count);

  // If count is zero, grab all samples
  if (count == 0)
    count = bcnt;

  // Don't pull more samples than are available
  if (count > bcnt)
    count = bcnt;

  // Make the list of adc samples
  Push(c1, y = cons(box(adc_get_processed_sample(id)), Nil));
  for (i = 1; i < count - 1; i++)
    Push(c1, y = cons(box(adc_get_processed_sample(id)), y));

  return Pop(c1);
#else
  err(NULL, NULL, "BUF_ENABLE_ADC not defined");
#endif
}

// Helper function:
//
// Insert one element (value) in a list (list) at a
// given position (pos). The function will return the
// new list with the inserted value.

static any ins_element(any list, int pos, int value) {
  any temp, tab;
  int i;

  tab = temp = list;
  temp = cons(box(value), nCdr(pos - 1, temp));
  for (i = pos - 1; i > 0; i--) {
    temp = cons(car(nth(i, tab)), temp);
  }
  return temp;
}

// (adc-insertsamples 'num 'lst 'num 'num) -> lst
any plisp_adc_insertsamples(any ex) {
#ifdef BUF_ENABLE_ADC
  unsigned id, i, startidx;
  u16 bcnt, count;
  any x, y, tab;
  
  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  id = unBox(y); // get id
  MOD_CHECK_ID(ex, adc, id);

  // get the list of samples
  x = cdr(x);
  NeedLst(ex, y = EVAL(car(x)));
  tab = y;

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  startidx = unBox(y); // get startidx
  if (startidx <= 0)
    err(ex, y, "idx must be > 0");

  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  count = unBox(y); // get count
  if (count == 0)
    err(ex, y, "count must be > 0");

  bcnt = adc_wait_samples(id, count);

  for (i = startidx; i < (count + startidx); i++)
    tab = ins_element(tab, i, adc_get_processed_sample(id));

  return tab;
#else
  err(NULL, NULL, "BUF_ENABLE_ADC not defined");
#endif
}

#endif // #if defined BUILD_ADC
