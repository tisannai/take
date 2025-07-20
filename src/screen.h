#ifndef SCREEN_H
#define SCREEN_H


/**
 * @file screen.h
 *
 * Screen defs.
 */

#include <signal.h>
#include <plinth.h>
#include "common.h"


/**
 * Char_map char classes.
 */
pl_enum( screen_char_class ){ text, tab, code, newline };


/**
 * Character is defined in the map by type and content (str).
 */
pl_struct( screen_char_def )
{
    screen_char_class_t type; /**< Character class. */
    char*               str;  /**< Character visual representation (string). */
};


/**
 * Global handle to currently used character map.
 */
extern screen_char_def_t screen_char_map;

/**
 * Default map declaration.
 */
extern screen_char_def_s screen_default_char_map[];



/**
 * Character info in the screen. Includes color info and character
 * code.
 */

pl_struct( char_info )
{
    uint8_t color; /**< Fore/background color code. */
    char    ch;    /**< Character value. */
};


/**
 * @{
 * Colors for screen text. */
#define SCR_COLOR_WHITE 0
#define SCR_COLOR_DEFAULT 0
#define SCR_COLOR_GREEN 1
#define SCR_COLOR_YELLOW 2
#define SCR_COLOR_RED 3
#define SCR_COLOR_BLUE 4
#define SCR_COLOR_BROWN 5
/** @} */



/**
 * Physical screen dimensions and off-screen content buffer.
 */
pl_struct( screen_info )
{
    /* Physical dimensions. */
    int x_min;  /**< Minimum x-coord. */
    int x_max;  /**< Maximum x-coord. */
    int y_min;  /**< Minimum y-coord. */
    int y_max;  /**< Maximum y-coord. */
    int x_size; /**< Size in x-coord. */
    int y_size; /**< Size in y-coord. */
    int size;   /**< Screen area (x*y). */

    plcm_s      bufmem; /**< Off-screen buffer memory. */
    char_info_t buf;    /**< Off-screen buffer. */

    pl_bool_t color; /**< Screen enables colors. */
};


/**
 * Logical window (text window) dimensions and state.
 */
pl_struct( win_info )
{

    /* Editing area dimensions: */
    int x_min_offset; /**< Minimum x coord (0) offset to screen. */
    int y_min_offset; /**< Minimum y coord (0) offset to screen. */
    int x_max_offset; /**< Maximum x coord (size-1) offset to screen. */
    int y_max_offset; /**< Maximum y coord (size-1) offset to screen. */

    /* Calculated absolutes relative to screen dimensions. Have to be
     updated when screen size changes. */
    int x_min; /**< Minimum x coord. */
    int x_max; /**< Maximum x coord. */
    int y_min; /**< Minimum y coord. */
    int y_max; /**< Maximum y coord. */

    int x; /**< Current x position. */
    int y; /**< Current y position. */

    screen_info_t si; /**< Screen info. */

    void* content; /**< Handle to window content. */

    /**
     * Enable flag for on-screen buffer updates. Flag is currently
     * always true and not needed.
     */
    pl_bool_t refresh;

    /**
     * Enable wrapped long lines (wrap-line-mode), i.e. the line wrap
     * char is allowed to be added. The tail of the is wrapped to the
     * next screen line, thus less physical lines are visible than there
     * are screen lines.
     *
     * When disable the scroll-line-mode is active. Lines that are not
     * current and are longer than what screen is able to fit, are
     * marked with '-' char at the line end. Current line is marked
     * either with '-' at beginning of line if current line position
     * overflows the screen, or with '-' at end of screen if line is
     * long and pos is in the first screen part of line.
     */
    pl_bool_t wrapline;
};



/**
 * @{
 * Window geometry macros. */
#define WI_X_SIZE( wi ) ( ( wi )->x_max - ( wi )->x_min + 1 )
#define WI_Y_SIZE( wi ) ( ( wi )->y_max - ( wi )->y_min + 1 )
#define WI_X_MIN( wi ) 0
#define WI_Y_MIN( wi ) 0
#define WI_X_MAX( wi ) ( ( WI_X_SIZE( wi ) ) - 1 )
#define WI_Y_MAX( wi ) ( ( WI_Y_SIZE( wi ) ) - 1 )
/** @} */


/** Screen lib callback. */
typedef void ( *screen_callback )( void* );

/** Screen lib error callback. */
typedef void ( *screen_fatal_callback )( const char* format, ... );


/** Data passed for screen_pre_win_resize/screen_post_win_resize callbacks. */
extern void* screen_win_resize_context;

/** Callback which is called before window size is performed. */
extern screen_callback screen_pre_win_resize;

/**
   Callback which is called after window size is performed.

   NOTE: callback must include at least the call to
   screen_update_window_geom for each win_info handle.
*/
extern screen_callback screen_post_win_resize;

/** Callback which is for fatal errors. */
extern screen_fatal_callback screen_fatal_error;

/** Status line position (default: -1, i.e. not existing). */
extern int screen_status_line;


/** Global screen handle. */
extern screen_info_t si;

/** Prototypes: */

screen_info_t screen_open( void );
void          screen_update_geom( screen_info_t si );
void*         screen_close( screen_info_t si );
win_info_t    screen_open_window_geom( screen_info_t si,
                                       int           x_min_offset,
                                       int           x_max_offset,
                                       int           y_min_offset,
                                       int           y_max_offset,
                                       pl_bool_t     wrapline );
void          screen_update_window_geom( win_info_t wi );
void          screen_init_win( win_info_t wi );
void          screen_close_window( win_info_t wi );
void          screen_clear_buf( screen_info_t si );
void          screen_clear_win( win_info_t wi );
void          screen_clear_line( win_info_t wi );
pl_bool_t     screen_inside_win( win_info_t wi );
pl_bool_t     screen_at_win_y_end( win_info_t wi );
pl_bool_t     screen_at_win_y_start( win_info_t wi );
pl_bool_t     screen_inside( screen_info_t si, int x, int y );
pl_bool_t     screen_setpos( win_info_t wi, int x, int y );
int           screen_set_str2( win_info_t wi, const char* str );
void          screen_dump( screen_info_t si );
void          screen_refresh( win_info_t wi );
int           screen_get_key( void );
void          screen_set_status( char* str );
int           screen_win_x_size( win_info_t wi );
int           screen_win_y_size( win_info_t wi );
int           screen_win_x_min( win_info_t wi );
int           screen_win_y_min( win_info_t wi );
int           screen_win_x_max( win_info_t wi );
int           screen_win_y_max( win_info_t wi );


void screen_set_status_with_color( char_info_t str, int len );
void screen_set_default_color( int color );
int  screen_set_color_str( win_info_t wi, const char* str, int color );


#endif
