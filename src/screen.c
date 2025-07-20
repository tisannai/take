/**
 * @file screen.c
 *
 * Screen manipulation routines.
 *
 */


#include <plinth.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "screen.h"
#include "config.h"

#include <termbox2.h>



/** Macro to index screen buffer array. */
#define BUFI( x, y ) ( ( y * ( si->x_size ) ) + x )

/** Macro to index screen buffer array through window. */
#define BUFWI( wi, x, y )                                          \
    ( ( ( y + wi->si->y_min + wi->y_min ) * ( wi->si->x_size ) ) + \
      ( x + wi->si->x_min + wi->x_min ) )

/** Convert exotic characters to space. */
#define SIMPLE_CHAR( c ) ( screen_char_map[ (uint8_t)c ].type == text ? c : ' ' )



/** Allow assertions for critical routines. */
#ifdef ml_do_debug
#    define ASSERT 1
#endif


/** Global variable for screen. */
screen_info_t si;


/** List of all active win_info. */
plcm_s winlist;


void*                 screen_win_resize_context = NULL;
screen_callback       screen_pre_win_resize = NULL;
screen_callback       screen_post_win_resize = NULL;
screen_fatal_callback screen_fatal_error = NULL;

int screen_status_line = -1;


/**
 * Global handle to currently used character map.
 */
screen_char_def_t screen_char_map;


/* clang-format off */

screen_char_def_s screen_default_char_map[] = {
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

/* clang-format on */



static int scr_default_color = SCR_COLOR_DEFAULT;


/* Termbox is able to utilize many colors. */

/** Fore and background colors. */
pl_struct( fg_bg_color )
{
    uint16_t fg; /**< Foreground. */
    uint16_t bg; /**< Background. */
};


/** Color mapping table. */
fg_bg_color_s scr_color_table[] = {
    { 15, 0 }, /* SCR_COLOR_DEFAULT */
    { 28, 0 }, /* SCR_COLOR_GREEN */
    { 11, 0 }, /* SCR_COLOR_YELLOW */
    { 9, 0 },  /* SCR_COLOR_RED */
    { 23, 0 }, /* SCR_COLOR_BLUE */
    { 68, 0 }, /* SCR_COLOR_BROWN */
    { 0, 0 },
};



/** Initialize terminal. */
void term_init( void )
{
    tb_init();
}


/** Return terminal width (-2). */
int term_width( void )
{
    return tb_width() - 2;
}


/** Return terminal height (-2). */
int term_height( void )
{
    return tb_height() - 1;
}


/** Return true is terminal has color. */
pl_bool_t term_has_color( void )
{
    return pl_true;
}


/** Setup terminal color mode. */
void term_setup_color( void )
{
    tb_set_output_mode( TB_OUTPUT_256 );
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


/**
 * Open screen and return the screen information.
 *
 *
 * @return Screen information including dimentions and such.
*/
screen_info_t screen_open( void )
{
    screen_info_t si;

    term_init();

    plcm_new( &winlist, 16 );

    si = pl_alloc_memory_for_type( screen_info_s );
    plcm_empty( &si->bufmem, 0 );
    si->buf = NULL;

    screen_update_geom( si );

    if ( si->color ) {
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
void screen_update_geom( screen_info_t si )
{
    /*
     * Physical measures:
     */

    si->x_min = 0;
    si->y_min = 0;
    /* Avoid bottom-right corner with curses (not COLS-1). */
    si->x_max = term_width();
    si->y_max = term_height();

    si->y_size = si->y_max - si->y_min + 1;
    si->x_size = si->x_max - si->x_min + 1;
    si->size = si->y_size * si->x_size;

    plcm_resize( &si->bufmem, si->size * sizeof( char_info_s ) );
    si->buf = plcm_data( &si->bufmem );

    if ( term_has_color() ) {
        si->color = pl_true;
    } else {
        si->color = pl_false;
    }

    screen_clear_buf( si );
}


/**
 * Close screen and free memory.
 *
 *
 * @param si Handle to screen.
 * @return NULL;
 */
void* screen_close( screen_info_t si )
{
    plcm_del( &winlist );
    plcm_del( &si->bufmem );
    pl_free_memory( si );
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
win_info_t screen_open_window_geom( screen_info_t si,
                                    int           x_min_offset,
                                    int           x_max_offset,
                                    int           y_min_offset,
                                    int           y_max_offset,
                                    pl_bool_t     wrapline )
{
    win_info_t wi;

    wi = pl_alloc_memory_for_type( win_info_s );
    wi->si = si;

    wi->x_min_offset = x_min_offset;
    wi->x_max_offset = x_max_offset;
    wi->y_min_offset = y_min_offset;
    wi->y_max_offset = y_max_offset;

    wi->wrapline = wrapline;

    screen_init_win( wi );

    plcm_store_for_type( &winlist, &wi, win_info_t );

    return wi;
}


/**
 * Initialize window structure after basic setup.
 *
 *
 * @param wi Window info.
 */
void screen_update_window_geom( win_info_t wi )
{

    if ( wi->x_min_offset >= 0 ) {
        wi->x_min = wi->x_min_offset;
    } else {
        wi->x_min = wi->si->x_size + wi->x_min_offset;
    }

    if ( wi->x_max_offset >= 0 ) {
        wi->x_max = wi->si->x_size - wi->x_max_offset - 1;
    } else {
        wi->x_max = ( -1 * wi->x_max_offset ) - 1;
    }

    if ( wi->y_min_offset >= 0 ) {
        wi->y_min = wi->y_min_offset;
    } else {
        wi->y_min = wi->si->y_size + wi->y_min_offset;
    }

    if ( wi->y_max_offset >= 0 ) {
        wi->y_max = wi->si->y_size - wi->y_max_offset - 1;
    } else {
        wi->y_max = ( -1 * wi->y_max_offset ) - 1;
    }

    assert( wi->x_min <= wi->x_max );
    assert( wi->y_min <= wi->y_max );
}


/**
 * Initialize window structure after basic setup.
 *
 *
 * @param wi Window info.
 */
void screen_init_win( win_info_t wi )
{

    screen_update_window_geom( wi );

    wi->x = 0;
    wi->y = 0;

    wi->refresh = pl_true;
    wi->content = NULL;

    screen_clear_win( wi );
}


/**
 * Deallocate win_info memory.
 *
 *
 * @param wi Window info.
 */
void screen_close_window( win_info_t wi )
{
    pl_free_memory( wi );
}


/**
 * Clear screen buffer.
 *
 *
 * @param si Screen info.
 */
void screen_clear_buf( screen_info_t si )
{
    int i;
    for ( i = 0; i < si->size; i++ ) {
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
void screen_clear_win( win_info_t wi )
{
    int x, y;
    int x_limit;

    /* Always clear one extra. */
    x_limit = WI_X_SIZE( wi ) + 1;

    for ( y = 0; y < WI_Y_SIZE( wi ); y++ ) {
        for ( x = 0; x < x_limit; x++ ) {
            wi->si->buf[ BUFWI( wi, x, y ) ].ch = 0;
            wi->si->buf[ BUFWI( wi, x, y ) ].color = SCR_COLOR_DEFAULT;
        }
    }
}


/**
 * Clear line in window area.
 *
 *
 * @param wi Window info.
 */
void screen_clear_line( win_info_t wi )
{
    int x;
    int x_limit;

    /* Always clear one extra. */
    x_limit = WI_X_SIZE( wi ) + 1;

    for ( x = 0; x < x_limit; x++ ) {
        wi->si->buf[ BUFWI( wi, x, wi->y ) ].ch = 0;
        wi->si->buf[ BUFWI( wi, x, wi->y ) ].color = SCR_COLOR_DEFAULT;
    }
}


/**
 * Both x and y inside edit area.
 *
 *
 * @param wi Window info to check.
 */
pl_bool_t screen_inside_win( win_info_t wi )
{
    if ( wi->x <= WI_X_MAX( wi ) && wi->y <= WI_Y_MAX( wi ) && wi->x >= WI_X_MIN( wi ) &&
         wi->y >= WI_Y_MIN( wi ) ) {
        return pl_true;
    } else {
        return pl_false;
    }
}


/**
 * At screen bottom limit?
 *
 *
 * @param wi Window info.
 *
 * @return True if at window bottom.
 */
pl_bool_t screen_at_win_y_end( win_info_t wi )
{
    if ( wi->y == WI_Y_MAX( wi ) ) {
        return pl_true;
    }
#ifdef ASSERT
    else if ( wi->y > WI_Y_MAX( wi ) ) {
        if ( screen_fatal_error ) {
            screen_fatal_error( "screen_at_win_y_end" );
        }
        return pl_false;
    }
#endif
    else {
        return pl_false;
    }
}


/**
 * At screen up limit?
 *
 *
 * @param wi Window info.
 *
 * @return True if at window bottom.
 */
pl_bool_t screen_at_win_y_start( win_info_t wi )
{
    if ( wi->y == WI_Y_MIN( wi ) ) {
        return pl_true;
    }
#ifdef ASSERT
    else if ( wi->y < WI_Y_MIN( wi ) ) {
        if ( screen_fatal_error ) {
            screen_fatal_error( "screen_at_win_y_start" );
        }
        return pl_false;
    }
#endif
    else {
        return pl_false;
    }
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
pl_bool_t screen_inside( screen_info_t si, int x, int y )
{
    if ( x <= si->x_max && y <= si->y_max && x >= si->x_min && y >= si->y_min ) {
        return pl_true;
    } else {
        return pl_false;
    }
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
pl_bool_t screen_setpos( win_info_t wi, int x, int y )
{
    win_info_s twi;

    twi = *wi;
    twi.x = x;
    twi.y = y;

    if ( screen_inside_win( &twi ) ) {
        wi->x = x;
        wi->y = y;
        return pl_true;
    } else {
        dbug( "screen_setpos: outside x %d, y %d\n", twi.x, twi.y );
        return pl_false;
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
int screen_set_str2( win_info_t wi, const char* str )
{
    int len = strlen( str );

    /* Write all chars to off-screen buffer unless they are
       overflowing. */
    for ( int i = 0; i < len; i++ ) {
        if ( ( wi->x + i ) <= wi->x_max + 1 ) {
            wi->si->buf[ BUFWI( wi, wi->x + i, wi->y ) ].ch = SIMPLE_CHAR( str[ i ] );
            wi->si->buf[ BUFWI( wi, wi->x + i, wi->y ) ].color = scr_default_color;
        }
    }

    return len;
}


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
int screen_set_color_str( win_info_t wi, const char* str, int color )
{
    int len = strlen( str );

    /* Write all chars to off-screen buffer unless they are
       overflowing. */
    for ( int i = 0; i < len; i++ ) {
        if ( ( wi->x + i ) <= wi->x_max + 1 ) {
            wi->si->buf[ BUFWI( wi, wi->x + i, wi->y ) ].ch = SIMPLE_CHAR( str[ i ] );
            wi->si->buf[ BUFWI( wi, wi->x + i, wi->y ) ].color = color;
        }
    }

    return len;
}


/**
 * Dump offline buffer to the screen.
 *
 *
 * @param si Screen info.
 */
void screen_dump( screen_info_t si )
{
    uint16_t fg, bg;
    uint32_t sch;

    for ( int y = 0; y < si->y_size; y++ ) {
        for ( int x = 0; x < si->x_size; x++ ) {
            if ( si->color ) {
                fg = scr_color_table[ si->buf[ BUFI( x, y ) ].color ].fg;
                bg = scr_color_table[ si->buf[ BUFI( x, y ) ].color ].bg;
            } else {
                fg = scr_color_table[ SCR_COLOR_DEFAULT ].fg;
                bg = scr_color_table[ SCR_COLOR_DEFAULT ].bg;
            }
            if ( si->buf[ BUFI( x, y ) ].ch == 0 ) {
                sch = ' ';
            } else {
                sch = si->buf[ BUFI( x, y ) ].ch;
            }

            tb_set_cell( x, y, sch, (uintattr_t)fg, (uintattr_t)bg );
        }
    }
}


/**
 * Refresh the current view.
 *
 *
 * @param wi Window info.
 */
void screen_refresh( win_info_t wi )
{
    if ( wi->refresh ) {
        tb_clear();
        screen_dump( wi->si );
        tb_set_cursor( wi->si->x_min + wi->x_min + wi->x, wi->si->y_min + wi->y_min + wi->y );
        tb_present();
    }
}


/**
 * Return a key press.
 *
 *
 * @return Key value.
 */
int screen_get_key( void )
{
    int             key;
    struct tb_event event;
    for ( ;; ) {
        tb_poll_event( &event );

        if ( event.type == TB_EVENT_RESIZE ) {
            /* Termbox has detected window resizing. */
            if ( screen_pre_win_resize ) {
                screen_pre_win_resize( screen_win_resize_context );
            }

            tb_clear();
            screen_update_geom( si );

            /* Update all living window geometries. */
            for ( win_info_p wl = plcm_data( &winlist ); (pl_t)wl < plcm_end( &winlist ); wl++ ) {
                screen_update_window_geom( *wl );
            }


            if ( screen_post_win_resize ) {
                screen_post_win_resize( screen_win_resize_context );
            }

        } else {
            break;
        }
    }

    if ( event.key == 0 ) {
        /* Normal character, i.e. ch has valid value. */
        key = event.ch;
    } else {
        /* Control character, i.e. key has valid value. */
        if ( event.key == TB_KEY_ENTER ) {
            /* Map Enter to CTRL_J. */
            key = NEWLINE;
        } else {
            key = event.key;
        }
    }

    if ( key >= 32 && key <= 126 ) {
        dbug( "key: %d (\"%c\")\n", key, (char)key );
    } else {
        dbug( "key: %d\n", key );
    }

    return key;
}


/**
 * Update status line with str.
 *
 *
 * @param str Status line content.
 */
void screen_set_status( char* str )
{
    for ( int i = 0; i < strlen( str ); i++ ) {
        si->buf[ BUFI( 0 + i, screen_status_line ) ].ch = str[ i ];
        si->buf[ BUFI( 0 + i, screen_status_line ) ].color = SCR_COLOR_GREEN;
    }
}


/**
 * Update status line with str.
 *
 *
 * @param str Status line content.
 * @param len Status line length.
 */
void screen_set_status_with_color( char_info_t str, int len )
{
    for ( int i = 0; i < len; i++ ) {
        si->buf[ BUFI( 0 + i, screen_status_line ) ].ch = str[ i ].ch;
        si->buf[ BUFI( 0 + i, screen_status_line ) ].color = str[ i ].color;
    }
}


/**
 * Return window x-size.
 */
int screen_win_x_size( win_info_t wi )
{
    return WI_X_SIZE( wi );
}


/**
 * Return window y-size.
 */
int screen_win_y_size( win_info_t wi )
{
    return WI_Y_SIZE( wi );
}


/**
 * Return window x-min.
 */
int screen_win_x_min( win_info_t wi )
{
    return WI_X_MIN( wi );
}


/**
 * Return window y-min.
 */
int screen_win_y_min( win_info_t wi )
{
    return WI_Y_MIN( wi );
}


/**
 * Return window x-max.
 */
int screen_win_x_max( win_info_t wi )
{
    return WI_X_MAX( wi );
}


/**
 * Return window y-max.
 */
int screen_win_y_max( win_info_t wi )
{
    return WI_Y_MAX( wi );
}
