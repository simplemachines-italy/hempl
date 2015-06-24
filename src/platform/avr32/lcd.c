// eLua module for Mizar32 LCD character display
// Modified to include support for Hempl.

#include "pico.h"

// Generic includes.
#include "platform.h"
#include "platform_conf.h"
#include "lcd.h"       
#include "i2c.h"

// The LCD firmware only runs at up to 50kHz on the I2C bus, so
// we bracket all I2C packets to the LCD module with two functions
// to be able to save, change and restore the I2C clock rate to what it was
// before.

// Declarations to save/restore the I2C clock rate
extern u32 i2c_delay;
static u32 old_i2c_delay;

static void lcd_start()
{
  old_i2c_delay = i2c_delay;
  i2c_delay = REQ_CPU_FREQ / LCD_BUS_FREQ / 2;
}

static void lcd_stop()
{
  i2c_delay = old_i2c_delay;
}

// Low-level functions to send LCD commands or data.
// The command and data packets differ only in the slave address used,
// so we coalesce them into a single function, generating smaller code.
//
// All three are designed to be used in a tail call:
//   return send_generic( data, len );

// Send a command or data packet.
// "address" is LCD_CMD for LCD commands, LCD_DATA for LCD data.
static int send_generic( u8 address, const u8 *data, int len )
{
  lcd_start();
  i2c_send( address, data, len, true );
  lcd_stop();
  return 0;
}

// Send an I2C read-data command and return the answer.
// "address" is LCD_GETPOS to read the cursor position,
//              LCD_BUTTONS for to read the buttons.
// The answer is always a single byte.
// Returns the number of bytes read (== 1) or 0 f the slave did not
// acknowledge its address.
static int recv_generic( u8 address )
{
  u8 retval;

  lcd_start();
  if( i2c_recv( address, &retval, 1, true ) < 0 ) {
    // The address was not acknowledged, so no slave is present.
    // There is no way to signal this to the Lua layer, so return a
    // harmless value (meaning no buttons pressed or cursor at (1,1)).
    retval = 0;
  }
  lcd_stop();

  return retval;
}

// Send a command byte
static int send_command( const u8 command )
{
  return send_generic( LCD_CMD, &command, 1 );
}

// Send data bytes
// This is used for printing data and for programming the user-defining chars
static int send_data( const u8 *data, int len )
{
  return send_generic( LCD_DATA, data, len );
}

// Return the current value of the address counter.
static u8 recv_address_counter()
{
  return recv_generic( LCD_GETPOS );
}

// Return the current state of the buttons, a bit mask in the bottom 5 bits
// of a byte.
static u8 recv_buttons()
{
  return recv_generic( LCD_BUTTONS );
}

// Turning the display on can only be achieved by simultaneously specifying the
// cursor type, so we have to remember what type of cursor they last set.
// Similarly, if they have turned the display off then set the cursor, this
// shouldn't turn the display on.

// Power-on setting is no cursor
#define DEFAULT_CURSOR_TYPE   LCD_CMD_CURSOR_NONE

static u8 cursor_type = DEFAULT_CURSOR_TYPE;
static u8 display_is_off = 0;     // Have they called display("off")?

// Helper function to set a cursor type if the display is on,
// or to remember which cursor they asked for, to be able to set it
// when they turn the display on.
static int set_cursor( u8 command_byte )
{
  cursor_type = command_byte;

  // Setting cursor type always turns the display on
  if ( display_is_off )
    return 0;
  else
    return send_command( cursor_type );
}

// ****************************************************************************
// LCD display module for picoLisp.

// (mizar32-lcd-reset) -> Nil
any plisp_lcd_reset(any ex) {
  cursor_type = DEFAULT_CURSOR_TYPE;
  display_is_off = 0;
  
  send_command(LCD_CMD_RESET);
  return Nil;
}

// (mizar32-lcd-setup shift-disp r-to-l) -> Nil
any plisp_lcd_setup(any ex) {
  any x, y;
  unsigned shift_disp, r_to_l;

  x = cdr(ex);
  NeedSym(ex, y = EVAL(car(x)));
  shift_disp = (y == T) ? 1 : 0;

  x = cdr(x);
  NeedSym(ex, y = EVAL(car(x)));
  r_to_l = (y == T) ? 1 : 0;

  send_command(LCD_CMD_ENTRYMODE +
	       shift_disp +
	       (!r_to_l) * 2);
  return Nil;
}

// (mizar32-lcd-clear) -> Nil
// Clear the display, reset its shiftedness and put the cursor at 1,1
any plisp_lcd_clear(any ex) {
  send_command(LCD_CMD_CLEAR);
  return Nil;
}

// (mizar32-lcd-home) -> Nil
// Reset the display's shiftedness and put the cursor at 1,1
any plisp_lcd_home(any ex) {
  send_command(LCD_CMD_HOME);
  return Nil;
}

// (mizar32-lcd-goto row col) -> Nil
// Move the cursor to the specified row (1 or 2) and
// column (1-40) in the character memory.
any plisp_lcd_goto(any ex) {
  any x, y;
  unsigned row, col, address;
  
  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  row = (unsigned)unBox(y);
  
  x = cdr(x);
  NeedNum(ex, y = EVAL(car(x)));
  col = (unsigned)unBox(y);
  
  if (row < 1 || row > 2 || col < 1 || col > 40)
    err(NULL, ex, "row/column must be 1-2 and 1-40");
  
  address = (row - 1) * 0x40 + (col - 1);
  send_command((u8) (LCD_CMD_DDADDR + address));
    
  return Nil;
}

/***
static void outString_lcd(char *s) {
  while (*s)
    send_data((const u8 *)s++, 1);
}

static void outNum_lcd(long n) {
  char buf[BITS/2];

  bufNum(buf, n);
  outString_lcd(buf);
} ***/

// Helpers for picoLisp LCD print function.
static void plisp_lcdh_prin(any x) {
  if (!isNil(x)) {
    if (isNum(x)) {
      u8 byte = (u8)unBox(x);
      send_data(&byte, 1);
      // outNum_lcd(unBox(x));
    }
    else if (isSym(x)) {
      int i, c;
      word w;
      u8 byte;

      for (x = name(x), c = getByte1(&i, &w, &x); c; c = getByte(&i, &w, &x)) {
	if (c != '^') {
	  byte = c;
	  send_data(&byte, 1);
	}
	else if (!(c = getByte(&i, &w, &x))) {
	  byte = '^';
	  send_data(&byte, 1);
	} 
	else if (c == '?') {
	  byte = 127;
	  send_data(&byte, 1);
	}
	else {
	  c &= 0x1F;
	  byte = (u8)c;
	  send_data(&byte, 1);
	}
      }
    }
    else {
      while (plisp_lcdh_prin(car(x)), !isNil(x = cdr(x))) {
	if (!isCell(x)) {
	  plisp_lcdh_prin(x);
	  break;
	}
      }
    }
  }
}

// (mizar32-lcd-prinl 'any ..) -> any
any plisp_lcd_prinl(any x) {
  any y = Nil;

  while (isCell(x = cdr(x)))
    plisp_lcdh_prin(y = EVAL(car(x)));
  return y;
}

// (mizar32-lcd-getpos) -> lst
any plisp_lcd_getpos(any x) {
  u8 addr = recv_address_counter();
  any y;
  cell c1;

  Push(c1, y = cons(box(((addr & 0x40) ? 2 : 1)), Nil));
  Push(c1, y = cons(box(((addr & 0x3F) + 1)), y));

  return Pop(c1);
}

// (mizar32-lcd-buttons) -> sym
any plisp_lcd_buttons(any x) {
  u8 code;                // bit code for buttons held
  char string[6];         // Up to 5 buttons and a \0
  char *stringp = string; // Where to write the next character;
  
  code = recv_buttons();
  if (code & LCD_BUTTON_SELECT) *stringp++ = 'S';
  if (code & LCD_BUTTON_LEFT) *stringp++ = 'L';
  if (code & LCD_BUTTON_RIGHT) *stringp++ = 'R';
  if (code & LCD_BUTTON_UP) *stringp++ = 'U';
  if (code & LCD_BUTTON_DOWN) *stringp++ = 'D';
  *stringp = '\0';

  return mkStr(string);
}

// Perform cursor operations selected by a
// string (a transient symbol in picoLisp)
// parameter.
//
// (mizar32-lcd-cursor 'sym) -> Nil
any plisp_lcd_cursor(any x) {
  static const char const *args[] = {
    "none",
    "block",
    "line",
    "left",
    "right",
    NULL
  };
  
  any y;
  x = cdr(x), y = EVAL(car(x));
  
  if (equal(mkStr(args[0]), y))
    set_cursor(LCD_CMD_CURSOR_NONE);
  else if (equal(mkStr(args[1]), y))
    set_cursor(LCD_CMD_CURSOR_BLOCK);
  else if (equal(mkStr(args[2]), y))
    set_cursor(LCD_CMD_CURSOR_LINE);
  else if (equal(mkStr(args[3]), y))
    set_cursor(LCD_CMD_SHIFT_CURSOR_LEFT);
  else if (equal(mkStr(args[4]), y))
    set_cursor(LCD_CMD_SHIFT_CURSOR_RIGHT);
  else
    err(NULL, y, "invalid cursor argument");

  return Nil;
}

// Perform display operations,
// selected by a string parameter.
//
// (mizar32-lcd-display 'sym) -> Nil
any plisp_lcd_display(any x) {
  static const char const *args[] = {
    "off",
    "on",
    "left",
    "right",
    NULL
  };

  any y;
  x = cdr(x), y = EVAL(car(x));

  if (equal(mkStr(args[0]), y)) {
    display_is_off = 1;
    send_command(LCD_CMD_DISPLAY_OFF);
  }
  else if (equal(mkStr(args[1]), y)) {
    display_is_off = 0;
    send_command(cursor_type);
  }
  else if (equal(mkStr(args[2]), y))
    send_command(LCD_CMD_SHIFT_DISPLAY_LEFT);
  else if (equal(mkStr(args[3]), y))
    send_command(LCD_CMD_SHIFT_DISPLAY_RIGHT);
  else
    err(NULL, x, "invalid display argument");

  return Nil;
}

// (mizar32-lcd-definechar 'num 'lst) -> Nil
// code: 0-7
// glyph: a list of up to 8 numbers with values 0-31.
//        If less than 8 are supplied, the bottom rows are blanked.
//        If more than 8 are supplied, the extra are ignored.
// The current cursor position in the character display RAM is preserved.
any plisp_lcd_definechar(any ex) {
  int code;        // The character code we are defining, 0-7
  size_t datalen;  // The number of elements in the glyph table
  size_t line = 0; // Which line of the char are we defining?
  u8 data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  int old_address; // The coded value for the current cursor position
  any x, y;

  // First parameter: glyph code to define.
  x = cdr(ex);
  NeedNum(ex, y = EVAL(car(x)));
  code = unBox(y);
  if (code < 0 || code > 7)
    err(ex, y, "user-defined characters have codes 0-7");

  // Second parameter: list of integer values to define the glyph
  x = cdr(x);
  NeedLst(ex, y = EVAL(car(x)));
  datalen = length(y);

  // Check all parameters before starting the I2C command.
  if (datalen >= 8) datalen = 8; // Ignore extra parameters
  for (line = 0; line < datalen; line++) {
    NeedNum(y, car(y));
    data[line] = unBox(car(y));
    y = EVAL(cdr(y));
  }

  old_address = recv_address_counter();
  send_command(LCD_CMD_CGADDR + code * 8);
  send_data(data, sizeof(data));

  // Move back to where we were
  send_command(LCD_CMD_DDADDR + old_address);
  return Nil;
}
