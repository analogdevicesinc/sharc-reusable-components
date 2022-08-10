// Terminal functions

#ifndef __TERM_H__
#define __TERM_H__

// ****************************************************************************
// Data types

// Terminal output function
typedef void ( *p_term_out )( char, void * );
// Terminal input function
typedef int ( *p_term_in )( int, void * );
// Terminal translate input function
typedef int ( *p_term_translate )( int, void * );

// Terminal input mode (parameter of p_term_in and term_getch())
#define TERM_INPUT_DONT_WAIT      0
#define TERM_INPUT_WAIT           1

typedef struct TERM_STATE {
    p_term_out term_out;             /**< Terminal output function pointer */
    p_term_in term_in;               /**< Terminal input function pointer */
    p_term_translate term_translate; /**< Terminal translate function pointer */
    unsigned term_num_lines;         /**< Terminal lines */
    unsigned term_num_cols;          /**< Terminal column */
    unsigned term_cx;                /**< Terminal current x */
    unsigned term_cy;                /**< Terminal current y */
    void *usr;                       /**< User defined pointer */
} TERM_STATE;

// ****************************************************************************
// Exported functions

// Terminal initialization
void term_init( TERM_STATE *t, unsigned lines, unsigned cols, p_term_out term_out_func,
                p_term_in term_in_func, p_term_translate term_translate_func,
                void *usr );
void term_deinit( TERM_STATE *t );

// Terminal output functions
void term_clrscr(TERM_STATE *t);
void term_clreol(TERM_STATE *t);
void term_gotoxy( TERM_STATE *t, unsigned x, unsigned y );
void term_up( TERM_STATE *t, unsigned delta );
void term_down( TERM_STATE *t, unsigned delta );
void term_left( TERM_STATE *t, unsigned delta );
void term_right( TERM_STATE *t, unsigned delta );
unsigned term_get_lines(TERM_STATE *t);
unsigned term_get_cols(TERM_STATE *t);
void term_putch( TERM_STATE *t, char ch );
void term_putstr( TERM_STATE *t, const char* str, unsigned size );
unsigned term_get_cx(TERM_STATE *t);
unsigned term_get_cy(TERM_STATE *t);
int term_getch( TERM_STATE *t, int mode );

#define TERM_KEYCODES\
  _D( KC_UP ),\
  _D( KC_DOWN ),\
  _D( KC_LEFT ),\
  _D( KC_RIGHT ),\
  _D( KC_HOME ),\
  _D( KC_END ),\
  _D( KC_PAGEUP ),\
  _D( KC_PAGEDOWN ),\
  _D( KC_ENTER ),\
  _D( KC_TAB ),\
  _D( KC_BACKSPACE ),\
  _D( KC_ESC ),\
  _D( KC_CTRL_Z ),\
  _D( KC_CTRL_A ),\
  _D( KC_CTRL_E ),\
  _D( KC_CTRL_C ),\
  _D( KC_CTRL_T ),\
  _D( KC_CTRL_U ),\
  _D( KC_CTRL_K ),\
  _D( KC_DEL ),\
  _D( KC_UNKNOWN )

// Terminal input functions
// Keyboard codes
#define _D( x ) x

enum
{
  term_dummy = 255,
  TERM_KEYCODES,
  TERM_FIRST_KEY = KC_UP,
  TERM_LAST_KEY = KC_UNKNOWN
};

#endif // #ifndef __TERM_H__
