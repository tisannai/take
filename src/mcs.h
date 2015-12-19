#ifndef MCS_H
#define MCS_H


/**
 * @file   mcs.h
 * 
 * @brief  Extensions for easy string creation within mcc.
 * 
 */


/**
 * Sprintf to end of Autoarr or create a new Autoarr if aa is NULL
 * using mcc_enlarge_resizer. Make space accordingly. The string is
 * null terminated.
 * 
 * @param aa Autoarr.
 * @param format Format string.
 * @param ... Formatting arguments.
 * @return Created aa (or original).
 */
mcc_p mcc_printf( mcc_p aa, const char* format, ... );


/**
 * Sprintf to beginning of Autoarr or create a new Autoarr if aa is NULL
 * using mcc_enlarge_resizer. Make space accordingly. The string is
 * null terminated.
 * 
 * @param aa Autoarr.
 * @param format Format string.
 * @param ... Formatting arguments.
 * @return Created aa (or original).
 */
mcc_p mcc_reprintf( mcc_p aa, const char* format, ... );


/**
 * Sprintf to end of Autoarr or create a new Autoarr if aa is NULL
 * using mcc_enlarge_resizer. Make space accordingly. The string is
 * null terminated.
 * 
 * @param aa Autoarr.
 * @param format Format string.
 * @param ap Variable arguments.
 * @return Created aa (or original).
 */
mcc_p mcc_vprintf( mcc_p aa, const char* format, va_list ap );




/**
 * Free Autoarr except for the data part. User can build the dataset
 * and after calling this function user can use the data in read-only
 * mode.
 * 
 * @param aa Autoarr descriptor.
 * @return Data part of Autoarr.
 */
char* mcc_strip( mcc_p aa );


/**
 * Convert null-terminated c-string to Autoarr.
 * 
 * @param str Source string.
 * 
 * @return Autoarr.
 */
mcc_p mcc_from_str( const char* str );


/**
 * Return Autoarr data as c-string. User have to make sure that data
 * is null terminated (e.g. result from mcc_printf).
 * 
 * @param aa Autoarr.
 * @return C-string.
 */
const char* mcc_as_str( mcc_p aa );


/**
 * Null terminate Autoarr data (c-string). User must ensure that there
 * are no inside nulls. Autoarr used is not changed, but space for the
 * extra null is added if necessary.
 * 
 * @param aa Autoarr.
 * 
 * @return C-string.
 */
const char* mcc_to_str( mcc_p aa );


/**
 * Null terminate Autoarr data (c-string) and replace included nulls
 * with nuller. Duplicate Autoarr if original is needed.
 * 
 * @param aa Autoarr.
 * @param nuller Null replacement char.
 * 
 * @return C-string.
 */
const char* mcc_to_cstr( mcc_p aa, char nuller );


/**
 * Remove last item from data if NEWLINE.
 * 
 * @param aa Autoarr.
 */
void mcc_chomp( mcc_p aa );


/**
 * Remove last item from data if matches trim.
 * conversion.
 * 
 * @param aa Autoarr.
 * @param trim Trim value.
 */
void mcc_trim_with( mcc_p aa, char trim );


/**
 * Return the size of the formatted string.
 *
 *
 * @param format Printf formatter.
 * @param ... Fields in format.
 *
 * @return Size of formatted string.
 */
mc_size_t mcc_format_size( const char* format, ... );


/**
 * Concatenate given strings and allocate space for them. The last
 * argument has to be NULL to terminate the list. Allocated memory has
 * to be freed after use.
 *
 *
 * @param first First string in concatenation.
 *
 * @return Concatenated strings.
 */
char* mcc_str_concat( const char *first, ... );


#endif
