// Terminal function
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "shell.h"
#include "shell_platform.h"
#include "term.h"

// Maximum size on an ANSI sequence
#define TERM_MAX_ANSI_SIZE        14

// *****************************************************************************
// Terminal functions

static void term_ansi( TERM_STATE *t, const char* fmt, ... )
{
  char seq[ TERM_MAX_ANSI_SIZE + 1 ];
  va_list ap;

  seq[ TERM_MAX_ANSI_SIZE ] = '\0';
  seq[ 0 ] = '\x1B';
  seq[ 1 ] = '[';
  va_start( ap, fmt );
  vsnprintf( seq + 2, TERM_MAX_ANSI_SIZE - 2, fmt, ap );
  va_end( ap );
  term_putstr( t, seq, strlen( seq ) );
}

// Clear the screen
void term_clrscr(TERM_STATE *t)
{
  term_ansi( t, "2J" );
  t->term_cx = t->term_cy = 0;
}

// Clear to end of line
void term_clreol(TERM_STATE *t)
{
  term_ansi( t, "K" );
}

// Move cursor to (x, y)
void term_gotoxy( TERM_STATE *t, unsigned x, unsigned y )
{
  term_ansi( t, "%u;%uH", y, x );
  t->term_cx = x;
  t->term_cy = y;
}

// Move cursor up "delta" lines
void term_up( TERM_STATE *t, unsigned delta )
{
  term_ansi( t, "%uA", delta );
  t->term_cy -= delta;
}

// Move cursor down "delta" lines
void term_down( TERM_STATE *t, unsigned delta )
{
  term_ansi( t, "%uB", delta );
  t->term_cy += delta;
}

// Move cursor right "delta" chars
void term_right( TERM_STATE *t, unsigned delta )
{
  term_ansi( t, "%uC", delta );
  t->term_cx -= delta;
}

// Move cursor left "delta" chars
void term_left( TERM_STATE *t, unsigned delta )
{
  term_ansi( t, "%uD", delta );
  t->term_cx += delta;
}

// Return the number of terminal lines
unsigned term_get_lines(TERM_STATE *t)
{
  return t->term_num_lines;
}

// Return the number of terminal columns
unsigned term_get_cols(TERM_STATE *t)
{
  return t->term_num_cols;
}

// Write a character to the terminal
void term_putch( TERM_STATE *t, char ch )
{
  if( ch == '\n' )
  {
    if( t->term_cy < t->term_num_lines )
      t->term_cy ++;
    t->term_cx = 0;
  }
  t->term_out( ch, t->usr );
}

// Write a string to the terminal
void term_putstr( TERM_STATE *t, const char* str, unsigned size )
{
  while( size )
  {
    t->term_out( *str ++, t->usr );
    size --;
  }
}

// Write a string of

// Return the cursor "x" position
unsigned term_get_cx(TERM_STATE *t)
{
  return t->term_cx;
}

// Return the cursor "y" position
unsigned term_get_cy(TERM_STATE *t)
{
  return t->term_cy;
}

// Return a char read from the terminal
// If "mode" is TERM_INPUT_DONT_WAIT, return the char only if it is available,
// otherwise return -1
// Calls the translate function to translate the terminal's physical key codes
// to logical key codes (defined in the term.h header)
int term_getch( TERM_STATE *t, int mode )
{
  int ch;

  if( ( ch = t->term_in( mode, t->usr ) ) == -1 )
    return -1;
  else
    return t->term_translate( ch, t->usr );
}

void term_init( TERM_STATE *t, unsigned lines, unsigned cols, p_term_out term_out_func,
                p_term_in term_in_func, p_term_translate term_translate_func,
                void *usr )
{
  t->term_num_lines = lines;
  t->term_num_cols = cols;
  t->term_out = term_out_func;
  t->term_in = term_in_func;
  t->term_translate = term_translate_func;
  t->usr = usr;
  t->term_cx = t->term_cy = 0;
}

void term_deinit( TERM_STATE *t )
{
}
