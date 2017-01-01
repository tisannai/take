#ifndef MC_H
#define MC_H


/**
 * @file mc.h
 *
 * @brief Mc library is small c-lib with basic memory allocation
 *   functions and definitions.
 *
 * @mainpage
 *
 * Mc library includes fundamental function for convenient
 * programming. It does not include any extra functionality so it can
 * easily be linked also statically to programs.
 *
 */


/*
 * Common headers:
 */

#ifndef MC_NO_INCLUDE
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>
#include <stdint.h>
#endif


/** mc-lib version. */
extern const char* mc_version;


/* ------------------------------------------------------------
 * Basic data types.
 * ------------------------------------------------------------ */

/** Boolean type. */
typedef enum mc_bool_e { mc_false = 0, mc_true = 1 } mc_bool_t;

/** Void type. */
typedef void mc_none;

/** Generic pointer. */
typedef void* mc_p;

/** Pointer to Generic pointer. */
typedef void** mc_pp;

/** Define type and type related pointers. */
#define mc_type(base,type) typedef base type ## _t;  \
  typedef type ## _t*  type ## _p;                   \
  typedef type ## _t** type ## _pp                   \

mc_type( char, mc_char );       /**< Character type. */
mc_type( int64_t, mc_int );     /**< Int type (64-bits). */
mc_type( uint64_t, mc_uint );   /**< Unsigned int type (64-bits). */
mc_type( int64_t, mc_size );    /**< Size of allocation type. */



/** Nil pointer. */
#define mc_nil NULL


/* ------------------------------------------------------------
 * Basic loops.
 * ------------------------------------------------------------ */

/** Forever loop. */
#define mc_loop for(;;)

/** For n-times loop. Loop counter is 'i'. */
#define mc_for_n(n) for(int i=0; i < (n); i++)

/** For n-times loop. Loop counter is 'x'. */
#define mc_for_n_x(n,x) for(int (x)=0; (x) < (n); (x)++)



/* ------------------------------------------------------------
 * Structure type definition.
 * ------------------------------------------------------------ */

/**
 * Define struct and the pointer (+reference types) called
 * "name". A struct is created with name+"_s", a typedef is created
 * with name+"_t" and a pointer to struct type is created as
 * name+"_p".
 *
 * Example:
 * @code
 *   Definition:
 *     mc_struct(point) {
 *        mc_int x;
 *        mc_int y;
 *     };
 *
 *   Becomes:
 *     typedef struct point_s point_t;
 *     typedef point_t* point_p;
 *     struct point_s {
 *       mc_int x;
 *       mc_int y;
 *     };
 * @endcode
 */
#define mc_struct(name) typedef struct name ## _s name ## _t; \
  typedef name ## _t* name ## _p;                             \
  struct name ## _s


/**
 * Same as @see mc_struct, but with pointer to pointer. */
#define mc_struct_pp(name) typedef struct name ## _s name ## _t; \
  typedef name ## _t* name ## _p;                                \
  typedef name ## _t** name ## _pp;                              \
  typedef name ## _t** name ## _pr;                              \
  struct name ## _s


/**
 * Standard types for struct if types are needed separate from struct
 * (forward declaration). */
#define mc_struct_type(name) typedef struct name ## _s name ## _t; \
  typedef name ## _t* name ## _p

/**
 * Same as @see mc_struct_type, but with pointer to pointer (forward
 * declaration). */
#define mc_struct_type_pp(name) typedef struct name ## _s name ## _t; \
  typedef name ## _t* name ## _p;                                     \
  typedef name ## _t** name ## _pp



/**
 * Define enumeration and corresponding type. An enum is created with
 * name+"_e", a typedef is created with name+"_t".
 *
 * Example:
 *
 *   mc_enum(some_runtype) {
 *     RUN_NONE,
 *     RUN_TASK,
 *     RUN_FUNC
 *   };
 *
 *   // Becomes...
 *   typedef enum mc_runtype_e mc_runtype_t;
 *   enum mc_runtype_e {
 *     RUN_NONE,
 *     RUN_TASK,
 *     RUN_FUNC
 *   };
 */
#define mc_enum(name) typedef enum name ## _e name ## _t; \
  enum name ## _e



/* ------------------------------------------------------------
 * Memory management.
 * ------------------------------------------------------------ */

#ifndef MC_NO_MEMMGT

/** Size of type. */
# define mc_sizeof(type) sizeof(type)


/**
 * Allocation memory for size and init the data to 0.
 *
 * @param size Bytesize of allocation.
 *
 * @return Generic pointer to allocation.
 */
mc_p mc_xmalloc( mc_size_t size );


/**
 * Copy memory content.
 *
 * @param src  Memory to copy.
 * @param dst  Destination for copy.
 * @param size Bytesize of copy area.
 *
 * @return NA.
 */
# define mc_memcpy(src,dst,size) memcpy(dst,src,size)


/**
 * Duplicate memory content.
 *
 * @param source Memory to duplicate.
 * @param size Bytesize of the duplication.
 *
 * @return Generic pointer to duplicate.
 */
mc_p mc_memdup( const mc_p source, mc_size_t size );


/**
 * Duplicate c-string (null-terminated string).
 *
 * @param str String to duplicate.
 *
 * @return Pointer to duplicate string.
 */
# define mc_strdup(str) ( (char*) mc_memdup((const mc_p)(str),sizeof(char)*strlen(str)+1) )


/**
 * Move memory content.
 *
 * @param src  Memory to move.
 * @param dst  Destination for move.
 * @param size Bytesize of copy area.
 *
 * @return NA.
 */
# define mc_memmove(src,dst,size) memmove(dst,src,size)


/**
 * Compare memory content.
 *
 * @param d1 Data set 1.
 * @param d2 Data set 2.
 * @param len Bytesize (length) of compare.
 *
 * @return True if match.
 */
mc_bool_t mc_memdiff( const mc_p d1, const mc_p d2, mc_size_t len );


/**
 * Free allocated memory.
 *
 * @param item Data item.
 *
 * @return nil
 */
mc_p mc_free( mc_p item );


/**
 * Allocate memory for size.
 *
 * @param size Bytecount for allocation.
 *
 * @return Generic pointer to allocation.
 */
# define mc_malloc(size)        mc_xmalloc(size)


/**
 * Reallocate item with new size.
 *
 * @param item Existing allocation.
 * @param size New size for allocation.
 *
 * @return Generic pointer to allocation.
 */
# define mc_realloc(item,size)  realloc(item,size)


/**
 * Clean memory region prt by n bytes.
 *
 * @param ptr Region start.
 * @param n   Number of bytes to clear.
 *
 * @return NA
 */
# define mc_memclr(ptr,n) memset(ptr,0,n)


/**
 * Allocate memory for type.
 *
 * @param type Type for allocation.
 *
 * @return Allocated memory casted to type.
 */
# define mc_new(type)           (type*) mc_xmalloc(mc_sizeof(type))


/**
 * Allocate memory for type n times.
 *
 * @param type Type for allocation.
 * @param n    Allocation multiplier.
 *
 * @return Allocated memory casted to type.
 */
# define mc_new_n(type,n)       (type*) mc_xmalloc(mc_sizeof(type)*(n))


/**
 * Copy data item of type.
 *
 * @param src  Item to copy.
 * @param dst  Destination for copy.
 * @param type Item type.
 *
 * @return Memory casted to type.
 */
# define mc_cpy(src,dst,type)        (type*) mc_memcpy(src,dst,mc_sizeof(type))


/**
 * Duplicate data item of type.
 *
 * @param item Item to duplicate.
 * @param type Item type.
 *
 * @return Duplicated memory casted to type.
 */
# define mc_dup(item,type)           (type*) mc_memdup(item,mc_sizeof(type))


/**
 * Move data item of type.
 *
 * @param src  Item to move.
 * @param dst  Destination for move.
 * @param type Item type.
 *
 * @return Memory casted to type.
 */
# define mc_move(src,dst,type)           (type*) mc_memmove(src,dst,mc_sizeof(type))


/**
 * De-allocate item.
 *
 * @param item Allocation.
 *
 * @return NULL.
 */
# define mc_del(item)           mc_free(item)

#endif



/* ------------------------------------------------------------
 * User interfacing.
 * ------------------------------------------------------------ */

/**
 * Print message to user. This function is usually called from a
 * function that is customized per program.
 *
 * Example:
 * @code
 *   void myprog_error( const char* format, ... )
 *   {
 *     va_list ap;
 *     myprog_err_cnt++;
 *     va_start( ap, format );
 *     mc_msg_with_prefix( stderr, "MyProg ERROR: ", format, ap );
 *     va_end( ap );
 *   }
 * @endcode
 *
 *
 * @param io File* pointer (stdout, stderr etc.)
 * @param prefix Standard prefix for message.
 * @param format Printf-style format string.
 * @param ap Variable arguments (prepared with va_start).
 */
void mc_msg_with_prefix( FILE* io,
                         const char* prefix,
                         const char* format,
                         va_list ap );


/**
 * Dummy function call for/as debuggers default breakpoint.
 *
 * Usage:
 * In c file:
 * @code
 *   ...
 *
 *   mc_break();
 *   // Segfault here...
 *   ...
 * @endcode
 *
 * In .gdbinit:
 * @code
 * ...
 * break mc_break
 * ...
 * @endcode
 *
 *
 */
void mc_break( void );

#endif
