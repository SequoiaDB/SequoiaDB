/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

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

#include "oss.hpp"
#include "ossMemPool.hpp"
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
        _ownedBuffer( FALSE ),
        _buffer( NULL )
      {
      }

      _utilCircularBuffer( T *buffer, UINT32 capacity )
      : _capacity( capacity ),
        _front( 0 ),
        _used( 0 ),
        _ownedBuffer( FALSE ),
        _buffer( buffer )
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
            T *buffer = (T *)( SDB_OSS_MALLOC( capacity * sizeof( T ) ) ) ;
            if ( NULL != buffer )
            {
               _buffer = (T *)buffer ;
               _capacity = capacity ;
               _ownedBuffer = TRUE ;
               return TRUE ;
            }
         }
         return FALSE ;
      }

      BOOLEAN initBuffer( T *buffer, UINT32 capacity )
      {
         finiBuffer() ;
         if ( capacity > 0 && NULL != buffer )
         {
            _buffer = buffer ;
            _capacity = capacity ;
            _ownedBuffer = FALSE ;
            return TRUE ;
         }
         return FALSE ;
      }

      // finalize buffer
      void finiBuffer()
      {
         if ( NULL != _buffer && _ownedBuffer )
         {
            SDB_OSS_FREE( _buffer ) ;
         }
         _capacity = 0 ;
         _front = 0 ;
         _used = 0 ;
         _buffer = NULL ;
         _ownedBuffer = FALSE ;
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
      // whether to own buffer
      BOOLEAN           _ownedBuffer ;
      // buffer
      T *               _buffer ;
   } ;

   /*
      _utilCircularStackBuffer define
    */
   template< typename T, UINT32 stackSize >
   class _utilCircularStackBuffer : public _utilCircularBuffer< T >
   {
   public:
      _utilCircularStackBuffer()
      : _utilCircularBuffer< T >( _stackBuffer, stackSize )
      {
      }

      ~_utilCircularStackBuffer()
      {
      }

   protected:
      T _stackBuffer[ stackSize ] ;
   } ;

   /*
      _utilCircularQueue define
    */
   // queue with circular buffer
   // WARNING: thread NOT safe
   template< typename T >
   class _utilCircularQueue : public SDBObject
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

   //The utilSPSCQueue is a single producer single consumer ring fifo queue.
   template< typename T >
   class utilSPSCQueue : public SDBObject
   {
   public:
      utilSPSCQueue() : _capacity( 0 ),
                        _mask( 0 ),
                        _buffer( NULL ),
                        _writeIndex( 0 ),
                        _readIndex( 0 )
      {
      }

      ~utilSPSCQueue()
      {
         _finiBuffer() ;
      }

      INT32 init( UINT64 capacity )
      {
         INT32 rc = SDB_OK ;
         SDB_ASSERT( capacity > 0, "capacity must be greater than 0" ) ;
         SDB_ASSERT( ( capacity & ( capacity - 1 ) ) == 0,
                     "capacity must be a power of 2" ) ;

         _finiBuffer() ;

         _buffer = (T *)( SDB_OSS_MALLOC( capacity * sizeof( T ) ) ) ;
         if ( NULL == _buffer )
         {
            rc = SDB_OOM ;
            goto error ;
         }

         _capacity = capacity ;
         _mask = capacity - 1 ;

      done:
         return rc ;
      error:
         goto done ;
      }

      BOOLEAN push( const T& value )
      {
         UINT64 writeIndex = _writeIndex.peek() ;

         if ( writeIndex - _readIndex.fetch() >= _capacity )
         {
            //queue full
            return FALSE ;
         }

         _buffer[writeIndex & _mask] = value ;

         _writeIndex.inc() ;

         return TRUE ;
      }

      BOOLEAN pop( T& value )
      {
         UINT64 readIndex = _readIndex.peek() ;

         if ( readIndex >= _writeIndex.fetch() )
         {
            //queue empty
            return FALSE ;
         }

         value = _buffer[readIndex & _mask] ;

         _readIndex.inc() ;

         return TRUE ;
      }
   private:
      void _finiBuffer()
      {
         SAFE_OSS_FREE( _buffer ) ;
         _capacity = 0 ;
      }

   private:
      UINT64 _capacity ;
      UINT64 _mask ;
      T*     _buffer ;

      ossAtomic64 _writeIndex ;
      ossAtomic64 _readIndex ;
   } ;
}

#endif // UTIL_CIRCULAR_QUEUE_HPP__
