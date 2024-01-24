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

   Source File Name = rtnRecycleBinManager.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for recycle bin manager.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnRecycleBinManager.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{

   /*
      _rtnRecycleBinManager implement
    */
   _rtnRecycleBinManager::_rtnRecycleBinManager()
   : _isConfValid( FALSE ),
     _returnCount( 0 ),
     _limitCheckFlag( 1 ),
     _emptyCheckFlag( 1 ),
     _isEmptyFlag( FALSE ),
     _rtnCB( NULL ),
     _dmsCB( NULL ),
     _dpsCB( NULL )
   {
   }

   _rtnRecycleBinManager::~_rtnRecycleBinManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR_INIT, "_rtnRecycleBinManager::init" )
   INT32 _rtnRecycleBinManager::init()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR_INIT ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      _rtnCB = krcb->getRTNCB() ;
      _dmsCB = krcb->getDMSCB() ;
      _dpsCB = krcb->getDPSCB() ;

      try
      {
         _hintRecyID = BSON( "" << UTIL_RECYCLEBIN_RECYID_INDEX_NAME ) ;
         _hintOrigID = BSON( "" << UTIL_RECYCLEBIN_ORIGID_INDEX_NAME ) ;
         _hintName = BSON( "" << UTIL_RECYCLEBIN_NAME_INDEX_NAME ) ;
         _hintOrigName = BSON( "" << UTIL_RECYCLEBIN_ORIGNAME_INDEX_NAME ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to init query hints, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR_FINI, "_rtnRecycleBinManager::fini" )
   void _rtnRecycleBinManager::fini()
   {
      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR_FINI ) ;

      _hintRecyID = BSONObj() ;
      _hintOrigID = BSONObj() ;
      _hintName = BSONObj() ;
      _hintOrigName = BSONObj() ;

      PD_TRACE_EXIT( SDB__RTNRECYBINMGR_FINI ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR_STARTBGJOB, "_rtnRecycleBinManager::startBGJob" )
   INT32 _rtnRecycleBinManager::startBGJob()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR_STARTBGJOB ) ;

      utilLightJob *job = NULL ;

      rc = _createBGJob( &job ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to submit drop recycle bin "
                   "background job, rc: %d", rc ) ;
      rc = job->submit( TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to submit drop recycle bin "
                   "background job, rc: %d", rc ) ;
      PD_LOG( PDDEBUG, "Submit drop recycle bin background job done" ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR_STARTBGJOB, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR_GETITEM_RECYNAME, "_rtnRecycleBinManager::getItem" )
   INT32 _rtnRecycleBinManager::getItem( const CHAR *recycleName,
                                         pmdEDUCB *cb,
                                         utilRecycleItem &item )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR_GETITEM_RECYNAME ) ;

      BSONObj itemObject ;

      rc = _getItemObject( recycleName, cb, itemObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item [%s], rc: %d",
                   recycleName, rc ) ;

      rc = item.fromBSON( itemObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse recycle item [%s] from BSON, "
                   "rc: %d", recycleName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR_GETITEM_RECYNAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR_GETITEM_ORIGID, "_rtnRecycleBinManager::getItemByOrigID" )
   INT32 _rtnRecycleBinManager::getItemByOrigID( utilCLUniqueID originID,
                                                 pmdEDUCB *cb,
                                                 utilRecycleItem &item )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR_GETITEM_ORIGID ) ;

      BSONObj itemObject ;

      rc = _getItemObjectByOrigID( originID, cb, itemObject ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get recycle item "
                   "[origin ID: %llu], rc: %d", originID, rc ) ;

      rc = item.fromBSON( itemObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse recycle item "
                   "[origin ID: %llu] from BSON, rc: %d", originID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR_GETITEM_ORIGID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

     // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR_GETITEM_RECYID, "_rtnRecycleBinManager::getItemByRecyID" )
   INT32 _rtnRecycleBinManager::getItemByRecyID( utilGlobalID recycleID,
                                                 pmdEDUCB *cb,
                                                 utilRecycleItem &item )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR_GETITEM_RECYID ) ;

      BSONObj itemObject ;

      rc = _getItemObjectByRecyID( recycleID, cb, itemObject ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get recycle item "
                   "[recycle ID: %llu], rc: %d", recycleID, rc ) ;

      rc = item.fromBSON( itemObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse recycle item "
                   "[recycle ID: %llu] from BSON, rc: %d", recycleID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR_GETITEM_RECYID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR_DROPEXPIREDITEMS, "_rtnRecycleBinManager::dropExpiredItems" )
   INT32 _rtnRecycleBinManager::dropExpiredItems( pmdEDUCB *cb,
                                                  BOOLEAN &isFinished )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR_DROPEXPIREDITEMS ) ;

      UINT64 currentTime = 0, expiredTime = 0 ;
      utilRecycleItem oldestItem ;
      UTIL_RECY_ITEM_LIST itemList ;
      UTIL_RECY_ITEM_LIST_IT iter ;

      utilRecycleBinConf conf = getConf() ;
      BOOLEAN checkLimit = FALSE ;
      BOOLEAN checkEmpty = FALSE ;

      isFinished = FALSE ;

      if ( conf.isTimeUnlimited() )
      {
         isFinished = TRUE ;
         goto done ;
      }

      currentTime = ossGetCurrentMilliseconds() ;
      expiredTime = currentTime - conf.getExpireTimeInMS() ;

      // check if we need to check limit of configure or empty of recycle bin
      checkLimit = _limitCheckFlag.compareAndSwap( 1, 0 ) &&
                   conf.getAutoDrop() &&
                   !conf.isCapacityUnlimited() ;
      checkEmpty = _emptyCheckFlag.compareAndSwap( 1, 0 ) ;
      if ( checkLimit || checkEmpty )
      {
         INT64 count = 0 ;

         rc = countAllItems( cb, count ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to count items, rc: %d", rc ) ;

         if ( 0 == count )
         {
            _isEmptyFlag = TRUE ;
         }
         else
         {
            _isEmptyFlag = FALSE ;

            // check if count is exceed maximum number of items
            if ( count > (INT64)( conf.getMaxItemNum() ) )
            {
               INT64 expectDropCount = count - conf.getMaxItemNum() ;
               rc = _getOldestItems( OSS_UINT64_MAX, expectDropCount, cb,
                                     itemList ) ;
               PD_RC_CHECK( rc, PDWARNING, "Failed to get oldest expired items, "
                            "rc: %d", rc ) ;
            }
            else
            {
               // passed limit check
               checkLimit = FALSE ;
            }
         }
         // passed empty check
         checkEmpty = FALSE ;
      }

      if ( _isEmptyFlag )
      {
         // recycle bin is empty
         PD_LOG( PDDEBUG, "Recycle bin is empty" ) ;
         isFinished = TRUE ;
         goto done ;
      }
      else if ( itemList.empty() )
      {
         if ( !_isOldestItemExpired( expiredTime ) )
         {
            // oldest item is not expired yet
            isFinished = TRUE ;
            goto done ;
         }

         rc = _getOldestItems( expiredTime, -1, cb, itemList ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get oldest expired items, "
                      "rc: %d", rc ) ;
      }

      for ( iter = itemList.begin() ;
            iter != itemList.end() ;
            ++ iter )
      {
         utilRecycleItem &item = *iter ;
         UINT32 returnCount = 0 ;
         BOOLEAN isDropped = FALSE ;

         // take drop latch
         ossScopedLock _lock( _getDropLatch() ) ;

         PD_CHECK( pmdIsPrimary(), SDB_CLS_NOT_PRIMARY, error, PDINFO,
                   "Failed to check primary status" ) ;
         PD_CHECK( !cb->isInterrupted(), SDB_APP_INTERRUPT, error, PDWARNING,
                   "Failed to drop item, session is interrupted" ) ;

         returnCount = _returnCount.fetch() ;
         if ( 0 != returnCount )
         {
            PD_LOG( PDINFO, "Pause dropping expired recycle items, found [%u] "
                    "returning contexts", returnCount ) ;
            goto done ;
         }

         rc = _dropItem( item, cb, 1, FALSE, isDropped ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to drop item "
                      "[origin %s, recycle %s], rc: %d", item.getOriginName(),
                      item.getRecycleName(), rc ) ;

         if ( !isDropped )
         {
            // not dropped yet, mark not finished
            isFinished = FALSE ;
            break ;
         }
      }

      // cache the oldest one if the list had been dropped entirely
      if ( iter == itemList.end() )
      {
         utilRecycleItem item ;
         BOOLEAN fromCache = FALSE ;
         rc = _getAndCacheOldestItem( cb, item, fromCache ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get oldest recycle item, "
                      "rc: %d", rc ) ;
         if ( item.isValid() )
         {
            if ( !_isOldestItemExpired( expiredTime ) )
            {
               isFinished = TRUE ;
            }
         }
         else
         {
            isFinished = TRUE ;
            // all items are dropped, check empty in next round
            checkEmpty = TRUE ;
         }
      }

   done:
      if ( checkLimit )
      {
         // make sure back ground job will start limit check again
         enableLimitCheck() ;
      }
      else if ( isFinished && checkEmpty )
      {
         // make sure back ground job will start empty check again
         enableEmptyCheck() ;
      }
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR_DROPEXPIREDITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__CREATEBGJOB, "_rtnRecycleBinManager::_createBGJob" )
   INT32 _rtnRecycleBinManager::_createBGJob( utilLightJob **pJob )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__CREATEBGJOB ) ;

      rtnDropRecycleBinBGJob *job = NULL ;

      SDB_ASSERT( NULL != pJob, "job pointer is invalid" ) ;

      job = SDB_OSS_NEW rtnDropRecycleBinBGJob( this ) ;
      PD_CHECK( NULL != job, SDB_OOM, error, PDERROR, "Failed to allocate BG "
                "job, rc: %d", rc ) ;

      *pJob = (utilLightJob *)job ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__CREATEBGJOB, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__GETITEMOBJ_RECYNAME, "_rtnRecycleBinManager::_getItemObject" )
   INT32 _rtnRecycleBinManager::_getItemObject( const CHAR *recycleName,
                                                pmdEDUCB *cb,
                                                BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__GETITEMOBJ_RECYNAME ) ;

      BSONObj matcher ;

      try
      {
         matcher = BSON( FIELD_NAME_RECYCLE_NAME << recycleName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _getItemObject( matcher, cb, object ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item [%s], rc: %d",
                   recycleName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__GETITEMOBJ_RECYNAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__GETITEMOBJ_ORIGID, "_rtnRecycleBinManager::_getItemObject" )
   INT32 _rtnRecycleBinManager::_getItemObjectByOrigID( utilCLUniqueID originID,
                                                        pmdEDUCB *cb,
                                                        BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__GETITEMOBJ_ORIGID ) ;

      BSONObj matcher ;

      try
      {
         matcher = BSON( FIELD_NAME_ORIGIN_ID << (INT64)originID ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _getItemObject( matcher, cb, object ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get recycle item "
                   "[origin ID: %llu], rc: %d", originID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__GETITEMOBJ_ORIGID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

    // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__GETITEMOBJ_RECYID, "_rtnRecycleBinManager::_getItemObjectByRecyID" )
   INT32 _rtnRecycleBinManager::_getItemObjectByRecyID( utilGlobalID recycleID,
                                                        pmdEDUCB *cb,
                                                        BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__GETITEMOBJ_RECYID ) ;

      BSONObj matcher ;

      try
      {
         matcher = BSON( FIELD_NAME_RECYCLE_ID << (INT64)recycleID ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _getItemObject( matcher, cb, object ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get recycle item "
                   "[recycle ID: %llu], rc: %d", recycleID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__GETITEMOBJ_RECYID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__GETITEMOBJ, "_rtnRecycleBinManager::_getItemObject" )
   INT32 _rtnRecycleBinManager::_getItemObject( const BSONObj &matcher,
                                                pmdEDUCB *cb,
                                                BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__GETITEMOBJ ) ;

      BSONObj dummy ;
      INT64 contextID = -1 ;
      rtnContextBuf buffObj ;

      rc = _getItems( matcher, dummy, dummy, 1, cb, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item, rc: %d", rc ) ;

      // get more
      rc = rtnGetMore( contextID, 1, buffObj, cb, _rtnCB ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_RECYCLE_ITEM_NOTEXIST ;
         }
         contextID = -1 ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get recycle item from context, "
                      "rc: %d", rc ) ;
      }

      try
      {
         BSONObj resultObj( buffObj.data() ) ;
         object = resultObj.copy() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to copy BSON object, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         _rtnCB->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__GETITEMOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__GETITEMS, "_rtnRecycleBinManager::_getItems" )
   INT32 _rtnRecycleBinManager::_getItems( const BSONObj &matcher,
                                           const BSONObj &orderBy,
                                           const BSONObj &hint,
                                           INT64 numToReturn,
                                           pmdEDUCB *cb,
                                           INT64 &contextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__GETITEMS ) ;

      rtnQueryOptions options ;
      options.setCLFullName( _getRecyItemCL() ) ;
      options.setQuery( matcher ) ;
      options.setOrderBy( orderBy ) ;
      options.setHint( hint ) ;
      options.setLimit( numToReturn ) ;

      // perform query
      rc = rtnQuery( options, cb, _dmsCB, _rtnCB, contextID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to execute query on collection [%s], "
                    "rc: %d", _getRecyItemCL(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__GETITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__GETITEMS_LIST, "_rtnRecycleBinManager::_getItems" )
   INT32 _rtnRecycleBinManager::_getItems( const BSONObj &matcher,
                                           const BSONObj &orderBy,
                                           const BSONObj &hint,
                                           INT64 numToReturn,
                                           pmdEDUCB *cb,
                                           UTIL_RECY_ITEM_LIST &itemList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__GETITEMS_LIST ) ;

      INT64 contextID = -1 ;

      // perform query
      rc = _getItems( matcher, orderBy, hint, numToReturn, cb, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to query recycle items, "
                   "rc: %d", rc ) ;

      while ( TRUE )
      {
         rtnContextBuf buffObj ;

         // get one result
         rc = rtnGetMore( contextID, 1, buffObj, cb, _rtnCB ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               contextID = -1 ;
               goto done ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to get recycle items from query, "
                         "rc: %d", rc ) ;
         }

         try
         {
            utilRecycleItem item ;

            // parse object
            BSONObj object( buffObj.data() ) ;
            rc = item.fromBSON( object ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get item from BSON, rc: %d",
                         rc ) ;

            // save to item list
            itemList.push_back( item ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to parse recycle item from BSON, "
                    "occur exception %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      if ( -1 != contextID )
      {
         _rtnCB->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__GETITEMS_LIST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__COUNTITEMS_HINT, "_rtnRecycleBinManager::_countItems" )
   INT32 _rtnRecycleBinManager::_countItems( const BSONObj &matcher,
                                             const BSONObj &hint,
                                             pmdEDUCB *cb,
                                             INT64 &count )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__COUNTITEMS_HINT ) ;

      rtnQueryOptions options ;
      options.setCLFullName( _getRecyItemCL() ) ;
      options.setQuery( matcher ) ;
      options.setHint( hint ) ;

      rc = rtnGetCount( options, _dmsCB, cb, _rtnCB, &count ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get count on collection [%s], "
                   "rc: %d", _getRecyItemCL(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__COUNTITEMS_HINT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__DELITEM, "_rtnRecycleBinManager::_deleteItem" )
   INT32 _rtnRecycleBinManager::_deleteItem( const utilRecycleItem &item,
                                             pmdEDUCB *cb,
                                             INT16 w,
                                             BOOLEAN ignoreNotExists )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__DELITEM ) ;

      BSONObj matcher ;
      UINT64 deletedCount = 0 ;

      try
      {
         matcher = BSON( FIELD_NAME_RECYCLE_NAME << item.getRecycleName() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _deleteItems( matcher, cb, w, deletedCount ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle item [%s], rc: %d",
                   item.getRecycleName(), rc ) ;

      PD_CHECK( ignoreNotExists || deletedCount > 0,
                SDB_RECYCLE_ITEM_NOTEXIST, error, PDWARNING,
                "Failed to delete recycle item [origin %s, recycle %s], "
                "it does not exist any more", item.getOriginName(),
                item.getRecycleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__DELITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__DELITEMS, "_rtnRecycleBinManager::_deleteItems" )
   INT32 _rtnRecycleBinManager::_deleteItems( const BSONObj &matcher,
                                              pmdEDUCB *cb,
                                              INT16 w,
                                              UINT64 &deletedCount )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__DELITEMS ) ;

      utilDeleteResult deleteResult ;
      rtnQueryOptions options ;

      deletedCount = 0 ;

      options.setCLFullName( _getRecyItemCL() ) ;
      options.setQuery( matcher ) ;

      rc = rtnDelete( options, cb, _dmsCB, _dpsCB, w, &deleteResult ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle items "
                   "from collection [%s], rc: %d", _getRecyItemCL(), rc ) ;

      deletedCount = deleteResult.deletedNum() ;

#if defined (_DEBUG)
      PD_LOG( PDDEBUG, "Deleted [%llu] items by [%s]", deletedCount,
              matcher.toPoolString().c_str() ) ;
#endif

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__DELITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__DELALLITEMS, "_rtnRecycleBinManager::_deleteAllItems" )
   INT32 _rtnRecycleBinManager::_deleteAllItems( pmdEDUCB *cb,
                                                 INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__DELALLITEMS ) ;

      const CHAR *recycleCollection = _getRecyItemCL() ;

      // use truncate instead of delete
      rc = rtnTruncCollectionCommand( recycleCollection, cb, _dmsCB, _dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete all recycle items from "
                   "collection [%s], rc: %d", recycleCollection, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__DELALLITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__GETOLDESTITEMS, "_rtnRecycleBinManager::_getOldestItems" )
   INT32 _rtnRecycleBinManager::_getOldestItems( UINT64 expiredTime,
                                                 INT64 numToReturn,
                                                 pmdEDUCB *cb,
                                                 UTIL_RECY_ITEM_LIST &itemList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__GETOLDESTITEMS ) ;

      BSONObj matcher, orderBy ;

      try
      {
         BSONObjBuilder matcherBuilder( 64 ) ;
         if ( expiredTime < OSS_UINT64_MAX )
         {
            /* matcher = { "RecycleTime": { "$cast": "timestamp",
             *                              "$lte": { $timestamp: XXXX } } }
             */
            BSONObjBuilder timeBuilder(
                  matcherBuilder.subobjStart( FIELD_NAME_RECYCLE_TIME ) ) ;
            timeBuilder.append( "$cast", "timestamp" ) ;
            timeBuilder.appendTimestamp( "$lte", (INT64)expiredTime, 0 ) ;
            timeBuilder.doneFast() ;
         }
         matcher = matcherBuilder.obj() ;

         // the recycle ID is increased with time
         orderBy = BSON( FIELD_NAME_RECYCLE_ID << 1 ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _getItems( matcher, orderBy, _hintRecyID, numToReturn, cb, itemList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the oldest recycle item, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__GETOLDESTITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__GETANDCACHEOLDESTITEM, "_rtnRecycleBinManager::_getAndCacheOldestItem" )
   INT32 _rtnRecycleBinManager::_getAndCacheOldestItem( pmdEDUCB *cb,
                                                        utilRecycleItem &item,
                                                        BOOLEAN &fromCache )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__GETANDCACHEOLDESTITEM ) ;

      fromCache = TRUE ;

      // get from cache first
      _getOldestItem( item ) ;

      // cache is not valid, get from disk
      if ( !item.isValid() )
      {
         fromCache = FALSE ;

         UTIL_RECY_ITEM_LIST itemList ;
         rc = _getOldestItems( OSS_UINT64_MAX, 1, cb, itemList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get oldest item list, rc: %d",
                      rc ) ;
         if ( !itemList.empty() )
         {
            item = itemList.front() ;

            // cache to oldest item
            _cacheOldestItem( item ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__GETANDCACHEOLDESTITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__ISOLDESTITEMEXPIRED, "_rtnRecycleBinManager::_isOldestItemExpired" )
   BOOLEAN _rtnRecycleBinManager::_isOldestItemExpired( UINT64 expiredTime )
   {
      BOOLEAN isExpired = FALSE ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__ISOLDESTITEMEXPIRED ) ;

      ossScopedLock lock( &_oldestItemLatch ) ;

      // oldest one is not expired yet
      if ( _oldestItem.isValid() )
      {
         if ( _oldestItem.getRecycleTime() <= expiredTime )
         {
            isExpired = TRUE ;
            _oldestItem.reset() ;
         }
      }
      else
      {
         // cache is invalid, we don't know yet, mark it expired
         isExpired = TRUE ;
      }

      PD_TRACE_EXIT( SDB__RTNRECYBINMGR__ISOLDESTITEMEXPIRED ) ;

      return isExpired ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__GETOLDESTITEM, "_rtnRecycleBinManager::_getOldestItem" )
   void _rtnRecycleBinManager::_getOldestItem( utilRecycleItem &item )
   {
      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__GETOLDESTITEM ) ;

      ossScopedLock lock( &_oldestItemLatch ) ;
      item = _oldestItem ;

      PD_TRACE_EXIT( SDB__RTNRECYBINMGR__GETOLDESTITEM ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__CACHEOLDESTITEM, "_rtnRecycleBinManager::_cacheOldestItem" )
   void _rtnRecycleBinManager::_cacheOldestItem( const utilRecycleItem &item )
   {
      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__CACHEOLDESTITEM ) ;

      ossScopedLock lock( &_oldestItemLatch ) ;

      if ( !_oldestItem.isValid() )
      {
         _oldestItem = item ;
      }

      PD_TRACE_EXIT( SDB__RTNRECYBINMGR__CACHEOLDESTITEM ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__RESETOLDESTITEM, "_rtnRecycleBinManager::_resetOldestItem" )
   void _rtnRecycleBinManager::_resetOldestItem( const utilRecycleItem &curItem )
   {
      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__RESETOLDESTITEM ) ;

      ossScopedLock lock( &_oldestItemLatch ) ;

      // only reset in below cases
      if ( _oldestItem.isValid() &&
           _oldestItem.getRecycleID() == curItem.getRecycleID() )
      {
         _oldestItem.reset() ;
      }

      PD_TRACE_EXIT( SDB__RTNRECYBINMGR__RESETOLDESTITEM ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__SAVEITEMOBJ, "_rtnRecycleBinManager::_saveItemObject" )
   INT32 _rtnRecycleBinManager::_saveItemObject( const BSONObj &object,
                                                 pmdEDUCB *cb,
                                                 INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__SAVEITEMOBJ ) ;

      rc = rtnInsert( _getRecyItemCL(), object, 1, 0, cb,
                      _dmsCB, _dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert object to collection [%s], "
                   "rc: %d", _getRecyItemCL(), rc ) ;

      enableEmptyCheck() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__SAVEITEMOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINMGR__UPDATEITEMS, "_rtnRecycleBinManager::_updateItems" )
   INT32 _rtnRecycleBinManager::_updateItems( const BSONObj &matcher,
                                              const BSONObj &updator,
                                              pmdEDUCB *cb,
                                              INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINMGR__UPDATEITEMS ) ;

      rtnQueryOptions options ;
      options.setCLFullName( _getRecyItemCL() ) ;
      options.setQuery( matcher ) ;

      rc = rtnUpdate( options, updator, cb, _dmsCB, _dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update items in collection [%s], "
                   "rc: %d", _getRecyItemCL(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRECYBINMGR__UPDATEITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _rtnDropRecycleBinBGJob implement
    */
   _rtnDropRecycleBinBGJob::_rtnDropRecycleBinBGJob(
                                          rtnRecycleBinManager *recycleBinMgr )
   : _recycleBinMgr( recycleBinMgr )
   {
      SDB_ASSERT( NULL != _recycleBinMgr,
                  "recycle bin manager is invalid" ) ;
   }

   _rtnDropRecycleBinBGJob::~_rtnDropRecycleBinBGJob()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRECYBINBGJOB_DOIT, "_rtnDropRecycleBinBGJob::doit" )
   INT32 _rtnDropRecycleBinBGJob::doit( IExecutor *pExe,
                                        UTIL_LJOB_DO_RESULT &result,
                                        UINT64 &sleepTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRECYBINBGJOB_DOIT ) ;

      // This is an async task, check primary first
      pmdEDUCB *cb = (pmdEDUCB *)pExe ;
      BOOLEAN isFinished = FALSE ;

      sleepTime = RTN_RECYCLE_RETRY_INTERVAL ;
      result = UTIL_LJOB_DO_CONT ;

      if ( PMD_IS_DB_DOWN() )
      {
         PD_LOG( PDDEBUG, "DB is down, stop to drop expired items" ) ;
         result = UTIL_LJOB_DO_FINISH ;
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }
      else if ( pExe->isInterrupted() || pExe->isForced() )
      {
         PD_LOG( PDWARNING, "Failed to check EDU, it is interrupted" ) ;
         result = UTIL_LJOB_DO_FINISH ;
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }
      else if ( !pmdIsPrimary() )
      {
         sleepTime = RTN_RECYCLE_CHECK_INTERVAL ;
         result = UTIL_LJOB_DO_CONT ;
         rc = SDB_CLS_NOT_PRIMARY ;
         goto error ;
      }

      PD_LOG( PDDEBUG, "Begin to drop expired recycle items" ) ;

      rc = _recycleBinMgr->dropExpiredItems( cb, isFinished ) ;
      if ( SDB_APP_INTERRUPT == rc )
      {
         PD_LOG( PDINFO, "Failed to check EDU, it is interrupted" ) ;
         result = UTIL_LJOB_DO_FINISH ;
         goto error ;
      }
      else if ( SDB_CLS_NOT_PRIMARY == rc )
      {
         PD_LOG( PDINFO, "Failed to check primary" ) ;
         sleepTime = RTN_RECYCLE_CHECK_INTERVAL ;
         result = UTIL_LJOB_DO_CONT ;
         goto error ;
      }
      else if ( SDB_RECYCLE_ITEM_NOTEXIST == rc )
      {
         // recycle item does not exist, retry later
         result = UTIL_LJOB_DO_CONT ;
         sleepTime = RTN_RECYCLE_RETRY_INTERVAL ;
         goto error ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to drop expired items, "
                   "rc: %d", rc ) ;

      if ( isFinished )
      {
         // current expired items are dropped, begin interval check
         sleepTime = RTN_RECYCLE_CHECK_INTERVAL ;
         result = UTIL_LJOB_DO_CONT ;
      }
      else
      {
         // current expired items are not dropped, begin next check
         // immediately
         sleepTime = RTN_RECYCLE_RETRY_INTERVAL ;
         result = UTIL_LJOB_DO_CONT ;
      }

   done:
      if ( UTIL_LJOB_DO_FINISH == result )
      {
         PD_LOG( PDDEBUG, "Stop to drop expired recycle items" ) ;
      }
      else
      {
         PD_LOG( PDDEBUG, "Pause to drop expired recycle items" ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNRECYBINBGJOB_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
