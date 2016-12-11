/**
 * @file prompt.c
 *
 * Prompting and message showing facility.
 *
 */


#include "mc.h"
#include "global.h"
#include "mcc.h"
#include "mcs.h"
#include "como.h"
#include "screen.h"
#include "prompt.h"


/** Screen x position. */
#define x_pos(p) ((p)->x0 + ((p)->bi - (p)->b0))


/**
 * Initialize Prompt object content.
 * 
 * @param wi Window ref.
 * @param user_prompt User prompt label (or NULL);
 * 
 * @return Prompt object.
 */
prompt_t* prompt_init( win_info* wi, char* user_prompt ) /*acfd*/
{
  prompt_t* p;

  p = mc_new( prompt_t );
  p->wi = wi;
  
  p->label = NULL;
  prompt_label( p, user_prompt );

  p->buf = NULL;

  return p;
}


/**
 * Free Prompt object.
 * 
 * @param p Prompt object.
 */
void prompt_close( prompt_t* p ) /*acfd*/
{
  screen_clear_win( p->wi );
  screen_refresh( p->wi );
  
  if ( p->buf )
    mcc_del( p->buf );
  mc_free( p->label );
  mc_free( p );
}


/**
 * Prepare user input buffer for input.
 * 
 * @param p Prompt object.
 */
void prompt_open_buffer( prompt_t* p ) /*acfd*/
{
  p->buf = mcc_new_size( 16 );
  p->b0 = 0;
  p->bi = 0;
}


/**
 * Free user input buffer.
 * 
 * @param p Prompt object.
 */
void prompt_close_buffer( prompt_t* p ) /*acfd*/
{
  if ( p->buf )
    p->buf = mcc_del( p->buf );
  p->b0 = 0;
  p->bi = 0;
}



/**
 * Set label for prompt or set message. Previous label string is
 * freed.
 * 
 * @param p Prompt object.
 * @param prompt Content for prompt-label or message.
 */
void prompt_label( prompt_t* p, char* prompt ) /*acfd*/
{
  if ( p->label )
    {
      mc_free( p->label );
      p->label = NULL;
    }

  if ( prompt )
    {
      p->x0 = strlen( prompt );
      p->label = mc_strdup( prompt );
    }
}


/**
 * Display user message.
 * 
 * @param p Prompt object.
 * @param msg Message to display.
 */
void prompt_msg( prompt_t* p, char* msg ) /*acfd*/
{
  prompt_label( p, msg );
  prompt_refresh( p );
}


/**
 * Refresh prompt content on screen, i.e. label and possible user
 * input.
 * 
 * @param p Prompt object.
 */
void prompt_refresh( prompt_t* p ) /*acfd*/
{

  screen_clear_win( p->wi );
  
  if ( p->label )
    {
      screen_setpos( p->wi, 0, 0 );
      screen_set_str2( p->wi, p->label );
    }

  if ( prompt_interacting( p ) )
    {
      const char* view;

      screen_setpos( p->wi, p->x0, 0 );
      view = mcc_to_str( p->buf );
      view += p->b0;
      screen_set_str2( p->wi, view );

      screen_setpos( p->wi, x_pos(p), 0 );
    }

  screen_refresh( p->wi );
}


/**
 * Return true if user interaction is ongoing.
 */
bool_t prompt_interacting( prompt_t* p ) /*acfd*/
{
  return ( p->buf != NULL );
}
  

/**
 * Interact using the prompt, i.e. get user input.
 * 
 * @param p Prompt object.
 * @param label Prompt-label.
 * 
 * @return Input given by the user.
 */
char* prompt_interact( prompt_t* p, char* label ) /*acfd*/
{
  bool_t done = false;
  int key;
  char* ret;

  prompt_label( p, label );
  prompt_open_buffer( p );

  /* Initial screen refresh. */
  prompt_refresh( p );
  
  /* Get user input. */
  for (;;)
    {
      key = screen_get_key();

      switch ( key )
        {

        case NEWLINE:
          done = true;
          ret = mc_strdup( mcc_to_str( p->buf ) );
          break;

        case ESC:
        case CTRL_G:
          done = true;
          ret = NULL;
          break;

        case CTRL_B:
          /* Backwards char. */
          if ( p->bi > 0 )
            {
              if ( p->wi->x <= p->x0 )
                {
                  p->b0--;
                  p->bi--;
                }
              else
                {
                  p->bi--;
                }
            }
          break;

        case CTRL_F:
          /* Forwards char. */
          if ( p->bi < p->buf->used )
            {
              if ( p->wi->x >= WI_X_MAX(p->wi) )
                {
                  p->b0++;
                  p->bi++;
                }
              else
                {
                  p->bi++;
                }
            }
          break;

        case CTRL_A:
          /* Beginning of line. */
          p->bi = 0;
          p->b0 = 0;
          break;

        case CTRL_E:
          /* End of line. */
          p->bi = p->buf->used;
          p->b0 = p->bi - WI_X_MAX(p->wi) + p->x0;
          if ( p->b0 < 0 )
            p->b0 = 0;
          break;

        case CTRL_D:
          /* Delete char. */
          if ( p->bi < p->buf->used )
            {
              mcc_delete_at( p->buf, p->bi );
            }
          break;

        case BS:
        case CTRL_H:
          /* Backspace char. */
          if ( p->bi > 0 )
            {
              if ( p->wi->x <= p->x0 )
                {
                  p->b0--;
                  p->bi--;
                }
              else
                {
                  p->bi--;
                }
              mcc_delete_at( p->buf, p->bi );
            }
          break;

        case CTRL_K:
          /* Kill line. */
          if ( p->bi < p->buf->used )
            {
              mcc_delete_n_at( p->buf, p->bi, p->buf->used - p->bi );
            }
          break;

        default:
          /* Add char. */
          if ( key >= 32 && key <= 126 )
            {
              mcc_insert_to( p->buf, p->bi, (char) key );
              if ( p->wi->x >= WI_X_MAX(p->wi) )
                {
                  p->b0++;
                  p->bi++;
                }
              else
                {
                  p->bi++;
                }
            }
          break;
        }

      if ( done )
        break;

      prompt_refresh( p );

    }

  dbug( "prompt ret: %s\n", ret );

  /* Cleanup. */
  prompt_close_buffer( p );
  prompt_label( p, NULL );

  prompt_refresh( p );

  return ret;
}

