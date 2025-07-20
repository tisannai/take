/**
 * @file common.c
 *
 * Common utility functions.
 *
 */


#include <stdio.h>
#include <stdarg.h>
#include "common.h"


plam_s balloc;


/** File handle for debug IO. */
static FILE* dbug_fh = NULL;


/**
 * Set debug file stream.
 *
 * @param io IO-stream to use for debug.
 */
void ml_dbug_io( FILE* io )
{
    dbug_fh = io;
}


/**
 * Open debug file stream.
 *
 * @param file Name for debug file.
 */
void ml_dbug_open( const char* file )
{
    dbug_fh = fopen( file, "w" );
}


/**
 * Debug printout for varargs.
 *
 * @param format Debug formatting.
 * @param args Argument list pointer.
 */
void ml_dbug_vprint( const char* format, va_list args )
{
    if ( dbug_fh ) {
        vfprintf( dbug_fh, format, args );
    }
    va_end( args );
    fflush( dbug_fh );
}


/**
 * Debug printout.
 *
 * @param format Debug formatting.
 * @param ... Formatting arguments.
 */
void ml_dbug_print( const char* format, ... )
{
    va_list args;
    va_start( args, format );
    ml_dbug_vprint( format, args );
}


/**
 * Close debug file stream.
 */
void ml_dbug_close( void )
{
    if ( dbug_fh ) {
        fclose( dbug_fh );
    }

    dbug_fh = NULL;
}
