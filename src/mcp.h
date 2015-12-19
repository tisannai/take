#ifndef MCP_H
#define MCP_H


/**
 * @file mcp.h
 *
 * @brief Automatic allocation for array of mcp.
 *
 * mcp-library is for automatic allocation for array of mcp. It
 * does not include any extra functionality so it can easily be linked
 * also statically to programs.
 *
 * mcp depends on types from "mc.h":
 * - Boolean: mc_bool_t
 * - Size: mc_size_t
 *
 * mcp depends on memory allocation functions from "mc":
 * - Allocation: mc_new_n
 * - Re-allocaxtion: mc_realloc
 *
 */



/** Autoarr Memory default size. */
#define MCP_DEFAULT_SIZE 128

/** Invalid index indicator. */
#define MCP_INVALID_INDEX -1


/** mcp-lib version. */
extern const char* mcp_version;


/** mcp-lib storage type. */




/** Sizeof in myc-style. */
#define mcp_sizeof (sizeof(void*))

/** Autoarr size in bytes. */
#define mcp_bytesize(aa) (mcp_sizeof*aa->size)

/** Autoarr size in bytes. */
#define mcp_usedsize(aa) (mcp_sizeof*aa->used)



/*
 * *************************************************************
 * Auto-resize memory allocation.
 */


/** mcp-lib storage type. */
typedef struct mcp_s mcp_t;

/** mcp-lib storage type ptr. */
typedef mcp_t* mcp_p;

/** Autoarr resizer function type. */
typedef mc_bool_t (*mcp_resize_func_t) ( mcp_p aa, mc_size_t newsize );

/** Handle to default Autoarr resizer function. */
extern mcp_resize_func_t mcp_resize_func;


/** Autoarr container. */
struct mcp_s
{
  /** Allocation. */
  void** data;

  /** Size of memory as unit count. */
  mc_size_t size;

  /** Usage count. */
  mc_size_t used;

  /** Resizer function. */
  mcp_resize_func_t resize;

};



/**
 * Return nth element in data as pointer that should be cast with
 * mcp_nth macro.
 * 
 * @param aa Autoarr.
 * @param nth Element index.
 * 
 * @return Element.
 */
#define mcp_nth_p(aa,nth) (&(((aa)->data)[nth]))


/**
 * Return the nth data from Autoarr casted to type.
 * 
 * @param aa Autoarr container.
 * @param nth Nth entry.
 * 
 * @return Referenced data.
 */
#define mcp_nth(aa,nth) ((aa)->data[nth])


/**
 * Default allocation size increase/decrease function. Called by
 * mcp_resize. Increases by factor of 2 and decreases by factor of
 * 2.
 * 
 * @param aa Autoarr to resize.
 * @param newsize Size to fit.
 * 
 * @return True if resizing was performed.
 */
mc_bool_t mcp_default_resizer( mcp_p aa, mc_size_t newsize );


/**
 * Optional resizer that only increases the allocation. Suitable for
 * things that only grow in their lifetime.
 * 
 * @param aa Autoarr to resize.
 * @param newsize Size to fit.
 * 
 * @return True if resizing was performed.
 */
mc_bool_t mcp_enlarge_resizer( mcp_p aa, mc_size_t newsize );


/**
 * Create new Autoarr Memory allocation. Size is not allowed to be
 * smaller than 1.
 * 
 * @param size Initial size.
 * 
 * @return Autoarr descriptor.
 */
mcp_p mcp_new_size( mc_size_t size );


/**
 * Create new Autoarr Memory allocation with default size (128).
 * 
 * @return Autoarr descriptor.
 */
mcp_p mcp_new( void );


/**
 * Free Autoarr descriptor and contained data.
 * 
 * @param aa Autoarr descriptor.
 */
mcp_p mcp_del( mcp_p aa );


/**
 * Copy Autoarr to another.
 * 
 * @param aa Source.
 * @param to Destination.
 */
void mcp_copy( mcp_p aa, mcp_p to );


/**
 * Duplicate Autoarr.
 * 
 * @param aa Autoarr to duplicate.
 * 
 * @return Autoarr descriptor.
 */
mcp_p mcp_dup( mcp_p aa );


/**
 * Delete content (set used to 0), but don't touch allocations.
 * 
 * @param aa Autoarr.
 */
void mcp_reset( mcp_p aa );


/**
 * Delete content, but don't touch allocations.
 * 
 * @param aa Autoarr.
 */
void mcp_delete_all( mcp_p aa );


/**
 * Resize Autoarr allocation using the registered resizer in Autoarr.
 * 
 * @param aa Autoarr descriptor.
 * @param size New size.
 */
void mcp_resize( mcp_p aa, mc_size_t size );


/**
 * Resize Autoarr allocation. Note that no checks are performed, so
 * data might be lost.
 * 
 * @param aa Autoarr descriptor.
 * @param size New size.
 */
void mcp_resize_to( mcp_p aa, mc_size_t size );


/**
 * Compact the allocation to used size.
 * 
 * @param aa Autoarr descriptor.
 */
void mcp_compact( mcp_p aa );


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
void mcp_insert_n_to( mcp_p aa, mc_size_t pos, void** data, mc_size_t len );


/**
 * Insert item at selected position. Existing data is shifted right.
 * 
 * @param aa Autoarr.
 * @param pos Insert position.
 * @param ch Item to add.
 */
void mcp_insert_to( mcp_p aa, mc_size_t pos, void* ch );


/**
 * Delete items from selected position. Existing data is shifted left
 * after deletion position. Function assumes item data unit. User must
 * take care of scaling if non-item is used.
 * 
 * @param aa Autoarr.
 * @param pos Delete position.
 * @param len Number of items to delete.
 */
void mcp_delete_n_at( mcp_p aa, mc_size_t pos, mc_size_t len );


/**
 * Delete item at selected position. Existing data is shifted left
 * after deletion position.
 * 
 * @param aa Autoarr.
 * @param pos Delete position.
 */
void mcp_delete_at( mcp_p aa, mc_size_t pos );


/**
 * Remove data from Autoarr (dealloc). Allocation is resized if usage
 * drops below half of the current allocation.
 * 
 * @param aa Autoarr descriptor.
 * @param len Number of items to remove from end.
 */
void mcp_delete_n_end( mcp_p aa, mc_size_t len );


/**
 * Assign Autoarr data. Start position can overlap existing data. More memory
 * is allocated if needed.
 * 
 * @param aa Autoarr descriptor.
 * @param pos Starting position for set.
 * @param data Source data.
 * @param len Source data length in units.
 */
void mcp_assign_to( mcp_p aa, mc_size_t pos, void** data, mc_size_t len );


/**
 * Assign Autoarr data from start. More memory is allocated if needed.
 * 
 * @param aa Autoarr descriptor.
 * @param data Source data.
 * @param len Source data length in units.
 */
void mcp_assign( mcp_p aa, void** data, mc_size_t len );


/**
 * Append one data after used position (concatenate).
 *
 * NOTE: when storing a pointer, the user have to provide the pointers
 * address, not the pointer value.
 * 
 * @param aa Autoarr descriptor.
 * @param data Source data address (pointer to data).
 */
void mcp_append( mcp_p aa, void* data );


/**
 * Append data after used position (concatenate).
 * 
 * @param aa Autoarr descriptor.
 * @param data Source data.
 * @param len Number of units to set.
 */
void mcp_append_n( mcp_p aa, void** data, mc_size_t len );


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
mc_bool_t mcp_append_unique( mcp_p aa, void* data );



/**
 * Prepend one item.
 * 
 * @param aa Autoarr descriptor.
 * @param data Data to prepend.
 */
void mcp_prepend( mcp_p aa, void* data );



/**
 * Prepend n items.
 * 
 * @param aa Autoarr descriptor.
 * @param data Source data address (pointer to data).
 * @param len Number of units to prepend.
 */
void mcp_prepend_n( mcp_p aa, void** data, mc_size_t len );



/**
 * Return first index where data is found.
 * 
 * @param aa Autoarr descriptor.
 * @param data Compare data.
 */
mc_size_t mcp_find_idx( mcp_p aa, void* data );


/**
 * Return true if data is found from Autoarr.
 * 
 * @param aa Autoarr descriptor.
 * @param data Compare data.
 */
mc_bool_t mcp_find( mcp_p aa, void* data );


/**
 * Terminate the data area with NULL. After termination the "data"
 * field of Autoarr can be used independently. However user must
 * @see mc_free the Autoarr ("data" is not freed).
 * 
 * @param aa Autoarr.
 * 
 * @return Element.
 */
 void mcp_terminate( mcp_p aa );


/**
 * Autoarr stack push (i.e. end of Autoarr).
 * 
 * @param s Stack (Autoarr).
 * @param item Pushed item.
 */
void mcp_push( mcp_p s, void* item );


/**
 * Autoarr stack pop (i.e. end of Autoarr).
 * 
 * @param s Stack (Autoarr).
 * 
 * @return Item from top of stack.
 */
void* mcp_pop( mcp_p s );


/**
 * Autoarr stack peek (i.e. end of Autoarr).
 * 
 * @param s Stack (Autoarr).
 * 
 * @return Item from top of stack.
 */
void* mcp_peek( mcp_p s );


/**
 * Return true if no elements in Autoarr.
 * 
 * @param aa Autoarr.
 * 
 * @return True if no entries.
 */
mc_bool_t mcp_empty( mcp_p aa );

#endif
