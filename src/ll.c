/**
 * @file ll.c
 *
 * Linked List library.
 */


#include <mc.h>
#include "ll.h"


/* autoc:version */
const char* ll_version = "0.0.1";



/* ------------------------------------------------------------
 * Internal functions:
 * ------------------------------------------------------------ */

/**
 * Delete node and data. Next node is returned whenever possible (not
 * last).
 *
 *
 * @param node Node to deallocate.
 *
 * @return Next node after delete (or prev if no next).
 */
ll* ll_node_after_delete( ll* node )
{
  ll* ret;

  ret = node;

  if ( node->prev && node->next )
    {
      // Connections: left <-> NODE <-> right
      node->prev->next = node->next;
      node->next->prev = node->prev;
      // Return next node.
      ret = node->next;
    }
  else if ( node->prev )
    {
      // Connections: left <-> NODE <-> *
      node->prev->next = NULL;
      // Return prev node.
      ret = node->prev;
    }
  else if ( node->next )
    {
      // Connections: * <-> NODE <-> right
      node->next->prev = NULL;
      // Return next node.
      ret = node->next;
    }
  else
    {
      // Last node.
      // Connections: * <-> NODE <-> *
      ret = NULL;
    }

  return ret;
}


/* ------------------------------------------------------------
 * Linked list:
 * ------------------------------------------------------------ */


ll* ll_new( void* data )
{
  ll* node;

  node = mc_new( ll );
  node->prev = NULL;
  node->next = NULL;
  node->data = data;
  return node;
}


void ll_rem( ll* node )
{
  mc_free( node );
}


void ll_rem_with_data( ll* node )
{
  mc_free( node->data );
  mc_free( node );
}


ll* ll_rem_list( ll* node )
{
  ll* next;

  node = ll_first( node );

  while ( node )
    {
      next = node->next;
      ll_rem( node );
      node = next;
    }
  
  return NULL;
}


ll* ll_rem_list_with_data( ll* node )
{
  ll* next;

  node = ll_first( node );

  while ( node )
    {
      next = node->next;
      ll_rem_with_data( node );
      node = next;
    }

  return NULL;
}


ll* ll_rem_list_with_func( ll* node, ll_rem_func rem )
{
  ll* next;

  node = ll_first( node );

  while ( node )
    {
      next = node->next;
      rem( node->data );
      ll_rem( node );
      node = next;
    }

  return NULL;
}


ll* ll_connect( ll* left, ll* right, ll* node )
{
  if ( left == NULL && right == NULL )
    {
      /* Empty list. */
    }
  else if ( left == NULL )
    {
      /* Prepending the list. */
      node->next = right;
      right->prev = node;
    }
  else if ( right == NULL )
    {
      /* Appending the list. */
      node->prev = left;
      left->next = node;
    }
  else
    {
      /* Inserting in the middle of the list. */
      left->next = node;
      node->prev = left;
      right->prev = node;
      node->next = right;
    }

  return node;
}


ll* ll_append( ll* anchor, ll* node )
{
  return ll_connect( anchor, anchor->next, node );
}


ll* ll_prepend( ll* anchor, ll* node )
{
  return ll_connect( anchor->prev, anchor, node );
}


ll* ll_add( ll* anchor, ll* node )
{
  if ( anchor )
    return ll_connect( anchor, anchor->next, node );
  else
    return ll_connect( NULL, NULL, node );
}


ll* ll_add_data( ll* anchor, void* data )
{
  return ll_add( anchor, ll_new( data ) );
}


ll* ll_add_data_end( ll* anchor, void* data )
{
  if ( anchor && anchor->prev == NULL )
    {
      ll_add_data( ll_last( anchor ),  data );
      return anchor;
    }
  else
    {
      return ll_first( ll_add_data( ll_last( anchor ),  data ) );
    }
}


ll* ll_add_last_data( ll* anchor, void* data )
{
  return ll_first( ll_add_data( anchor,  data ) );
}


ll* ll_add_unique_data( ll* anchor, void* data, ll_comp_func comp )
{
  if ( ll_find_with( ll_first( anchor ), data, comp ) )
    return anchor;

  return ll_add( anchor, ll_new( data ) );
}


ll* ll_synced_add( ll* anchor, ll* node, ll** first )
{
  if ( anchor )
    return ll_connect( anchor, anchor->next, node );
  else
    {
      *first = node;
      return ll_connect( NULL, NULL, node );
    }
}


ll* ll_synced_add_data( ll* anchor, void* data, ll** first )
{
  return ll_synced_add( anchor, ll_new( data), first );
}


ll* llp_append( ll** anchor, ll* node )
{
  if ( *anchor )
    *anchor = ll_connect( *anchor, (*anchor)->next, node );
  else
    *anchor = ll_connect( NULL, NULL, node );

  return *anchor;
}


ll* llp_prepend( ll** anchor, ll* node )
{
  if ( *anchor )
    *anchor = ll_connect( (*anchor)->prev, *anchor, node );
  else
    *anchor = ll_connect( NULL, NULL, node );

  return *anchor;
}


ll* ll_pull( ll** actnode )
{
  ll* node;

  node = *actnode;

  if ( node->prev && node->next )
    {
      // Connections: left <-> NODE <-> right
      node->prev->next = node->next;
      node->next->prev = node->prev;
      // Return next node.
      *actnode = node->next;
    }
  else if ( node->prev )
    {
      // Connections: left <-> NODE <-> *
      node->prev->next = NULL;
      // Return prev node.
      *actnode = node->prev;
    }
  else if ( node->next )
    {
      // Connections: * <-> NODE <-> right
      node->next->prev = NULL;
      // Return next node.
      *actnode = node->next;
    }
  else
    {
      // Last node.
      // Connections: * <-> NODE <-> *
      *actnode = NULL;
    }

  return node;
}


ll* ll_delete( ll* node )
{
  ll* ret;
  ret = ll_node_after_delete( node );
  ll_rem( node );
  return ret;
}



ll* ll_delete_sweep( ll* node )
{
  ll* ret;
  if ( node->next )
    {
      ret = ll_node_after_delete( node );
    }
  else
    {
      ret = ll_node_after_delete( node );
      ret = NULL;
    }
  ll_rem( node );
  return ret;
}


ll* ll_delete_with_data( ll* node )
{
  ll* ret;
  ret = ll_node_after_delete( node );
  ll_rem_with_data( node );
  return ret;
}


ll* ll_delete_and_update( ll* node, ll** anchor )
{
  if ( *anchor == node )
    {
      *anchor = ll_delete( node );
      return *anchor;
    }
  else
    return ll_delete( node );
}


ll* ll_first( ll* node )
{
  if ( node == NULL )
    return node;

  while ( node->prev != NULL ) node = node->prev;

  return node;
}


ll* ll_last( ll* node )
{
  if ( node == NULL )
    return node;

  while ( node->next != NULL ) node = node->next;

  return node;
}


ll* ll_first_set( ll** node )
{
  *node = ll_first( *node );
  return (*node);
}


ll* ll_last_set( ll** node )
{
  *node = ll_last( *node );
  return *node;
}


ll* ll_find_node( ll* node, ll* find )
{
  ll* cur;

  cur = node;

  while ( cur )
    {
      if ( cur == find )
        return cur;

      cur = cur->next;
    }
    
  return NULL;
}


ll* ll_find_data( ll* node, void* data )
{
  ll* cur;

  cur = node;

  while ( cur )
    {
      if ( cur->data == data )
        return cur;

      cur = cur->next;
    }
    
  return NULL;
}


ll* ll_find_with( ll* node,
                  void* find,
                  ll_comp_func compare )
{
  ll* cur;

  cur = node;

  while ( cur )
    {
      if ( compare( find, cur->data ) )
        return cur;

      cur = cur->next;
    }
    
  return NULL;
}


int64_t ll_length( ll* node )
{
  return ll_tail_length( ll_first( node ) );
}


int64_t ll_tail_length( ll* node )
{
  int64_t n;
  ll* cur;

  cur = node;
  n = 0;

  while ( cur )
    {
      cur = cur->next;
      n++;
    }
    
  return n;
}


inline ll* ll_next( ll* node )
{
  if ( node )
    return node->next;
  else
    return node;
}


inline ll* ll_prev( ll* node )
{
  if ( node )
    return node->prev;
  else
    return node;
}


inline ll* ll_next_set( ll** node )
{
  if ( *node )
    *node = (*node)->next;
  return (*node);
}


inline ll* ll_prev_set( ll** node )
{
  if ( *node )
    *node = (*node)->prev;
  return (*node);
}


int64_t ll_next_n( ll** node, int64_t n )
{
  int i;
  ll* tmp;

  tmp = *node;

  for ( i = 0; i < n; i++ )
    {
      if ( tmp->next == NULL )
        break;
      tmp = tmp->next;
    }

  *node = tmp;

  return i;
}


int64_t ll_prev_n( ll** node, int64_t n )
{
  int i;
  ll* tmp;

  tmp = *node;

  for ( i = 0; i < n; i++ )
    {
      if ( tmp->prev == NULL )
        break;
      tmp = tmp->prev;
    }

  *node = tmp;

  return i;
}


inline mc_bool_t ll_at_first( ll* node )
{
  if ( node->prev == NULL )
    return mc_true;
  else
    return mc_false;
}


inline mc_bool_t ll_at_last( ll* node )
{
  if ( node->next == NULL )
    return mc_true;
  else
    return mc_false;
}


void ll_each_func( ll* node, void (*func) (void* data) )
{
  while ( node )
    {
      func( node->data );
      node = node->next;
    }
}


void ll_each_with_arg( ll* node,
                       void (*func) (void* data, void* arg),
                       void* arg )
{
  while ( node )
    {
      func( node->data, arg );
      node = node->next;
    }
}


ll* ll_split( ll* node )
{
  ll* left;

  left = node->prev;

  if ( left )
    left->next = NULL;
        
  node->prev = NULL;

  return left;
}


void** ll_to_array( ll* node )
{
  return ll_tail_to_array( ll_first( node ) );
}  


void** ll_tail_to_array( ll* node )
{
  void** arr;
  int64_t i;
  ll* n;
  
  arr = mc_new_n( void*, ll_tail_length( node ) + 1 );
  
  n = node;
  i = 0;
  while ( n )
    {
      arr[i] = n->data;
      ll_next_set( &n );
      i++;
    }

  arr[i] = NULL;

  return arr;
}  


ll* ll_from_array( void** arr )
{
  int64_t i;
  ll* list = NULL;
  
  i = 0;
  while ( arr[i] )
    {
      list = ll_add( list, ll_new( (void*) arr[i] ) );
      i++;
    }

  return ll_first( list );
}  


ll* ll_from_sized_array( void** arr, size_t size )
{
  int64_t i;
  ll* list = NULL;
  
  i = 0;
  while ( i < size )
    {
      list = ll_add( list, ll_new( (void*) arr[i] ) );
      i++;
    }

  return ll_first( list );
}  


ll* ll_duplicate( ll* node )
{
  ll* t;
  ll* dup = NULL;

  t = node;

  while ( t )
    {
      dup = ll_add( dup, ll_new( t->data ) );
      t = ll_next( t );
    }

  dup = ll_first( dup );
  
  return dup;
}


void ll_push( ll** node, void* data )
{
  if ( *node )
    *node = ll_prepend( *node, ll_new( data ) );
  else
    *node = ll_connect( NULL, NULL, ll_new( data ) );
}


ll* ll_pop_with_rem( ll* node, ll_rem_func rem, void** data )
{
  if ( data )
    *data = node->data;
  if ( rem )
    rem( node->data );
  return ll_delete( node );
}


void* ll_pop( ll** node )
{
  void* data;
  *node = ll_pop_with_rem( *node, NULL, &data );
  return data;
}


ll* ll_sort( ll* list, ll_compar comp )
{
  /* This is a mergesort algorithm. */

  /* Trivial case. */
  if ( list == NULL || list->next == NULL )
    return list;

  ll* right = list;
  ll* temp  = list;
  ll* last  = list;
  ll* result = 0;
  ll* next   = 0;
  ll* tail   = 0;

  /* Find halfway through the list (by running two pointers, one at
     twice the speed of the other). */
  while ( temp && temp->next )
    {
      last = right;
      right = right->next;
      temp = temp->next->next;
    }

  /* Break the list in two. (prev pointers are broken here, but we fix
     later) */
  last->next = 0;

  /* Recurse on the two smaller lists: */
  list = ll_sort( list, comp );
  right = ll_sort( right, comp );

  /* Merge: */
  while ( list || right )
    {
      /* Take from empty lists, or compare: */
      if ( right == NULL ) {
        next = list;
        list = list->next;

      } else if ( list == NULL ) {
        next = right;
        right = right->next;

      } else if ( comp( list, right ) < 0 ) {
        next = list;
        list = list->next;

      } else {
        next = right;
        right = right->next;
      }

      if ( result == NULL ) {
        result = next;
      } else {
        tail->next = next;
      }

      next->prev = tail;
      tail = next;
    }

  return result;
}




/* ------------------------------------------------------------
 * Super/sub list:
 * ------------------------------------------------------------ */


ll* ll_hier_add( ll* anchor, ll* sublist )
{
  if ( sublist )
    return ll_add_data( anchor, ll_first( sublist ) );
  else
    return anchor;
}


ll* ll_hier_add_last( ll* anchor, ll* sublist )
{
  if ( sublist )
    return ll_first( ll_add_data( anchor, ll_first( sublist ) ) );
  else
    return ll_first( anchor );
}


inline ll* ll_hier_first( ll* node )
{
  if ( node )
    {    
      node = ll_first( node );
      node->prev = node;
      return ll_first( (ll*) node->data );
    }
  else
    return NULL;
}


inline ll* ll_hier_next( ll* superfirst )
{
  ll* supernode, *subnode;

  /* Current supernode. */
  supernode = superfirst->prev;

  /* Current subnode (i.e. sublist node). */
  subnode = (ll*) supernode->data;


  if ( subnode->next )
    {
      /* Continue on this sublist. */
      subnode = ll_next( subnode );
      supernode->data = subnode;
      return subnode;
    }
  else
    {
      /* This sublist list is at the end. Try to continue with the
         next sublist or return NULL. */
      
      /* Move pointer to the start of sublist for next iteration. */
      supernode->data = (void*) ll_first( subnode );

      if ( supernode->next )
        {
          /* Another sublist exists, continue with the new sublist. */
          supernode = supernode->next;

          /* Save superlist node for next iteration. */
          superfirst->prev = supernode;

          /* Make sure that we return first of next sublist. */
          subnode = ll_first( (ll*) supernode->data );

          return subnode;
        }
      else
        {
          /* End of all sublists. Set current supernode to start of
             superlist again. */
          superfirst->prev = superfirst;

          return NULL;
        }
    }
}
