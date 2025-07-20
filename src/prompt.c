/**
 * @file prompt.c
 *
 * Prompting and message showing facility.
 *
 */


#include <plinth.h>
#include <string.h>
#include "common.h"
#include "como.h"
#include "screen.h"
#include "prompt.h"


/** Screen x position. */
#define x_pos( p ) ( ( p )->x0 + ( ( p )->bi - ( p )->b0 ) )


/**
 * Initialize Prompt object content.
 *
 * @param wi Window ref.
 * @param user_prompt User prompt label (or NULL);
 *
 * @return Prompt object.
 */
prompt_t prompt_init( prompt_t p, win_info_t wi, const char* user_prompt )
{
    p->wi = wi;

    plcm_use( &p->label, p->labelmem, 128 );
    prompt_label( p, user_prompt );

    plcm_empty( &p->buf, 256 );
    p->interaction = pl_false;

    return p;
}


/**
 * Free Prompt object.
 *
 * @param p Prompt object.
 */
void prompt_close( prompt_t p )
{
    screen_clear_win( p->wi );
    screen_refresh( p->wi );
    plcm_del( &p->buf );
    plcm_del( &p->label );
}


/**
 * Prepare user input buffer for input.
 *
 * @param p Prompt object.
 */
void prompt_open_buffer( prompt_t p )
{
    plcm_resize( &p->buf, 0 );
    p->interaction = pl_true;
    p->b0 = 0;
    p->bi = 0;
}


/**
 * Free user input buffer.
 *
 * @param p Prompt object.
 */
void prompt_close_buffer( prompt_t p )
{
    plcm_reset( &p->buf );
    p->interaction = pl_false;
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
void prompt_label( prompt_t p, const char* prompt )
{
    if ( prompt ) {
        p->x0 = strlen( prompt );
        plss_set( &p->label, plsr_from_string( prompt ) );
    } else {
        plcm_reset( &p->label );
    }
}


/**
 * Display user message.
 *
 * @param p Prompt object.
 * @param msg Message to display.
 */
void prompt_msg( prompt_t p, const char* msg )
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
void prompt_refresh( prompt_t p )
{
    screen_clear_win( p->wi );

    if ( !plss_is_empty( &p->label ) ) {
        screen_setpos( p->wi, 0, 0 );
        screen_set_str2( p->wi, plss_string( &p->label ) );
    }

    if ( prompt_interacting( p ) ) {
        const char* view;

        screen_setpos( p->wi, p->x0, 0 );
        view = plss_string( &p->buf );
        view += p->b0;
        screen_set_str2( p->wi, view );

        screen_setpos( p->wi, x_pos( p ), 0 );
    }

    screen_refresh( p->wi );
}


/**
 * Return true if user interaction is ongoing.
 */
pl_bool_t prompt_interacting( prompt_t p )
{
    return p->interaction;
}


/**
 * Interact using the prompt, i.e. get user input.
 *
 * @param p Prompt object.
 * @param label Prompt-label.
 *
 * @return Input given by the user.
 */
const char* prompt_interact( prompt_t p, const char* label )
{
    pl_bool_t   done = pl_false;
    int         key;
    const char* ret;

    prompt_label( p, label );
    prompt_open_buffer( p );

    /* Initial screen refresh. */
    prompt_refresh( p );

    /* Get user input. */
    for ( ;; ) {

        key = screen_get_key();

        switch ( key ) {

            case NEWLINE:
                done = pl_true;
                ret = plss_string( &p->buf );
                break;

            case ESC:
            case CTRL_G:
                done = pl_true;
                ret = NULL;
                break;

            case CTRL_B:
                /* Backwards char. */
                if ( p->bi > 0 ) {
                    if ( p->wi->x <= p->x0 ) {
                        p->b0--;
                        p->bi--;
                    } else {
                        p->bi--;
                    }
                }
                break;

            case CTRL_F:
                /* Forwards char. */
                if ( p->bi < plss_length( &p->buf ) ) {
                    if ( p->wi->x >= WI_X_MAX( p->wi ) ) {
                        p->b0++;
                        p->bi++;
                    } else {
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
                p->bi = plss_length( &p->buf );
                p->b0 = p->bi - WI_X_MAX( p->wi ) + p->x0;
                if ( p->b0 < 0 ) {
                    p->b0 = 0;
                }
                break;

            case CTRL_D:
                /* Delete char. */
                if ( p->bi < plss_length( &p->buf ) ) {
                    plss_remove( &p->buf, p->bi, 1 );
                }
                break;

            case BS:
            case CTRL_H:
                /* Backspace char. */
                if ( p->bi > 0 ) {
                    if ( p->wi->x <= p->x0 ) {
                        p->b0--;
                        p->bi--;
                    } else {
                        p->bi--;
                    }
                    plss_remove( &p->buf, p->bi, 1 );
                }
                break;

            case CTRL_K:
                /* Kill line. */
                if ( p->bi < plss_length( &p->buf ) ) {
                    plss_remove( &p->buf, p->bi, plss_length( &p->buf ) - p->bi );
                }
                break;

            default:
                /* Add char. */
                if ( key >= 32 && key <= 126 ) {
                    char c;
                    c = key;
                    plss_insert( &p->buf, p->bi, &c, 1 );
                    if ( p->wi->x >= WI_X_MAX( p->wi ) ) {
                        p->b0++;
                        p->bi++;
                    } else {
                        p->bi++;
                    }
                }
                break;
        }

        if ( done ) {
            break;
        }

        prompt_refresh( p );
    }

    dbug( "prompt ret: %s\n", ret );

    /* Cleanup. */
    prompt_close_buffer( p );
    prompt_label( p, NULL );

    prompt_refresh( p );

    return ret;
}
