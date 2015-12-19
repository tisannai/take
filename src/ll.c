/**
 * @file ll.c
 *
 * Linked List library.
 */


#include <mc.h>
#include "ll.h"


/* autoc:version */
const char* ll_version = "0.0.1";


ll* ll_new( void )
{
  ll* node;

  node = mc_new(ll);
  node->prev = NULL;
  node->next = NULL;
  node->data = NULL;
  return node;
}


ll* ll_new_set_data( void* data )
{
  ll* node;

  node = mc_new(ll);
  node->prev = NULL;
  node->next = NULL;
  node->data = data;
  return node;
}


ll* ll_new_with_data_alloc( size_t data_size )
{
  ll* node;

  node = mc_new(ll);
  node->prev = NULL;
  node->next = NULL;
  node->data = mc_malloc( data_size );
  return node;
}


ll* ll_new_with_new_set_data( size_t data_size, void* data )
{
  ll* node;

  node = mc_new(ll);
  node->prev = NULL;
  node->next = NULL;
  node->data = mc_malloc( data_size );

  memcpy( node->data, data, data_size );

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

  node = ll_start( node );

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

  node = ll_start( node );

  while ( node )
    {
      next = node->next;
      ll_rem_with_data( node );
      node = next;
    }

  return NULL;
}


ll* ll_rem_list_with_func( ll* node, void (*func) (void* data) )
{
  ll* next;

  node = ll_start( node );

  while ( node )
    {
      next = node->next;
      func( node->data );
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
  return ll_add( anchor, ll_new_set_data( data ) );
}


ll* ll_add_last_data( ll* anchor, void* data )
{
  return ll_start( ll_add_data( anchor,  data ) );
}


ll* ll_add_unique_data( ll* anchor, void* data, ll_comp_func comp )
{
  if ( ll_find_with( ll_start( anchor ), data, comp ) )
    return anchor;

  return ll_add( anchor, ll_new_set_data( data ) );
}


ll* ll_add_hier( ll* anchor, ll* sublist )
{
  if ( sublist )
    return ll_add_data( anchor, ll_start( sublist ) );
  else
    return anchor;
}


ll* ll_add_last_hier( ll* anchor, ll* sublist )
{
  if ( sublist )
    return ll_start( ll_add_data( anchor, ll_start( sublist ) ) );
  else
    return ll_start( anchor );
}


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


ll* ll_start( ll* node )
{
  if ( node == NULL )
    return node;

  while ( node->prev != NULL ) node = node->prev;

  return node;
}


ll* ll_end( ll* node )
{
  if ( node == NULL )
    return node;

  while ( node->next != NULL ) node = node->next;

  return node;
}


ll* ll_start2( ll** node )
{
  *node = ll_start( *node );
  return (*node);
}


ll* ll_end2( ll** node )
{
  *node = ll_end( *node );
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
  return ll_length2( ll_start( node ) );
}


int64_t ll_length2( ll* node )
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


inline ll* ll_next2( ll** node )
{
  if ( *node )
    *node = (*node)->next;
  return (*node);
}


inline ll* ll_prev2( ll** node )
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


inline ll* ll_hier_first( ll* node )
{
  if ( node )
    {    
      node->prev = node;
      return ll_start( (ll*) node->data );
    }
  else
    return NULL;
}


inline ll* ll_hier_next( ll* node )
{
  ll* subnode, *supernode;

  /* Superlist start node. */
  supernode = node;

  /* Current supernode. */
  node = node->prev;

  /* Current subnode. */
  subnode = (ll*) node->data;


  if ( subnode->next )
    {
      /* Continue on this sublist. */
      subnode = ll_next( subnode );
      node->data = subnode;
      return subnode;
    }
  else
    {
      /* This sublist list is at the end. Try to continue with the
         next sublist or return NULL. */
      
      /* Move pointer to the start of sublist for next iteration. */
      node->data = (void*) ll_start( subnode );

      if ( node->next )
        {
          /* Another sublist exists, continue with the new sublist. */
          node->prev = node->next;
          node = node->prev;
          subnode = ll_start( (ll*) node->data );
          return subnode;
        }
      else
        {
          /* End of all sublists. */

          /* Move super-list pointer to the start of super list for
             next iteration, i.e. revert prev of superlist's first node
             back to NULL. */
          supernode->prev = NULL;
          
          return NULL;
        }
    }
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


inline mc_bool_t ll_at_start( ll* node )
{
  if ( node->prev == NULL )
    return true;
  else
    return false;
}


inline mc_bool_t ll_at_end( ll* node )
{
  if ( node->next == NULL )
    return true;
  else
    return false;
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
  return ll_to_array2( ll_start( node ) );
}  


void** ll_to_array2( ll* node )
{
  void** arr;
  int64_t i;
  ll* n;
  
  arr = mc_new_n( void*, ll_length2( node ) + 1 );
  
  n = node;
  i = 0;
  while ( n )
    {
      arr[i] = n->data;
      ll_next2( &n );
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
      list = ll_add( list, ll_new_set_data( (void*) arr[i] ) );
      i++;
    }

  return ll_start( list );
}  


ll* ll_from_sized_array( void** arr, size_t size )
{
  int64_t i;
  ll* list = NULL;
  
  i = 0;
  while ( i < size )
    {
      list = ll_add( list, ll_new_set_data( (void*) arr[i] ) );
      i++;
    }

  return ll_start( list );
}  


ll* ll_duplicate( ll* node )
{
  ll* t;
  ll* dup = NULL;

  t = node;

  while ( t )
    {
      dup = ll_add( dup, ll_new_set_data( t->data ) );
      t = ll_next( t );
    }

  dup = ll_start( dup );
  
  return dup;
}


void** ll_freeze( ll* node )
{
  void** freeze;
  int64_t len;
  int64_t i;

  node = ll_start( node );
  len = ll_length2( node );
  freeze = mc_new_n( void*, len+1 );

  for ( i = 0; i < len; i++ )
    {
      freeze[ i ] = node->data;
      ll_next2( &node );
    }

  freeze[ i ] = NULL;

  return freeze;
}


ll* ll_unfreeze( void** freeze )
{
  ll* node;

  if ( freeze[ 0 ] == NULL )
    return NULL;

  node = ll_new();
  node->data = freeze[ 0 ];
  
  for ( int64_t i = 1; freeze[ i ]; i++ )
    {
      node = ll_append( node, ll_new_set_data( freeze[ i ] ) );
    }

  return ll_start( node );
}


ll* ll_push( ll* node, void* data )
{
  node = ll_end( node );
  return ll_add_data( node, data );
}


ll* ll_pop( ll* node, void** data )
{
  return ll_pop_with_rem( node, NULL, data );
}


ll* ll_pop_with_rem( ll* node, ll_dealloc_func dealloc, void** data )
{
  node = ll_end( node );
  if ( data )
    *data = node->data;
  if ( dealloc )
    dealloc( node->data );
  return ll_delete( node );
}
