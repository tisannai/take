/**
 * @file mcp.c
 *
 * Automatic allocation for array of mcp.
 */

/*
 * Common headers:
 */
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <mc.h>
#include "mcp.h"


const char* mcp_version = "0.0.1";



/*
 * *************************************************************
 * Auto-resize memory allocation.
 */


mc_bool_t mcp_default_resizer( mcp_p aa, mc_size_t newsize )
{
  if ( newsize > aa->size )
    {
      while ( newsize > aa->size )
        aa->size = aa->size*2;
      mcp_resize_to( aa, aa->size );
      return true;
    }
  else if ( newsize < aa->size/2 )
    {
      /* Ensure that size is same or more than 1 and ensure that size is
         always bigger than aa->used. */
      while ( ( aa->size / 2 ) >= 1 &&
              ( aa->size / 2 ) > newsize )
        aa->size /= 2;
      mcp_resize_to( aa, aa->size );
      return true;
    }
  else
    return false;
}


mc_bool_t mcp_enlarge_resizer( mcp_p aa, mc_size_t newsize )
{
  if ( newsize > aa->size )
    {
      while ( newsize > aa->size )
        aa->size = aa->size*2;
      mcp_resize_to( aa, aa->size );
      return true;
    }
  else
    return false;
}


/** Resize function pointer for Autoarr Memory. */
mcp_resize_func_t mcp_resize_func = mcp_default_resizer;


mcp_p mcp_new_size( mc_size_t size )
{
  mcp_p aa;

  assert( size >= 1 );

  aa = mc_new_n( mcp_t, 1 );
  aa->size = size;
  aa->used = 0;
  aa->data = mc_new_n( void*, size );
  aa->resize = mcp_default_resizer;

  return aa;
}


mcp_p mcp_new( void )
{
  return mcp_new_size( MCP_DEFAULT_SIZE );
}


mcp_p mcp_del( mcp_p aa )
{
  if ( aa )
    {
      mc_del( (void*) aa->data );
      mc_del( aa );
    }
  return NULL;
}


void mcp_copy( mcp_p aa, mcp_p to )
{
  if ( aa->used > to->used )
    mcp_resize_to( to, aa->used );
  to->used = aa->used;
//  memcpy( (void*) to->data, (void*) aa->data, mcp_usedsize(aa) );
  mc_memcpy( aa->data, to->data, mcp_usedsize(aa) );
}


mcp_p mcp_dup( mcp_p aa )
{
  mcp_p dup;
  dup = mcp_new_size( aa->size );
  mcp_copy( aa, dup );
  return dup;
}


void mcp_reset( mcp_p aa )
{
  aa->used = 0;
}


void mcp_delete_all( mcp_p aa )
{
  aa->used = 0;
}



void mcp_resize( mcp_p aa, mc_size_t size )
{
  aa->resize( aa, size );
}



void mcp_resize_to( mcp_p aa, mc_size_t size )
{
  aa->size = size;
  aa->data = ( void** ) mc_realloc( (void*) aa->data, mcp_bytesize(aa) );
}


void mcp_compact( mcp_p aa )
{
  aa->size = aa->used;
  aa->data = ( void** ) mc_realloc( (void*) aa->data, mcp_bytesize(aa) );
}


void mcp_insert_n_to( mcp_p aa, mc_size_t pos, void** data, mc_size_t len )
{

  /* Disallow holes. */
  assert( pos <= aa->used );

  mcp_resize( aa, aa->used+len );

  /* Move data to make room for new. */
  if ( pos < aa->used )
    {
//      memmove( &( aa->data[ pos+len ] ),
//               &( aa->data[pos] ),
//               ( aa->used - pos ) * mcp_sizeof );
      mc_memmove( &( aa->data[pos] ),
                  &( aa->data[ pos+len ] ),
                  ( aa->used - pos ) * mcp_sizeof );
    }

//  memcpy( &( aa->data[pos] ), data, len * mcp_sizeof );
  mc_memcpy( data, &( aa->data[pos] ), len * mcp_sizeof );

  aa->used += len;
}


void mcp_insert_to( mcp_p aa, mc_size_t pos, void* data )
{
  mcp_insert_n_to( aa, pos, &data, 1 );
}


void mcp_delete_n_at( mcp_p aa, mc_size_t pos, mc_size_t len )
{

  /* Disallow holes. */
  assert( pos <= aa->used );
  assert( pos+len <= aa->size );

  /* Move data to make room for new. */
  if ( pos < aa->used )
    mc_memmove( &( aa->data[ pos+len ] ),
                &( aa->data[ pos ] ),
                ( aa->used - pos - len ) * mcp_sizeof );
  //    memmove( &( aa->data[ pos ] ),
  //             &( aa->data[ pos+len ] ),
  //             ( aa->used - pos - len ) * mcp_sizeof );

  aa->used -= len;

  mcp_resize( aa, aa->used-len );
}


void mcp_delete_at( mcp_p aa, mc_size_t pos )
{
  mcp_delete_n_at( aa, pos, 1 );
}


void mcp_delete_n_end( mcp_p aa, mc_size_t len )
{
  /* Check for holes in rem. */
  assert( len <= aa->used );

  aa->used -= len;
  mcp_resize( aa, aa->used );
}


void mcp_assign_to( mcp_p aa, mc_size_t pos, void** data, mc_size_t len )
{
  /* Overwrite. */
  mc_size_t ow;

  ow = aa->used - pos;

  /* Check for holes in set. */
  assert( ow >= 0 );

  if ( ( len - ow ) > 0 )
    {
      /* Grow before copy. */
      mcp_resize( aa, ( aa->used + len - ow ) );
      aa->used += ( len - ow );
    }
  
  /* Copy new data. */
//  memcpy( &( aa->data[ pos ] ), data, len * mcp_sizeof );
  mc_memcpy( data, &( aa->data[ pos ] ), len * mcp_sizeof );

}


void mcp_assign( mcp_p aa, void** data, mc_size_t len )
{
  mcp_reset( aa );
  mcp_insert_n_to( aa, 0, data, len );
}


void mcp_append( mcp_p aa, void* data )
{
  mcp_insert_n_to( aa, aa->used, &data, 1 );
}


void mcp_append_n( mcp_p aa, void** data, mc_size_t len )
{
  mcp_insert_n_to( aa, aa->used, data, len );
}


mc_bool_t mcp_append_unique( mcp_p aa, void* data )
{
  if ( !mcp_find( aa, data ) )
    {
      mcp_append( aa, data );
      return true;
    }
  else
    {
      return false;
    }
}


void mcp_prepend( mcp_p aa, void* data )
{
  mcp_insert_n_to( aa, 0, &data, 1 );
}


void mcp_prepend_n( mcp_p aa, void** data, mc_size_t len )
{
  mcp_insert_n_to( aa, 0, data, len );
}


mc_size_t mcp_find_idx( mcp_p aa, void* data )
{
  for ( mc_size_t i = 0; i < aa->used; i++ )
    {
      if ( aa->data[ i ] == data )
        return i;
    }
  
  return MCP_INVALID_INDEX;
}


mc_bool_t mcp_find( mcp_p aa, void* data )
{
  mc_size_t idx;

  idx = mcp_find_idx( aa, data );

  if ( idx != MCP_INVALID_INDEX )
    return true;
  else
    return false;
}


void mcp_terminate( mcp_p aa )
{
  if ( aa->data[ aa->used-1 ] != 0 )
    {
      mcp_append( aa, 0 );
      aa->used--;
    }
}


void mcp_push( mcp_p s, void* item )
{
  mcp_append( s, item );
}


void* mcp_pop( mcp_p s )
{
  void* d;
  
  d = s->data[ s->used-1 ];
  mcp_delete_n_end( s, 1 );
  return d;
}


void* mcp_peek( mcp_p s )
{
  return s->data[ s->used-1 ];
}


mc_bool_t mcp_empty( mcp_p aa )
{
  if ( aa->used == 0 )
    return true;
  else
    return false;
}
