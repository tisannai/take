#ifndef MCC_H
#define MCC_H


/**
 * @file mcc.h
 *
 * @brief Automatic allocation for array of char* type.
 *
 * @mainpage
 *
 * mcc-library provides automatic allocation for array of char* type
 * items. It is compact and does not include any extra functionality,
 * so it is also suitable for static linking.
 *
 * mcc depends on types from mc-lib:
 * - Boolean:  mc_bool_t
 * - Size:     mc_size_t
 *
 * mcc depends on memory allocation functions from mc-lib:
 * - Allocation:      mc_new_n
 * - Re-allocaxtion:  mc_realloc
 *
 */



/** Autoarr Memory default size. */
#define MCC_DEFAULT_SIZE 128

/** Invalid index indicator. */
#define MCC_INVALID_INDEX -1


/** mcc-lib version. */
extern const char* mcc_version;


/** mcc-lib storage type. */



/** Sizeof in mcc-style. */
#define mcc_sizeof (sizeof(char))

/** Autoarr size in bytes. */
#define mcc_bytesize(aa) (mcc_sizeof*aa->size)

/** Autoarr size in bytes. */
#define mcc_usedsize(aa) (mcc_sizeof*aa->used)



/* ------------------------------------------------------------
 * Auto-resize memory allocation.
 * ------------------------------------------------------------ */

/** Autoarr data structure type. */
typedef struct mcc_s mcc_t;

/** Pointer to autoarr data structure type. */
typedef mcc_t* mcc_p;

/** Autoarr resizer function type. */
typedef mc_bool_t (*mcc_resize_func_t) ( mcc_p aa, mc_size_t newsize );

/** Handle to default Autoarr resizer function. */
extern mcc_resize_func_t mcc_resize_func;


/** Autoarr container. */
struct mcc_s
{
  char* data;                   /**< Allocation. */
  mc_size_t size;               /**< Size of memory as unit count. */
  mc_size_t used;               /**< Usage count. */
  mcc_resize_func_t resize;     /**< Resizer function. */
};



/**
 * Return nth element in data as pointer that should be cast with
 * mcc_nth macro.
 *
 * @param aa Autoarr.
 * @param nth Element index.
 *
 * @return Element.
 */
#define mcc_nth_p(aa,nth) (&(((aa)->data)[nth]))


/**
 * Return the nth data from Autoarr casted to type.
 *
 * @param aa Autoarr container.
 * @param nth Nth entry.
 *
 * @return Referenced data.
 */
#define mcc_nth(aa,nth) ((aa)->data[nth])


/**
 * Default allocation size increase/decrease function. Called by
 * mcc_resize. Increases by factor of 2 and decreases by factor of
 * 2.
 *
 * @param aa Autoarr to resize.
 * @param newsize Size to fit.
 *
 * @return True if resizing was performed.
 */
mc_bool_t mcc_default_resizer( mcc_p aa, mc_size_t newsize );


/**
 * Optional resizer that only increases the allocation. Suitable for
 * things that only grow in their lifetime.
 *
 * @param aa Autoarr to resize.
 * @param newsize Size to fit.
 *
 * @return True if resizing was performed.
 */
mc_bool_t mcc_enlarge_resizer( mcc_p aa, mc_size_t newsize );


/**
 * Create new Autoarr Memory allocation. Size is not allowed to be
 * smaller than 1.
 *
 * @param size Initial size.
 *
 * @return Autoarr descriptor.
 */
mcc_p mcc_new_size( mc_size_t size );


/**
 * Create new Autoarr Memory allocation with default size (128).
 *
 * @return Autoarr descriptor.
 */
mcc_p mcc_new( void );


/**
 * Free Autoarr descriptor and contained data.
 *
 * @param aa Autoarr descriptor.
 */
mcc_p mcc_del( mcc_p aa );


/**
 * Copy Autoarr to another.
 *
 * @param aa Source.
 * @param to Destination.
 */
void mcc_copy( mcc_p aa, mcc_p to );


/**
 * Duplicate Autoarr.
 *
 * @param aa Autoarr to duplicate.
 *
 * @return Autoarr descriptor.
 */
mcc_p mcc_dup( mcc_p aa );


/**
 * Delete content (set used to 0), but don't touch allocations.
 *
 * @param aa Autoarr.
 */
void mcc_reset( mcc_p aa );


/**
 * Delete content, but don't touch allocations.
 *
 * @param aa Autoarr.
 */
void mcc_delete_all( mcc_p aa );


/**
 * Resize Autoarr allocation using the registered resizer in Autoarr.
 *
 * @param aa Autoarr descriptor.
 * @param size New size.
 */
void mcc_resize( mcc_p aa, mc_size_t size );


/**
 * Resize Autoarr allocation. Note that no checks are performed, so
 * data might be lost.
 *
 * @param aa Autoarr descriptor.
 * @param size New size.
 */
void mcc_resize_to( mcc_p aa, mc_size_t size );


/**
 * Compact the allocation to used size.
 *
 * @param aa Autoarr descriptor.
 */
void mcc_compact( mcc_p aa );


/**
 * Insert items at selected position. Existing data is shifted
 * right. Function assumes item data unit. User must take care of
 * scaling if non-item is used.
 *
 * @param aa Autoarr.
 * @param pos Insert position.
 * @param data Data to add.
 * @param len Number of items to add.
 */
void mcc_insert_n_to( mcc_p aa, mc_size_t pos, char* data, mc_size_t len );


/**
 * Insert item at selected position. Existing data is shifted right.
 *
 * @param aa Autoarr.
 * @param pos Insert position.
 * @param ch Item to add.
 */
void mcc_insert_to( mcc_p aa, mc_size_t pos, char ch );


/**
 * Delete items from selected position. Existing data is shifted left
 * after deletion position. Function assumes item data unit. User must
 * take care of scaling if non-item is used.
 *
 * @param aa Autoarr.
 * @param pos Delete position.
 * @param len Number of items to delete.
 */
void mcc_delete_n_at( mcc_p aa, mc_size_t pos, mc_size_t len );


/**
 * Delete item at selected position. Existing data is shifted left
 * after deletion position.
 *
 * @param aa Autoarr.
 * @param pos Delete position.
 */
void mcc_delete_at( mcc_p aa, mc_size_t pos );


/**
 * Remove data from Autoarr (dealloc). Allocation is resized if usage
 * drops below half of the current allocation.
 *
 * @param aa Autoarr descriptor.
 * @param len Number of items to remove from end.
 */
void mcc_delete_n_end( mcc_p aa, mc_size_t len );


/**
 * Assign Autoarr data. Start position can overlap existing data. More memory
 * is allocated if needed.
 *
 * @param aa Autoarr descriptor.
 * @param pos Starting position for set.
 * @param data Source data.
 * @param len Source data length in units.
 */
void mcc_assign_to( mcc_p aa, mc_size_t pos, char* data, mc_size_t len );


/**
 * Assign Autoarr data from start. More memory is allocated if needed.
 *
 * @param aa Autoarr descriptor.
 * @param data Source data.
 * @param len Source data length in units.
 */
void mcc_assign( mcc_p aa, char* data, mc_size_t len );


/**
 * Append one data after used position (concatenate).
 *
 * NOTE: when storing a pointer, the user have to provide the pointers
 * address, not the pointer value.
 *
 * @param aa Autoarr descriptor.
 * @param data Source data address (pointer to data).
 */
void mcc_append( mcc_p aa, char data );


/**
 * Append data after used position (concatenate).
 *
 * @param aa Autoarr descriptor.
 * @param data Source data.
 * @param len Number of units to set.
 */
void mcc_append_n( mcc_p aa, char* data, mc_size_t len );


/**
 * TODO 141214_0723: not tested yet.
 *
 * Append one data after used position (concatenate) if not in the
 * list.
 *
 * @param aa Autoarr descriptor.
 * @param data Source data address (pointer to data).
 * @return True if data added.
 */
mc_bool_t mcc_append_unique( mcc_p aa, char data );



/**
 * Prepend one item.
 *
 * @param aa Autoarr descriptor.
 * @param data Data to prepend.
 */
void mcc_prepend( mcc_p aa, char data );



/**
 * Prepend n items.
 *
 * @param aa Autoarr descriptor.
 * @param data Source data address (pointer to data).
 * @param len Number of units to prepend.
 */
void mcc_prepend_n( mcc_p aa, char* data, mc_size_t len );



/**
 * Return first index where data is found.
 *
 * @param aa Autoarr descriptor.
 * @param data Compare data.
 */
mc_size_t mcc_find_idx( mcc_p aa, char data );


/**
 * Return true if data is found from Autoarr.
 *
 * @param aa Autoarr descriptor.
 * @param data Compare data.
 */
mc_bool_t mcc_find( mcc_p aa, char data );


/**
 * Terminate the data area with NULL. After termination the "data"
 * field of Autoarr can be used independently. However user must
 * @see mc_free the Autoarr ("data" is not freed).
 *
 * @param aa Autoarr.
 *
 * @return Element.
 */
 void mcc_terminate( mcc_p aa );


/**
 * Autoarr stack push (i.e. end of Autoarr).
 *
 * @param s Stack (Autoarr).
 * @param item Pushed item.
 */
void mcc_push( mcc_p s, char item );


/**
 * Autoarr stack pop (i.e. end of Autoarr).
 *
 * @param s Stack (Autoarr).
 *
 * @return Item from top of stack.
 */
char mcc_pop( mcc_p s );


/**
 * Autoarr stack peek (i.e. end of Autoarr).
 *
 * @param s Stack (Autoarr).
 *
 * @return Item from top of stack.
 */
char mcc_peek( mcc_p s );


/**
 * Return true if no elements in Autoarr.
 *
 * @param aa Autoarr.
 *
 * @return True if no entries.
 */
mc_bool_t mcc_empty( mcc_p aa );


#endif
