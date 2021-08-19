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

   Source File Name = utilMinHeap.hpp

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

#ifndef UTILMINHEAP_HPP_
#define UTILMINHEAP_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "pd.hpp"
#include "ossMem.hpp"

#define UTIL_MIN_HEAP_DEFAULT_SIZE 10

namespace engine
{
   template<typename T, typename Compare>
   class _utilMinHeap : public SDBObject
   {
   public:
      _utilMinHeap( Compare cmp )
      :_root(NULL),
       _cmp(cmp),
       _dataSize(0),
       _spcSize(0)
      {
         _allocate( 10 ) ;
      }

      virtual ~_utilMinHeap()
      {
         if ( NULL != _root )
         {
            SDB_OSS_FREE( _root ) ;
            _root = NULL ;
         }
      }

   public:
      const UINT32 dataSize()const
      {
         return _dataSize ;
      }

      BOOLEAN more() const
      {
         return 0 != _dataSize ;
      }

      /// if need to release mem, add a function named resize().
      void clear()
      {
         _dataSize = 0 ;
         return ;
      }

      INT32 push( const T &t )
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

         _root[_dataSize] = t ;
         _shiftUp( _dataSize ) ;
         ++_dataSize ;
      done:
         return rc ;
      error:
         goto done ;
      }

      INT32 pop( T &t )
      {
         INT32 rc = SDB_OK ;
         if ( 0 == _dataSize )
         {
            rc = SDB_CLS_EMPTY_HEAP ;
            goto error ;
         }

         t = _root[0] ;
         _root[0] = _root[_dataSize - 1] ;
         --_dataSize ;
         _shiftDown( 0 ) ;
      done:
         return rc ;
      error:
         goto done ;
      }

      INT32 root( T &t )
      {
         INT32 rc = SDB_OK ;
         if ( 0 == _dataSize )
         {
            rc = SDB_CLS_EMPTY_HEAP ;
            goto error ;
         }
         t = _root[0] ;
      done:
         return rc ;
      error:
         goto done ;
      }

   private:
      INT32 _allocate( UINT32 size )
      {
         INT32 rc = SDB_OK ;
         T *pOld = _root ;
         _root = ( T * )SDB_OSS_REALLOC( _root,
                                         sizeof(T) * ( size + _spcSize ) ) ;
         if ( NULL == _root )
         {
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            _root = pOld ;
            rc = SDB_OOM ;
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
         SDB_ASSERT( 0 != child, "root has no father" ) ;
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

      void _shiftUp( const UINT32 &sub )
      {
         UINT32 up = sub ;
         while ( 0 != up )
         {
            if ( _cmp(_root[up],
                 _root[_father( up )]) )
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
            else if ( !_cmp(_root[_rightChild( down )],
                        _root[_leftChild( down )]))
            {
               swap = _leftChild( down ) ;
            }
            else
            {
               swap = _rightChild( down ) ;
            }

            if ( _cmp(_root[swap], _root[down]) )
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
         T tmp = _root[sub1] ;
         _root[sub1] = _root[sub2] ;
         _root[sub2] = tmp ;
      }

   private:
      T *_root ;
      Compare _cmp ;
      UINT32 _dataSize ; /// datasize
      UINT32 _spcSize ; /// totalSize
   } ;
}

#endif

