#ifndef COMMON_H
#define COMMON_H


/**
 * @file common.h
 *
 * Program common defs.
 */

#include <plinth.h>


/* Base allocator. */
#define balloc_node_size ( 1 << 20 )
#define balloc_cont_size ( ( balloc_node_size / 4 ) * 2 )
extern plam_s balloc;



/**
 * Defines:
 */

/** Value of newline char. */
#define NEWLINE 10

/** Value of escape char. */
#define ESC 27

/** Value of backspace. */
#define BS 127



/**
 * @{
 * Control char defines. */
#define CTRL_SPACE 0
#define CTRL_A 1
#define CTRL_B 2
#define CTRL_C 3
#define CTRL_D 4
#define CTRL_E 5
#define CTRL_F 6
#define CTRL_G 7
#define CTRL_H 8
#define CTRL_I 9
#define CTRL_J 10
#define CTRL_K 11
#define CTRL_L 12
#define CTRL_M 13
#define CTRL_N 14
#define CTRL_O 15
#define CTRL_P 16
#define CTRL_Q 17
#define CTRL_R 18
#define CTRL_S 19
#define CTRL_T 20
#define CTRL_U 21
#define CTRL_V 22
#define CTRL_W 23
#define CTRL_X 24
#define CTRL_Y 25
#define CTRL_Z 26
/** @} */



/**
 * How to control debugging:
 *
 * Change:
 *   from: if 0
 *   to:   if 1
 * to enable debugging.
 */
#if 0
#    define ml_do_debug 1
#    define dbug( args... ) ( ml_dbug_print( args ) )
#else
#    define dbug( args... ) \
        {                   \
        }
#endif


/**
 * Function protos:
 */
void ml_dbug_open( const char* file );
void ml_dbug_print( const char* format, ... );
void ml_dbug_close( void );


#endif
