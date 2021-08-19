/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = utilCircularQueue.hpp

   Descriptive Name = Circular Queue Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2020  HGM  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_CIRCULAR_QUEUE_HPP__
#define UTIL_CIRCULAR_QUEUE_HPP__

#include "utilMemListPool.hpp"
#include "pd.hpp"

#pragma warning( disable: 4200 )

namespace engine
{

   /*
      _utilCircularBuffer define
    */
   // circular buffer holds a buffer which could be recycle by queue
   // WARNING: thread NOT safe
   template< typename T >
   class _utilCircularBuffer : public SDBObject
   {
   public:
      _utilCircularBuffer()
      : _capacity( 0 ),
        _front( 0 ),
        _used( 0 ),
        _buffer( NULL )
      {
      }

      ~_utilCircularBuffer()
      {
         finiBuffer() ;
      }

   public:
      // initialize buffer
      BOOLEAN initBuffer( UINT32 capacity )
      {
         finiBuffer() ;

         if ( capacity > 0 )
         {
            T *buffer = (T *)( SDB_THREAD_ALLOC( capacity * sizeof( T ) ) ) ;
            if ( NULL != buffer )
            {
               _buffer = (T *)buffer ;
               _capacity = capacity ;
               return TRUE ;
            }
         }
         return FALSE ;
      }

      // finalize buffer
      void finiBuffer()
      {
         if ( NULL != _buffer )
         {
            SDB_THREAD_FREE( _buffer ) ;
         }
         _capacity = 0 ;
         _front = 0 ;
         _used = 0 ;
         _buffer = NULL ;
      }

      // get capacity of buffer
      UINT32 getCapacity() const
      {
         return _capacity ;
      }

      // get used size of buffer
      UINT32 getUsedSize() const
      {
         return _used ;
      }

      // check if buffer is full
      BOOLEAN isFull() const
      {
         return _capacity == _used ;
      }

      // check if buffer is empty
      BOOLEAN isEmpty() const
      {
         return 0 == _used ;
      }

      // push the last element
      BOOLEAN pushBack( const T &value )
      {
         if ( _capacity > 0 && _used < _capacity )
         {
            SDB_ASSERT( NULL != _buffer, "buffer is invalid" ) ;
            UINT32 nextIndex = ( _front + _used ) % _capacity ;
            _buffer[ nextIndex ] = value ;
            ++ _used ;

            return TRUE ;
         }

         return FALSE ;
      }

      // pop the first element
      BOOLEAN popFront()
      {
         if ( _used > 0 )
         {
            _front = ( _front + 1 ) % _capacity ;
            -- _used ;
            return TRUE ;
         }
         return FALSE ;
      }

      // get the first pointer
      T *getFront()
      {
         if ( 0 == _used )
         {
            return NULL ;
         }
         SDB_ASSERT( NULL != _buffer, "buffer is invalid" ) ;
         return &( _buffer[ _front ] ) ;
      }

      // get the first pointer
      const T *getFront() const
      {
         if ( 0 == _used )
         {
            return NULL ;
         }
         SDB_ASSERT( NULL != _buffer, "buffer is invalid" ) ;
         return &( _buffer[ _front ] ) ;
      }

      // get the last pointer
      T *getBack()
      {
         if ( 0 == _used )
         {
            return NULL ;
         }
         SDB_ASSERT( NULL != _buffer, "buffer is invalid" ) ;
         return &( _buffer[ ( _front + _used - 1 ) % _capacity ] ) ;
      }

      // get the last pointer
      const T *getBack() const
      {
         if ( 0 == _used )
         {
            return NULL ;
         }
         SDB_ASSERT( NULL != _buffer, "buffer is invalid" ) ;
         return &( _buffer[ ( _front + _used - 1 ) % _capacity ] ) ;
      }

   protected:
      // capacity of buffer
      UINT32            _capacity ;
      // begin of used positions
      UINT32            _front ;
      // size of used positions
      UINT32            _used ;
      // buffer
      T *               _buffer ;
   } ;

   /*
      _utilCircularQueue define
    */
   // queue with circular buffer
   // WARNING: thread NOT safe
   template< typename T >
   class _utilCircularQueue : public utilPooledObject
   {
   public:
      typedef size_t       size_type ;
      typedef T            value_type ;
      typedef T *          pointer ;
      typedef const T *    const_pointer ;
      typedef T &          reference ;
      typedef const T &    const_reference ;

   public:
      _utilCircularQueue( _utilCircularBuffer<T> *buffer = NULL )
      : _buffer( buffer )
      {
      }

      _utilCircularQueue( const _utilCircularQueue<T> &queue )
      : _buffer( queue._buffer ),
        _extQueue( queue._extQueue )
      {
      }

      ~_utilCircularQueue()
      {
      }

   public:
      // check if empty
      bool empty() const
      {
         return ( NULL == _buffer || _buffer->isEmpty() ) && _extQueue.empty() ;
      }

      // get size
      size_type size() const
      {
         size_type res = ( NULL != _buffer ) ? ( _buffer->getUsedSize() ) : 0 ;
         res += _extQueue.size() ;
         return res ;
      }

      // get the first element
      value_type &front()
      {
         if ( NULL != _buffer )
         {
            value_type *ptr = _buffer->getFront() ;
            if ( NULL != ptr )
            {
               return *ptr ;
            }
         }
         return _extQueue.front() ;
      }

      // get the first element
      const value_type &front() const
      {
         if ( NULL != _buffer )
         {
            const value_type *ptr = _buffer->getFront() ;
            if ( NULL != ptr )
            {
               return *ptr ;
            }
         }
         return _extQueue.front() ;
      }

      // get the last element
      value_type &back()
      {
         if ( NULL != _buffer )
         {
            value_type *ptr = _buffer->getBack() ;
            if ( NULL != ptr )
            {
               return *ptr ;
            }
         }
         return _extQueue.back() ;
      }

      // get the last element
      const value_type &back() const
      {
         if ( NULL != _buffer )
         {
            const value_type *ptr = _buffer->getBack() ;
            if ( NULL != ptr )
            {
               return *ptr ;
            }
         }
         return _extQueue.back() ;
      }

      // push the last element
      void push_back( const value_type &value )
      {
         if ( NULL == _buffer )
         {
            // buffer is invalid, use external queue
            _extQueue.push_back( value ) ;
         }
         else if ( !_buffer->pushBack( value ) )
         {
            // buffer is full, use external queue
            _extQueue.push_back( value ) ;
         }
      }

      // pop the first element
      void pop_front()
      {
         if ( NULL == _buffer )
         {
            // buffer is invalid
            _extQueue.pop_front() ;
         }
         else if ( _buffer->popFront() )
         {
            // check if we need to move one value from external queue
            if ( !_extQueue.empty() )
            {
               _buffer->pushBack( _extQueue.front() ) ;
               _extQueue.pop_front() ;
            }
         }
      }

   protected:
      // circular buffer
      _utilCircularBuffer<T> *   _buffer ;
      // external queue ( to be used when buffer is full )
      ossPoolDeque<T>            _extQueue ;
   } ;

}

#endif // UTIL_CIRCULAR_QUEUE_HPP__
