/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = clsSyncMinHeap.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLSSYNCMINHEAP_HPP_
#define CLSSYNCMINHEAP_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "clsDef.hpp"
#include "ossMem.hpp"
#include "pd.hpp"
#include <iostream>

namespace engine
{
   const UINT32 CLS_DEFAULT_HEAP_SIZE = 1 ;
   const UINT32 CLS_HEAP_ROOT = 0 ;

   class _clsSyncMinHeap : public SDBObject
   {
   public:
      _clsSyncMinHeap( const UINT32 &size = CLS_DEFAULT_HEAP_SIZE ):
                       _heap( NULL ),
                        _spcSize( 0 ),
                        _dataSize( 0 )
      {
         _allocate( size ) ;
      }

      ~_clsSyncMinHeap()
      {
         if ( NULL != _heap )
         {
            SDB_OSS_FREE( _heap ) ;
         }
      }


   public:
      OSS_INLINE const UINT32 &dataSize() const
      {
         return _dataSize ;
      }

      OSS_INLINE const UINT32 &spcSize() const
      {
         return _spcSize ;
      }

      OSS_INLINE void clear()
      {
         _dataSize = 0 ;
         return ;
      }

      const _clsSyncSession &operator[]( const UINT32 &sub)
      {
         return _heap[sub] ;
      }

      OSS_INLINE INT32 push( const _clsSyncSession &session )
      {
         INT32 rc = SDB_OK ;
         if ( _dataSize == _spcSize )
         {
            rc = _allocate( _spcSize ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }

         _heap[_dataSize] = session;
         _shiftUp( _dataSize ) ;
         ++_dataSize ;
      done:
         return rc ;
      error:
         goto done ;
      }

      OSS_INLINE INT32 pop( _clsSyncSession &session )
      {
         INT32 rc = SDB_OK ;
         if ( 0 == _dataSize )
         {
            rc = SDB_CLS_EMPTY_HEAP ;
            goto error ;
         }
         session = _heap[CLS_HEAP_ROOT] ;
         _heap[CLS_HEAP_ROOT] = _heap[_dataSize - 1] ;
         --_dataSize ;
         _shiftDown( CLS_HEAP_ROOT ) ;
      done:
         return rc ;
      error:
         goto done ;
      }

      OSS_INLINE INT32 erase( const UINT32 &sub )
      {
         INT32 rc = SDB_OK ;
         if ( 0 == _dataSize ||
              _dataSize <= sub )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            _heap[sub] =  _heap[_dataSize - 1] ;
            --_dataSize ;
            _shiftDown( sub ) ;
         }
      done:
         return rc ;
      error:
         goto done ;
      }

      OSS_INLINE INT32 root( _clsSyncSession &session )
      {
         INT32 rc = SDB_OK ;
         if ( 0 == _dataSize )
         {
            rc = SDB_CLS_EMPTY_HEAP ;
            goto error ;
         }
         session = _heap[CLS_HEAP_ROOT] ;
      done:
         return rc ;
      error:
         goto done ;
      }

      OSS_INLINE void dump()
      {
         for ( UINT32 i = 0; i < _dataSize; i++ )
         {
            std::cout << _heap[i].endLsn
                      << " : " << _heap[i].eduCB
                      << std::endl ;
         }
         std::cout << std::endl ;
      }
   private:
      INT32 _allocate( const UINT32 &size )
      {
         INT32 rc = SDB_OK ;
         _clsSyncSession *pOld = _heap ;
         _heap = ( _clsSyncSession * ) SDB_OSS_REALLOC( _heap,
            sizeof( _clsSyncSession ) * ( size + _spcSize ) ) ;
         if ( NULL == _heap )
         {
            rc = SDB_OOM ;
            _heap = pOld ;
            PD_LOG( PDERROR, "allocate mem err." ) ;
            goto error ;
         }
         _spcSize += size ;

      done:
         return rc ;
      error:
         goto done ;
      }

      UINT32 _father( const UINT32 &child )
      {
         SDB_ASSERT( CLS_HEAP_ROOT != child, "root has no father" ) ;
         return ( child - 1 ) / 2 ;
      }

      UINT32 _leftChild( const UINT32 &father )
      {
         return father * 2 + 1 ;
      }

      UINT32 _rightChild( const UINT32 &father )
      {
         return father * 2 + 2 ;
      }

      OSS_INLINE void _shiftUp( const UINT32 &sub )
      {
         UINT32 up = sub ;
         while ( CLS_HEAP_ROOT != up )
         {
            if ( _heap[up] <
                 _heap[_father( up )] )
            {
               _swap( up, _father( up ) ) ;
               up = _father( up ) ;
            }
            else
            {
               break ;
            }
         }
         return ;
      }

      OSS_INLINE void _shiftDown( const UINT32 &sub )
      {
         UINT32 down = sub ;
         UINT32 swap = 0 ;
         while ( _leftChild( down ) < _dataSize )
         {
            if ( _dataSize <=  _rightChild( down ) )
            {
               swap = _leftChild( down ) ;
            }
            else if ( _heap[_leftChild( down )] <=
                      _heap[_rightChild( down )] )
            {
               swap = _leftChild( down ) ;
            }
            else
            {
               swap = _rightChild( down ) ;
            }

            if ( _heap[swap] < _heap[down] )
            {
               _swap( down, swap ) ;
               down = swap ;
            }
            else
            {
               break ;
            }
         }
         return ;
      }

      OSS_INLINE void _swap( const UINT32 &sub1,
                         const UINT32 &sub2 )
      {
         _clsSyncSession tmp = _heap[sub1] ;
         _heap[sub1] = _heap[sub2] ;
         _heap[sub2] = tmp ;
      }

   private:
      _clsSyncSession *_heap ;
      UINT32 _spcSize ;
      UINT32 _dataSize ;
   } ;
}

#endif

