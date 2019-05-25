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

#define RTN_SORT_USE_INSERTSORT        64
#define RTN_SORT_SAME_SWAP_THRESHOLD   0.1
#define RTN_SORT_RANDOM_NUM            100

#define RTN_SORT_SWAP( a, b ) \
        do\
        {\
           _rtnSortTuple *tmp = *(a) ;\
           *(a) = *(b) ;\
           *(b) = tmp ;\
        } while(0)

namespace engine
{
   _rtnInternalSorting::_rtnInternalSorting( const BSONObj &orderby,
                                             CHAR *buf, UINT64 size,
                                             INT64 limit )
   :_order( Ordering::make( orderby ) ),
    _begin( buf ),
    _totalSize( size ),
    _headOffset( 0 ),
    _tailOffset( size ),
    _objNum( 0 ),
    _fetched( 0 ),
    _recursion(0),
    _limit( limit )
   {
   }

   _rtnInternalSorting::~_rtnInternalSorting()
   {
   }

   INT32 _rtnInternalSorting::push( const BSONObj& keyObj, const CHAR* obj,
                                    INT32 objLen, BSONElement* arrEle )
   {
      INT32 rc = SDB_OK ;
      _rtnSortTuple *tuple ;
      const CHAR* key = keyObj.objdata() ;
      INT32 keyLen = keyObj.objsize() ;

      SDB_ASSERT( NULL != key, "key can't be NULL" ) ;
      SDB_ASSERT( NULL != obj, "obj can't be NULL" ) ;
      SDB_ASSERT( keyLen > 0, "keyLen must be greater than 0") ;
      SDB_ASSERT( objLen > 0, "objLen must be greater than 0") ;
      SDB_ASSERT( _headOffset <= _tailOffset, "impossible" ) ;

      if ( _tailOffset - _headOffset <
           ( keyLen + objLen + sizeof(_rtnSortTuple) + sizeof( _rtnSortTuple *) ) )
      {
         rc = SDB_HIT_HIGH_WATERMARK ;
         goto error ;
      }

      /* mem begin                                                                                      mem end
       *  | |_rtnSortTuple *| _rtnSortTuple *| ...| _rtnSortTuple | keyObj | obj |  ... | _rtnSortTuple | keyOj| obj |
       */

      _tailOffset -= ( objLen + keyLen + sizeof(_rtnSortTuple) );
      tuple = ( _rtnSortTuple * )( _begin + _tailOffset ) ;

      ossMemcpy( ( CHAR * )tuple + sizeof( _rtnSortTuple ), key, keyLen ) ;
      ossMemcpy( ( CHAR * )tuple + sizeof( _rtnSortTuple ) + keyLen, obj, objLen ) ;
      tuple->setLen( keyLen, objLen ) ;
      if ( NULL == arrEle || arrEle->eoo() )
      {
         tuple->setHash( 0, 0 ) ;
      }
      else
      {
         ixmMakeHashValue( *arrEle, tuple->hashValue() ) ;
      }

      *(( _rtnSortTuple ** )( _begin + _headOffset )) = tuple ;
      _headOffset += sizeof( _rtnSortTuple * ) ;

      ++_objNum ;

      SDB_ASSERT( _headOffset <= _tailOffset, "impossible" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnInternalSorting::clearBuf()
   {
      _headOffset = 0 ;
      _tailOffset = _totalSize ;
      _objNum = 0 ;
      _fetched = 0 ;
      return ;
   }

   INT32 _rtnInternalSorting::next( _rtnSortTuple **tuple )
   {
      INT32 rc = SDB_OK ;

      if ( !more() )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      *tuple = *(( _rtnSortTuple **)
                 (_fetched * sizeof( _rtnSortTuple * ) +
                  _begin ));
      SDB_ASSERT( NULL != *tuple, "can not be NULL" ) ;

      ++_fetched ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnInternalSorting::sort( _pmdEDUCB *cb )
   {
      PD_LOG( PDDEBUG, "begin to do internal sort. number of"
                       " obj:%d", _objNum ) ;
      INT32 rc = SDB_OK ;
      if ( 0 == _objNum )
      {
         goto done ;
      }

/*
      for ( UINT32 i = 0; i < RTN_SORT_RANDOM_NUM; i++ )
      {
         _rands.push_back( ossRand() ) ;
      }
*/
      _recursion = 0 ;
      rc = _quickSort( (_rtnSortTuple **)(_begin),
                       (_rtnSortTuple **)
                       (_begin + sizeof(_rtnSortTuple **) * (_objNum - 1)),
                       cb ) ;

      if ( SDB_OK != rc )
      {
         goto error ;
      }

      PD_LOG( PDDEBUG, "quick sorting recursion:%lld", _recursion ) ;
   done:
      return rc ;
   error:
      goto done ;
   }


   INT32 _rtnInternalSorting::_partition( _rtnSortTuple **left,
                                          _rtnSortTuple **right,
                                          _rtnSortTuple **&leftAxis,
                                          _rtnSortTuple **&rightAxis )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( left < right, "impossible" ) ;
      SDB_ASSERT( NULL != *left && NULL != *right, "can not be NULL" ) ;

      _rtnSortTuple **mid = left + (( right - left ) >> 1 ) ;
      _rtnSortTuple **randPtr = NULL ;

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

      if ( j + 1 < right )
      {
         randPtr = j + 1 + ossRand() % ( right - j - 1 ) ;
         RTN_SORT_SWAP( right, randPtr ) ;
      }

      leftAxis = j ;
      rightAxis = j ;

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
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnInternalSorting::_swapLeftSameKey( _rtnSortTuple **left,
                                                _rtnSortTuple **right,
                                                _rtnSortTuple **&axis )
   {
      INT32 rc = SDB_OK ;
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
      return rc ;
   }

   INT32 _rtnInternalSorting::_swapRightSameKey( _rtnSortTuple **left,
                                                _rtnSortTuple **right,
                                                _rtnSortTuple **&axis )
   {
      INT32 rc = SDB_OK ;
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
      return rc ;
   }

   INT32 _rtnInternalSorting::_quickSort( _rtnSortTuple **left,
                                          _rtnSortTuple **right,
                                          _pmdEDUCB *cb )
   {
      SDB_ASSERT( left <= right, "impossible" ) ;

      INT32 rc = SDB_OK ;
      _rtnSortTuple **leftAxis = NULL ;
      _rtnSortTuple **rightAxis = NULL ;
      ++_recursion ;

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
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnInternalSorting::_insertSort( _rtnSortTuple **left,
                                           _rtnSortTuple **right )
   {
      INT32 rc = SDB_OK ;
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
      return rc ;
   error:
      goto done ;
   }

   /*INT32 _rtnInternalSorting::_setHashFromObj( const BSONObj &obj,
                                               _rtnSortTuple *tuple )
   {
      SDB_ASSERT( NULL != tuple, "can not be NULL" ) ;
      INT32 rc = SDB_OK ;
      BOOLEAN set = FALSE ;
      BSONObjIterator itr( _orderObj ) ;
      while ( itr.more() )
      {
         BSONElement orderEle = itr.next() ;
         SDB_ASSERT( !orderEle.eoo(), "can not be eoo" ) ;
         BSONElement arrEle = obj.getField( orderEle.fieldName() ) ;
         if ( Array == arrEle.type() )
         {
            UINT32 hash1 = ossHash( arrEle.value(),
                                    arrEle.valuesize() ) ;
            UINT32 hash2 = ossHash( arrEle.value(),
                                    arrEle.valuesize(), 3 ) ;
            tuple->setHash( hash1, hash2 ) ;
            set = TRUE ;
            break ;
         }
      }

      if ( !set )
      {
         PD_LOG( PDERROR, "can not find array ele in obj."
                 "orderby[%s] obj[%s]",
                 _orderObj.toString(FALSE, TRUE).c_str(),
                 obj.toString(FALSE, TRUE).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }*/
}

