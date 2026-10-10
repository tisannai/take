/**
 * @file take.c
 *
 * @mainpage
 *
 * List Selection Utility by Tero Isannainen, (c) Copyright 2015, 2025.
 *
 * Purpose of Take is to provide a convenient way of selecting items
 * from a list within a text terminal. Take is best tool for filtering
 * when the items do not share substrings or similar. Grep and such
 * are good when the selection shares substrings.
 *
 * Take can be used for example:
 *  - delete files
 *  - select files for version control
 *  - select files for tar package
 */

#ifdef HAVE_VFORK
#    define _DEFAULT_SOURCE
#endif

#include <plinth.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <regex.h>
#include <como.h>

#include <termal.h>



/* abu-version */
const char* take_version = "0.2.0";

#define ENABLE_MARK_COLOR


/*
 * Implementation features:
 * - Screen clear
 * - Line list display:
 *   - List container
 *   - Unlimited line length.
 *   - Visible part start line
 *   - Screen size Y and X
 *   - List item:
 *     - Content
 *     - Selected?
 *   - List refresh
 * - Status line
 * - Interactive Prompt:
 *   - Prompt label
 *   - Editing commands: add_char, move_left, move_right, delete_left, delete_right,
 *       line_start, line_end, enter, interrupt, kill_line
 *   - Unlimited prompt buffer
 *
 *
 * - Extra feature:
 *   - Long line display (not done yet, 150312_1750)
 *
 */


// #define TAKE_SKIP_INTERACTION


/** Selectable line type. NOTE: Also used for help and command view. */
pl_struct( select_line )
{
    const char* text;   /**< Line content. */
    pl_bool_t   marked; /**< Selected flag. */
};


/** Collection of selectable lines with viewing info. */
pl_struct( lines_view )
{
    plcm_s   lines;     /**< Line container for select_line. */
    pl_pos_t firstline; /**< First visible line index. */
    pl_pos_t curline;   /**< Current line index. */
    plam_t   alloc;     /**< Allocator. */
};


/** Collection of selectable lines with viewing info. */
pl_struct( select_lines )
{
    lines_view_t lv;           /**< Current lines_view. */
    td_point_s   screen_dims;  /**< Screen dimensions. */
    tm_color_s   unmark_color; /**< Color for unmarked lines. */
    tm_color_s   mark_color;   /**< Color for marked lines. */
    char*        user_message; /**< Message for the user. */
    int          line_status;  /**< Line mode Status. */
    char*        find_status;  /**< Find mode Status. */
    plam_t       alloc;        /**< Allocator. */
};


/** Base allocator. */
#define balloc_node_size ( 1 << 20 )
#define balloc_cont_size ( balloc_node_size / 2 )
static plam_s balloc;


/** Terminal. */
static termal_s tm;


/** Arbitrary length string buffer. Used from multiple functions for
    temporary storage for building new values. */
static plcm_s work_buf;

/** Buffer used for line reading from FILE handles. */
static plcm_s line_buf;

/** Buffer for user inputs. Overlapping lifetime to work_buf and
    therefore needs to be separate. */
static plcm_s user_buf;



static char* help_list[] = {
    "  J: Toggle selection and move down",
    "  K: Toggle selection and move up",
    "  j: Move down",
    "  k: Move up",
    "  n: Move down page",
    "  p: Move up page",
    "  b: Move to beginning of list",
    "  e: Move to end of list",
    "  g: Goto to line",
    "  G: Goto last line",
    "  s: Select current",
    "  r: Reject current",
    "  t: Toggle current",
    "  S: Select all items",
    "  R: Reject all items",
    "  T: Toggle all items",
    "  c: Toggle the next <count> items",
    "  m: Select items matching the prompted regexp (case sensitive)",
    "  M: Select items matching the prompted regexp (case insensitive)",
    "  f: Find mode with case sensitive matching (Keys: j,k,s,r,t,RET,ESC)",
    "  F: Find mode with case insensitive matching (Keys: j,k,s,r,t,RET,ESC)",
    "  v: View the list of commands that would be executed",
    "  i: View the current list entry content (if a text file)",
    "  l: Center list view on screen around current line",
    "  h: Show command help",
    "  x: Quit and execute output-command for selection",
    "  q: Quit and skip output-command execution",
    NULL
};



/**
 * Exit program.
 *
 */
static void take_exit( int status )
{
    /* Cleanup everything. */

    plam_del( &balloc );
    plcm_del( &line_buf );
    plcm_del( &work_buf );

    if ( termal_is_open( &tm ) ) {
        termal_close( &tm );
    }
    como_end();
    exit( status );
}


/**
 * General purpose signal handler for Take. Always close the screen.
 *
 * @param signo Signal number (not used).
 */
static void take_sig_handler( int signo )
{
    take_exit( EXIT_FAILURE );
}


/**
 * Fatal error printout.
 *
 * @param format Format string for message.
 */
static void take_fatal( const char* format, ... )
{
    va_list ap;
    va_start( ap, format );
    fputs( "Take FATAL: ", stderr );
    vfprintf( stderr, format, ap );
    va_end( ap );
    fputc( '\n', stderr );
    take_exit( EXIT_FAILURE );
}


/**
 * Fatal error printout.
 *
 * @param format Format string for message.
 */
static void take_error( const char* format, ... )
{
    va_list ap;
    va_start( ap, format );
    fputs( "Take FATAL: ", stderr );
    vfprintf( stderr, format, ap );
    fputc( '\n', stderr );
    va_end( ap );
}


/**
 * Display user documentation.
 *
 * @param pl_none
 *
 * @return
 */
static pl_none cli_documentation( pl_none )
{
    for ( int i = 0; help_list[ i ]; i++ ) {
        puts( help_list[ i ] );
    }
    take_exit( EXIT_FAILURE );
}


/**
 * Return a line from file stream, without the newline.
 */
static plcm_t get_line_from_file( FILE* fh )
{
    return plss_read_line( &line_buf, fh );
}


/**
 * Add line to select_lines.
 */
static pl_none lines_view_add( lines_view_t lv, plcm_t text )
{
    select_line_t line;
    const char*   linetext;

    linetext = plam_store( lv->alloc, (pl_t)plss_string( text ), plss_length( text ) + 1 );
    line = plcm_get_ref_for_type( &lv->lines, select_line_s );
    line->text = linetext;
    line->marked = pl_false;
}


/**
 * Reference a line from select_lines by position (index).
 */
static select_line_t lines_view_ref( lines_view_t lv, pl_pos_t pos )
{
    return plcm_ref_for_type( &lv->lines, pos, select_line_s );
}


/**
 * Return select_lines count.
 */
static pl_size_t lines_view_count( lines_view_t lv )
{
    return plcm_used_for_type( &lv->lines, select_line_s );
}


/**
 * Initialize select_lines struct.
 *
 * @param sl     Select-lines handle.
 * @param lv     Lines-view handle.
 * @param alloc  Allocator for select-lines.
 *
 * @return None.
 */
static pl_none select_lines_init( select_lines_t sl, lines_view_t lv, plam_t alloc )
{
    sl->lv = lv;

    sl->unmark_color = termal_color_default();
    sl->mark_color = termal_color_make( 0x09, 0x00 );

    sl->user_message = NULL;
    sl->line_status = -1;
    sl->find_status = NULL;

    sl->alloc = alloc;
}


/**
 * Create Select_lines, i.e. lines container.
 *
 * @return Line container.
 */
static pl_none lines_view_new( lines_view_t lv, plam_t alloc, pl_size_t init_line_count )
{
    plcm_use_plam( &lv->lines, alloc, init_line_count * sizeof( select_line_s ) );
    lv->firstline = 0;
    lv->curline = 0;
    lv->alloc = alloc;
}


/**
 * Free Select_lines.
 *
 * @param sl Select_lines object.
 *
 * @return NULL.
 */
static lines_view_t lines_view_rem( lines_view_t lv )
{
    /* The line text is allocated from talloc, and is deallocated
       elsewhere. */
    plcm_del( &lv->lines );
    return NULL;
}


/**
 * Return screen index of y-axis.
 *
 * @param sl     Select-lines handle.
 *
 * @return Index.
 */
static pl_pos_t screen_line( select_lines_t sl )
{
    return ( sl->lv->curline - sl->lv->firstline );
}


/**
 * Return status line index on screen.
 *
 * @param sl     Select-lines handle.
 *
 * @return Index.
 */
static pl_pos_t screen_status_line( select_lines_t sl )
{
    return sl->screen_dims.y - 1;
}


/**
 * Return last lines-view line index.
 *
 * @param sl     Select-lines handle.
 *
 * @return Index.
 */
static pl_pos_t screen_last_view_line( select_lines_t sl )
{
    return sl->screen_dims.y - 2;
}


/**
 * Normalize current line after terminal size have (potentially)
 * changed.
 *
 * @param sl     Select-lines handle.
 *
 * @return None.
 */
static pl_none normalize_lines_view( select_lines_t sl )
{
    if ( screen_line( sl ) > screen_last_view_line( sl ) ) {
        sl->lv->firstline = sl->lv->curline - screen_last_view_line( sl );
    }
}


/**
 * Prompt input from user.
 *
 * @param sl     Select-lines handle.
 * @param prompt Prompt text.
 * @param text   User response storage.
 *
 * @return Prompt status.
 */
static tm_prompt_t prompt_user( select_lines_t sl, char* prompt, plcm_t text )
{
    tm_prompt_t res;

    plcm_clear( text );
    res = termal_prompt( &tm,
                         plsr_from_string( prompt ),
                         td_point_make( 0, sl->screen_dims.y - 1 ),
                         sl->screen_dims.x - 4,
                         text,
                         pl_true );

    return res;
}


/**
 * Update line_status label with current line number.
 *
 * @param sl Select_lines object.
 */
static void line_status_update( select_lines_t sl )
{
    /* Create line number display. */
    plcm_clear( &work_buf );
    plss_format_string( &work_buf, " %7d", (int)( sl->lv->curline + 1 ) );
    plss_format_string( &work_buf, "  " );
    if ( sl->find_status ) {
        plss_format_string( &work_buf, "%s   ", sl->find_status );
    } else {
        plss_format_string( &work_buf, "    " );
    }

    if ( sl->user_message ) {
        plss_format_string( &work_buf, "%s", sl->user_message );
    }

    termal_draw_string_in_color( &tm,
                                 td_point_make( 0, screen_status_line( sl ) ),
                                 plsr_from_plcm( &work_buf ),
                                 sl->unmark_color );
}


/**
 * Display Select_lines on screen. Marked lines have '*' in front of
 * the text.
 *
 * @param sl Select_lines object.
 */
static void select_lines_display( select_lines_t sl )
{
    select_line_t line;

    termal_clear( &tm );

    /* Show all visible lines or upto end of list. */
    for ( int i = 0;
          ( i < screen_status_line( sl ) ) &&
          ( ( sl->lv->firstline + i ) < ( plcm_used_for_type( &sl->lv->lines, select_line_s ) ) );
          i++ ) {
        line = lines_view_ref( sl->lv, sl->lv->firstline + i );
        plcm_reset( &work_buf );

#ifdef ENABLE_MARK_COLOR

        plss_format_string( &work_buf, "%s", line->text );
        if ( line->marked ) {
            termal_draw_string_in_color(
                &tm, td_point_make( 0, i ), plsr_from_plcm( &work_buf ), sl->mark_color );
        } else {
            termal_draw_string_in_color(
                &tm, td_point_make( 0, i ), plsr_from_plcm( &work_buf ), sl->unmark_color );
        }

#else
        if ( line->marked ) {
            plss_format_string( &work_buf, "* %s", line->text );
        } else {
            plss_format_string( &work_buf, "  %s", line->text );
        }

        termal_draw_string_in_color(
            &tm, td_point_make( 0, i ), plss_string( &work_buf ), sl->unmark_color );
#endif
    }

    termal_cursor_set( &tm, td_point_make( 0, screen_line( sl ) ) );

    line_status_update( sl );

    termal_refresh( &tm );
}


/**
 * Toggle "marked" on current line.
 *
 * @param sl Select_lines object.
 */
static void select_lines_toggle_mark( select_lines_t sl )
{
    select_line_t line;
    line = lines_view_ref( sl->lv, sl->lv->curline );
    if ( line->marked ) {
        line->marked = pl_false;
    } else {
        line->marked = pl_true;
    }
}


/**
 * Set "marked" attribute to selected value.
 *
 * @param sl Select_lines object.
 * @param marked Value for marked status.
 */
static void select_lines_set_mark_to( select_lines_t sl, pl_bool_t marked )
{
    select_line_t line;
    line = lines_view_ref( sl->lv, sl->lv->curline );
    line->marked = marked;
}


/**
 * Set item selected.
 *
 * @param sl Select_lines object.
 */
static void select_lines_set_mark( select_lines_t sl )
{
    select_lines_set_mark_to( sl, pl_true );
}


/**
 * Set item non-selected.
 *
 * @param sl Select_lines object.
 */
static void select_lines_reset_mark( select_lines_t sl )
{
    select_lines_set_mark_to( sl, pl_false );
}


/**
 * Move n step down on line list.
 *
 * @param sl Select_lines object.
 * @param n Move count.
 *
 * @return Number of step performed.
 */
static int select_lines_move_down_n( select_lines_t sl, int n )
{
    int i;

    for ( i = 0; i < n; i++ ) {
        if ( sl->lv->curline < lines_view_count( sl->lv ) - 1 ) {
            if ( screen_line( sl ) >= screen_last_view_line( sl ) ) {
                sl->lv->firstline++;
            }
            sl->lv->curline++;
        } else {
            break;
        }
    }

    return i;
}


/**
 * Move one line down.
 *
 * @param sl Select_lines object.
 *
 * @return True if success.
 */
static pl_bool_t select_lines_move_down( select_lines_t sl )
{
    if ( select_lines_move_down_n( sl, 1 ) == 1 ) {
        return pl_true;
    } else {
        return pl_false;
    }
}


/**
 * Move n step up on line list.
 *
 * @param sl Select_lines object.
 * @param n Move count.
 *
 * @return Number of step performed.
 */
static int select_lines_move_up_n( select_lines_t sl, int n )
{
    int i;

    for ( i = 0; i < n; i++ ) {
        if ( sl->lv->curline > 0 ) {
            if ( screen_line( sl ) == 0 ) {
                sl->lv->firstline--;
            }
            sl->lv->curline--;
        } else {
            break;
        }
    }

    return i;
}


/**
 * Move one line up.
 *
 * @param sl Select_lines object.
 *
 * @return True if success.
 */
static pl_bool_t select_lines_move_up( select_lines_t sl )
{
    if ( select_lines_move_up_n( sl, 1 ) == 1 ) {
        return pl_true;
    } else {
        return pl_false;
    }
}


/**
 * Create list content from shell command.
 *
 * @param sl Select_lines object.
 * @param cmd Shell command.
 */
static void list_from_command( select_lines_t sl, const char* cmd )
{
    FILE* fh;

    fh = popen( cmd, "r" );

    if ( !fh ) {
        take_fatal( "Could not execute: %s", cmd );
    }

    for ( ;; ) {
        if ( get_line_from_file( fh ) ) {
            lines_view_add( sl->lv, &line_buf );
        } else {
            break;
        }
    }

    pclose( fh );
}


/**
 * qsort callback function for sorting strings.
 *
 * @param a First str to compare.
 * @param b Second str to compare.
 *
 * @return strcmp return values.
 */
static int list_sort_cmp( const void* a, const void* b )
{
    const char** ia = (const char**)a;
    const char** ib = (const char**)b;
    return strcmp( *ia, *ib );
}


/**
 * Create line content from current directory entries in ascending
 * order. "." and ".."  are not used.
 *
 * @param sl Select_lines object.
 * @param dirname Directory name for listing.
 */
static void list_from_dir( select_lines_t sl, const char* dirname )
{
    DIR*           dir;
    struct dirent* entry;

    dir = opendir( dirname );

    if ( dir ) {
        plcm_s unsorted;
        char*  tmpdir;

        plam_s talloc;
        plam_use_plam( &talloc, &balloc, balloc_cont_size );
        plcm_use_plam( &unsorted, &talloc, balloc_cont_size );

        while ( ( entry = readdir( dir ) ) != NULL ) {
            /* Skip pwd and upper dir. */
            if ( strcmp( ".", entry->d_name ) && strcmp( "..", entry->d_name ) ) {
                tmpdir = plam_store( &talloc, entry->d_name, strlen( entry->d_name ) + 1 );
                plcm_store_ptr( &unsorted, tmpdir );
            }
        }

        closedir( dir );

        /* Sort entries into alphabetical order. */
        qsort( plcm_data( &unsorted ), plcm_used_ptr( &unsorted ), sizeof( void* ), list_sort_cmp );

        /* Add sorted entries to lines list with dirname. */
        for ( char** tmpname = plcm_data( &unsorted ); tmpname < (char**)plcm_end( &unsorted );
              tmpname++ ) {
            plss_reformat_string( &work_buf, "%s/%s", dirname, *tmpname );
            lines_view_add( sl->lv, &work_buf );
        }

        plcm_del( &unsorted );
        plam_del( &talloc );
        plam_put( &balloc, balloc_cont_size );
    }
}


/**
 * Create line content by reading stdin (piped input).
 *
 * @param sl Select_lines object.
 */
static void list_from_stdin( select_lines_t sl )
{
    FILE* fh;

    fh = stdin;
    for ( ;; ) {
        if ( get_line_from_file( fh ) ) {
            lines_view_add( sl->lv, &line_buf );
        } else {
            break;
        }
    }
}


/**
 * Replace the "@" special chars with selected lines. Also replace
 * "@_" with "@" on output.
 *
 * @param [in] cmd Command string.
 * @param [in] arg Replacement for special chars.
 * @param [out] buf String buffer for replacement result.
 */
static void process_cmd_escapes( char* cmd, const char* arg, plcm_t buf )
{
    /* Char index for cmd string. */
    int ci;

    plcm_reset( buf );

    ci = 0;

    for ( ;; ) {

        if ( cmd[ ci ] == 0 ) {
            /* End of command. */
            break;
        }

        if ( cmd[ ci ] == '@' ) {
            /* Replace with arg. */
            plss_append_string( buf, arg );
            ci++;
        } else if ( cmd[ ci ] == '%' ) {
            if ( cmd[ ci + 1 ] == '@' ) {
                /* Literal @ to output. */
                plss_append_char( buf, '@' );
                ci += 2;
            } else if ( cmd[ ci + 1 ] == '%' ) {
                /* Literal % to output. */
                plss_append_char( buf, '%' );
                ci += 2;
            } else {
                /* Copy to output. */
                plss_append_char( buf, cmd[ ci ] );
                ci++;
            }
        } else {
            /* Copy to output. */
            plss_append_char( buf, cmd[ ci ] );
            ci++;
        }
    }
}


/**
 * Execute command or just place the command to output stream (if
 * non-null).
 *
 * @param cmd Command to execute (or store).
 * @param fh Output stream (or NULL).
 */
static void execute_cmd( const char* cmd, FILE* fh )
{
    if ( fh ) {
        fputs( cmd, fh );
        fputc( '\n', fh );
    } else {

#ifdef HAVE_VFORK

        /* When take includes a long list, a lot of memory is
         allocated. vfork does not duplicate the memory allocation and
         thus system (execl) calls start significantly faster. */

        pid_t child;
        child = vfork();
        if ( child == 0 ) {
            execl( "/bin/sh", "sh", "-c", cmd, (char*)0 );
        } else if ( child == -1 ) {
            /* Failure. */
            take_error( "Could not execute: \"%s\"\n  reason: \"%s\"", cmd, strerror( errno ) );
        } else {
            int status;
            waitpid( child, &status, 0 );
        }

#else

        system( cmd );

#endif
    }
}


/**
 * View lines from "view_sl" on top of "sl". After viewing the "sl"
 * status is reverted back on display. Used for help and commands
 * view.
 *
 * @param sl Select_lines object.
 * @param view_sl View list.
 */
static void select_lines_view( select_lines_t sl )
{
    int       key;
    pl_pos_t  cur;
    pl_pos_t  max;
    pl_bool_t done;

    /* Display all lines. */
    select_lines_display( sl );

    done = pl_false;

    /* Get user input. */
    for ( ;; ) {

        key = termal_get_key( &tm );

        switch ( key ) {

            case TM_KEY_ESCAPE:
            case TM_KEY_CTRL_G:
            case TM_KEY_RETURN:
            case 'q':
                done = pl_true;
                break;

            case 'n':
                cur = screen_line( sl );
                max = screen_last_view_line( sl );
                if ( cur != max ) {
                    select_lines_move_down_n( sl, max - cur );
                } else {
                    select_lines_move_down_n( sl, max + 1 );
                }
                break;

            case 'p':
                cur = screen_line( sl );
                max = screen_last_view_line( sl );
                if ( cur != 0 ) {
                    select_lines_move_up_n( sl, cur );
                } else {
                    select_lines_move_up_n( sl, max + 1 );
                }
                break;


            default:
                break;
        }

        if ( done ) {
            break;
        }

        select_lines_display( sl );
    }
}


/**
 * Show command help on screen.
 *
 * NOTE: Keep the help list in sync with the manual page.
 *
 * @param sl Select_lines object.
 */
static void show_command_help( select_lines_t sl )
{
    lines_view_s help_lv;
    plam_s       talloc;
    lines_view_t org_lv;

    org_lv = sl->lv;
    sl->lv = &help_lv;
    plam_use_plam( &talloc, &balloc, balloc_cont_size );
    lines_view_new( &help_lv, &talloc, 128 );

    plcm_s tmp;
    for ( int i = 0; help_list[ i ]; i++ ) {
        lines_view_add( sl->lv, plss_from_plsr( &tmp, plsr_from_string( help_list[ i ] ) ) );
    }

    select_lines_view( sl );
    lines_view_rem( &help_lv );
    plam_put( &balloc, balloc_cont_size );
    sl->lv = org_lv;
    normalize_lines_view( sl );
}


/**
 * Show file content on screen.
 *
 * @param sl Select_lines object.
 * @param filename Filename for file to display.
 */
static void show_file_content( select_lines_t sl, const char* filename )
{
    FILE* fh;

    fh = fopen( filename, "r" );
    if ( !fh ) {
        /* Can't open file, abort. */
        return;
    }

    lines_view_s file_lv;
    plam_s       talloc;
    lines_view_t org_lv;

    org_lv = sl->lv;
    sl->lv = &file_lv;
    plam_use_plam( &talloc, &balloc, balloc_cont_size );
    lines_view_new( &file_lv, &talloc, 1024 );

    for ( ;; ) {
        if ( get_line_from_file( fh ) ) {
            lines_view_add( &file_lv, &line_buf );
        } else {
            break;
        }
    }

    select_lines_view( sl );
    lines_view_rem( &file_lv );
    plam_put( &balloc, balloc_cont_size );
    sl->lv = org_lv;
    normalize_lines_view( sl );

    fclose( fh );
}


/**
 * Create a list of shell command executions for selected items.
 *
 * @param sl     Select_lines object.
 * @param cmds   Lines-view for commands.
 * @param talloc Allocator for commands.
 *
 * @return None.
 */
static pl_none select_lines_create_commands( select_lines_t sl, lines_view_t cmds, plam_t talloc )
{
    como_opt_t opt;
    char*      command = NULL;

    if ( ( opt = como_given( "command" ) ) ) {
        command = opt->value[ 0 ];
    }

    if ( ( opt = como_given( "auto" ) ) ) {
        command = opt->value[ 0 ];
    }

    if ( !command ) {
        /* No command to use, so use the default command. */
        command = "echo @";
    }


    lines_view_new( cmds, talloc, plcm_used_for_type( &sl->lv->lines, select_line_s ) );

    if ( ( opt = como_given( "join" ) ) ) {

        char* join_str;

        /* Join all selected items with join string. */

        if ( opt->valuecnt > 0 ) {
            join_str = opt->value[ 0 ];
        } else {
            join_str = " ";
        }

        pl_bool_t nonfirst;
        plsr_s    join;

        nonfirst = pl_false;
        join = plsr_from_string( join_str );

        plcm_reset( &work_buf );
        for ( select_line_t line = plcm_data( &sl->lv->lines );
              (pl_t)line < plcm_end( &sl->lv->lines );
              line++ ) {
            if ( line->marked ) {
                if ( nonfirst ) {
                    plss_append( &work_buf, join );
                } else {
                    nonfirst = pl_true;
                }
                plss_append_string( &work_buf, line->text );
            }
        }


        /* Take of &work_buf before its overwritten. */
        plcm_declare( tmpstr, 1024 );
        plss_append( &tmpstr, plss_ref( &work_buf ) );

        /* Create command. */
        process_cmd_escapes( command, plss_string( &tmpstr ), &work_buf );

        /* Free the copy. */
        plcm_del( &tmpstr );

        lines_view_add( cmds, &work_buf );

    } else {

        /* Create command for each selected item individually. */
        for ( select_line_t line = plcm_data( &sl->lv->lines );
              (pl_t)line < plcm_end( &sl->lv->lines );
              line++ ) {
            if ( line->marked ) {
                process_cmd_escapes( command, line->text, &work_buf );
                lines_view_add( cmds, &work_buf );
            }
        }
    }
}


/**
 * Remove Regex object.
 *
 * @param re Regex object.
 */
static void regex_rem( regex_t* re )
{
    if ( re ) {
        regfree( re );
    }
}


/**
 * Create Regex object from pattern either in case sensitive or
 * insensitive mode.
 *
 * @param pattern Regex pattern.
 * @param case_sensitive Case sensitivity option.
 *
 * @return Regex object (or NULL on failure).
 */
static regex_t* regex_new( regex_t* re, const char* pattern, pl_bool_t case_sensitive )
{
    int ret;

    if ( case_sensitive ) {
        ret = regcomp( re, pattern, REG_EXTENDED | REG_NOSUB );
    } else {
        ret = regcomp( re, pattern, REG_EXTENDED | REG_ICASE | REG_NOSUB );
    }

    if ( ret != 0 ) {
        /* Failure. */
        regex_rem( re );
        return NULL;
    }

    return re;
}


/**
 * Select all lines that match the regex pattern.
 *
 * @param sl Select_lines object.
 * @param pattern Regex pattern.
 * @param case_sensitive Case sensitivity option.
 */
static void select_lines_mark_matching( select_lines_t sl,
                                        const char*    pattern,
                                        pl_bool_t      case_sensitive )
{
    regex_t re;

    if ( regex_new( &re, pattern, case_sensitive ) == NULL ) {
        sl->user_message = "Error in regexp!";
        return;
    }

    select_line_t line;
    for ( pl_pos_t i = 0; i < lines_view_count( sl->lv ); i++ ) {
        line = lines_view_ref( sl->lv, i );
        if ( regexec( &re, line->text, 0, NULL, 0 ) == 0 ) {
            line->marked = pl_true;
        }
    }

    regex_rem( &re );
}


/**
 * Find the next (either forward or backward) line matching the Regex
 * object.
 *
 * @param sl Select_lines object.
 * @param re Regex object.
 * @param forward Seach direction.
 *
 * @return The number of steps required to selected direction to reach
 *   the line (-1 if re not found).
 */
static pl_pos_t select_lines_find_next( select_lines_t sl, regex_t* re, pl_bool_t forward )
{
    pl_pos_t offset;
    pl_pos_t limit;

    if ( forward ) {
        offset = 1;
        limit = lines_view_count( sl->lv );
    } else {
        offset = -1;
        limit = -1;
    }

    select_line_t line;
    pl_pos_t      ret;

    ret = 0;
    for ( pl_pos_t idx = sl->lv->curline; idx != limit; idx = idx + offset ) {
        line = lines_view_ref( sl->lv, idx );
        if ( regexec( re, line->text, 0, NULL, 0 ) == 0 ) {
            return ret;
        }
        ret++;
    }

    return -1;
}


/**
 * Save the line state from lines_view object to another.
 *
 * @param from State origin.
 * @param to State target.
 */
static void lines_view_save_position( lines_view_t from, lines_view_t to )
{
    to->firstline = from->firstline;
    to->curline = from->curline;
}


/**
 * Interactive search list lines.
 *
 * @param sl Select_lines object.
 * @param pattern Regex pattern to search.
 * @param case_sensitive Case sensitivity option.
 */
static void select_lines_find_interactive( select_lines_t sl,
                                           const char*    pattern,
                                           pl_bool_t      case_sensitive )
{
    lines_view_s org_lv;
    lines_view_s prev_lv;
    int          key;
    pl_bool_t    done;
    pl_bool_t    use_org;
    regex_t      re;
    pl_pos_t     offset;
    pl_bool_t    first_search;

    /* Save search start position. */
    org_lv = *sl->lv;
    prev_lv = *sl->lv;

    if ( !regex_new( &re, pattern, case_sensitive ) ) {
        sl->user_message = "Error in regexp!";
        return;
    }

    sl->find_status = "F";

    done = pl_false;
    use_org = pl_false;
    first_search = pl_true;

    /* Get user input. */
    for ( ;; ) {

        select_lines_display( sl );

        key = termal_get_key( &tm );

        switch ( key ) {

            case TM_KEY_ESCAPE:
            case TM_KEY_CTRL_G:
            case 'q':
                done = pl_true;
                /* Revert back to original line. */
                use_org = pl_true;
                break;

            case TM_KEY_RETURN:
                done = pl_true;
                /* Stay on line that had last match. */
                use_org = pl_false;
                break;

            case 's':
                select_lines_set_mark( sl );
                break;

            case 'r':
                select_lines_reset_mark( sl );
                break;

            case 't':
                select_lines_toggle_mark( sl );
                break;

            case 'j':
                /* Save current position in case nothing is found. */
                lines_view_save_position( sl->lv, &prev_lv );

                /* For first search also the current line is searched. */
                if ( first_search || select_lines_move_down( sl ) ) {
                    if ( ( offset = select_lines_find_next( sl, &re, pl_true ) ) != -1 ) {
                        /* Found new matching line. */
                        select_lines_move_down_n( sl, offset );
                    } else {
                        /* Nothing found, revert back to search origin. */
                        lines_view_save_position( &prev_lv, sl->lv );
                    }
                }

                first_search = pl_false;

                break;

            case 'k':

                /* Same comments as for 'j' ... */
                lines_view_save_position( sl->lv, &prev_lv );

                if ( first_search || select_lines_move_up( sl ) ) {
                    if ( ( offset = select_lines_find_next( sl, &re, pl_false ) ) != -1 ) {
                        select_lines_move_up_n( sl, offset );
                    } else {
                        lines_view_save_position( &prev_lv, sl->lv );
                    }
                }

                first_search = pl_false;

                break;

            default:
                break;
        }

        if ( done ) {
            break;
        }
    }

    if ( use_org ) {
        lines_view_save_position( &org_lv, sl->lv );
    }

    regex_rem( &re );

    sl->find_status = NULL;

    select_lines_display( sl );
}


/**
 * Put current line in the center of the list view.
 *
 * @param sl Select_lines object.
 */
static void select_lines_center_view( select_lines_t sl )
{
    int half_cnt;
    int dist;
    int offset;
    int newfirst;

    half_cnt = sl->screen_dims.y / 2;
    dist = screen_line( sl );
    offset = dist - half_cnt;
    newfirst = sl->lv->firstline + offset;
    if ( newfirst >= 0 ) {
        sl->lv->firstline = newfirst;
    }
}


/**
 * View a list of commands that would be executed.
 *
 * @param sl Select_lines object.
 */
static void select_lines_view_commands( select_lines_t sl )
{
    lines_view_s cmds_lv;
    plam_s       talloc;
    lines_view_t org_lv;

    plam_use_plam( &talloc, &balloc, balloc_cont_size );
    select_lines_create_commands( sl, &cmds_lv, &talloc );
    org_lv = sl->lv;
    sl->lv = &cmds_lv;
    select_lines_view( sl );
    lines_view_rem( &cmds_lv );
    plam_put( &balloc, balloc_cont_size );
    sl->lv = org_lv;
    normalize_lines_view( sl );
}


/**
 * Interact with the user to get line selection.
 *
 * @param sl Select_lines object.
 *
 * @return True if the selection should be used for execution.
 */
static pl_bool_t interaction( select_lines_t sl )
{
    int           key;
    pl_bool_t     execute;
    pl_bool_t     done;
    select_line_t line;
    pl_pos_t      cur;
    pl_pos_t      max;
    tm_prompt_t   res;
    const char*   input;

    execute = pl_false;
    done = pl_false;

    /* Get user input. */
    for ( ;; ) {

        select_lines_display( sl );

        key = termal_get_key( &tm );

        /* Clear pending user messages. */
        sl->user_message = NULL;
        sl->find_status = NULL;

        switch ( key ) {

            case 'q':
                done = pl_true;
                execute = pl_false;
                break;

            case 'x':
                done = pl_true;
                execute = pl_true;
                break;

            case 'J':
                select_lines_toggle_mark( sl );
                select_lines_move_down( sl );
                break;

            case 'K':
                select_lines_toggle_mark( sl );
                select_lines_move_up( sl );
                break;

            case 'j':
                select_lines_move_down( sl );
                break;

            case 'k':
                select_lines_move_up( sl );
                break;

            case 'n':
                cur = screen_line( sl );
                max = screen_last_view_line( sl );
                if ( cur != max ) {
                    select_lines_move_down_n( sl, max - cur );
                } else {
                    select_lines_move_down_n( sl, max + 1 );
                }
                break;

            case 'p':
                cur = screen_line( sl );
                max = screen_last_view_line( sl );
                if ( cur != 0 ) {
                    select_lines_move_up_n( sl, cur );
                } else {
                    select_lines_move_up_n( sl, max + 1 );
                }
                break;

            case 'b':
                select_lines_move_up_n( sl, sl->lv->curline );
                break;

            case 'e':
                select_lines_move_down_n( sl, lines_view_count( sl->lv ) - 1 - sl->lv->curline );
                break;

            case 'g':
                res = prompt_user( sl, "goto (+/- #): ", &user_buf );
                input = plss_string( &user_buf );

                if ( res == tm_prompt_ok ) {

                    pl_pos_t cnt;

                    switch ( input[ 0 ] ) {

                        case '+':
                            cnt = strtol( &( input[ 1 ] ), NULL, 0 );
                            select_lines_move_down_n( sl, cnt );
                            break;

                        case '-':
                            cnt = strtol( &( input[ 1 ] ), NULL, 0 );
                            select_lines_move_up_n( sl, cnt );
                            break;

                        default:
                            cnt = strtol( &( input[ 0 ] ), NULL, 0 );
                            if ( cnt > ( sl->lv->curline + 1 ) ) {
                                select_lines_move_down_n( sl, cnt - ( sl->lv->curline + 1 ) );
                            } else {
                                select_lines_move_up_n( sl, ( sl->lv->curline + 1 ) - cnt );
                            }
                            break;
                    }
                }
                break;

            case 'G':
                sl->lv->curline = lines_view_count( sl->lv ) - 1;
                if ( screen_status_line( sl ) > lines_view_count( sl->lv ) ) {
                    sl->lv->firstline = 0;
                } else {
                    sl->lv->firstline = sl->lv->curline - screen_status_line( sl ) + 1;
                }
                break;

            case 's':
                select_lines_set_mark( sl );
                break;

            case 'r':
                select_lines_reset_mark( sl );
                break;

            case 't':
                select_lines_toggle_mark( sl );
                break;

            case 'S':
                for ( pl_pos_t i = 0; i < lines_view_count( sl->lv ); i++ ) {
                    line = lines_view_ref( sl->lv, i );
                    line->marked = pl_true;
                }
                break;

            case 'R':
                for ( pl_pos_t i = 0; i < lines_view_count( sl->lv ); i++ ) {
                    line = lines_view_ref( sl->lv, i );
                    line->marked = pl_false;
                }
                break;

            case 'T':
                for ( pl_pos_t i = 0; i < lines_view_count( sl->lv ); i++ ) {
                    line = lines_view_ref( sl->lv, i );
                    line->marked = !line->marked;
                }
                break;

            case 'c':
                res = prompt_user( sl, "count (+/- #): ", &user_buf );
                input = plss_string( &user_buf );

                if ( res == tm_prompt_ok ) {
                    pl_pos_t cnt;
                    void ( *set_func )( select_lines_t sl );

                    switch ( input[ 0 ] ) {

                        case '+':
                            cnt = strtol( &( input[ 1 ] ), NULL, 0 );
                            set_func = select_lines_set_mark;
                            break;

                        case '-':
                            cnt = strtol( &( input[ 1 ] ), NULL, 0 );
                            set_func = select_lines_reset_mark;
                            break;

                        default:
                            cnt = strtol( &( input[ 0 ] ), NULL, 0 );
                            set_func = select_lines_toggle_mark;
                            break;
                    }

                    for ( pl_pos_t i = 0; i < cnt; i++ ) {
                        set_func( sl );
                        select_lines_move_down( sl );
                    }
                }
                break;

            case 'm':
            case 'M': {
                res = prompt_user( sl, "pattern: ", &user_buf );
                input = plss_string( &user_buf );
                if ( res == tm_prompt_ok ) {
                    select_lines_mark_matching( sl, input, ( key == 'm' ) );
                }
                break;
            }

            case 'f':
            case 'F': {
                res = prompt_user( sl, "pattern: ", &user_buf );
                input = plss_string( &user_buf );
                if ( res == tm_prompt_ok ) {
                    select_lines_display( sl );
                    select_lines_find_interactive( sl, input, ( key == 'f' ) );
                }
                break;
            }

            case 'l':
                select_lines_center_view( sl );
                break;

            case 'v':
                select_lines_view_commands( sl );
                break;

            case 'i': {
                /* Inspect the file at cursor and ensure that it constains
                   ASCII text. */
                select_line_t line;

                line = lines_view_ref( sl->lv, sl->lv->curline );
                plss_reformat_string( &work_buf, "file %s | grep -q \"ASCII text\"", line->text );

                if ( system( plss_string( &work_buf ) ) == 0 ) {
                    show_file_content( sl, line->text );
                }
                break;
            }

            case 'h':
                show_command_help( sl );
                break;

            default:
                break;
        }

        if ( done ) {
            break;
        }
    }

    return execute;
}


/**
 * Callback for terminal window resizing.
 *
 * @param data Callback context (i.e. screen_win_resize_context).
 */
static pl_none termal_resize_callback( termal_t termal, pl_t user_data )
{
    select_lines_t sl;

    sl = user_data;
    sl->screen_dims = termal_dimensions( termal );
    normalize_lines_view( sl );
    select_lines_display( sl );
}


/**
 * Prepare screen and interact with the user.
 *
 * @param sl Select_lines object.
 * @return True if the selection should be used for execution.
 */
static pl_bool_t setup_and_interact( select_lines_t sl )
{
    /* Prepare the terminal window content. */

    /* Handle all signals, i.e. just revert back to line output. */
    for ( int i = SIGHUP; i < SIGTERM; i++ ) {
        signal( i, take_sig_handler );
    }

    tm = termal_open( td_point_make( 20, 4 ) );

    termal_register_callbacks( &tm, NULL, termal_resize_callback, sl );
    termal_resize_callback( &tm, sl );

    pl_bool_t execute;
    execute = interaction( sl );

    termal_close( &tm );

    return execute;
}


/**
 * Pre-select all lines.
 *
 * @param sl Select_lines object.
 */
static void select_lines_presel_all( select_lines_t sl )
{
    select_line_t line;
    for ( pl_pos_t i = 0; i < lines_view_count( sl->lv ); i++ ) {
        line = lines_view_ref( sl->lv, i );
        line->marked = pl_true;
    }
}


/**
 * Pre-select (toggle) all lines in list.
 *
 * @param sl Select_lines object.
 * @param list List of lines as strings.
 */
static void select_lines_presel_listed( select_lines_t sl, char** list )
{
    pl_pos_t      idx;
    select_line_t line;

    for ( int i = 0; list[ i ]; i++ ) {
        idx = (pl_pos_t)( strtol( list[ i ], NULL, 0 ) - 1 );
        if ( idx < lines_view_count( sl->lv ) ) {
            line = lines_view_ref( sl->lv, idx );
            line->marked = !line->marked;
        }
    }
}


/** presel_file parser enum. */
enum number_fsm_e
{
    find_first,
    find_end
};


/**
 * Pre-select (toggle) all lines listed in filename. Any space
 * character can be separator between number.
 *
 * @param sl Select_lines object.
 * @param filename File containing lines to select.
 */
static void select_lines_presel_file( select_lines_t sl, const char* filename )
{
    FILE*             fh;
    int               ch;
    enum number_fsm_e fsm;

    fh = fopen( filename, "r" );
    if ( !fh ) {
        take_fatal( "Could not open output file: %s", filename );
    }

    /*
     * Implement presel_file parsing as FSM. The spaces between numbers
     * can be any whitespace (SPACE or NEWLINE etc).
     */

    plcm_reset( &work_buf );

    fsm = find_first;
    ch = fgetc( fh );

    for ( ;; ) {

        switch ( fsm ) {

            case find_first:

                /* Find first char of number. */

                if ( isdigit( ch ) ) {
                    /* Eat out spaces. */
                    fsm = find_end;
                } else {
                    ch = fgetc( fh );
                }

                break;


            case find_end:

                /* Collect chars until space or EOF is reached. */

                {
                    select_line_t line;
                    pl_pos_t      idx;

                    if ( isdigit( ch ) ) {
                        plss_append_char( &work_buf, (char)ch );
                    } else {
                        /* Mark line with found number. */
                        idx = strtol( plss_string( &work_buf ), NULL, 0 ) - 1;
                        plcm_reset( &work_buf );
                        if ( idx < lines_view_count( sl->lv ) ) {
                            line = lines_view_ref( sl->lv, idx );
                            line->marked = !line->marked;
                        }

                        fsm = find_first;
                    }

                    ch = fgetc( fh );
                }
                break;

            default:
                break;
        }

        if ( ch == EOF ) {
            break;
        }
    }

    fclose( fh );
}


/**
 * Main function sections:
 * - Process command line arguments.
 * - Create selection list.
 * - Optional preselection.
 * - Initialize screen and interact.
 * - Execute selection (or similar).
 *
 * @param argc Arg count.
 * @param argv Arg list content.
 *
 * @return Program return value.
 */
int main( int argc, char** argv )
{
    como_opt_t opt;
    char       header[ 128 ];

    /* clang-format off */
    como_maincmd(
        "take",
        "Tero Isannainen",
        "2015, 2025",
        { COMO_EXC_SWITCH,
          "doc", "-d",
                 "Documentation (quick guide)." },
        { COMO_OPT_SINGLE,
          "input", "-i",
                   "Input list generation command." },
        { COMO_OPT_ANY,
          "list", "-l",
                  "Directory listing as input (default: <curdir>)." },
        { COMO_OPT_SINGLE,
          "command", "-c",
                     "Output processing command. Display selection if not given." },
        { COMO_OPT_SINGLE,
          "auto", "-a",
                  "Current dir entries as input and execute <auto>." },
        { COMO_OPT_ANY,
          "join", "-j",
                  "Join selection with <join> (default <join>: \" \")." },
        { COMO_SWITCH,
          "presel", "-p",
                    "Preselect all." },
        { COMO_OPT_MULTI,
          "presel_list", "-pl",
                         "Preselect listed lines (1..n)." },
        { COMO_OPT_SINGLE,
          "presel_file", "-pf",
                         "Preselect listed lines from <presel_file>." },
        { COMO_SWITCH,
          "batch", "-b",
                   "Batch mode (requires preselection)." },
        { COMO_SWITCH,
          "selected", "-s",
                      "Show selected line number at exit." },
        { COMO_OPT_ANY,
          "no_exec", "-x",
                     "No execution, output command(s) to <no_exec> (default: stdout)." }
        );
    /* clang-format on */

    sprintf( header, "\n  take - List Selector v%s\n\n", take_version );
    como_conf_header( header );

    como_finish();

    if ( ( opt = como_given( "doc" ) ) ) {
        cli_documentation();
    }


    /* Setup the base allocator for a 1MB node. */
    plam_new( &balloc, balloc_node_size );


    /* Lines container. */
    select_lines_s sl;
    lines_view_s   lv;

    lines_view_new( &lv, &balloc, 8192 );
    select_lines_init( &sl, &lv, &balloc );

    /* Initialize common string (line content) buffers. */
    plcm_new( &line_buf, 8192 );
    plcm_new( &work_buf, 2048 );


    if ( ( opt = como_given( "list" ) ) ) {
        if ( opt->valuecnt > 0 ) {
            /* User specified directory to list. */
            list_from_dir( &sl, opt->value[ 0 ] );
        } else {
            list_from_dir( &sl, "." );
        }
    } else if ( como_given( "auto" ) ) {
        list_from_dir( &sl, "." );
    } else if ( ( opt = como_given( "input" ) ) ) {
        list_from_command( &sl, opt->value[ 0 ] );
    } else {
        /* Input from stdin. */
        if ( !isatty( fileno( stdin ) ) ) {
            list_from_stdin( &sl );
        }
    }


    if ( plcm_used_ptr( &sl.lv->lines ) == 0 ) {
        take_fatal( "No input for Take" );
    }


    if ( como_given( "presel" ) ) {
        select_lines_presel_all( &sl );
    }

    if ( ( opt = como_given( "presel_list" ) ) ) {
        select_lines_presel_listed( &sl, opt->value );
    }

    if ( ( opt = como_given( "presel_file" ) ) ) {
        select_lines_presel_file( &sl, opt->value[ 0 ] );
    }


    pl_bool_t execute;

#ifdef TAKE_SKIP_INTERACTION

    /* Skipping interaction helps terminal based debugging. */
    execute = pl_true;

#else

    if ( !como_given( "batch" ) ) {
        execute = setup_and_interact( &sl );
    } else {
        execute = pl_true;
    }

#endif


    if ( !execute ) {
        /* Selection execution skipped. */
        take_exit( EXIT_FAILURE );
    }


    /* Use the user selection for output. */

    FILE* no_exec_fh;

    no_exec_fh = NULL;
    opt = como_opt( "no_exec" );
    if ( opt->given ) {
        if ( opt->valuecnt > 0 ) {
            no_exec_fh = fopen( opt->value[ 0 ], "w" );
            if ( !no_exec_fh ) {
                take_fatal( "Could not open output file: %s", opt->value[ 0 ] );
            }
        } else {
            no_exec_fh = stdout;
        }
    }


    if ( como_given( "selected" ) ) {

        FILE* fh;

        if ( como_given( "no_exec" ) ) {
            fh = no_exec_fh;
        } else {
            fh = stdout;
        }

        select_line_t line;
        for ( pl_pos_t i = 0; i < plcm_used_ptr( &sl.lv->lines ); i++ ) {
            line = lines_view_ref( sl.lv, i );
            if ( line->marked ) {
                fprintf( fh, "%ld\n", ( i + 1 ) );
            }
        }

        if ( fh != stdout ) {
            fclose( fh );
        }

        take_exit( EXIT_SUCCESS );
    }


    /* Execute selection using command(s). */
    lines_view_s cmds;

    select_lines_create_commands( &sl, &cmds, &balloc );

    for ( select_line_t line = plcm_data( &cmds.lines ); (pl_t)line < plcm_end( &cmds.lines );
          line++ ) {
        execute_cmd( line->text, no_exec_fh );
    }

    /* Close files. */
    if ( no_exec_fh && no_exec_fh != stdout ) {
        fclose( no_exec_fh );
    }

    take_exit( EXIT_SUCCESS );
}
