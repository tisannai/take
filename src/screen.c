/**
 * @file screen.c
 *
 * Screen manipulation routines.
 *
 */


#include "mc.h"
#include "global.h"
#include "screen.h"
#include "config.h"

#ifdef USE_TERMBOX
# include <termbox.h>
#else
# include <ncurses.h>
#endif


/** Macro to index screen buffer array. */
#define BUFI(x,y) ((y*(si->x_size))+x)

/** Macro to index screen buffer array through window. */
#define BUFWI(wi,x,y) (((y + wi->si->y_min + wi->y_min)*(wi->si->x_size)) + ( x + wi->si->x_min + wi->x_min ))

/** Convert exotic characters to space. */
#define SIMPLE_CHAR(c) (screen_char_map[(uchar)c].type == text ? c : ' ')



/** Allow assertions for critical routines. */
#ifdef ml_do_debug
# define ASSERT 1
#endif


/** Global variable for screen. */
screen_info* si;


/** List of all active win_info. */
ll* winlist = NULL;


void* screen_win_resize_context = NULL;
screen_callback screen_pre_win_resize = NULL;
screen_callback screen_post_win_resize = NULL;
screen_fatal_callback screen_fatal_error = NULL;

int screen_status_line = -1;


/**
 * Global handle to currently used character map.
 */
screen_char_def_t* screen_char_map;


screen_char_def_t screen_default_char_map[] = {
  {code,"x00"},
  {code,"x01"},
  {code,"x02"},
  {code,"x03"},
  {code,"x04"},
  {code,"x05"},
  {code,"x06"},
  {code,"x07"},
  {code,"x08"},
  {tab,"x09"},
  {newline,"x0A"},
  {code,"x0B"},
  {code,"x0C"},
  {code,"x0D"},
  {code,"x0E"},
  {code,"x0F"},
  {code,"x10"},
  {code,"x11"},
  {code,"x12"},
  {code,"x13"},
  {code,"x14"},
  {code,"x15"},
  {code,"x16"},
  {code,"x17"},
  {code,"x18"},
  {code,"x19"},
  {code,"x1A"},
  {code,"x1B"},
  {code,"x1C"},
  {code,"x1D"},
  {code,"x1E"},
  {code,"x1F"},
  {text," "},
  {text,"!"},
  {text,"\""},
  {text,"#"},
  {text,"$"},
  {text,"%"},
  {text,"&"},
  {text,"'"},
  {text,"("},
  {text,")"},
  {text,"*"},
  {text,"+"},
  {text,","},
  {text,"-"},
  {text,"."},
  {text,"/"},
  {text,"0"},
  {text,"1"},
  {text,"2"},
  {text,"3"},
  {text,"4"},
  {text,"5"},
  {text,"6"},
  {text,"7"},
  {text,"8"},
  {text,"9"},
  {text,":"},
  {text,";"},
  {text,"<"},
  {text,"="},
  {text,">"},
  {text,"?"},
  {text,"@"},
  {text,"A"},
  {text,"B"},
  {text,"C"},
  {text,"D"},
  {text,"E"},
  {text,"F"},
  {text,"G"},
  {text,"H"},
  {text,"I"},
  {text,"J"},
  {text,"K"},
  {text,"L"},
  {text,"M"},
  {text,"N"},
  {text,"O"},
  {text,"P"},
  {text,"Q"},
  {text,"R"},
  {text,"S"},
  {text,"T"},
  {text,"U"},
  {text,"V"},
  {text,"W"},
  {text,"X"},
  {text,"Y"},
  {text,"Z"},
  {text,"["},
  {text,"\\"},
  {text,"]"},
  {text,"^"},
  {text,"_"},
  {text,"`"},
  {text,"a"},
  {text,"b"},
  {text,"c"},
  {text,"d"},
  {text,"e"},
  {text,"f"},
  {text,"g"},
  {text,"h"},
  {text,"i"},
  {text,"j"},
  {text,"k"},
  {text,"l"},
  {text,"m"},
  {text,"n"},
  {text,"o"},
  {text,"p"},
  {text,"q"},
  {text,"r"},
  {text,"s"},
  {text,"t"},
  {text,"u"},
  {text,"v"},
  {text,"w"},
  {text,"x"},
  {text,"y"},
  {text,"z"},
  {text,"{"},
  {text,"|"},
  {text,"}"},
  {text,"~"},
  {code,"x7F"},
  {code,"x80"},
  {code,"x81"},
  {code,"x82"},
  {code,"x83"},
  {code,"x84"},
  {code,"x85"},
  {code,"x86"},
  {code,"x87"},
  {code,"x88"},
  {code,"x89"},
  {code,"x8A"},
  {code,"x8B"},
  {code,"x8C"},
  {code,"x8D"},
  {code,"x8E"},
  {code,"x8F"},
  {code,"x90"},
  {code,"x91"},
  {code,"x92"},
  {code,"x93"},
  {code,"x94"},
  {code,"x95"},
  {code,"x96"},
  {code,"x97"},
  {code,"x98"},
  {code,"x99"},
  {code,"x9A"},
  {code,"x9B"},
  {code,"x9C"},
  {code,"x9D"},
  {code,"x9E"},
  {code,"x9F"},
  {code,"xA0"},
  {code,"xA1"},
  {code,"xA2"},
  {code,"xA3"},
  {code,"xA4"},
  {code,"xA5"},
  {code,"xA6"},
  {code,"xA7"},
  {code,"xA8"},
  {code,"xA9"},
  {code,"xAA"},
  {code,"xAB"},
  {code,"xAC"},
  {code,"xAD"},
  {code,"xAE"},
  {code,"xAF"},
  {code,"xB0"},
  {code,"xB1"},
  {code,"xB2"},
  {code,"xB3"},
  {code,"xB4"},
  {code,"xB5"},
  {code,"xB6"},
  {code,"xB7"},
  {code,"xB8"},
  {code,"xB9"},
  {code,"xBA"},
  {code,"xBB"},
  {code,"xBC"},
  {code,"xBD"},
  {code,"xBE"},
  {code,"xBF"},
  {code,"xC0"},
  {code,"xC1"},
  {code,"xC2"},
  {code,"xC3"},
  {code,"xC4"},
  {code,"xC5"},
  {code,"xC6"},
  {code,"xC7"},
  {code,"xC8"},
  {code,"xC9"},
  {code,"xCA"},
  {code,"xCB"},
  {code,"xCC"},
  {code,"xCD"},
  {code,"xCE"},
  {code,"xCF"},
  {code,"xD0"},
  {code,"xD1"},
  {code,"xD2"},
  {code,"xD3"},
  {code,"xD4"},
  {code,"xD5"},
  {code,"xD6"},
  {code,"xD7"},
  {code,"xD8"},
  {code,"xD9"},
  {code,"xDA"},
  {code,"xDB"},
  {code,"xDC"},
  {code,"xDD"},
  {code,"xDE"},
  {code,"xDF"},
  {code,"xE0"},
  {code,"xE1"},
  {code,"xE2"},
  {code,"xE3"},
  {code,"xE4"},
  {code,"xE5"},
  {code,"xE6"},
  {code,"xE7"},
  {code,"xE8"},
  {code,"xE9"},
  {code,"xEA"},
  {code,"xEB"},
  {code,"xEC"},
  {code,"xED"},
  {code,"xEE"},
  {code,"xEF"},
  {code,"xF0"},
  {code,"xF1"},
  {code,"xF2"},
  {code,"xF3"},
  {code,"xF4"},
  {code,"xF5"},
  {code,"xF6"},
  {code,"xF7"},
  {code,"xF8"},
  {code,"xF9"},
  {code,"xFA"},
  {code,"xFB"},
  {code,"xFC"},
  {code,"xFD"},
  {code,"xFE"},
  {code,"xFF"},
};




static int scr_default_color = SCR_COLOR_DEFAULT;


#ifdef USE_TERMBOX

/* Termbox is able to utilize many colors. */

/** Fore and background colors. */
typedef struct fg_bg_color_s {
  uint16_t fg;                  /**< Foreground. */
  uint16_t bg;                  /**< Background. */
} fg_bg_color;


/** Color mapping table. */
fg_bg_color scr_color_table[] = {
  { 15, 0  }, /* SCR_COLOR_DEFAULT */
  { 28, 0  }, /* SCR_COLOR_GREEN */
  { 11, 0  }, /* SCR_COLOR_YELLOW */
  { 9,  0  }, /* SCR_COLOR_RED */
  { 23, 0  }, /* SCR_COLOR_BLUE */
  { 68, 0  }, /* SCR_COLOR_BROWN */
  { 0, 0 },
};

#endif



/** Set of functions that interface to either ncurses or termbox. */

#ifdef USE_TERMBOX

/** Initialize terminal. */
void term_init( void )
{
  tb_init();
}

/** Return terminal width (-2). */
int term_width( void )
{
  return tb_width()-2;
}

/** Return terminal height (-2). */
int term_height( void )
{
  return tb_height()-1;
}

/** Return true is terminal has color. */
bool_t term_has_color( void )
{
  return true;
}

/** Setup terminal color mode. */
void term_setup_color( void )
{
  tb_select_output_mode( TB_OUTPUT_256 );
}

/** Close terminal. */
void term_close( void )
{
  tb_shutdown();
}

/** Set default color for terminal. */
void screen_set_default_color( int color )
{
  scr_default_color = color;
}


# else


/** Initialize terminal. */
void term_init( void )
{
  /* Start curses mode. */
  initscr();

  /* Line buffering disabled. */

#ifdef ml_do_debug

  cbreak();

#else

  /* raw is normal setup. */
  raw();

#endif

  /* Don't echo() while we do getch. */
  noecho();
}

/**
 * Color setup.
 * 
 */
void term_setup_color( void )
{
  start_color();
  init_pair( SCR_COLOR_DEFAULT, COLOR_WHITE, COLOR_BLACK );
  init_pair( SCR_COLOR_GREEN, COLOR_BLACK, COLOR_WHITE );
}

/**
 * Return terminal width.
 * 
 * @return Width.
 */
int term_width( void )
{
  return COLS-2;
}

/**
 * Terminal height.
 * 
 * 
 * @return Height.
 */
int term_height( void )
{
  return LINES-1;
}

/**
 * Has color check.
 * 
 * 
 * @return True if has color.
 */
bool_t term_has_color( void )
{
  return ( has_colors() == TRUE );
}

/**
 * Close terminal.
 * 
 */
void term_close( void )
{
  endwin();
}

/** Default color setting. */
void screen_set_default_color( int color ) {}


#endif



/**
 * Open screen and return the screen information.
 *
 *
 * @return Screen information includind dimentions and such. 
*/
/* autoc:c_func_decl:screen_open */
screen_info* screen_open( void )
{
  screen_info* si;


  term_init();

  si = mc_new( screen_info );
  si->buf = NULL;

  screen_update_geom( si );

  if ( si->color )
    {
      term_setup_color();
    }

  screen_char_map = screen_default_char_map;

  return si;
}



/**
 * Update screen geometry figures according to current window size.
 *
 *
 * @param si Handle to screen.
 */
/* autoc:c_func_decl:screen_update_geom */
void screen_update_geom( screen_info* si )
{

  /*
   * Physical measures:
   */

  if ( si->buf )
    mc_free( si->buf );

  si->x_min = 0;
  si->y_min = 0;
  /* Avoid bottom-right corner with curses (not COLS-1). */
  si->x_max = term_width();
  si->y_max = term_height();

  si->y_size = si->y_max - si->y_min + 1;
  si->x_size = si->x_max - si->x_min + 1;
  si->size = si->y_size * si->x_size;

  si->buf = mc_new_n(char_info,(si->size));

  /* Ask ncurses if colors are available. */
  if ( term_has_color() )
    {
      si->color = true;
    }
  else
    si->color = false;

  screen_clear_buf( si );
}



/**
 * Close screen and free memory.
 *
 *
 * @param si Handle to screen.
 * @return NULL;
 */
/* autoc:c_func_decl:screen_close */
void* screen_close( screen_info* si )
{
  mc_free( si->buf );
  mc_free( si );
  term_close();
  return NULL;
}




/**
 * Open a new window with geometry (offsets from screen
 * boundaries). If offset is negative, it is taken from the opposite
 * border (compared to positive value) as absolute value minus one,
 * i.e. it is possible to specify 0 offset from opposite border.
 *
 * Window is registered to winlist.
 *
 *
 * @param si Handle to screen.
 * @param x_min_offset Offset from left.
 * @param x_max_offset Offset from right.
 * @param y_min_offset Offset from top.
 * @param y_max_offset Offset from bottom.
 * @param wrapline True if wraplines allowed.
 *
 * @return Initialized window info.
 */
/* autoc:c_func_decl:screen_open_window_geom */
win_info* screen_open_window_geom( screen_info* si,
                                   int x_min_offset,
                                   int x_max_offset,
                                   int y_min_offset,
                                   int y_max_offset,
                                   bool_t wrapline )
{
  win_info* wi;

  wi = mc_new(win_info);
  wi->si = si;

  wi->x_min_offset = x_min_offset;
  wi->x_max_offset = x_max_offset;
  wi->y_min_offset = y_min_offset;
  wi->y_max_offset = y_max_offset;

  wi->wrapline = wrapline;

  screen_init_win( wi );

  winlist = ll_add_data( winlist, wi );

  return wi;
}


/**
 * Initialize window structure after basic setup.
 *
 *
 * @param wi Window info.
 */
/* autoc:c_func_decl:screen_update_window_geom */
void screen_update_window_geom( win_info* wi )
{

  if ( wi->x_min_offset >= 0 )
    wi->x_min = wi->x_min_offset;
  else
    wi->x_min = wi->si->x_size + wi->x_min_offset;

  if ( wi->x_max_offset >= 0 )
    wi->x_max = wi->si->x_size - wi->x_max_offset - 1;
  else
    wi->x_max = ( -1 * wi->x_max_offset ) - 1;

  if ( wi->y_min_offset >= 0 )
    wi->y_min = wi->y_min_offset;
  else
    wi->y_min = wi->si->y_size + wi->y_min_offset;

  if ( wi->y_max_offset >= 0 )
    wi->y_max = wi->si->y_size - wi->y_max_offset - 1;
  else
    wi->y_max = ( -1 * wi->y_max_offset ) - 1;

  assert( wi->x_min <= wi->x_max );
  assert( wi->y_min <= wi->y_max );

}


/**
 * Initialize window structure after basic setup.
 *
 *
 * @param wi Window info.
 */
/* autoc:c_func_decl:screen_init_win */
void screen_init_win( win_info* wi )
{

  screen_update_window_geom( wi );

  wi->x = 0;
  wi->y = 0;

  wi->refresh = true;
  wi->content = NULL;

  screen_clear_win( wi );
}


/**
 * Deallocate win_info memory.
 *
 *
 * @param wi Window info.
 */
/* autoc:c_func_decl:screen_close_window */
void screen_close_window( win_info* wi )
{
  ll* win_node;

  win_node = ll_find_data( ll_begin( winlist ), wi );
  if ( win_node )
    ll_delete_and_update( win_node, &winlist );

  mc_free( wi );
}


/**
 * Return list of active windows.
 *
 * @return Window list.
 */
ll* screen_winlist( void ) /*acfd*/
{
  return winlist;
}


/**
 * Clear screen buffer.
 *
 *
 * @param si Screen info.
 */
/* autoc:c_func_decl:screen_clear_buf */
void screen_clear_buf( screen_info* si )
{
  int i;
  for ( i = 0; i < si->size; i++ )
    {
      si->buf[ i ].ch = 0;
      si->buf[ i ].color = SCR_COLOR_DEFAULT;
    }
}


/**
 * Clear window area.
 *
 *
 * @param wi Window info.
 */
/* autoc:c_func_decl:screen_clear_win */
void screen_clear_win( win_info* wi )
{
  int x, y;
  int x_limit;

  /* Always clear one extra. */
  x_limit = WI_X_SIZE(wi)+1;

  for ( y = 0; y < WI_Y_SIZE(wi); y++ )
    {
      for ( x = 0; x < x_limit; x++ )
        {
          wi->si->buf[ BUFWI(wi,x,y) ].ch = 0;
          wi->si->buf[ BUFWI(wi,x,y) ].color = SCR_COLOR_DEFAULT;
        }
    }
}


/**
 * Clear line in window area.
 *
 *
 * @param wi Window info.
 */
/* autoc:c_func_decl:screen_clear_line */
void screen_clear_line( win_info* wi )
{
  int x;
  int x_limit;

  /* Always clear one extra. */
  x_limit = WI_X_SIZE(wi)+1;

  for ( x = 0; x < x_limit; x++ )
    {
      wi->si->buf[ BUFWI(wi,x,wi->y) ].ch = 0;
      wi->si->buf[ BUFWI(wi,x,wi->y) ].color = SCR_COLOR_DEFAULT;
    }
}


/**
 * Both x and y inside edit area.
 *
 *
 * @param wi Window info to check.
 */
/* autoc:c_func_decl:screen_inside_win */
bool_t screen_inside_win( win_info* wi )
{
  if ( wi->x <= WI_X_MAX(wi)
       && wi->y <= WI_Y_MAX(wi)
       && wi->x >= WI_X_MIN(wi)
       && wi->y >= WI_Y_MIN(wi) )
    return true;
  else
    return false;
}


/**
 * At screen bottom limit?
 *
 *
 * @param wi Window info.
 *
 * @return True if at window bottom.
 */
/* autoc:c_func_decl:screen_at_win_y_end */
bool_t screen_at_win_y_end( win_info* wi )
{
  if ( wi->y == WI_Y_MAX(wi) )
    return true;
#ifdef ASSERT
  else if ( wi->y > WI_Y_MAX(wi) )
    {
      if ( screen_fatal_error )
        screen_fatal_error( "screen_at_win_y_end" );
      return false;
    }
#endif
  else
    return false;
}


/**
 * At screen up limit?
 *
 *
 * @param wi Window info.
 *
 * @return True if at window bottom.
 */
/* autoc:c_func_decl:screen_at_win_y_start */
bool_t screen_at_win_y_start( win_info* wi )
{
  if ( wi->y == WI_Y_MIN(wi) )
    return true;
#ifdef ASSERT
  else if ( wi->y < WI_Y_MIN(wi) )
    {
      if ( screen_fatal_error )
        screen_fatal_error( "screen_at_win_y_start" );
      return false;
    }
#endif
  else
    return false;
}


/**
 * Both x and y inside screen area.
 *
 *
 * @param si Screen info.
 * @param x X coord.
 * @param y Y coord.
 *
 * @return True if inside.
 */
/* autoc:c_func_decl:screen_inside */
bool_t screen_inside( screen_info* si, int x, int y )
{
  if ( x <= si->x_max
       && y <= si->y_max
       && x >= si->x_min
       && y >= si->y_min )
    return true;
  else
    return false;
}


/**
 * Set position in the screen. Return true if operation is successful.
 *
 *
 * @param wi Window info.
 * @param x X coord.
 * @param y Y coord.
 *
 * @return True if inside.
 */
/* autoc:c_func_decl:screen_setpos */
bool_t screen_setpos( win_info* wi, int x, int y )
{
  win_info twi;

  twi = *wi;
  twi.x = x;
  twi.y = y;

  if ( screen_inside_win( &twi ) )
    {
      wi->x = x;
      wi->y = y;
      return true;
    }
  else
    {
      dbug( "screen_setpos: outside x %d, y %d\n", twi.x, twi.y );
      return false;
    }
}


/**
 * Write string to window (screen) to current position and return
 * strlen. Overflowing characters will not be displayed. Window
 * position does not change.
 *
 *
 * @param wi Window info.
 * @param str String to add.
 *
 * @return Number of added chars.
 */
/* autoc:c_func_decl:screen_set_str2 */
int screen_set_str2( win_info* wi, const char* str )
{
  int len = strlen( str );

  /* Write all chars to off-screen buffer unless they are
     overflowing. */
  for ( int i = 0; i < len; i++ )
    {
      if ( (wi->x+i) <= wi->x_max+1 )
        {
          wi->si->buf[ BUFWI(wi,wi->x+i,wi->y) ].ch = SIMPLE_CHAR( str[ i ] );
          wi->si->buf[ BUFWI(wi,wi->x+i,wi->y) ].color = scr_default_color;
        }
    }

  return len;
}


#ifdef USE_TERMBOX
/**
 * Write string to window (screen) with color to current position and
 * return strlen. Overflowing characters will not be displayed. Window
 * position does not change.
 *
 *
 * @param wi Window info.
 * @param str String to add.
 * @param color Used color.
 *
 * @return Number of added chars.
 */
/* autoc:c_func_decl:screen_set_str2 */
int screen_set_color_str( win_info* wi, char* str, int color )
{
  int len = strlen( str );

  /* Write all chars to off-screen buffer unless they are
     overflowing. */
  for ( int i = 0; i < len; i++ )
    {
      if ( (wi->x+i) <= wi->x_max+1 )
        {
          wi->si->buf[ BUFWI(wi,wi->x+i,wi->y) ].ch = SIMPLE_CHAR( str[ i ] );
          wi->si->buf[ BUFWI(wi,wi->x+i,wi->y) ].color = color;
        }
    }

  return len;
}
#endif


/**
 * Dump offline buffer to the screen.
 *
 *
 * @param si Screen info.
 */
/* autoc:c_func_decl:screen_dump */
void screen_dump( screen_info* si )
{

#ifdef USE_TERMBOX

  uint16_t fg, bg;

  for ( int y = 0; y < si->y_size; y++ )
    {
      for ( int x = 0; x < si->x_size; x++ )
        {
          if ( si->color )
            {
              fg = scr_color_table[ si->buf[ BUFI(x,y) ].color ].fg;
              bg = scr_color_table[ si->buf[ BUFI(x,y) ].color ].bg;
            }
          else
            {
              fg = scr_color_table[ SCR_COLOR_DEFAULT ].fg;
              bg = scr_color_table[ SCR_COLOR_DEFAULT ].bg;
            }
          tb_change_cell( x, y, si->buf[ BUFI(x,y) ].ch, fg, bg );
        }
    }

# else

  int y;
  char tmpstr[ 1024 ];

  for ( y = 0; y < si->y_size; y++ )
    {
      if ( si->color && ( y == screen_status_line ) )
        attron( COLOR_PAIR( SCR_COLOR_GREEN ) );

      for ( int x = 0; x < si->x_size; x++ )
        tmpstr[ x ] = si->buf[ BUFI(x,y) ].ch;

      mvaddnstr( y, 0, tmpstr, si->x_size );

      if ( si->color && ( y == screen_status_line ) )
        attroff( COLOR_PAIR( SCR_COLOR_GREEN ) );
    }

#endif

}


/**
 * Refresh the current view.
 *
 *
 * @param wi Window info.
 */
/* autoc:c_func_decl:screen_refresh */
void screen_refresh( win_info* wi )
{
  if ( wi->refresh )
    {

#ifdef USE_TERMBOX

      tb_clear();
      screen_dump( wi->si );
      tb_set_cursor( wi->si->x_min + wi->x_min + wi->x,
                     wi->si->y_min + wi->y_min + wi->y );
      tb_present();

# else

      erase();
      screen_dump( wi->si );
      dbug( "screen_refresh: x %d, y %d\n", wi->x, wi->y );
      move( wi->si->y_min + wi->y_min + wi->y,
            wi->si->x_min + wi->x_min + wi->x );
      refresh();

#endif

    }
}


/**
 * Return a key press.
 *
 *
 * @return Key value.
 */
/* autoc:c_func_decl:screen_get_key */
int screen_get_key( void )
{

#ifdef USE_TERMBOX

  int key;
  struct tb_event event;
  for (;;)
    {
      tb_poll_event( &event );

      if ( event.type == TB_EVENT_RESIZE )
        {
          /* Termbox has detected window resizing. */
          if ( screen_pre_win_resize )
            screen_pre_win_resize( screen_win_resize_context );

          tb_clear();
          screen_update_geom( si );

          /* Update all living window geometries. */
          for ( ll* wl = ll_start( winlist ); wl; wl = ll_next( wl ) )
            {
              screen_update_window_geom( ( win_info* ) wl->data );
            }

          if ( screen_post_win_resize )
            screen_post_win_resize( screen_win_resize_context );

        }
      else
        {
          break;
        }
    }

  if ( event.key == 0 )
    {
      /* Normal character, i.e. ch has valid value. */
      key = event.ch;
    }
  else
    {
      /* Control character, i.e. key has valid value. */
      if ( event.key == TB_KEY_ENTER )
        /* Map Enter to CTRL_J. */
        key = NEWLINE;
      else
        key = event.key;
    }

  if ( key >= 32 && key <= 126 )
    {
      dbug( "key: %d (\"%c\")\n", key, (char) key );
    }
  else
    {
      dbug( "key: %d\n", key );
    }

  return key;

#else

  int key;

  for (;;)
    {
      key = getch();

      if ( key == KEY_RESIZE )
        {

          /* Curses has detected window resising. */
          if ( screen_pre_win_resize )
            screen_pre_win_resize( screen_win_resize_context );

          clear();
          refresh();
          screen_update_geom( si );

          /* Update all living window geometries. */
          for ( ll* wl = ll_start( winlist ); wl; wl = ll_next( wl ) )
            {
              screen_update_window_geom( ( win_info* ) wl->data );
            }

          if ( screen_post_win_resize )
            screen_post_win_resize( screen_win_resize_context );

        }
      else
        {
          break;
        }
    }

  if ( key >= 32 && key <= 126 )
    {
      dbug( "key: %d (\"%c\")\n", key, (char) key );
    }
  else
    {
      dbug( "key: %d\n", key );
    }

  return key;

#endif

}


/**
 * Update status line with str.
 *
 *
 * @param str Status line content.
 */
/* autoc:c_func_decl:screen_set_status */
void screen_set_status( char* str )
{
  for ( int i = 0; i < strlen(str); i++ )
    {
      si->buf[ BUFI(0+i,screen_status_line) ].ch = str[ i ];
      si->buf[ BUFI(0+i,screen_status_line) ].color = SCR_COLOR_GREEN;
    }
}



#ifdef USE_TERMBOX

/**
 * Update status line with str.
 *
 *
 * @param str Status line content.
 * @param len Status line length.
 */
void screen_set_status_with_color( char_info* str, int len )
{
  for ( int i = 0; i < len; i++ )
    {
      si->buf[ BUFI(0+i,screen_status_line) ].ch = str[ i ].ch;
      si->buf[ BUFI(0+i,screen_status_line) ].color = str[ i ].color;
    }
}

#endif


/**
 * Return window x-size.
 */
int screen_win_x_size( win_info* wi ) /*acfd*/
{
  return WI_X_SIZE(wi);
}


/**
 * Return window y-size.
 */
int screen_win_y_size( win_info* wi ) /*acfd*/
{
  return WI_Y_SIZE(wi);
}


/**
 * Return window x-min.
 */
int screen_win_x_min( win_info* wi ) /*acfd*/
{
  return WI_X_MIN(wi);
}


/**
 * Return window y-min.
 */
int screen_win_y_min( win_info* wi ) /*acfd*/
{
  return WI_Y_MIN(wi);
}


/**
 * Return window x-max.
 */
int screen_win_x_max( win_info* wi ) /*acfd*/
{
  return WI_X_MAX(wi);
}


/**
 * Return window y-max.
 */
int screen_win_y_max( win_info* wi ) /*acfd*/
{
  return WI_Y_MAX(wi);
}
