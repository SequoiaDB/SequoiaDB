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

   Source File Name = rtnInternalSotring.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnInternalSorting.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "pmdEDU.hpp"
#include "ossUtil.hpp"
#include "ixm_common.hpp"
#include "rtnTrace.hpp"

#define RTN_SORT_USE_INSERTSORT        64
#define RTN_SORT_SAME_SWAP_THRESHOLD   0.1

#define RTN_SORT_SWAP( a, b ) \
        do\
        {\
           _rtnSortTuple *tmp = *(a) ;\
           *(a) = *(b) ;\
           *(b) = tmp ;\
        } while(0)

namespace engine
{
   ///_rtnInternalSorting
   _rtnInternalSorting::_rtnInternalSorting( const BSONObj &orderby,
                                             utilCommBuff *tupleDirectory,
                                             utilCommBuff *tupleBuff,
                                             INT64 limit )
   :_order( Ordering::make( orderby ) ),
    _tupleDirectory( tupleDirectory ),
    _tupleBuff( tupleBuff ),
    _objNum( 0 ),
    _fetched( 0 ),
    _recursion( 0 ),
    _limit( limit ),
    _maxRecordSize( 0 )
   {
   }

   _rtnInternalSorting::~_rtnInternalSorting()
   {
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINTERNALSORTING_PUSH, "_rtnInternalSorting::push" )
   INT32 _rtnInternalSorting::push( const BSONObj& keyObj, const CHAR* obj,
                                    INT32 objLen, BSONElement* arrEle )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNINTERNALSORTING_PUSH ) ;
      UINT64 offset = 0 ;
      _rtnSortTuple *tuple = NULL ;
      _rtnSortTuple tupleTmp ;
      const CHAR* key = keyObj.objdata() ;
      INT32 keyLen = keyObj.objsize() ;

      SDB_ASSERT( NULL != key, "key can't be NULL" ) ;
      SDB_ASSERT( NULL != obj, "obj can't be NULL" ) ;
      SDB_ASSERT( keyLen > 0, "keyLen must be greater than 0") ;
      SDB_ASSERT( objLen > 0, "objLen must be greater than 0") ;

      rc = _tupleDirectory->ensureSpace( sizeof( _rtnSortTuple *) ) ;
      if ( rc )
      {
         if ( SDB_HIT_HIGH_WATERMARK != rc )
         {
            PD_LOG( PDERROR, "Ensure space of size[%u] in sort buffer "
                             "failed[%d]", sizeof( _rtnSortTuple *), rc ) ;
         }
         goto error ;
      }

      rc = _tupleBuff->ensureSpace( sizeof(_rtnSortTuple) + keyLen + objLen ) ;
      if ( rc )
      {
         if ( SDB_HIT_HIGH_WATERMARK != rc )
         {
            PD_LOG( PDERROR, "Ensure space of size[%u] in sort buffer "
                             "failed[%d]",
                    sizeof(_rtnSortTuple) + keyLen + objLen, rc ) ;
         }
         goto error ;
      }

      tupleTmp.setLen( keyLen, objLen ) ;
      if ( !arrEle || arrEle->eoo() )
      {
         tupleTmp.setHash( 0, 0 ) ;
      }
      else
      {
         ixmMakeHashValue( *arrEle, tupleTmp.hashValue() ) ;
      }

      rc = _tupleBuff->append( (const CHAR *)&tupleTmp, sizeof( _rtnSortTuple ),
                               &offset ) ;
      PD_RC_CHECK( rc, PDERROR, "Append tuple header into sort buffer "
                                "failed[%d]", rc ) ;
      rc = _tupleBuff->append( key, keyLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Append sort key into sort buffer failed[%d]",
                   rc ) ;

      rc = _tupleBuff->append( obj, objLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Append sort object into sort buffer "
                                "failed[%d]", rc ) ;

      tuple = (_rtnSortTuple *)( _tupleBuff->offset2Addr( offset ) ) ;

      rc = _tupleDirectory->append( (CHAR *)&tuple, sizeof(const CHAR *) ) ;
      PD_RC_CHECK( rc, PDERROR, "Append sort tuple pointer into sort buffer "
                                "failed[%d]", rc ) ;

      if ( (UINT32)objLen > _maxRecordSize )
      {
         _maxRecordSize = (UINT32)objLen ;
      }

      ++_objNum ;

   done:
      PD_TRACE_EXITRC( SDB__RTNINTERNALSORTING_PUSH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINTERNALSORTING_CLEARBUF, "_rtnInternalSorting::clearBuf" )
   void _rtnInternalSorting::clearBuf()
   {
      PD_TRACE_ENTRY( SDB__RTNINTERNALSORTING_CLEARBUF ) ;

      _tupleDirectory->reset() ;
      _tupleBuff->reset() ;

      _objNum = 0 ;
      _fetched = 0 ;

      PD_TRACE_EXIT( SDB__RTNINTERNALSORTING_CLEARBUF ) ;
      return ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINTERNALSORTING_NEXT, "_rtnInternalSorting::next" )
   INT32 _rtnInternalSorting::next( _rtnSortTuple **tuple )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNINTERNALSORTING_NEXT ) ;

      if ( !more() )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      *tuple = *((_rtnSortTuple **) ( _tupleDirectory->offset2Addr( 0 ) +
                                      sizeof(_rtnSortTuple *) * _fetched ) ) ;
      SDB_ASSERT( NULL != *tuple, "can not be NULL" ) ;

      ++_fetched ;
   done:
      PD_TRACE_EXITRC( SDB__RTNINTERNALSORTING_NEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINTERNALSORTING_SORT, "_rtnInternalSorting::sort" )
   INT32 _rtnInternalSorting::sort( _pmdEDUCB *cb )
   {
      PD_LOG( PDDEBUG, "begin to do internal sort. number of"
                       " obj:%d", _objNum ) ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNINTERNALSORTING_SORT ) ;

      if ( 0 == _objNum )
      {
         goto done ;
      }

      _recursion = 0 ;
      rc = _quickSort( (_rtnSortTuple **)(_tupleDirectory->offset2Addr(0)),
                       (_rtnSortTuple **)
                       (_tupleDirectory->offset2Addr(sizeof(_rtnSortTuple *)
                                                     * ( _objNum - 1 ))),
                       cb ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      PD_LOG( PDDEBUG, "quick sorting recursion:%lld", _recursion ) ;
   done:
      PD_TRACE_EXITRC( SDB__RTNINTERNALSORTING_SORT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINTERNALSORTING__PARTITION, "_rtnInternalSorting::_partition" )
   INT32 _rtnInternalSorting::_partition( _rtnSortTuple **left,
                                          _rtnSortTuple **right,
                                          _rtnSortTuple **&leftAxis,
                                          _rtnSortTuple **&rightAxis )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNINTERNALSORTING__PARTITION ) ;
      SDB_ASSERT( left < right, "impossible" ) ;
      SDB_ASSERT( NULL != *left && NULL != *right, "can not be NULL" ) ;

      _rtnSortTuple **mid = left + (( right - left ) >> 1 ) ;
      _rtnSortTuple **randPtr = NULL ;

      /// woCompare 's cost may be expensive. think about use it
      /// only when range is large.
      try
      {
         if ( 0 < (*left)->compare( *mid, _order ) )
         {
            RTN_SORT_SWAP( mid, left ) ;
         }
         if ( 0 < (*left)->compare( *right, _order ) )
         {
            RTN_SORT_SWAP( left, right ) ;
         }
         if ( 0 < (*right)->compare( *mid, _order ) )
         {
            RTN_SORT_SWAP( mid, right ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      {
      _rtnSortTuple **pivot = right ;
      _rtnSortTuple **i = left ;
      _rtnSortTuple **j = right - 1 ;
      FLOAT64 sameNum = 0 ;

      while ( i < j )
      {
         try
         {
            INT32 compare = (*pivot)->compare( *j, _order ) ;
            if ( 0 > compare )
            {
               --j ;
               continue;
            }
            else if ( 0 == compare )
            {
               ++sameNum ;
               --j ;
               continue;
            }
            else
            {
               /// do nothing.
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         while ( i < j )
         {
            try
            {
               INT32 compare = (*pivot)->compare( *i, _order) ;
               if ( 0 < compare )
               {
                  ++i ;
               }
               else if ( 0 == compare )
               {
                  ++sameNum ;
                  break ;
               }
               else
               {
                  break ;
               }
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }

         if ( i < j )
         {
            RTN_SORT_SWAP( i, j ) ;
            ++i ;
            --j ;
         }
         else
         {
            break ;
         }
      }

      if ( i == j )
      {
         try
         {
            if ( 0 > (*pivot)->compare( *j, _order))
            {
               RTN_SORT_SWAP( pivot, j ) ;
            }
            else if ( j + 1 < pivot )
            {
               ++j ;
               RTN_SORT_SWAP( pivot, j ) ;
            }
            else
            {
               j = pivot ;
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      else
      {
         j = i;
         RTN_SORT_SWAP( pivot, j ) ;
      }

      /// the right maybe the pivot( near to left), so we change the right
      /// to rand pos
      if ( j + 1 < right )
      {
         randPtr = j + 1 + ossRand() % ( right - j - 1 ) ;
         RTN_SORT_SWAP( right, randPtr ) ;
      }

      leftAxis = j ;
      rightAxis = j ;

      /// collect all the obj which is the same to pivot.
      /// it can reduce the number of recursive.
      if ( RTN_SORT_SAME_SWAP_THRESHOLD < ( sameNum / (right - left + 1) ))
      {
         rc = _swapLeftSameKey( left, j, leftAxis ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to swap same key:%d", rc ) ;
            goto error ;
         }

         rc = _swapRightSameKey( j, right, rightAxis ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to swap same key:%d", rc ) ;
            goto error ;
         }
      }
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNINTERNALSORTING__PARTITION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINTERNALSORTING__SWAPLEFTWAMEKEY, "_rtnInternalSorting::_swapLeftSameKey" )
   INT32 _rtnInternalSorting::_swapLeftSameKey( _rtnSortTuple **left,
                                                _rtnSortTuple **right,
                                                _rtnSortTuple **&axis )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNINTERNALSORTING__SWAPLEFTWAMEKEY ) ;
      _rtnSortTuple *pivot = *right ;
      axis = right ;
      _rtnSortTuple **i = left ;
      _rtnSortTuple **j = right - 1 ;
      while ( i < j )
      {
         if ( 0 != pivot->compare( *i, _order ) )
         {
            ++i ;
            continue ;
         }

         while ( i < j &&
                 0 == pivot->compare( *j, _order ))
         {
            axis = j-- ;
         }

         if ( i < j )
         {
            RTN_SORT_SWAP( i, j ) ;
            ++i ;
            axis = j-- ;
         }
         else
         {
            break ;
         }
      }
      PD_TRACE_EXITRC( SDB__RTNINTERNALSORTING__SWAPLEFTWAMEKEY, rc ) ;
      return rc ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINTERNALSORTING__SWAPRIGHTWAMEKEY, "_rtnInternalSorting::_swapRightSameKey" )
   INT32 _rtnInternalSorting::_swapRightSameKey( _rtnSortTuple **left,
                                                _rtnSortTuple **right,
                                                _rtnSortTuple **&axis )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNINTERNALSORTING__SWAPRIGHTWAMEKEY ) ;
      _rtnSortTuple *pivot = *left ;
      axis = left ;
      _rtnSortTuple **i = left + 1 ;
      _rtnSortTuple **j = right ;
      while ( i < j )
      {
         if ( 0 != pivot->compare( *j, _order ) )
         {
            --j ;
            continue ;
         }

         while ( i < j &&
                 0 == pivot->compare( *i, _order ))
         {
            axis = i++ ;
         }

         if ( i < j )
         {
            RTN_SORT_SWAP( i, j ) ;
            --j ;
            axis = i++ ;
         }
         else
         {
            break ;
         }
      }
      PD_TRACE_EXITRC( SDB__RTNINTERNALSORTING__SWAPRIGHTWAMEKEY, rc ) ;
      return rc ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINTERNALSORTING__QUICKSORT, "_rtnInternalSorting::_quickSort" )
   INT32 _rtnInternalSorting::_quickSort( _rtnSortTuple **left,
                                          _rtnSortTuple **right,
                                          _pmdEDUCB *cb )
   {
      SDB_ASSERT( left <= right, "impossible" ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNINTERNALSORTING__QUICKSORT ) ;
      _rtnSortTuple **leftAxis = NULL ;
      _rtnSortTuple **rightAxis = NULL ;
      ++_recursion ;

      /// can stop when recuresive call this func.
      if ( cb->isInterrupted() )
      {
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }

      if ( left == right )
      {
         goto done ;
      }

      if ( right - left < RTN_SORT_USE_INSERTSORT )
      {
         rc = _insertSort( left, right ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         goto done ;
      }

      rc = _partition( left, right, leftAxis, rightAxis ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( left < leftAxis - 1 )
      {
         rc = _quickSort( left, leftAxis - 1, cb ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      /// if left is more than limit, don't to sort the right
      if ( _limit > 0 && rightAxis - left + 1 >= _limit )
      {
         goto done ;
      }

      if ( rightAxis + 1 < right )
      {
         rc = _quickSort( rightAxis + 1, right, cb ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNINTERNALSORTING__QUICKSORT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINTERNALSORTING__INSERTSORT, "_rtnInternalSorting::_insertSort" )
   INT32 _rtnInternalSorting::_insertSort( _rtnSortTuple **left,
                                           _rtnSortTuple **right )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNINTERNALSORTING__INSERTSORT ) ;
      SDB_ASSERT( left < right, "impossible" ) ;
      for ( _rtnSortTuple **i = left + 1;
            i <= right;
            i++ )
      {
         try
         {
            for ( _rtnSortTuple **j = i;
                  j > left;
                  j-- )
            {
               if ( 0 > (*j)->compare( *(j - 1), _order ))
               {
                  RTN_SORT_SWAP( j, j - 1 ) ;
               }
               else
               {
                  break ;
               }
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "unexcepted err happened:%s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNINTERNALSORTING__INSERTSORT, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

