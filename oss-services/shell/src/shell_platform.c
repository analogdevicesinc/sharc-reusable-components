#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#include "shell.h"
#include "shell_platform.h"
#include "term.h"

static int term_translate( int data, void *usr )
{
  static int escape = 0;
  static int escape_char = 0;

  if (escape)
  {
   switch (escape)
   {
      case 1:
         if (data == '[')
            escape = 2;
         else if (data == 0x1B) {
            escape = 0;
            return(KC_ESC);
         }
         else
            escape = 0;
         break;
      case 2:
         if( data >= 'A' && data <= 'D' )
         {
            escape = 0;
            switch( data )
            {
               case 'A':
                  return KC_UP;
               case 'B':
                  return KC_DOWN;
               case 'C':
                  return KC_RIGHT;
               case 'D':
                  return KC_LEFT;
            }
         }
         else if( data > '0' && data < '7' )
         {
            escape_char = data;
            escape = 3;
         }
         break;
      case 3:
         escape = 0;
         if (data == '~')
         {
            switch( escape_char )
            {
               case '1':
                  return KC_HOME;
               case '4':
                  return KC_END;
               case '5':
                  return KC_PAGEUP;
               case '6':
                  return KC_PAGEDOWN;
            }
         }
         break;
      default:
         break;
   }
   return KC_UNKNOWN;
  }
  else if( isprint( data ) )
    return data;
  else if( data == 0x1B ) // escape sequence
  {
     escape = 1;
  }
  else if( data == 0x0D )
  {
    return KC_ENTER;
  }
  else
  {
    switch( data )
    {
      case 0x09:
        return KC_TAB;
      case 0x7F:
        //return KC_DEL;
        return KC_BACKSPACE;
      case 0x08:
        return KC_BACKSPACE;
      case 26:
        return KC_CTRL_Z;
      case 1:
        return KC_CTRL_A;
      case 5:
        return KC_CTRL_E;
      case 3:
        return KC_CTRL_C;
      case 20:
        return KC_CTRL_T;
      case 21:
        return KC_CTRL_U;
      case 11:
        return KC_CTRL_K;
    }
  }
  return KC_UNKNOWN;
}

void shell_platform_init(SHELL_CONTEXT *ctx, p_term_out term_out, p_term_in term_in)
{
    term_init(&ctx->t, SHELL_LINES, SHELL_COLUMNS, term_out, term_in, term_translate, ctx->usr);
}

void shell_platform_deinit(SHELL_CONTEXT *ctx)
{
    term_deinit(&ctx->t);
}
