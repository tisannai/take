#ifndef PROMPT_H
#define PROMPT_H


/**
 * @file prompt.h
 *
 * Prompt defs.
 */


/** Prompt/label content. */
pl_struct( prompt )
{
    win_info_t wi;              /**< Screen window ref. */
    char       labelmem[ 128 ]; /**< Memory for label storage. */
    plcm_s     label;           /**< Prompt label or message content. */
    plcm_s     buf;             /**< User input buffer, NULL if in message mode. */
    int        x0;              /**< User input x start coord on screen. */
    int        b0;              /**< Index to user input visible part start. */
    int        bi;              /**< User input cursor position. */
    pl_bool_t  interaction;     /**< Is prompt for interaction (or only for labeling). */
};


prompt_t    prompt_init( prompt_t p, win_info_t wi, const char* user_prompt );
void        prompt_close( prompt_t p );
void        prompt_open_buffer( prompt_t p );
void        prompt_close_buffer( prompt_t p );
void        prompt_label( prompt_t p, const char* prompt );
void        prompt_msg( prompt_t p, const char* msg );
void        prompt_refresh( prompt_t p );
pl_bool_t   prompt_interacting( prompt_t p );
const char* prompt_interact( prompt_t p, const char* label );

#endif
