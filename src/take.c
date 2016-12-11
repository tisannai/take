/**
 * @file take.c
 *
 * @mainpage
 *
 * List Selection Utility by Tero Isannainen, (c) Copyright 2015.
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

#include "config.h"

#ifdef HAVE_VFORK
# define _DEFAULT_SOURCE
#endif


#include "mc.h"

#include <dirent.h>
#include <regex.h>
#include <como.h>

#include "mcc.h"
#include "mcs.h"
#include "mcp.h"
#include "global.h"
#include "screen.h"
#include "prompt.h"

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <sys/wait.h>


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


/** Line index type with possibility to have negative indeces (for
    calculation, i.e. not to be used for indexing. */
typedef int64_t line_index_t;


/** Selectable line type. NOTE: Also used for help and command view. */
typedef struct select_line_s
{
  char* text;      /**< Line content. */
  bool_t marked;   /**< Selected flag. */
} select_line_t;


/** Collection of selectable lines with viewing info. */
typedef struct select_lines_s
{
  mcp_p lines;              /**< Line container. */
  line_index_t firstline;   /**< First visible line index. */
  line_index_t curline;     /**< Current line index. */
  win_info* list_wi;        /**< Screen window ref. */
  prompt_t* prompt;         /**< Prompt ref. */
  prompt_t* line_status;    /**< Line mode Status. */
  prompt_t* find_status;    /**< Find mode Status. */
} select_lines_t;



/*
 * Allocation that require cleanup before exit (no memleaks).
 */

/** Lines container. */
static select_lines_t* sl = NULL;

/** Command list. */
static select_lines_t* cmds = NULL;



/** Arbitrary length string buffer. Used from multiple functions. */
static mcc_p strbuf = NULL;



/** Default breakpoint. */
void gdb_break( void ) { return; }


/* Forward decl. */
select_lines_t* select_lines_rem( select_lines_t* sl );



/**
 * De-allocate heap memory.
 * 
 */
void mem_cleanup( void )
{
  if ( strbuf )
    mcc_del( strbuf );

  if ( sl )
    select_lines_rem( sl );

  if ( cmds )
    select_lines_rem( cmds );

#ifdef ml_do_debug
  ml_dbug_close();
#endif

}


/**
 * Exit program.
 * 
 */
void take_exit( int status )
{
  /* Cleanup everything. */

  mem_cleanup();

  if ( si != NULL )
    {
      si = screen_close( si );
    }
  como_end();
  exit( status );
}


/**
 * General purpose signal handler for Take. Always close the screen.
 *
 * @param signo Signal number (not used).
 */
void take_sig_handler( int signo )
{
  take_exit( EXIT_FAILURE );
}


/**
 * Fatal error printout.
 * 
 * @param format Format string for message.
 */
void take_fatal( const char* format, ... )
{
  va_list ap;
  va_start( ap, format );
  mc_msg_with_prefix( stderr, "Take FATAL: ", format, ap );
  va_end( ap );
  take_exit( EXIT_FAILURE );
}


/**
 * Fatal error printout.
 * 
 * @param format Format string for message.
 */
void take_error( const char* format, ... )
{
  va_list ap;
  va_start( ap, format );
  mc_msg_with_prefix( stderr, "Take ERROR: ", format, ap );
  va_end( ap );
}


/**
 * Create new line object.
 *
 * @param text Line content.
 *
 * @return Line object.
 */
select_line_t* select_line_new( char* text )
{
  select_line_t* ret;

  ret = mc_new( select_line_t );
  ret->text = text;
  ret->marked = mc_false;

  return ret;
}


/**
 * Create Select_lines, i.e. lines container.
 *
 * @return Line container.
 */
select_lines_t* select_lines_new( void )
{
  select_lines_t* ret;

  ret = mc_new( select_lines_t );

  ret->lines = mcp_new();
  ret->firstline = 0;
  ret->curline = 0;

  ret->list_wi = NULL;
  ret->prompt = NULL;
  ret->line_status = NULL;
  ret->find_status = NULL;

  return ret;
}


/**
 * Free Select_lines.
 *
 * @param sl Select_lines object.
 *
 * @return NULL.
 */
select_lines_t* select_lines_rem( select_lines_t* sl )
{
  select_line_t* line;

  mc_for_n( sl->lines->used )
    {
      line = sl->lines->data[i];
      mc_free( line->text );
      mc_free( line );
    }
  mcp_del( sl->lines );
  mc_free( sl );
  return NULL;
}


/**
 * Update line_status label with current line number.
 *
 * @param sl Select_lines object.
 */
void line_status_update( select_lines_t* sl )
{

  /* Create line number display. */
  mcc_reset( strbuf );
  mcc_printf( strbuf, "%*d",
              screen_win_x_size( sl->line_status->wi ),
              (int)sl->curline + 1 );

  /* Cut from left the part that does not fit into the field. */
  int overflow = strbuf->used - screen_win_x_size( sl->line_status->wi );
  if ( overflow > 0 )
    {
      mcc_delete_n_at( strbuf, 0, overflow );
      mcc_assign_to( strbuf, 0, "*", 1 );
    }

  prompt_label( sl->line_status, (char*) mcc_to_str( strbuf ) );

}


/**
 * Display Select_lines on screen. Marked lines have '*' in front of
 * the text.
 *
 * @param sl Select_lines object.
 */
void select_lines_display( select_lines_t* sl )
{
  select_line_t* line;
  win_info* wi = sl->list_wi;

  line_status_update( sl );
  prompt_refresh( sl->line_status );
  prompt_refresh( sl->find_status );

  if ( !prompt_interacting( sl->prompt ) )
    prompt_refresh( sl->prompt );

  screen_clear_win( wi );

  /* Show all visible lines or upto end of list. */
  for ( int i = WI_Y_MIN(wi);
        i < WI_Y_SIZE(wi) &&
          ( sl->firstline + i ) < sl->lines->used;
        i++ )
    {
      line = mcc_nth( sl->lines, sl->firstline + i );
      mcc_reset( strbuf );

#ifdef ENABLE_MARK_COLOR

      mcc_printf( strbuf, "%s", line->text );
      screen_setpos( wi, 0, i );
      if ( line->marked )
        screen_set_color_str( wi, (char*) mcc_to_str( strbuf ), SCR_COLOR_RED );
      else
        screen_set_color_str( wi, (char*) mcc_to_str( strbuf ), SCR_COLOR_DEFAULT );

# else
      if ( line->marked )
        mcc_printf( strbuf, "* %s", line->text );
      else
        mcc_printf( strbuf, "  %s", line->text );

      screen_setpos( wi, 0, i );
      screen_set_str2( wi, (char*) mcc_to_str( strbuf ) );
#endif

    }

  screen_setpos( wi, 0, sl->curline - sl->firstline );

  if ( prompt_interacting( sl->prompt ) )
    prompt_refresh( sl->prompt );

  screen_refresh( wi );
}


/**
 * Toggle "marked" on current line.
 *
 * @param sl Select_lines object.
 */
void select_lines_toggle_mark( select_lines_t* sl )
{
  select_line_t* line;
  line = mcc_nth( sl->lines, sl->curline );
  if ( line->marked )
    line->marked = mc_false;
  else
    line->marked = mc_true;
}


/**
 * Set "marked" attribute to selected value.
 *
 * @param sl Select_lines object.
 * @param marked Value for marked status.
 */
void select_lines_set_mark_to( select_lines_t* sl, bool_t marked )
{
  select_line_t* line;
  line = mcc_nth( sl->lines, sl->curline );
  line->marked = marked;
}


/**
 * Set item selected.
 *
 * @param sl Select_lines object.
 */
void select_lines_set_mark( select_lines_t* sl )
{
  select_lines_set_mark_to( sl, mc_true );
}


/**
 * Set item non-selected.
 *
 * @param sl Select_lines object.
 */
void select_lines_reset_mark( select_lines_t* sl )
{
  select_lines_set_mark_to( sl, mc_false );
}


/**
 * Move n step down on line list.
 *
 * @param sl Select_lines object.
 * @param n Move count.
 *
 * @return Number of step performed.
 */
int select_lines_move_down_n( select_lines_t* sl, int n )
{
  int i;
  win_info* wi = sl->list_wi;

  for ( i = 0; i < n; i++ )
    {
      if ( sl->curline < sl->lines->used-1 )
        {
          if ( screen_at_win_y_end( wi ) )
            {
              sl->firstline++;
            }
          else
            {
              wi->y++;
            }
          sl->curline++;
        }
      else
        {
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
bool_t select_lines_move_down( select_lines_t* sl )
{
  if ( select_lines_move_down_n( sl, 1 ) == 1 )
    return mc_true;
  else
    return mc_false;
}


/**
 * Move n step up on line list.
 *
 * @param sl Select_lines object.
 * @param n Move count.
 *
 * @return Number of step performed.
 */
int select_lines_move_up_n( select_lines_t* sl, int n )
{
  int i;
  win_info* wi = sl->list_wi;

  for ( i = 0; i < n; i++ )
    {
      if ( sl->curline > 0 )
        {
          if ( screen_at_win_y_start( wi ) )
            {
              sl->firstline--;
            }
          else
            {
              wi->y--;
            }
          sl->curline--;
        }
      else
        {
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
bool_t select_lines_move_up( select_lines_t* sl )
{
  if ( select_lines_move_up_n( sl, 1 ) == 1 )
    return mc_true;
  else
    return mc_false;
}


/**
 * Create list content from shell command.
 *
 * @param sl Select_lines object.
 * @param cmd Shell command.
 */
void list_from_command( select_lines_t* sl, char* cmd )
{
  FILE* fh;
  char* text;
  ssize_t ret;
  size_t linelen;
  select_line_t* line;

  fh = popen( cmd, "r" );

  if ( !fh )
    take_fatal( "Could not execute: %s", cmd );

  mc_loop
    {
      text = NULL;
      linelen = 0;
      ret = getline( &text, &linelen, fh );
      if ( ret != -1 )
        {
          dbug( "getline org text: \"%s\"\n", text );
          /* Remove newline. */
          if ( ret > 0 && text[ ret-1 ] == '\n' )
            text[ ret-1 ] = 0;
          dbug( "getline mod text: \"%s\"\n", text );
          line = select_line_new( text );
          mcp_append( sl->lines, line );
        }
      else
        {
          /* End of input. */
          free( text );
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
int list_sort_cmp( const void* a, const void* b )
{
    const char **ia = (const char **) a;
    const char **ib = (const char **) b;
    return strcmp( *ia, *ib );
}


/**
 * Create line content from current directory entries in ascending
 * order. "." and ".."  are not used.
 *
 * @param sl Select_lines object.
 * @param dirname Directory name for listing.
 */
void list_from_dir( select_lines_t* sl, char* dirname )
{
  DIR* dir;
  struct dirent* entry;

  dir = opendir( dirname );

  if ( dir )
    {
      mcp_p unsorted = mcp_new_size( 128 );
      select_line_t* line;
      char* tmp;

      while ( ( entry = readdir( dir ) ) != NULL )
        {
          /* Skip pwd and upper dir. */
          if ( strcmp( ".", entry->d_name ) &&
               strcmp( "..", entry->d_name ) )
            {
              tmp = mc_strdup( entry->d_name );
              mcp_append( unsorted, tmp );
            }
        }

      closedir( dir );

      /* Sort entries into alphabetical order. */
      qsort( (void*) unsorted->data,
             unsorted->used,
             mcp_sizeof,
             list_sort_cmp );

      /* Add sorted entries to lines list. */
      for ( int i = 0; i < unsorted->used; i++ )
        {
          line = select_line_new( mcp_nth( unsorted, i ) );
          mcp_append( sl->lines, line );
        }

      mcp_del( unsorted );
    }
}


/**
 * Create line content by reading stdin (piped input).
 *
 * @param sl Select_lines object.
 */
void list_from_stdin( select_lines_t* sl )
{
  FILE* fh = stdin;
  char* text;
  ssize_t ret;
  size_t linelen;
  select_line_t* line;

  mc_loop
    {
      text = NULL;
      linelen = 0;
      ret = getline( &text, &linelen, fh );
      if ( ret != -1 )
        {
          /* Remove newline. */
          if ( ret > 0 && text[ ret-1 ] == '\n')
            text[ ret-1 ] = 0;
          line = select_line_new( text );
          mcp_append( sl->lines, line );
        }
      else
        {
          /* End of input. */
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
void process_cmd_escapes( char* cmd, char* arg, mcc_p buf )
{
  /* Char index for cmd string. */
  int ci;

  mcc_reset( buf );

  ci = 0;

  mc_loop
    {
      if ( cmd[ ci ] == 0 )
        /* End of command. */
        break;

      if ( cmd[ ci ] == '@' )
        {
          if ( cmd[ ci+1 ] == '_' )
            {
              /* Literal @ to output. */
              mcc_append( buf, '@' );
              ci += 2;
            }
          else
            {
              /* Replace with arg. */
              mcc_append_n( buf, arg, strlen( arg ) );
              ci++;
            }
        }
      else
        {
          /* Copy to output. */
          mcc_append( buf, cmd[ ci ] );
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
void execute_cmd( const char* cmd, FILE* fh )
{
  if ( fh )
    {
      fputs( cmd, fh );
      fputc( '\n', fh );
    }
  else
    {

#ifdef HAVE_VFORK

      /* When take includes a long list, a lot of memory is
         allocated. vfork does not duplicate the memory allocation and
         thus system (execl) calls start significantly faster. */

      pid_t child;
      child = vfork();
      if ( child == 0 )
        {
          execl( "/bin/sh", "sh", "-c", cmd, (char*) 0 );
        }
      else if ( child == -1 )
        {
          /* Failure. */
          take_error( "Could not execute: \"%s\"\n  reason: \"%s\"",
                      cmd,
                      strerror( errno )
                      );
        }
      else
        {
          int status;
          waitpid( child, &status, 0 );
        }

# else

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
void select_lines_view( select_lines_t* sl, select_lines_t* view_sl )
{
  win_info* wi;

  /* Current view list for window resize purposes. */
  screen_win_resize_context = view_sl;

  /* Copy window state. */
  view_sl->list_wi = sl->list_wi;
  view_sl->prompt = sl->prompt;
  view_sl->line_status = sl->line_status;
  view_sl->find_status = sl->find_status;
  wi = view_sl->list_wi;


  /* Display all lines. */
  select_lines_display( view_sl );


  int key;
  bool_t done = mc_false;

  /* Get user input. */
  mc_loop
    {
      key = screen_get_key();

      switch ( key )
        {

        case ESC:
        case CTRL_G:
        case NEWLINE:
          done = mc_true;
          break;

        case 'n':
          if ( wi->y != WI_Y_MAX( wi ) )
            select_lines_move_down_n( view_sl, WI_Y_MAX( wi ) - wi->y );
          else
            select_lines_move_down_n( view_sl, WI_Y_SIZE( wi ) );
          break;

        case 'p':
          if ( wi->y != 0 )
            select_lines_move_up_n( view_sl, wi->y );
          else
            select_lines_move_up_n( view_sl, WI_Y_SIZE( wi ) );
          break;


        default:
          break;
        }

      if ( done )
        break;

      select_lines_display( view_sl );

    }

  /* Revert back to original. */
  screen_win_resize_context = sl;

}


/**
 * Show command help on screen.
 *
 * NOTE: Keep the help list in sync with the manual page.
 *
 * @param sl Select_lines object.
 */
void show_command_help( select_lines_t* sl )
{
  char* help_list[ ] = {
    "\"J\": Toggle selection and move down",
    "\"K\": Toggle selection and move up",
    "\"j\": Move down",
    "\"k\": Move up",
    "\"n\": Move down page",
    "\"p\": Move up page",
    "\"b\": Move to beginning of list",
    "\"e\": Move to end of list",
    "\"g\": Goto to line",
    "\"s\": Select current",
    "\"r\": Reject current,"
    "\"t\": Toggle current",
    "\"S\": Select all items",
    "\"R\": Reject all items",
    "\"T\": Toggle all items",
    "\"c\": Toggle the next \"count\" items",
    "\"m\": Select items matching the prompted regexp (case insensitive)",
    "\"M\": Select items matching the prompted regexp (case sensitive)",
    "\"f\": Find mode with case sensitive matching (Keys: j,k,s,r,t,RET,ESC)",
    "\"F\": Find mode with case insensitive matching (Keys: j,k,s,r,t,RET,ESC)",
    "\"v\": View the list of commands that would be executed",
    "\"i\": View the current list entry content (if a text file)",
    "\"l\": Center list view on screen around current line",
    "\"h\": Show command help",
    "\"x\": Quit and execute output-command for selection",
    "\"q\": Quit and skip output-command execution",
    NULL
  };

  select_lines_t* help_sl;

  help_sl = select_lines_new();

  select_line_t* line;
  for ( int i = 0; help_list[i]; i++ )
    {
      line = select_line_new( mc_strdup( help_list[i] ) );
      mcp_append( help_sl->lines, line );
    }

  select_lines_view( sl, help_sl );

  select_lines_rem( help_sl );
}


/**
 * Show file content on screen.
 *
 * @param sl Select_lines object.
 * @param filename Filename for file to display.
 */
void show_file_content( select_lines_t* sl, char* filename )
{
  select_lines_t* file_sl;
  FILE* fh;

  fh = fopen( filename, "r" );
  if ( !fh )
    /* Can't open file, abort. */
    return;

  file_sl = select_lines_new();

  select_line_t* line;
  ssize_t ret;
  char* text;
  size_t linelen;

  mc_loop
    {
      text = NULL;
      linelen = 0;
      ret = getline( &text, &linelen, fh );

      if ( ret != -1 )
        {
          /* Remove newline. */
          if ( ret > 0 && text[ ret-1 ] == '\n')
            text[ ret-1 ] = 0;
          line = select_line_new( text );
          mcp_append( file_sl->lines, line );
        }
      else
        {
          /* End of input. */
          break;
        }
    }

  select_lines_view( sl, file_sl );
  select_lines_rem( file_sl );
}


/**
 * Create a list of shell command executions for selected items.
 *
 * @param sl Select_lines object.
 *
 * @return Command list.
 */
select_lines_t* select_lines_create_commands( select_lines_t* sl )
{
  select_lines_t* cmds;
  como_opt_t* opt;
  char* command = NULL;

  cmds = select_lines_new();

  if ( ( opt = como_given( "command" ) ) )
    command = opt->value[0];

  if ( ( opt = como_given( "auto" ) ) )
    command = opt->value[0];


  if ( !command )
    /* No command to use, so use the default command. */
    command = "echo @";



  if ( (opt = como_given( "join" ) ) )
    {
      char* join_str;

      /* Join all selected items with join string. */

      if ( opt->valuecnt > 0 )
        join_str = opt->value[ 0 ];
      else
        join_str = " ";

      select_line_t* line;
      bool_t nonfirst = mc_false;

      mcc_reset( strbuf );
      for ( line_index_t i = 0; i < sl->lines->used; i++ )
        {
          line = mcp_nth( sl->lines, i );
          if ( line->marked )
            {
              if ( nonfirst )
                mcc_append_n( strbuf, join_str, strlen( join_str ) );
              else
                nonfirst = mc_true;

              mcc_append_n( strbuf, line->text, strlen( line->text ) );
            }
        }

      char* tmpstr;

      /* Take of strbuf before its overwritten. */
      tmpstr = mc_strdup( mcc_to_str( strbuf ) );

      /* Create command. */
      process_cmd_escapes( command, tmpstr, strbuf );

      /* Free the copy. */
      mc_free( tmpstr );

      line = select_line_new( mc_strdup( mcc_to_str( strbuf ) ) );
      mcp_append( cmds->lines, line );
    }
  else
    {

      /* Create command for each selected item individually. */

      select_line_t* line;
      for ( line_index_t i = 0; i < sl->lines->used; i++ )
        {
          line = mcp_nth( sl->lines, i );
          if ( line->marked )
            {
              /* Create command. */
              process_cmd_escapes( command, line->text, strbuf );
              line = select_line_new( mc_strdup( mcc_to_str( strbuf ) ) );
              mcp_append( cmds->lines, line );
            }
        }

    }

  return cmds;
}


/**
 * Remove Regex object.
 *
 * @param re Regex object.
 */
void regex_rem( regex_t* re )
{
  if ( re )
    {
      regfree( re );
      mc_free( re );
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
regex_t* regex_new( char* pattern, bool_t case_sensitive )
{
  regex_t* re;
  int ret;

  re = mc_new( regex_t );

  if ( case_sensitive )
    ret = regcomp( re, pattern,
                   REG_EXTENDED | REG_NOSUB );
  else
    ret = regcomp( re, pattern,
                   REG_EXTENDED | REG_ICASE | REG_NOSUB );

  if ( ret != 0 )
    {
      /* Failure. */
      regex_rem( re );
      re = NULL;
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
void select_lines_mark_matching( select_lines_t* sl,
                                 char* pattern,
                                 bool_t case_sensitive )
{
  regex_t* re;

  re = regex_new( pattern, case_sensitive );

  if ( !re )
    {
      prompt_msg( sl->prompt, "Error in regexp!" );
      return;
    }

  select_line_t* line;
  for ( line_index_t i = 0; i < sl->lines->used; i++ )
    {
      line = mcp_nth( sl->lines, i );
      if ( regexec( re, line->text, 0, NULL, 0 ) == 0 )
        line->marked = mc_true;
    }

  regex_rem( re );
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
line_index_t select_lines_find_next( select_lines_t* sl, regex_t* re, bool_t forward )
{
  line_index_t offset, limit;

  if ( forward )
    {
      offset = 1;
      limit = sl->lines->used;
    }
  else
    {
      offset = -1;
      limit = -1;
    }

  select_line_t* line;
  line_index_t ret = 0;

  for ( line_index_t idx = sl->curline;
        idx != limit;
        idx = idx + offset )
    {
      line = mcp_nth( sl->lines, idx );
      if ( regexec( re, line->text, 0, NULL, 0 ) == 0 )
        return ret;
      ret++;
    }

  return -1;
}


/**
 * Save the line state from Select_lines object to another.
 *
 * @param from State origin.
 * @param to State target.
 */
void select_lines_save_position( select_lines_t* from, select_lines_t* to )
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
void select_lines_find_interactive( select_lines_t* sl,
                                    char* pattern,
                                    bool_t case_sensitive )
{
  select_lines_t org_sl;
  select_lines_t prev_sl;
  int key;
  bool_t done = mc_false;
  bool_t use_org = mc_false;
  regex_t* re;
  line_index_t offset;
  bool_t first_search = mc_true;

  /* Save search start position. */
  org_sl = *sl;
  prev_sl = *sl;

  re = regex_new( pattern, case_sensitive );

  if ( !re )
    {
      prompt_msg( sl->prompt, "Error in regexp!" );
      return;
    }

  prompt_label( sl->find_status, "F" );
  select_lines_display( sl );


  /* Get user input. */
  mc_loop
    {
      key = screen_get_key();

      switch ( key )
        {

        case ESC:
        case CTRL_G:
          done = mc_true;
          /* Revert back to original line. */
          use_org = mc_true;
          break;

        case NEWLINE:
          done = mc_true;
          /* Stay on line that had last match. */
          use_org = mc_false;
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
          select_lines_save_position( sl, &prev_sl );

          /* For first search also the current line is searched. */
          if ( first_search || select_lines_move_down( sl ) )
            {
              if ( ( offset = select_lines_find_next( sl, re, mc_true ) ) != -1 )
                /* Found new matching line. */
                select_lines_move_down_n( sl, offset );
              else
                /* Nothing found, revert back to search origin. */
                select_lines_save_position( &prev_sl, sl );
            }

          first_search = mc_false;

          break;


        case 'k':

          /* Same comments as for 'j' ... */
          select_lines_save_position( sl, &prev_sl );

          if ( first_search || select_lines_move_up( sl ) )
            {
              if ( ( offset = select_lines_find_next( sl, re, mc_false ) ) != -1 )
                select_lines_move_up_n( sl, offset );
              else
                select_lines_save_position( &prev_sl, sl );
            }

          first_search = mc_false;

          break;


        default:
          break;
        }

      if ( done )
        break;

      select_lines_display( sl );

    }

  if ( use_org )
    select_lines_save_position( &org_sl, sl );

  regex_rem( re );

  prompt_label( sl->find_status, NULL );

  select_lines_display( sl );
}


/**
 * Put current line in the center of the list view.
 *
 * @param sl Select_lines object.
 */
void select_lines_center_view( select_lines_t* sl )
{
  int half_cnt = WI_Y_SIZE(sl->list_wi) / 2;
  int dist = sl->curline - sl->firstline;
  int offset;
  int newfirst;

  offset = dist - half_cnt;
  newfirst = sl->firstline + offset;
  if ( newfirst >= 0 )
    {
      sl->firstline = newfirst;
    }
}


/**
 * View a list of commands that would be executed.
 *
 * @param sl Select_lines object.
 */
void select_lines_view_commands( select_lines_t* sl )
{
  select_lines_t* cmds_sl;

  cmds_sl = select_lines_create_commands( sl );

  if ( cmds_sl )
    {
      select_lines_view( sl, cmds_sl );
      select_lines_rem( cmds_sl );
    }
  else
    {
      prompt_msg( sl->prompt, "No command was given!" );
    }
}


/**
 * Interact with the user to get line selection.
 *
 * @param sl Select_lines object.
 *
 * @return True if the selection should be used for execution.
 */
bool_t interaction( select_lines_t* sl )
{
  int key;
  bool_t execute = mc_false;
  bool_t done = mc_false;
  select_line_t* line;

  win_info* wi = sl->list_wi;

  /* Display all lines. */
  select_lines_display( sl );

  /* Get user input. */
  mc_loop
    {
      key = screen_get_key();

      /* Clear pending user messages. */
      prompt_msg( sl->prompt, NULL );

      switch ( key )
        {

        case 'q':
          done = mc_true;
          execute = mc_false;
          break;

        case 'x':
          done = mc_true;
          execute = mc_true;
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
          if ( wi->y != WI_Y_MAX( wi ) )
            select_lines_move_down_n( sl, WI_Y_MAX( wi ) - wi->y );
          else
            select_lines_move_down_n( sl, WI_Y_SIZE( wi ) );
          break;

        case 'p':
          if ( wi->y != 0 )
            select_lines_move_up_n( sl, wi->y );
          else
            select_lines_move_up_n( sl, WI_Y_SIZE( wi ) );
          break;

        case 'b':
          select_lines_move_up_n( sl, sl->curline );
          break;

        case 'e':
          select_lines_move_down_n( sl, sl->lines->used - 1 - sl->curline );
          break;

        case 'g':
          {
            char* input;

            input = prompt_interact( sl->prompt, "goto (+/- #): " );

            if ( input )
              {
                line_index_t cnt;

                switch ( input[0] )
                  {

                  case '+':
                    cnt = strtol( &(input[1]), NULL, 0 );
                    select_lines_move_down_n( sl, cnt );
                    break;

                  case '-':
                    cnt = strtol( &(input[1]), NULL, 0 );
                    select_lines_move_up_n( sl, cnt );
                    break;

                  default:
                    cnt = strtol( &(input[0]), NULL, 0 );
                    if ( cnt > (sl->curline+1) )
                      select_lines_move_down_n( sl, cnt - (sl->curline+1) );
                    else
                      select_lines_move_up_n( sl, (sl->curline+1) - cnt );
                    break;
                  }
              }
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
          for ( line_index_t i = 0; i < sl->lines->used; i++ )
            {
              line = mcp_nth( sl->lines, i );
              line->marked = mc_true;
            }
          break;

        case 'R':
          for ( line_index_t i = 0; i < sl->lines->used; i++ )
            {
              line = mcp_nth( sl->lines, i );
              line->marked = mc_false;
            }
          break;

        case 'T':
          for ( line_index_t i = 0; i < sl->lines->used; i++ )
            {
              line = mcp_nth( sl->lines, i );
              line->marked = !line->marked;
            }
          break;

        case 'c':
          {
            char* input;

            input = prompt_interact( sl->prompt, "count (+/- #): " );

            if ( input )
              {
                line_index_t cnt;
                void (*set_func) ( select_lines_t* sl );

                switch ( input[0] )
                  {

                  case '+':
                    cnt = strtol( &(input[1]), NULL, 0 );
                    set_func = select_lines_set_mark;
                    break;

                  case '-':
                    cnt = strtol( &(input[1]), NULL, 0 );
                    set_func = select_lines_reset_mark;
                    break;

                  default:
                    cnt = strtol( &(input[0]), NULL, 0 );
                    set_func = select_lines_toggle_mark;
                    break;
                  }

                for ( line_index_t i = 0; i < cnt; i++ )
                  {
                    set_func( sl );
                    select_lines_move_down( sl );
                  }
              }
          }
          break;

        case 'm':
        case 'M':
          {
            char* input;

            input = prompt_interact( sl->prompt, "pattern: " );

            if ( input )
              {
                select_lines_mark_matching( sl, input, (key == 'M') );
                mc_free( input );
              }
          }
          break;

        case 'f':
        case 'F':
          {
            char* input;

            input = prompt_interact( sl->prompt, "pattern: " );

            if ( input )
              {
                select_lines_display( sl );
                select_lines_find_interactive( sl,
                                               input,
                                               (key=='F') );
                mc_free( input );
              }
          }
          break;

        case 'l':
          select_lines_center_view( sl );
          break;

        case 'v':
          select_lines_view_commands( sl );
          break;

        case 'i':
          {
            /* Inspect the file at cursor and ensure that it constains
               ASCII text. */

            select_line_t* line;

            line = mcp_nth( sl->lines, sl->curline );
            
            mcc_reset( strbuf );
            mcc_printf( strbuf, "file %s | grep -q \"ASCII text\"", line->text );

            if ( system( (char*) mcc_to_str( strbuf ) ) == 0 )
              {
                show_file_content( sl, line->text );
              }
          }
          break;

        case 'h':
          show_command_help( sl );
          break;

        default:
          break;
        }

      if ( done )
        break;

      select_lines_display( sl );

    }

  return execute;
}


/**
 * Callback for terminal window resizing.
 *
 * @param data Callback context (i.e. screen_win_resize_context).
 */
void win_resize_callback( void* data )
{
  select_lines_t* sl;
  sl = data;
  select_lines_display( sl );
}


/**
 * Prepare screen and interact with the user.
 *
 * @param sl Select_lines object.
 * @return True if the selection should be used for execution.
 */
bool_t setup_and_interact( select_lines_t* sl )
{
  /* Prepare the terminal window content. */

  win_info* prompt_wi;
  win_info* line_status_wi;
  win_info* find_status_wi;

  /* Handle all signals, i.e. just revert back to line output. */
  for ( int i = SIGHUP; i < SIGTERM; i++ )
    signal( i, take_sig_handler );
  
  si = screen_open();

  /* Set callbacks. */
  screen_win_resize_context = sl;
  screen_post_win_resize = win_resize_callback;
  screen_fatal_error = take_fatal;

  sl->list_wi = screen_open_window_geom( si, 0, 1, 0, 1, mc_false );


  /* Status display offset from window right towards left. */
  int find_status_field_pos = 4;
  int line_status_field_pos = find_status_field_pos + 10;

  prompt_wi = screen_open_window_geom( sl->list_wi->si,
                                       0,
                                       line_status_field_pos+1,
                                       -1,
                                       0,
                                       mc_false
                                       );
  sl->prompt = prompt_init( prompt_wi, NULL );
  
  line_status_wi = screen_open_window_geom( sl->list_wi->si,
                                            -line_status_field_pos,
                                            find_status_field_pos+1,
                                            -1,
                                            0,
                                            mc_false
                                            );
  sl->line_status = prompt_init( line_status_wi, NULL );
  
  find_status_wi = screen_open_window_geom( sl->list_wi->si,
                                            -find_status_field_pos,
                                            1,
                                            -1,
                                            0,
                                            mc_false
                                            );
  sl->find_status = prompt_init( find_status_wi, NULL );

  bool_t execute;
  execute = interaction( sl );

//  si = screen_close( si );
  gdb_break();

  prompt_close( sl->prompt );
  screen_close_window( prompt_wi );

  prompt_close( sl->line_status );
  screen_close_window( line_status_wi );

  prompt_close( sl->find_status );
  screen_close_window( find_status_wi );

  screen_close_window( sl->list_wi );

  si = screen_close( si );

  return execute;
}


/**
 * Pre-select all lines.
 *
 * @param sl Select_lines object.
 */
void select_lines_presel_all( select_lines_t* sl )
{
  select_line_t* line;
  for ( line_index_t i = 0; i < sl->lines->used; i++ )
    {
      line = mcp_nth( sl->lines, i );
      line->marked = mc_true;
    }
}


/**
 * Pre-select (toggle) all lines in list.
 * 
 * @param sl Select_lines object.
 * @param list List of lines as strings.
 */
void select_lines_presel_listed( select_lines_t* sl, char** list )
{
  line_index_t idx;
  select_line_t* line;

  for ( int i = 0; list[i]; i++ )
    {
      idx = (line_index_t) ( strtol( list[ i ], NULL, 0 ) - 1 );
      if ( idx < sl->lines->used )
        {
          line = mcp_nth( sl->lines, idx );
          line->marked = !line->marked;
        }
    }
}


/** presel_file parser enum. */
enum number_fsm_e { find_first, find_end };


/**
 * Pre-select (toggle) all lines listed in filename. Any space
 * character can be separator between number.
 * 
 * @param sl Select_lines object.
 * @param filename File containing lines to select.
 */
void select_lines_presel_file( select_lines_t* sl, char* filename )
{
  FILE* fh;
  int ch;
  enum number_fsm_e fsm = find_first;

  fh = fopen( filename, "r" );
  if ( !fh )
    take_fatal( "Could not open output file: %s", filename );
  
  /*
   * Implement presel_file parsing as FSM. The spaces between numbers
   * can be any whitespace (SPACE or NEWLINE etc).
   */

  mcc_reset( strbuf );

  ch = fgetc( fh );

  mc_loop
    {

      switch ( fsm )
        {

        case find_first:

          /* Find first char of number. */

          if ( isdigit( ch ) )
            /* Eat out spaces. */
            fsm = find_end;
          else
            ch = fgetc( fh );

          break;


        case find_end:

          /* Collect chars until space or EOF is reached. */

          {
            select_line_t* line;
            line_index_t idx;

            if ( isdigit( ch ) )
              {
                mcc_append( strbuf, (char) ch );
              }
            else
              {
                /* Mark line with found number. */
                idx = strtol( mcc_to_str( strbuf ), NULL, 0 ) - 1;
                mcc_reset( strbuf );
                if ( idx < sl->lines->used )
                  {
                    line = mcp_nth( sl->lines, idx );
                    line->marked = !line->marked;
                  }
                
                fsm = find_first;
              }

            ch = fgetc( fh );
          }
          break;
          
        default: break;
        }
      
      if ( ch == EOF )
        break;
    }
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
   como_opt_t* opt;
   char header[ 128 ];

   /* Define command line options. */
   como_maincmd( "take", "Tero Isannainen", "2015",
     { COMO_OPT_SINGLE, "input", "-i", "Input list generation command." },
     { COMO_OPT_ANY, "list", "-l", "Directory listing as input (default: <curdir>)." },
     { COMO_OPT_SINGLE, "command", "-c", "Output processing command. Display selection if not given." },
     { COMO_OPT_SINGLE, "auto", "-a", "Current dir entries as input and execute <auto>." },
     { COMO_OPT_ANY, "join", "-j", "Join selection with <join> (default <join>: \" \")." },
     { COMO_SWITCH, "presel", "-p", "Preselect all." },
     { COMO_OPT_MULTI, "presel_list", "-pl", "Preselect listed lines (1..n)." },
     { COMO_OPT_SINGLE, "presel_file", "-pf", "Preselect listed lines from <presel_file>." },
     { COMO_SWITCH, "batch", "-b", "Batch mode (requires preselection)." },
     { COMO_SWITCH, "selected", "-s", "Show selected line number at exit." },
     { COMO_OPT_ANY, "no_exec", "-x", "No execution, display/store command(s) to <no_exec> (default: stdout)." }
     );

   sprintf( header, "\n  take - List Selector v%s\n\n", VERSION );
   como_conf_header( header );

   como_finish();


#ifdef ml_do_debug
   ml_dbug_open( "debug.txt" );
#endif


  /* Lines container. */
  sl = select_lines_new();


  /* Initialize common string (line content) buffer. */
  strbuf = mcc_new_size( 16 );


  if ( ( opt = como_given( "list" ) ) )
    {
      if ( opt->valuecnt > 0 )
        /* User specified directory to list. */
        list_from_dir( sl, opt->value[ 0 ] );
      else
        list_from_dir( sl, "." );
    }
  else if ( como_given( "auto" ) )
    {
      list_from_dir( sl, "." );
    }
  else if ( ( opt = como_given( "input" ) ) )
    {
      list_from_command( sl, opt->value[0] );
    }
  else
    {
      /* Input from stdin. */
      if ( !isatty( fileno( stdin ) ) )
        list_from_stdin( sl );
    }


  if ( sl->lines->used == 0 )
    {
      take_fatal( "No input for Take" );
    }


  if ( como_given( "presel" ) )
    {
      select_lines_presel_all( sl );
    }

  if ( ( opt = como_given( "presel_list" ) ) )
    {
      select_lines_presel_listed( sl, opt->value );
    }

  if ( ( opt = como_given( "presel_file" ) ) )
    {
      select_lines_presel_file( sl, opt->value[0] );
    }


  bool_t execute = mc_false;

#ifdef TAKE_SKIP_INTERACTION

  /* Skipping interaction helps terminal based debugging. */
  execute = mc_true;

# else


  if ( !como_given( "batch" ) )
    {
      execute = setup_and_interact( sl );
    }
  else
    execute = mc_true;

#endif


  if ( !execute )
    {
      /* Selection execution skipped. */
      take_exit( EXIT_FAILURE );
    }


  /* Use the user selection for output. */

  FILE* no_exec_fh = NULL;
  opt = como_opt( "no_exec" );
  if ( opt->given )
    {
      if ( opt->valuecnt > 0 )
        {
          no_exec_fh = fopen( opt->value[0], "w" );
          if ( !no_exec_fh )
            {
              take_fatal( "Could not open output file: %s", opt->value[0] );
            }
        }
      else
        {
          no_exec_fh = stdout;
        }
    }


  if ( como_given( "selected" ) )
    {
      FILE* fh;

      if ( como_given( "no_exec" ) )
        fh = no_exec_fh;
      else
        fh = stdout;

      select_line_t* line;
      for ( line_index_t i = 0; i < sl->lines->used; i++ )
        {
          line = mcp_nth( sl->lines, i );
          if ( line->marked )
            {
              fprintf( fh, "%ld\n", (i+1) );
            }
        }

      if ( fh != stdout )
        fclose( fh );

      take_exit( EXIT_SUCCESS );
    }


  if ( !como_given( "command" ) && !como_given( "auto" ) )
    {
      if ( !no_exec_fh )
        no_exec_fh = stdout;
    }

  /* Execute selection using command(s). */
  select_line_t* line;

  cmds = select_lines_create_commands( sl );

  for ( line_index_t i = 0; i < cmds->lines->used; i++ )
    {
      line = mcp_nth( cmds->lines, i );
      execute_cmd( line->text, no_exec_fh );
    }

  /* Close files. */
  if ( no_exec_fh && no_exec_fh != stdout )
    fclose( no_exec_fh );

  take_exit( EXIT_SUCCESS );
}
