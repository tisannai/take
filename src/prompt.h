#ifndef PROMPT_H
#define PROMPT_H

/**
 * @file prompt.h
 *
 * Prompt defs.
 */


/** Prompt/label content. */
typedef struct prompt_s {
  win_info* wi;  /**< Screen window ref. */
  char* label;   /**< Prompt label or message content. */
  mcc_t* buf;    /**< User input buffer, NULL if in message mode. */
  int x0;        /**< User input x start coord on screen. */
  int b0;        /**< Index to user input visible part start. */
  int bi;        /**< User input cursor position. */
} prompt_t;



/* autoc:c_func_decl:begin */
prompt_t * prompt_init( win_info * wi, char * user_prompt );
void prompt_close( prompt_t * p );
void prompt_open_buffer( prompt_t * p );
void prompt_close_buffer( prompt_t * p );
void prompt_label( prompt_t * p, char * prompt );
void prompt_msg( prompt_t * p, char * msg );
void prompt_refresh( prompt_t * p );
bool_t prompt_interacting( prompt_t * p );
char * prompt_interact( prompt_t * p, char * label );
/* autoc:c_func_decl:end */

#endif
