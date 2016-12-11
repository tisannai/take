#ifndef LL_H
#define LL_H


/**
 * @file ll.h
 *
 * Linked list of "nodes" in both directions (doubly-linked). Each
 * node has the following members: link to previous node ("prev"),
 * link to next node ("next"), and pointer to node data ("data").
 *
 * Linked list supports also sublist/superlist concept. Sublists are
 * "normal" ll's, and they are contained by the superlist which is
 * also an ll, but it re-defines the node members. The superlist is
 * travelled only forwards.
 *
 * User must maintain superlist node as start of superlist,
 * superfirst. The "prev" member of this node is the link to current
 * superlist node. Node "data" points to the sublist.
 *
 *
 * @see ll_s
 *
 * @verbatim
 *
 *                                               ^
 *        +-------+     +-------+     +-------+  |
 *        |   next|---->|   next|---->|   next|--+
 *    +---|prev   |<----|prev   |<----|prev   |
 *    |   |       |     |       |     |       |
 *    v   |       |     |       |     |       |
 *        | data  |     | data  |     | data  |
 *        +-------+     +-------+     +-------+
 *          |             |             |
 *          +--->         +--->         +--->
 *
 * @endverbatim
 *
 */


/** ll-lib version. */
extern const char* ll_version;


/**
 * Macro for list iteration.
 *
 * @param list List node.
 * @param iter Iterator variable.
 */
#define ll_each(list,iter) for(ll*(iter)=(list);(iter);(iter)=(iter)->next)



/**
 * Doubly-linked list node.
 */
mc_struct(ll) {
  struct ll_s* prev;    /**< Previous node. */
  struct ll_s* next;    /**< Next     node. */
  void* data;           /**< Data  content. */
};


/** Shorter version of base type. */
typedef ll_t ll;


/** ll-data de-allocator. */
typedef void (*ll_rem_func) ( void* data );

/** ll-data compare. Return true if match. Function for find. */
typedef mc_bool_t (*ll_comp_func) ( void* data1, void* data2 );

/**
 * Compare function proto for sorting.
 *
 * Return -1 for left smaller.
 * Return 0 for equal.
 * Return 1 for left bigger.
 */
typedef int (*ll_compar) ( const ll* left, const ll* right );


/* ------------------------------------------------------------
 * Linked list:
 * ------------------------------------------------------------ */

/**
 * Allocate ll node only (no data).
 *
 * @param data Pointer to stored node data (can be NULL).
 *
 * @return Allocated node.
 */
ll* ll_new( void* data );


/**
 * Deallocate ll only (not data).
 *
 *
 * @param node Node to deallocate.
 */
void ll_rem( ll* node );


/**
 * Deallocate ll and data.
 *
 *
 * @param node Node (including data) to deallocate.
 */
void ll_rem_with_data( ll* node );


/**
 * Deallocate all ll (not data).
 *
 *
 * @param node Reference to list to deallocate.
 */
ll* ll_rem_list( ll* node );


/**
 * Deallocate all ll and data.
 *
 *
 * @param node Reference to list (including data) to deallocate.
 */
ll* ll_rem_list_with_data( ll* node );


/**
 * Deallocate all ll and data using func.
 *
 *
 * @param node Reference to list (including data) to deallocate.
 * @param rem  Function to use for data deallocation.
 */
ll* ll_rem_list_with_func( ll* node, ll_rem_func rem );


/**
 * Connect the node between left and right nodes. If one of them (or
 * both) are NULLs then the operation is the same as prepending the
 * list or appending the list.
 *
 *
 * @param left Left side node.
 * @param right Right side node.
 * @param node Node to connect.
 *
 * @return Connected node.
 */
ll* ll_connect( ll* left, ll* right, ll* node );


/**
 * Append after (append).
 *
 *
 * @param anchor Node reference to append.
 * @param node Node to append.
 *
 * @return Allocated node.
 */
ll* ll_append( ll* anchor, ll* node );


/**
 * Prepend before (insert).
 *
 *
 * @param anchor Node reference to prepend.
 * @param node Node to prepend.
 *
 * @return Allocated node.
 */
ll* ll_prepend( ll* anchor, ll* node );


/**
 * Add after node after anchor. Anchor can also be NULL. Easy creation
 * of list from scratch.
 *
 *
 * @param anchor Node reference to append.
 * @param node Node to append.
 *
 * @return Allocated node.
 */
ll* ll_add( ll* anchor, ll* node );


/**
 * Add after data after anchor. Anchor can also be NULL. Easy creation
 * of list from scratch with only data pointer passed.
 *
 *
 * @param anchor Node reference to append.
 * @param data   Data to append. Node is created for data.
 *
 * @return Allocated node.
 */
ll* ll_add_data( ll* anchor, void* data );


/**
 * Add data to end of list. Anchor can also be NULL. Easy creation
 * of list from scratch with only data pointer passed.
 *
 *
 * @param anchor Node reference to append.
 * @param data Data to append. Node is created for data.
 *
 * @return First node of list.
 */
ll* ll_add_data_end( ll* anchor, void* data );


/**
 * ll_add_data and return list start.
 *
 *
 * @param anchor Node reference to append.
 * @param data   Data to append. Node is created for data.
 *
 * @return First node of list.
 */
ll* ll_add_last_data( ll* anchor, void* data );


/**
 * Add new data to list only if it unique. The supplied comp function
 * is used for comparison.
 *
 * @param anchor Node reference to append.
 * @param data   Data to add.
 * @param comp   Data comparison function.
 *
 * @return       Allocated node (or anchor if data not unique).
 */
ll* ll_add_unique_data( ll* anchor, void* data, ll_comp_func comp );


/**
 * Add after node after anchor. Anchor can also be NULL. Easy creation
 * of list from scratch. First is updated if the addition is the first
 * node and first is non-null.
 *
 *
 * @param anchor Node reference to append.
 * @param node   Node to append.
 * @param first  First node of the list.
 *
 * @return Allocated node.
 */
ll* ll_synced_add( ll* anchor, ll* node, ll** first );


/**
 * Add after data after anchor. Anchor can also be NULL. Easy creation
 * of list from scratch with only data pointer passed. First is
 * updated if the addition is the first node and first is non-null.
 *
 *
 * @param anchor Node reference to append.
 * @param data   Data to append. Node is created for data.
 * @param first  First node of the list.
 *
 * @return Allocated node.
 */
ll* ll_synced_add_data( ll* anchor, void* data, ll** first );


/**
 * Append to pointed list. Anchor is always updated to point to the
 * latests node added, even when list is empty.
 *
 * @param anchor Reference to node reference to append.
 * @param node   Node to append.
 *
 * @return Added node.
 */
ll* llp_append( ll** anchor, ll* node );


/**
 * Prepend to pointed list. Anchor is always updated to point to the
 * latests node added, even when list is empty.
 *
 * @param anchor Reference to node reference to prepend.
 * @param node   Node to prepend.
 *
 * @return Added node.
 */
ll* llp_prepend( ll** anchor, ll* node );


/**
 * Pull node out of its place. Fix the list connections.
 *
 *
 * @param actnode Pointer to node to pull out.
 *
 * @return Pulled-out node.
 */
ll* ll_pull( ll** actnode );


/**
 * Delete node and data. Next node is returned whenever possible (not
 * last).
 *
 *
 * @param node Node to deallocate.
 *
 * @return Next node after delete (or prev if no next).
 */
ll* ll_delete( ll* node );


/**
 * Delete node and data. Next node is always returned (even NULL).
 *
 *
 * @param node Node to deallocate.
 *
 * @return Next node after delete (or prev if no next).
 */
ll* ll_delete_sweep( ll* node );

/**
 * Delete node and data. Next node is returned whenever possible (not
 * last).
 *
 *
 * @param node Node to deallocate.
 *
 * @return Next node after delete (or prev if no next).
 */
ll* ll_delete_with_data( ll* node );


/**
 * Delete node and data. Next node is returned whenever possible (not
 * last). Anchor is updated if node is same as anchor, i.e. anchor
 * needs to be updated if the node that anchor is pointing to is
 * removed.
 *
 * @param node Node to remove.
 * @param anchor Access point to list (requires update).
 */
ll* ll_delete_and_update( ll* node, ll** anchor );


/**
 * Return first in the list.
 *
 *
 * @param node Some node in the list.
 *
 * @return First node in list.
 */
ll* ll_first( ll* node );


/** Alias to ll_first. */
#define ll_start ll_first

/** Alias to ll_first. */
#define ll_begin ll_first


/**
 * Return last in the list.
 *
 *
 * @param node Some node in the list.
 *
 * @return Last node in list.
 */
ll* ll_last( ll* node );


/** Alias to ll_last. */
#define ll_end ll_last


/**
 * Set to first in the list.
 *
 *
 * @param [in,out] node Some node in the list.
 */
ll* ll_first_set( ll** node );


/**
 * Set to last in the list.
 *
 *
 * @param node Some node in the list.
 *
 * @return Last node in list.
 */
ll* ll_last_set( ll** node );


/**
 * Find node from list.
 *
 *
 * @param node Some node in the list. Search from current location.
 * @param find Node to find.
 *
 * @return Return node if found, otherwise NULL.
 */
ll* ll_find_node( ll* node, ll* find );


/**
 * Find node containing data from list.
 *
 *
 * @param node Some node in the list. Search from current location.
 * @param data Data (pointer) to match.
 *
 * @return Return node if found, otherwise NULL.
 */
ll* ll_find_data( ll* node, void* data );


/**
 * Find node containing data from list with provided compare function.
 *
 *
 * @param node Some node in the list. Search from current location.
 * @param find Data to find.
 * @param comp Function pointer to compare data.
 *
 * @return Return node if found, otherwise NULL.
 */
ll* ll_find_with( ll* node,
                  void* find,
                  ll_comp_func comp );


/**
 * Calculate list length.
 *
 *
 * @param node Some node in the list.
 *
 * @return Number of nodes in the list.
 */
int64_t ll_length( ll* node );


/**
 * Calculate list length from node onwards.
 *
 *
 * @param node Some node in the list.
 *
 * @return Number of nodes in the list.
 */
int64_t ll_tail_length( ll* node );


/**
 * Return next node.
 *
 *
 * @param node Reference node.
 *
 * @return Next node.
 */
ll* ll_next( ll* node );


/**
 * Return previous node.
 *
 *
 * @param node Reference node.
 *
 * @return Previous node.
 */
ll* ll_prev( ll* node );


/**
 * Set next node.
 *
 *
 * @param [in,out] node Reference node address.
 *
 * @return Next node.
 */
ll* ll_next_set( ll** node );


/**
 * Set prev node.
 *
 *
 * @param [in,out] node Reference node address.
 *
 * @return Previous node.
 */
ll* ll_prev_set( ll** node );


/**
 * Move forward by n nodes.
 *
 *
 * @param [in,out] node Reference node to update.
 * @param n Move count.
 *
 * @return Number of steps performed.
 */
int64_t ll_next_n( ll** node, int64_t n );


/**
 * Move backwards by n nodes.
 *
 *
 * @param [in,out] node Reference node to update.
 * @param n Move count.
 *
 * @return Number of steps performed.
 */
int64_t ll_prev_n( ll** node, int64_t n );


/**
 * Node at start of list.
 *
 *
 * @param node Reference node.
 *
 * @return True if node is first.
 */
mc_bool_t ll_at_first( ll* node );


/**
 * Node at end of list.
 *
 *
 * @param node Reference node.
 *
 * @return True if node is last.
 */
mc_bool_t ll_at_last( ll* node );


/**
 * List iterator, i.e. call func for each node data.
 *
 *
 * @param node Reference to list (including data) to deallocate.
 * @param func Function pointer to call for each iteration, with args:
 *   (void* data).
 */
void ll_each_func( ll* node, void (*func) (void* data) );


/**
 * List iterator, i.e call func for each node data and pass additional
 * arg (for context).
 *
 *
 * @param node Reference to list (including data) to deallocate.
 * @param func Function pointer to call for each iteration, with args:
 *   (void* data, void* arg).
 * @param arg Addition arg for function.
 */
void ll_each_with_arg( ll* node,
                       void (*func) (void* data, void* arg),
                       void* arg );


/**
 * Split list at node making node the first node on the right hand
 * side list. Left hand side list hand is returned and will point to
 * the end of the list.
 *
 *
 * @param node Node from which right hand side list begins after split
 *   (split point).
 */
ll* ll_split( ll* node );


/**
 * Convert list to array. Array with NULL termination is
 * allocated. All data items from list are copied (reference, no
 * content) to the array from list beginning (see: ll_tail_to_array).
 *
 * @param node List reference.
 *
 * @return Array including data items.
 */
void** ll_to_array( ll* node );


/**
 * Convert list to array. Array with NULL termination is
 * allocated. All data items from list are copied (reference, no
 * content) to the array from node onwards.
 *
 * @param node List reference.
 *
 * @return Array including data items.
 */
void** ll_tail_to_array( ll* node );


/**
 * Convert array to list. Array must be NULL terminated.
 * All data items from array are copied (reference, no
 * content) to the list.
 *
 * @param arr Source array.
 *
 * @return List with containing array data items.
 */
ll* ll_from_array( void** arr );


/**
 * Convert array to list. Array does not have to be NULL terminated.
 * All data items from array are copied (reference, no content) to the
 * list.
 *
 * @param arr Source array.
 * @param size Size of array.
 */
ll* ll_from_sized_array( void** arr, size_t size );


/**
 * Duplicate list, but not the data.
 *
 * @param node Start point for duplication.
 *
 * @return Duplicate at start.
 */
ll* ll_duplicate( ll* node );


/**
 * Push data item to stack, i.e. to the start of the list. Node must
 * be NULL or at the start of the list.
 *
 * @param node Start of list node pointer.
 * @param data Data to add.
 *
 * @return List end node.
 */
void ll_push( ll** node, void* data );


/**
 * Pop data item from stack.
 *
 * @param node Start of list node pointer.
 *
 * @return Data.
 */
void* ll_pop( ll** node );


/**
 * Pop data item from the back of the list and free the data if
 * dealloc is non-null by calling dealloc callback.
 *
 * @param node Some node in the list.
 * @param rem  Function that performs data deallocation (if NULL, not used).
 * @param data Pointer to data storage location (no passing if NULL).
 *
 * @return List end node (after delete).
 */
ll* ll_pop_with_rem( ll* node, ll_rem_func rem, void** data );


/**
 * Sort list.
 *
 * @param list List begin node.
 * @param comp Compare function.
 *
 * @return First node of sorted list.
 */
ll* ll_sort( ll* list, ll_compar comp );




/* ------------------------------------------------------------
 * Super/sub list:
 * ------------------------------------------------------------ */


/**
 * Macro for hier list iteration.
 *
 * @param list Hier list node.
 * @param iter Iterator variable.
 */
#define ll_hier_each(list,iter) for(ll*(iter)=ll_hier_first(list);(iter);(iter)=ll_hier_next(list))


/**
 * Add sublist to super list container. Anchor can also be NULL.
 *
 * For examples:
 * @see ll_hier_first
 *
 *
 * @param anchor Node reference to append.
 * @param sublist Sublist to append. Node is created for sublist.
 *
 * @return Allocated superlist node.
 */
ll* ll_hier_add( ll* anchor, ll* sublist );


/**
 * ll_hier_add and return list start;
 *
 * For examples:
 * @see ll_hier_first
 *
 *
 * @param anchor Node reference to append.
 * @param sublist Sublist to append. Node is created for sublist.
 *
 * @return First node of superlist.
 */
ll* ll_hier_add_last( ll* anchor, ll* sublist );


/**
 * Return first entry in the superlist.
 *
 * Example:
   @code
     ll* subnode, *supernode;
     ll* supernode;
     superlist = ll_hier_add( superlist, sublist1 );
     superlist = ll_hier_add( superlist, sublist2 );
     for ( subnode = ll_hier_first( superlist ); subnode; subnode = ll_hier_next( superlist ) )
       {
         printf( "%s\n", ( (char*) subnode->data ) );
       }
   @endcode
 *
 * @param node Super list node.
 *
 * @return Sublist node or NULL.
 */

ll* ll_hier_first( ll* node );


/**
 * Iterate over super/sub list. This function is intended to read-only
 * iteration over complete lists.
 *
 * Move to the next sublist node. "node" is an ll in which data points
 * to sub ll. Each sublist is travelled upto the end, and next follows
 * when a sublist is at the end. When sublist end is encountered the
 * super list "data" is automatically returned to the start of sublist
 * for the next iteration.
 *
 * For examples:
 * @see ll_hier_first
 *
 * @param superfirst Super list node (first).
 *
 * @return Sublist node or NULL.
 */
ll* ll_hier_next( ll* superfirst );


#endif
