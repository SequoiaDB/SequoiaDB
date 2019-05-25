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

   Source File Name = rtnSorting.cpp

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

#include "rtnSorting.hpp"
#include "pmd.hpp"
#include "pmdEDU.hpp"
#include "rtnCB.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "rtnInternalSorting.hpp"
#include "rtnSortTuple.hpp"

namespace engine
{
   _rtnSorting::_rtnSorting()
   :_sortBuf(NULL),
    _totalBufSize(0),
    _step(RTN_SORT_STEP_BEGIN),
    _cb(NULL),
    _internalBlk(NULL),
    _mergeBlk(NULL),
    _blkBegin(0),
    _fino(0),
    _limit(-1),
    _cpBuf( NULL ),
    _cpBufSize( 4 * 1024 * 1024 )
   {

   }

   _rtnSorting::~_rtnSorting()
   {
      SAFE_OSS_FREE( _sortBuf ) ;
      SAFE_OSS_FREE( _cpBuf ) ;
      SAFE_OSS_DELETE( _internalBlk ) ;
      SAFE_OSS_DELETE( _mergeBlk ) ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSORTING_INIT, "_rtnSorting::init" )
   INT32 _rtnSorting::init( UINT64 bufSize,
                            const BSONObj &orderby,
                            SINT64 fino,
                            SINT64 limit,
                            _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSORTING_INIT ) ;
      SDB_ASSERT( RTN_SORT_MIN_BUFSIZE <= bufSize,
                  "bufSize should >= 64MB" ) ;
      SDB_ASSERT( NULL != cb, "impossible" ) ;
      SDB_ASSERT( !orderby.isEmpty(), "impossible" ) ;
      UINT64 realSize = bufSize * 1024 * 1024 ;

      _sortBuf = ( CHAR * )SDB_OSS_MALLOC( realSize ) ;
      if ( NULL == _sortBuf )
      {
         PD_LOG( PDERROR, "failed to allocate _sortBuf." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _totalBufSize = realSize ;
      _cb = cb ;
      _orderby = orderby.getOwned() ;
      _fino = fino ;
      _limit = limit ;

      _internalBlk = SDB_OSS_NEW _rtnInternalSorting( _orderby,
                                                      _sortBuf,
                                                      _totalBufSize,
                                                      _limit ) ;
      if ( NULL == _internalBlk )
      {
         PD_LOG( PDERROR, "failed to allocate _internalBlk." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      PD_LOG( PDDEBUG, "sort into: bufsize[%lld(MB)],"
              "filename:[%lld], order by[%s]",
              bufSize, fino, orderby.toString(FALSE, TRUE).c_str() ) ;
   done:
      PD_TRACE_EXITRC(  SDB__RTNSORTING_INIT, rc ) ;
      return rc ;
   error:
      SAFE_OSS_FREE( _sortBuf ) ;
      SAFE_OSS_DELETE( _internalBlk ) ;
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSORTING_PUSH, "_rtnSorting::push" )
   INT32 _rtnSorting::push( const BSONObj& key,
                            const CHAR* obj, INT32 objLen,
                            BSONElement* arrEle, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSORTING_PUSH ) ;
      SDB_ASSERT( RTN_SORT_STEP_BEGIN == _step, "can't push obj yet" ) ;

      for(;;)
      {
         rc = _internalBlk->push( key, obj, objLen, arrEle ) ;
         if ( SDB_OK == rc )
         {
            goto done ;
         }
         else if ( SDB_HIT_HIGH_WATERMARK == rc )
         {
            rc = _internalBlk->sort( cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to exec internal sort:%d", rc ) ;
               goto error ;
            }

            PD_LOG( PDDEBUG, "begin to mv blks to file" ) ;
            rc = _moveToExternalBlks( _internalBlk, _blks, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to move data to ex blk:%d", rc ) ;
               goto error ;
            }

            _internalBlk->clearBuf() ;
         }
         else
         {
            PD_LOG( PDERROR, "failed to push to sort mem:%d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC(  SDB__RTNSORTING_PUSH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSORTING_SORT, "_rtnSorting::sort" )
   INT32 _rtnSorting::sort( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSORTING_SORT ) ;
      rc = _internalBlk->sort( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec internal sort:%d", rc ) ;
         goto error ;
      }
      if ( _blks.empty() )
      {
         _step = RTN_SORT_STEP_FETCH_FROM_INTER ;
      }
      else
      {
         rc = _moveToExternalBlks( _internalBlk, _blks, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to move data to ex blk:%d", rc ) ;
            goto error ;
         }

         PD_LOG( PDDEBUG, "total size of unit:%lld", _unit.totalSize() ) ;
         SAFE_OSS_DELETE( _internalBlk ) ;

         SDB_ASSERT( NULL == _mergeBlk, "impossible" ) ;
         _mergeBlk = SDB_OSS_NEW _rtnMergeSorting( &_unit, _orderby ) ;
         if ( NULL == _mergeBlk )
         {
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         rc = _mergeBlk->init( _sortBuf, _totalBufSize,
                               _blks, _limit ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to init merge sort:%d", rc ) ;
            goto error ;
         }

         _step = RTN_SORT_STEP_FETCH_FROM_MERGE ;
      }

   done:
      PD_TRACE_EXITRC(  SDB__RTNSORTING_SORT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB__RTNSORTING_FETCH, "_rtnSorting::fetch" )
   INT32 _rtnSorting::fetch( BSONObj &key, const CHAR** obj, INT32* objLen, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSORTING_FETCH ) ;

      SDB_ASSERT( RTN_SORT_STEP_BEGIN != _step, "must sort before fetching" ) ;

      if ( RTN_SORT_STEP_FETCH_FROM_INTER == _step )
      {
         rc = _fetchFromInter( key, obj, objLen ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG( PDERROR, "failed to fetch from inter:%d", rc ) ;
            }
            goto error ;
         }
      }
      else if ( RTN_SORT_STEP_FETCH_FROM_MERGE == _step )
      {
         rc = _fetchFromExter( key, obj, objLen, cb ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG( PDERROR, "failed to fetch from exter:%d", rc ) ;
            }
            goto error ;
         }
      }
      else
      {
         SDB_ASSERT( FALSE, "impossible" ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNSORTING_FETCH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB__RTNSORTING__MVTEXBLKS, "_rtnSorting::_moveToExternalBlks" )
   INT32 _rtnSorting::_moveToExternalBlks( _rtnInternalSorting *inter,
                                           RTN_SORT_BLKS &blks,
                                           _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__RTNSORTING__MVTEXBLKS ) ;
      INT32 rc = SDB_OK ;
      UINT64 mvSize = 0 ;
      _rtnSortTuple *tuple = NULL ;
      UINT32 inBufSize = 0 ;
      UINT32 notInBufSize = 0 ;

      if ( !_unit.isOpened() )
      {
         rc = _unit.openFile( pmdGetOptionCB()->getTmpPath(),
                              _fino ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      if ( NULL == _cpBuf )
      {
         _cpBuf = ( CHAR * )SDB_OSS_MALLOC( _cpBufSize ) ;
         if ( NULL == _cpBuf )
         {
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error ;
         } 
      }

      do
      {
         UINT32 cpSize = 0 ;
         if ( NULL == tuple )
         {
            if ( inter->more() )
            {
               rc = inter->next( &tuple ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to fetch next from internal blk:%d", rc ) ;
                  goto error ;
               }
               notInBufSize = tuple->len() ;
            }
            else
            {
               break ;
            }
         }

         cpSize = notInBufSize <= ( _cpBufSize - inBufSize ) ?
                  notInBufSize : ( _cpBufSize - inBufSize ) ;
         ossMemcpy( _cpBuf + inBufSize,
                    ( const CHAR * )tuple + ( tuple->len() - notInBufSize ),
                    cpSize ) ;
         notInBufSize -= cpSize ;
         inBufSize += cpSize ;

         if ( 0 == notInBufSize  )
         {
            tuple = NULL ;
         }

         if ( inBufSize == _cpBufSize )
         {
            rc = _unit.write( _cpBuf, _cpBufSize, TRUE ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to extend unit:%d", rc ) ;
               goto error ;
            }
            mvSize += _cpBufSize ;
            inBufSize = 0 ;
         }
         
      } while ( TRUE ) ;

      if ( 0 < inBufSize )
      {
         rc = _unit.write( _cpBuf, inBufSize, TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to extend unit:%d", rc ) ;
            goto error ;
         }
         mvSize += inBufSize ;
         inBufSize = 0 ; 
      }

      if ( 0 != mvSize )
      {
         dmsTmpBlk blk ;
         rc = _unit.buildBlk( _blkBegin, mvSize, blk ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build blk:%d", rc ) ;
            goto error ;
         }

         blks.push_back( blk ) ;
         _blkBegin += mvSize ;
         PD_LOG( PDDEBUG, "build blk[%s]", blk.toString().c_str() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNSORTING__MVTEXBLKS , rc ) ;
      return rc ;
   error:
      goto done ;
   }


   //PD_TRACE_DECLARE_FUNCTION( SDB__RTNSORTING__FETCHFROMINTER, "_rtnSorting::_fetchFromInter")
   INT32 _rtnSorting::_fetchFromInter( BSONObj &key, const CHAR** obj, INT32* objLen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNSORTING__FETCHFROMINTER ) ;
      _rtnSortTuple *tuple = NULL ;
      if ( _internalBlk->more() )
      {
         rc = _internalBlk->next( &tuple ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to fetch next from internal blk:%d", rc ) ;
            goto error ;
         }
         SDB_ASSERT( NULL != tuple && NULL != tuple->obj(),
                     "can not be NULL" ) ;
         key = BSONObj( tuple->key() ) ;
         *obj = tuple->obj() ;
         *objLen = (INT32)( tuple->objLen() ) ;
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNSORTING__FETCHFROMINTER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB__RTNSORTING__FETCHFROMEXTER, "_rtnSorting::_fetchFromExter")
   INT32 _rtnSorting::_fetchFromExter( BSONObj &key, const CHAR** obj, INT32* objLen, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      rc = _mergeBlk->fetch( key, obj, objLen, cb ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
}

