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

   Source File Name = catRecycleBinManager.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for catalog node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "catCommon.hpp"
#include "catRecycleBinManager.hpp"
#include "rtn.hpp"
#include "utilCSKeyName.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "../bson/bson.hpp"

using namespace bson ;

namespace engine
{

   // maximum retry count for dropping oldest recycle item
   #define CAT_RECYCLE_MAX_RETRY ( 5 )

   /*
      _catRecycleBinManager implement
    */
   _catRecycleBinManager::_catRecycleBinManager()
   : _BASE(),
     _reservedCount( 0 )
   {
   }

   _catRecycleBinManager::~_catRecycleBinManager()
   {
   }



   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_INIT, "_catRecycleBinManager::init" )
   INT32 _catRecycleBinManager::init( const utilRecycleBinConf &conf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_INIT ) ;

      rc = _BASE::init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init based recycle bin manager, "
                   "rc: %d", rc ) ;

      // set configure in cache
      setConf( conf ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_ACTIVE, "_catRecycleBinManager::active" )
   INT32 _catRecycleBinManager::active( const utilRecycleBinConf &conf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_ACTIVE ) ;

      // set configure in cache
      setConf( conf ) ;

      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_ACTIVE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_UPDATECONF, "_catRecycleBinManager::updateConf" )
   INT32 _catRecycleBinManager::updateConf( const utilRecycleBinConf &newConf,
                                            pmdEDUCB *cb,
                                            INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_UPDATECONF ) ;

      rc = catUpdateRecycleBinConf( newConf, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update recycle bin conf, "
                   "rc: %d", rc ) ;

      setConf( newConf ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_UPDATECONF, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__GETITEMSINCS, "_catRecycleBinManager::_getItemsInCS" )
   INT32 _catRecycleBinManager::_getItemsInCS( utilCSUniqueID csUniqueID,
                                               pmdEDUCB *cb,
                                               UTIL_RECY_ITEM_LIST &itemList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__GETITEMSINCS ) ;

      BSONObj matcher, dummy ;

      rc = utilGetRecyCLsInCSBounds( FIELD_NAME_ORIGIN_ID,
                                     csUniqueID,
                                     matcher ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get bounds of recycled collections "
                   "with collection space unique ID [%u], rc: %d", csUniqueID,
                   rc ) ;

      rc = _getItems( matcher, dummy, dummy, -1, cb, itemList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle items from "
                   "collection space [%u], rc: %d", csUniqueID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__GETITEMSINCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_DROPITEM, "_catRecycleBinManager::dropItem" )
   INT32 _catRecycleBinManager::dropItem( const utilRecycleItem &item,
                                          pmdEDUCB *cb,
                                          INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_DROPITEM ) ;

      BOOLEAN isDropped = FALSE ;

      rc = _dropItem( item, cb, w, TRUE, isDropped ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item [origin %s, "
                   "recycle %s], rc: %d", item.getOriginName(),
                   item.getRecycleName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_DROPITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__DROPITEM, "_catRecycleBinManager::_dropItem" )
   INT32 _catRecycleBinManager::_dropItem( const utilRecycleItem &item,
                                           pmdEDUCB *cb,
                                           INT16 w,
                                           BOOLEAN ignoreNotExists,
                                           BOOLEAN &isDropped )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__DROPITEM ) ;

      isDropped = FALSE ;

      if ( UTIL_RECYCLE_CS == item.getType() )
      {
         UTIL_RECY_ITEM_LIST relatedItems ;
         utilCSUniqueID csUniqueID = (utilCSUniqueID)( item.getOriginID() ) ;
         rc = _getItemsInCS( csUniqueID, cb, relatedItems ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get related collection "
                      "recycle items, rc: %d", rc ) ;

         for ( UTIL_RECY_ITEM_LIST_IT iter = relatedItems.begin() ;
               iter != relatedItems.end() ;
               ++ iter )
         {
            utilRecycleItem &tmpItem = *iter ;
            rc = _dropItemImpl( tmpItem, cb, w, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item "
                         "[origin %s, recycle %s], rc: %d",
                         tmpItem.getOriginName(), tmpItem.getRecycleName(),
                         rc ) ;
         }
      }

      rc = _dropItemImpl( item, cb, w, ignoreNotExists ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item [origin %s, "
                   "recycle %s], rc: %d",  item.getOriginName(),
                   item.getRecycleName(), rc ) ;

      isDropped = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__DROPITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__DROPITEMIMPL, "_catRecycleBinManager::_dropItemImpl" )
   INT32 _catRecycleBinManager::_dropItemImpl( const utilRecycleItem &item,
                                               pmdEDUCB *cb,
                                               INT16 w,
                                               BOOLEAN ignoreNotExists )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__DROPITEMIMPL ) ;

      BSONObj matcher ;

      try
      {
         matcher = BSON( FIELD_NAME_RECYCLE_ID <<
                         (INT64)( item.getRecycleID() ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      if ( UTIL_RECYCLE_CS == item.getType() )
      {
         rc = _deleteObjects( UTIL_RECYCLE_IDX, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                      "index objects, rc: %d", rc ) ;

         rc = _deleteObjects( UTIL_RECYCLE_SEQ, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                      "sequence objects, rc: %d", rc ) ;

         rc = _deleteObjects( UTIL_RECYCLE_CL, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                      "collection objects, rc: %d", rc ) ;

         rc = _deleteObjects( UTIL_RECYCLE_CS, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                      "collection space objects, rc: %d", rc ) ;
      }
      else if ( UTIL_RECYCLE_CL == item.getType() )
      {
         rc = _deleteObjects( UTIL_RECYCLE_IDX, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                      "index objects, rc: %d", rc ) ;

         rc = _deleteObjects( UTIL_RECYCLE_SEQ, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                      "sequence objects, rc: %d", rc ) ;

         rc = _deleteObjects( UTIL_RECYCLE_CL, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                      "collection objects, rc: %d", rc ) ;
      }
      else
      {
         SDB_ASSERT( FALSE, "invalid recycle type" ) ;
         PD_LOG( PDWARNING, "Found invalid recycle type [%d]",
                 item.getType() ) ;
      }

      rc = _deleteItem( item, cb, w, ignoreNotExists ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle item [%s], rc: %d",
                   item.getRecycleName(), rc ) ;

      PD_LOG( PDEVENT, "Dropped recycle item [origin %s, recycle %s]",
              item.getOriginName(), item.getRecycleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__DROPITEMIMPL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_DROPALLITEMS, "_catRecycleBinManager::dropAllItems" )
   INT32 _catRecycleBinManager::dropAllItems( pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_DROPALLITEMS ) ;


      rc = _deleteAllObjects( UTIL_RECYCLE_IDX, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle "
                   "index objects, rc: %d", rc ) ;

      rc = _deleteAllObjects( UTIL_RECYCLE_SEQ, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete all recycle "
                   "sequence objects, rc: %d", rc ) ;

      rc = _deleteAllObjects( UTIL_RECYCLE_CL, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete all recycle "
                   "collection objects, rc: %d", rc ) ;

      rc = _deleteAllObjects( UTIL_RECYCLE_CS, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete all recycle "
                   "collection space objects, rc: %d", rc ) ;

      rc = _deleteAllItems( cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete all recycle items, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_DROPALLITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__DELOBJS, "_catRecycleBinManager::_deleteObjects" )
   INT32 _catRecycleBinManager::_deleteObjects( UTIL_RECYCLE_TYPE type,
                                                const bson::BSONObj &matcher,
                                                pmdEDUCB *cb,
                                                INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__DELOBJS ) ;

      rtnQueryOptions options ;
      const CHAR *recycleCollection = catGetRecycleBinRecyCL( type ) ;

      options.setCLFullName( recycleCollection ) ;
      options.setQuery( matcher ) ;

      rc = rtnDelete( options, cb, _dmsCB, _dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete objects from recycle "
                   "collection [%s], rc: %d", recycleCollection, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__DELOBJS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__DELALLOBJS, "_catRecycleBinManager::_deleteAllObjects" )
   INT32 _catRecycleBinManager::_deleteAllObjects( UTIL_RECYCLE_TYPE type,
                                                   pmdEDUCB *cb,
                                                   INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__DELALLOBJS ) ;

      const CHAR *recycleCollection = catGetRecycleBinRecyCL( type ) ;

      // use truncate instead of delete
      rc = rtnTruncCollectionCommand( recycleCollection, cb, _dmsCB, _dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete all objects from recycle "
                   "collection [%s], rc: %d", recycleCollection, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__DELALLOBJS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_COUNTITEMSINCS, "_catRecycleBinManager::countItemsInCS" )
   INT32 _catRecycleBinManager::countItemsInCS( utilCSUniqueID csUniqueID,
                                                pmdEDUCB *cb,
                                                INT64 &count )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_COUNTITEMSINCS ) ;

      BSONObj matcher ;

      rc = utilGetRecyCLsInCSBounds( FIELD_NAME_ORIGIN_ID,
                                     csUniqueID,
                                     matcher ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get bounds of recycled collections "
                   "with collection space unique ID [%u], rc: %d", csUniqueID,
                   rc ) ;

      rc = _countItems( matcher, _hintOrigID, cb, count ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get counts of items for "
                   "collection space [%u], rc: %d", csUniqueID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_COUNTITEMSINCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_DROPITEMSINCS, "_catRecycleBinManager::dropItemsInCS" )
   INT32 _catRecycleBinManager::dropItemsInCS( utilCSUniqueID csUniqueID,
                                               pmdEDUCB *cb,
                                               INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_DROPITEMSINCS ) ;

      UTIL_RECY_ITEM_LIST relatedItems ;
      rc = _getItemsInCS( csUniqueID, cb, relatedItems ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get related collection "
                   "recycle items in collection space, rc: %d", rc ) ;

      for ( UTIL_RECY_ITEM_LIST_IT iter = relatedItems.begin() ;
            iter != relatedItems.end() ;
            ++ iter )
      {
         utilRecycleItem &tmpItem = *iter ;
         rc = _dropItemImpl( tmpItem, cb, w, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item "
                      "[origin %s, recycle %s], rc: %d",
                      tmpItem.getOriginName(), tmpItem.getRecycleName(),
                      rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_DROPITEMSINCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_PREPAREITEM, "_catRecycleBinManager::prepareItem" )
   INT32 _catRecycleBinManager::prepareItem( utilRecycleItem &item,
                                             UTIL_RECY_ITEM_LIST &droppingItems,
                                             catCtxLockMgr &lockMgr,
                                             pmdEDUCB *cb,
                                             INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_PREPAREITEM ) ;

      utilRecycleID recycleID = UTIL_RECYCLEID_NULL ;
      BOOLEAN isAvailable = FALSE, isLimitedByVersion = FALSE ;
      const CHAR *originName = item.getOriginName() ;
      utilCLUniqueID originID = item.getOriginID() ;
      BOOLEAN needDoubleCheckAvail = TRUE ;

      utilRecycleBinConf conf = getConf() ;

      rc = _checkAvailable( originName, originID, conf, cb, isAvailable,
                            isLimitedByVersion ) ;
      if( SDB_RECYCLE_FULL == rc && conf.getAutoDrop() )
      {
         utilRecycleItem droppingItem ;
         if ( isLimitedByVersion )
         {
            rc = _tryFindOldestItem( originName, originID, cb, droppingItem ) ;
            if ( SDB_LOCK_FAILED == rc )
            {
               SDB_ASSERT( conf.getAutoDrop(), "conf can be AutoDrop" ) ;
               rc = _checkDelayLockFailed( conf ) ;
               needDoubleCheckAvail = FALSE ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to drop oldest recycle item "
                         "for [%s] in different versions, rc: %d",
                         originName, rc ) ;
         }
         else
         {
            rc = _tryFindOldestItem( cb, droppingItem ) ;
            if ( SDB_LOCK_FAILED == rc )
            {
               SDB_ASSERT( conf.getAutoDrop(), "conf can be AutoDrop" ) ;
               rc = _checkDelayLockFailed( conf ) ;
               needDoubleCheckAvail = FALSE ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to drop oldest recycle item, "
                         "rc: %d", rc ) ;
         }
         if ( droppingItem.isValid() )
         {
            PD_LOG( PDDEBUG, "Released capacity for recycle item, dropping "
                    "recycle item [%s]", droppingItem.getRecycleName() ) ;

            try
            {
               droppingItems.push_back( droppingItem ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to add dropping item, "
                       "occur exception %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }

            isAvailable = TRUE ;
            rc = SDB_OK ;
         }
         else if ( needDoubleCheckAvail )
         {
            // didn't drop any oldest item, may background job do it,
            // we can check available again
            rc = _checkAvailable( originName, originID, conf, cb, isAvailable,
                                  isLimitedByVersion ) ;
         }
      }
      if ( SDB_RECYCLE_FULL == rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to check recycle bin, "
                     "the number of recycle bin items is up to limit [%s: %d]",
                     isLimitedByVersion ?
                           FIELD_NAME_MAXVERNUM : FIELD_NAME_MAXITEMNUM,
                     isLimitedByVersion ?
                           conf.getMaxVersionNum() : conf.getMaxItemNum() ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to check recycle bin, rc: %d",
                   rc ) ;

      if ( !isAvailable )
      {
         goto done ;
      }

      // acquire recycle ID
      rc = _acquireID( recycleID, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to acquire recycle ID, "
                   "rc: %d", rc ) ;

      // initialize recycle item
      item.init( recycleID ) ;

      // lock items
      rc = _tryLockItemsForRecycle( item, droppingItems, cb, lockMgr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock recycle items, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_PREPAREITEM, rc ) ;
      return rc ;

   error:
      item.reset() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_ACQID, "_catRecycleBinManager::_acquireID" )
   INT32 _catRecycleBinManager::_acquireID( utilRecycleID &recycleID,
                                            pmdEDUCB *cb,
                                            INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_ACQID ) ;

      rc = catIncAndFetchRecycleID( recycleID, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle ID, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_ACQID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__CHECKAVAILABLE, "_catRecycleBinManager::_checkAvailable" )
   INT32 _catRecycleBinManager::_checkAvailable( const CHAR *originName,
                                                 utilCLUniqueID originID,
                                                 const utilRecycleBinConf &conf,
                                                 pmdEDUCB *cb,
                                                 BOOLEAN &isAvailable,
                                                 BOOLEAN &isLimitedByVersion )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__CHECKAVAILABLE ) ;

      isAvailable = FALSE ;
      isLimitedByVersion = FALSE ;

      if ( !conf.isEnabled() )
      {
         goto done ;
      }

      rc = _checkVersion( originName, originID, conf, cb ) ;
      if ( SDB_RECYCLE_FULL == rc )
      {
         isLimitedByVersion = TRUE ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to check version number of "
                   "recycle bin, rc: %d", rc ) ;

      rc = _checkCapacity( conf, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check capacity of recycle bin, "
                   "rc: %d", rc ) ;

      isAvailable = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__CHECKAVAILABLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_COMMITITEM, "_catRecycleBinManager::commitItem" )
   INT32 _catRecycleBinManager::commitItem( utilRecycleItem &item,
                                            pmdEDUCB *cb,
                                            INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_COMMITITEM ) ;

      if ( !item.isValid() )
      {
         goto done ;
      }

      rc = _recycleItem( item, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to recycle item "
                   "[origin %s, recycle %s], rc: %d", item.getOriginName(),
                   item.getRecycleName(), rc ) ;

      // save recycle item itself
      rc = _saveItem( item, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save recycle item, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_COMMITITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_RTRNITEM, "_catRecycleBinManager::returnItem" )
   INT32 _catRecycleBinManager::returnItem( utilRecycleItem &item,
                                            catRecycleReturnInfo &info,
                                            pmdEDUCB *cb,
                                            INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_RTRNITEM ) ;

      if ( UTIL_RECYCLE_CS == item.getType() )
      {
         rc = _returnCSObjects( item, info, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to return collection space "
                      "objects, rc: %d", rc ) ;

         rc = _returnCLObjects( item, info, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to return collection objects, "
                      "rc: %d", rc ) ;

         rc = _returnSeqObjects( item, info, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to return sequence objects, "
                      "rc: %d", rc ) ;

         rc = _returnIdxObjects( item, info, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to return index objects, "
                      "rc: %d", rc ) ;

         rc = _unsetCSRecycled( item, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update recycle items in "
                      "collection space [%s], rc: %d",
                      item.getOriginName(), rc ) ;

      }
      else if ( UTIL_RECYCLE_CL == item.getType() )
      {
         rc = _returnCLObjects( item, info, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to return collection objects, "
                      "rc: %d", rc ) ;

         rc = _returnSeqObjects( item, info, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to return sequence objects, "
                      "rc: %d", rc ) ;

         rc = _returnIdxObjects( item, info, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to return index objects, "
                      "rc: %d", rc ) ;
      }
      else
      {
         SDB_ASSERT( FALSE, "invalid recycle type" ) ;
         PD_LOG( PDWARNING, "Found invalid recycle type [%d]",
                 item.getType() ) ;
      }

      rc = _dropItemImpl( item, cb, w, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item, rc: %d",
                   rc ) ;

      PD_LOG( PDEVENT, "Returned recycle item [origin %s, recycle %s]",
              item.getOriginName(), item.getRecycleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_RTRNITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__TRYLOCKITEM, "_catRecycleBinManager::tryLockItem" )
   INT32 _catRecycleBinManager::tryLockItem( const utilRecycleItem &item,
                                             pmdEDUCB *cb,
                                             OSS_LATCH_MODE mode,
                                             catCtxLockMgr &lockMgr,
                                             ossPoolSet< utilCSUniqueID > *lockedCS,
                                             BOOLEAN isCheckSubCL )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__TRYLOCKITEM ) ;
      try
      {

         if ( UTIL_RECYCLE_CL == item.getType() )
         {
            if ( ( NULL != lockedCS &&
                   !lockedCS->count( utilGetCSUniqueID( item.getOriginID() ) ) ) ||
                   NULL == lockedCS )
            {
               PD_CHECK( lockMgr.tryLockRecycleItem( item, mode ),
                         SDB_LOCK_FAILED, error, PDERROR,
                         "Failed to lock recycle item [origin %s, recycle %s]",
                         item.getOriginName(), item.getRecycleName() ) ;
            }
            if ( item.isMainCL() && isCheckSubCL )
            {
               ossPoolSet< utilCSUniqueID > lockedSubCLCS ;
               lockedSubCLCS.insert( utilGetCSUniqueID( item.getOriginID() )  ) ;
               _catRecycleSubCLLocker sublocker( this, item, lockMgr, mode,
                                                 lockedCS, lockedSubCLCS ) ;
               rc = processObjects( sublocker, cb , 1 ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to process lock subcl item, "
                            "rc: %d", rc ) ;
            }
         }
         else if ( UTIL_RECYCLE_CS == item.getType() )
         {
            if ( NULL != lockedCS &&
                 !lockedCS->count( (utilCSUniqueID)( item.getOriginID() ) ) )
            {
               PD_CHECK( lockMgr.tryLockRecycleItem( item, mode ),
                         SDB_LOCK_FAILED, error, PDERROR,
                         "Failed to lock recycle item [origin %s, recycle %s]",
                         item.getOriginName(), item.getRecycleName() ) ;

               lockedCS->insert( (utilCSUniqueID)( item.getOriginID() ) ) ;
            }
            else if ( NULL == lockedCS )
            {
               PD_CHECK( lockMgr.tryLockRecycleItem( item, mode ),
                         SDB_LOCK_FAILED, error, PDERROR,
                         "Failed to lock recycle item [origin %s, recycle %s]",
                         item.getOriginName(), item.getRecycleName() ) ;
            }
         }
         else
         {
            SDB_ASSERT( FALSE, "Invalid type for recycle item" ) ;
         }

      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to lock recycle items, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__TRYLOCKITEM, rc ) ;
      return rc ;

   error:
      goto done ;

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__RTRNCSOBJS, "_catRecycleBinManager::_returnCSObjects" )
   INT32 _catRecycleBinManager::_returnCSObjects( utilRecycleItem &item,
                                                  catRecycleReturnInfo &info,
                                                  pmdEDUCB *cb,
                                                  INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__RTRNCSOBJS ) ;

      catReturnCSProcessor processor( this, item, info ) ;

      rc = processObjects( processor, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process return collection spaces, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__RTRNCSOBJS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__RTRNCLOBJS, "_catRecycleBinManager::_returnCLObjects" )
   INT32 _catRecycleBinManager::_returnCLObjects( utilRecycleItem &item,
                                                  catRecycleReturnInfo &info,
                                                  pmdEDUCB *cb,
                                                  INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__RTRNCLOBJS ) ;

      catReturnCLProcessor processor( this, item, info ) ;

      rc = processObjects( processor, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process return collections, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__RTRNCLOBJS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__RTRNSEQOBJS, "_catRecycleBinManager::_returnSeqObjects" )
   INT32 _catRecycleBinManager::_returnSeqObjects( utilRecycleItem &item,
                                                   catRecycleReturnInfo &info,
                                                   pmdEDUCB *cb,
                                                   INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__RTRNSEQOBJS ) ;

      catReturnSeqProcessor processor( this, item, info ) ;

      rc = processObjects( processor, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process return sequences, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__RTRNSEQOBJS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__RTRNIDXOBJS, "_catRecycleBinManager::_returnIdxObjects" )
   INT32 _catRecycleBinManager::_returnIdxObjects( utilRecycleItem &item,
                                                   catRecycleReturnInfo &info,
                                                   pmdEDUCB *cb,
                                                   INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__RTRNIDXOBJS ) ;

      catReturnIdxProcessor processor( this, item, info,
                                       ( IXM_EXTENT_TYPE_TEXT |
                                         IXM_EXTENT_TYPE_GLOBAL ) ) ;

      rc = processObjects( processor, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process return indexes, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__RTRNIDXOBJS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__RECYITEM, "_catRecycleBinManager::_recycleItem" )
   INT32 _catRecycleBinManager::_recycleItem( utilRecycleItem &item,
                                              pmdEDUCB *cb,
                                              INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__RECYITEM ) ;

      switch ( item.getType() )
      {
         case UTIL_RECYCLE_CS :
         {
            rc = _recycleCSObjects( item, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle collection space "
                         "objects, rc: %d", rc ) ;

            rc = _recycleCLObjects( item, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle collection objects, "
                         "rc: %d", rc ) ;

            rc = _recycleSeqObjects( item, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle sequence objects, "
                         "rc: %d", rc ) ;

            rc = _recycleIdxObjects( item, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle index objects, "
                         "rc: %d", rc ) ;

            // update recycle items for collections inside this collections
            // space
            rc = _setCSRecycled( item, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to update recycle items in "
                         "collection space [%s], rc: %d",
                         item.getOriginName(), rc ) ;

            break ;
         }
         case UTIL_RECYCLE_CL :
         {
            rc = _recycleCLObjects( item, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle collection objects, "
                         "rc: %d", rc ) ;

            rc = _recycleSeqObjects( item, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle sequence objects, "
                         "rc: %d", rc ) ;

            rc = _recycleIdxObjects( item, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle index objects, "
                         "rc: %d", rc ) ;

            break ;
         }
         default :
         {
            SDB_ASSERT( FALSE, "invalid recycle type" ) ;
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to recycle "
                      "item, invalid recycle type [%d]", item.getType() ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__RECYITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__COPYCSOBJS, "_catRecycleBinManager::_recycleCSObjects" )
   INT32 _catRecycleBinManager::_recycleCSObjects( utilRecycleItem &item,
                                                   pmdEDUCB *cb,
                                                   INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__COPYCSOBJS ) ;

      catRecycleCSProcessor processor( this, item ) ;
      rc = processObjects( processor, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process recycle collection spaces, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__COPYCSOBJS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__RECYCLOBJS, "_catRecycleBinManager::_recycleCLObjects" )
   INT32 _catRecycleBinManager::_recycleCLObjects( utilRecycleItem &item,
                                                   pmdEDUCB *cb,
                                                   INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__RECYCLOBJS ) ;

      catRecycleCLProcessor processor( this, item ) ;
      rc = processObjects( processor, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process recycle collections, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__RECYCLOBJS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__RECYSEQOBJS, "_catRecycleBinManager::_recycleSeqObjects" )
   INT32 _catRecycleBinManager::_recycleSeqObjects( utilRecycleItem &item,
                                                    pmdEDUCB *cb,
                                                    INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__RECYSEQOBJS ) ;

      catRecycleSeqProcessor processor( this, item ) ;
      rc = processObjects( processor, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process recycle sequences, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__RECYSEQOBJS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__RECYIDXOBJS, "_catRecycleBinManager::_recycleIdxObjects" )
   INT32 _catRecycleBinManager::_recycleIdxObjects( utilRecycleItem &item,
                                                    pmdEDUCB *cb,
                                                    INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__RECYIDXOBJS ) ;

      catRecycleIdxProcessor processor( this, item ) ;
      rc = processObjects( processor, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process recycle indexes, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__RECYIDXOBJS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__SETCSRECY, "_catRecycleBinManager::_setCSRecycled" )
   INT32 _catRecycleBinManager::_setCSRecycled( const utilRecycleItem &item,
                                                pmdEDUCB *cb,
                                                INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__SETCSRECY ) ;

      SDB_ASSERT( UTIL_RECYCLE_CS == item.getType(),
                  "should be recycle item for collection space" ) ;

      BSONObj matcher, updator ;

      utilCSUniqueID csUniqueID = (utilCSUniqueID)( item.getOriginID() ) ;
      rc = utilGetRecyCLsInCSBounds( FIELD_NAME_ORIGIN_ID,
                                     csUniqueID,
                                     matcher ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get bounds of recycled collections "
                   "with collection space unique ID [%u], rc: %d", csUniqueID,
                   rc ) ;

      try
      {
         updator = BSON( "$set" <<
                         BSON( FIELD_NAME_RECYCLE_ISCSRECY << true ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON object, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _updateItems( matcher, updator, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update times for recycle "
                   "collection space [%s], rc: %d", item.getOriginName(),
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__SETCSRECY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__UNSETCSRECY, "_catRecycleBinManager::_unsetCSRecycled" )
   INT32 _catRecycleBinManager::_unsetCSRecycled( const utilRecycleItem &item,
                                                  pmdEDUCB *cb,
                                                  INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__UNSETCSRECY ) ;

      SDB_ASSERT( UTIL_RECYCLE_CS == item.getType(),
                  "should be recycle item for collection space" ) ;

      BSONObj matcher, updator ;

      utilCSUniqueID csUniqueID = (utilCSUniqueID)( item.getOriginID() ) ;
      rc = utilGetRecyCLsInCSBounds( FIELD_NAME_ORIGIN_ID,
                                     csUniqueID,
                                     matcher ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get bounds of recycled collections "
                   "with collection space unique ID [%u], rc: %d", csUniqueID,
                   rc ) ;

      try
      {
         updator = BSON( "$unset" <<
                         BSON( FIELD_NAME_RECYCLE_ISCSRECY << true ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON object, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _updateItems( matcher, updator, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update times for recycle "
                   "collection space [%s], rc: %d", item.getOriginName(),
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__UNSETCSRECY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__CHKCAPACITY, "_catRecycleBinManager::_checkCapacity" )
   INT32 _catRecycleBinManager::_checkCapacity( const utilRecycleBinConf &conf,
                                                pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__CHKCAPACITY ) ;

      BSONObj matcher ;
      UINT64 recycleCount = 0 ;

      if ( conf.isCapacityUnlimited() )
      {
         PD_LOG( PDDEBUG, "Passed capacity check of recycle bin, "
                 "which is unlimited" ) ;
         goto done ;
      }

      // quickly check counts with all items
      rc = _countItems( matcher, cb, (INT64 &)recycleCount ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle count, rc: %d", rc ) ;

      PD_CHECK( recycleCount + _reservedCount < (UINT64)( conf.getMaxItemNum() ),
                SDB_RECYCLE_FULL, error, PDWARNING,
                "Failed to check capacity of recycle bin, it is full "
                "[current %llu, reserved %llu, max %llu]",
                recycleCount, (UINT64)_reservedCount,
                (UINT64)( conf.getMaxItemNum() ) ) ;

      PD_LOG( PDDEBUG, "Passed capacity check of recycle bin [%llu/%llu]",
              recycleCount, (UINT64)( conf.getMaxItemNum() ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__CHKCAPACITY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__CHKVERSION, "_catRecycleBinManager::_checkVersion" )
   INT32 _catRecycleBinManager::_checkVersion( const CHAR *originName,
                                               utilCLUniqueID originID,
                                               const utilRecycleBinConf &conf,
                                               pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__CHKVERSION ) ;

      BSONObj matcher ;
      UINT64 recycleCountUID = 0, recycleCountName = 0 ;

      if ( conf.isVersionUnlimited() )
      {
         PD_LOG( PDDEBUG, "Passed version number check of recycle bin, "
                 "is is unlimited" ) ;
         goto done ;
      }

      // count with the same UID
      try
      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_ORIGIN_ID, (INT64)originID ) ;
         matcher = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _countItems( matcher, _hintOrigID, cb, (INT64 &)recycleCountUID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle count, rc: %d", rc ) ;

      PD_CHECK( recycleCountUID < (UINT64)( conf.getMaxVersionNum() ),
                SDB_RECYCLE_FULL, error, PDWARNING,
                "Failed to check version number of recycle bin, it is full "
                "[current with the same unique ID %llu, max %llu]",
                recycleCountUID, (UINT64)( conf.getMaxVersionNum() ) ) ;

      // count with the same name, but with different unique ID
      try
      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_ORIGIN_NAME, originName ) ;
         builder.append( FIELD_NAME_ORIGIN_ID,
                         BSON( "$ne" << (INT64)originID ) ) ;
         matcher = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _countItems( matcher, _hintOrigName, cb, (INT64 &)recycleCountName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle count, rc: %d", rc ) ;

      PD_CHECK( recycleCountName + recycleCountUID <
                                           (UINT64)( conf.getMaxVersionNum() ),
                SDB_RECYCLE_FULL, error, PDWARNING,
                "Failed to check version number of recycle bin, it is full "
                "[current %llu, max %llu]",
                recycleCountName + recycleCountUID,
                (UINT64)( conf.getMaxVersionNum() ) ) ;

      PD_LOG( PDDEBUG, "Passed version number check of recycle bin [%llu/%llu]",
              recycleCountName + recycleCountUID,
              (UINT64)( conf.getMaxVersionNum() ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__CHKVERSION, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__TRYLOCKITEMSFORRECYCLE, "_catRecycleBinManager::_tryLockItemsForRecycle" )
   INT32 _catRecycleBinManager::_tryLockItemsForRecycle( const utilRecycleItem &item,
                                                         const UTIL_RECY_ITEM_LIST &droppingItems,
                                                         pmdEDUCB *cb,
                                                         catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__TRYLOCKITEMSFORRECYCLE ) ;

      try
      {
         {
            ossPoolSet< utilCSUniqueID > lockedCS ;
            catCtxLockMgr dropItemLockMgr ;
            for ( UTIL_RECY_ITEM_LIST_CIT iter = droppingItems.begin() ;
                  iter != droppingItems.end() ;
                  ++ iter )
            {
               const utilRecycleItem &droppingItem = *iter ;
               if ( UTIL_RECYCLE_CS == droppingItem.getType() )
               {
                  rc = tryLockItem( droppingItem, cb, EXCLUSIVE, dropItemLockMgr, &lockedCS ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to lock recycle item, "
                               "rc: %d", rc ) ;
               }
            }

            for ( UTIL_RECY_ITEM_LIST_CIT iter = droppingItems.begin() ;
                  iter != droppingItems.end() ;
                  ++ iter )
            {
               const utilRecycleItem &droppingItem = *iter ;
               if ( UTIL_RECYCLE_CS == droppingItem.getType() )
               {
                  continue ;
               }
               rc = tryLockItem( droppingItem, cb, EXCLUSIVE, dropItemLockMgr, &lockedCS ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to lock recycle item, "
                            "rc: %d", rc ) ;
            }
         }

         rc = tryLockItem( item, cb, EXCLUSIVE, lockMgr, NULL, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock recycle item, "
                      "rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to lock recycle items, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__TRYLOCKITEMSFORRECYCLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__SAVEITEM, "_catRecycleBinManager::_saveItem" )
   INT32 _catRecycleBinManager::_saveItem( utilRecycleItem &item,
                                           pmdEDUCB *cb,
                                           INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__SAVEITEM ) ;

      BSONObj itemObject ;

      rc = item.toBSON( itemObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for "
                   "recycle item [%s] for [%s], rc: %d",
                   item.getRecycleName(), item.getOriginName(), rc ) ;

      rc = _saveItemObject( itemObject, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save recycle item [%s], "
                   "rc: %d", item.getRecycleName(), item.getOriginName(),
                   rc ) ;

      PD_LOG( PDEVENT, "Saved recycle item [origin %s, recycle %s]",
              item.getOriginName(), item.getRecycleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__SAVEITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__TRYFINDOLDESTITEM, "_catRecycleBinManager::_tryFindOldestItem" )
   INT32 _catRecycleBinManager::_tryFindOldestItem( pmdEDUCB *cb,
                                                    utilRecycleItem &oldestItem )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__TRYFINDOLDESTITEM ) ;

      utilRecycleItem candidate ;
      while ( TRUE )
      {
         utilRecycleItem tempItem ;
         BOOLEAN fromCache = FALSE ;

         rc = _getAndCacheOldestItem( cb, tempItem, fromCache ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get oldest recycle item, rc: %d",
                      rc ) ;

         // no oldest item is found
         if ( !tempItem.isValid() )
         {
            PD_LOG( PDINFO, "No oldest item is found" ) ;
            goto done ;
         }

         if ( fromCache )
         {
            BSONObj recycleObject ;
            rc = _getItemObject( tempItem.getRecycleName(), cb, recycleObject ) ;
            if ( SDB_RECYCLE_ITEM_NOTEXIST == rc )
            {
               rc = SDB_OK ;
               _resetOldestItem( tempItem ) ;
               continue ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item [%s], "
                         "rc: %d", tempItem.getRecycleName(), rc ) ;
         }

         candidate = tempItem ;

         break ;
      }

      if ( candidate.isValid() )
      {
         // check if item has been locked, if so, try later items
         catCtxLockMgr lockMgr ;
         rc = tryLockItem( candidate, cb, EXCLUSIVE, lockMgr ) ;
         if ( SDB_LOCK_FAILED == rc )
         {
            rc = SDB_OK ;
            BSONObj matcher, orderBy ;
            UTIL_RECY_ITEM_LIST candItemList ;
            BOOLEAN foundCandidate = FALSE ;

            PD_LOG( PDDEBUG, "Failed to lock oldest item "
                    "[origin %s, recycle %s], try move on later "
                    "available one", candidate.getOriginName(),
                    candidate.getRecycleName() ) ;

            try
            {
               matcher =
                     BSON( FIELD_NAME_RECYCLE_ID <<
                           BSON( "$gt" <<
                                 (INT64)( candidate.getRecycleID() ) ) ) ;
               // the recycle ID is increased with time
               orderBy = BSON( FIELD_NAME_RECYCLE_ID << 1 ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to build recycle item matcher, "
                       "occur exception %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }

            rc = _getItems( matcher, orderBy, _hintRecyID,
                            CAT_RECYCLE_MAX_RETRY, cb, candItemList ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get oldest items after "
                         "item [origin %s, recycle %s], rc: %d",
                         candidate.getOriginName(),
                         candidate.getRecycleName(), rc ) ;

            for ( UTIL_RECY_ITEM_LIST_IT iter = candItemList.begin() ;
                  iter != candItemList.end() ;
                  ++ iter )
            {
               utilRecycleItem &tmpItem = *iter ;
               rc = tryLockItem( tmpItem, cb, EXCLUSIVE, lockMgr ) ;
               if ( SDB_OK == rc )
               {
                  // found candidate
                  PD_LOG( PDDEBUG, "Found later available item "
                          "[origin %s, recycle %s]", tmpItem.getOriginName(),
                          tmpItem.getRecycleName() ) ;
                  candidate = tmpItem ;
                  foundCandidate = TRUE ;
                  break ;
               }
               else if ( SDB_LOCK_FAILED == rc )
               {
                  rc = SDB_OK ;
                  continue ;
               }
               else
               {
                  PD_RC_CHECK( rc, PDERROR, "Failed to lock tmp item "
                               "[origin %s, recycle %s], rc: %d",
                               tmpItem.getOriginName(),
                               tmpItem.getRecycleName(), rc ) ;
               }
            }

            PD_CHECK( foundCandidate, SDB_LOCK_FAILED, error, PDERROR,
                      "Failed to lock dropping item [origin %s, recycle %s]",
                      candidate.getOriginName(),
                      candidate.getRecycleName() ) ;
         }
         else if ( SDB_OK != rc )
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to lock oldest item "
                         "[origin %s, recycle %s], rc: %d",
                         candidate.getOriginName(),
                         candidate.getRecycleName(), rc ) ;
         }
      }
      oldestItem = candidate ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__TRYFINDOLDESTITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__TRYFINDOLDESTITEM_VER, "_catRecycleBinManager::_tryFindOldestItem" )
   INT32 _catRecycleBinManager::_tryFindOldestItem( const CHAR *originName,
                                                    utilCLUniqueID originID,
                                                    pmdEDUCB *cb,
                                                    utilRecycleItem &oldestItem )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__TRYFINDOLDESTITEM_VER ) ;

      utilRecycleItem tempItemName, tempItemUID, tempItem ;
      utilRecycleItem candidate ;

      // fetch oldest item by name
      rc = _getOldestItem( originName, cb, tempItemName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the oldest recycle item "
                   "by name [%s], rc: %d", originName, rc ) ;

      // fetch oldest item by unique ID
      rc = _getOldestItem( originID, cb, tempItemUID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the oldest recycle item "
                   "by unique ID [%llu], rc: %d", originID, rc ) ;

      if ( tempItemName.isValid() && tempItemUID.isValid() )
      {
         tempItem =
               ( tempItemName.getRecycleID() < tempItemUID.getRecycleID() ) ?
                     tempItemName : tempItemUID ;
      }
      else if ( tempItemName.isValid() )
      {
         tempItem = tempItemName ;
      }
      else if ( tempItemUID.isValid() )
      {
         tempItem = tempItemUID ;
      }
      else
      {
         PD_LOG( PDINFO, "No oldest version item is found" ) ;
         goto done ;
      }

      candidate = tempItem ;

      if ( candidate.isValid() )
      {
         // check if item has been locked, if so, try later items
         catCtxLockMgr lockMgr ;
         rc = tryLockItem( candidate, cb, EXCLUSIVE, lockMgr ) ;
         if ( SDB_LOCK_FAILED == rc )
         {
            rc = SDB_OK ;
            UINT32 retryCount = 0 ;
            BSONObj matcherName, matcherUID, orderBy ;
            UTIL_RECY_ITEM_LIST candNameItemList,
                                candUIDItemList ;
            UTIL_RECY_ITEM_LIST_IT iterName, iterUID ;
            BOOLEAN foundCandidate = FALSE ;

            PD_LOG( PDDEBUG, "Failed to lock oldest item "
                    "[origin %s, recycle %s], try move on later "
                    "available one", candidate.getOriginName(),
                    candidate.getRecycleName() ) ;

            try
            {
               matcherName =
                     BSON( FIELD_NAME_ORIGIN_NAME << originName <<
                           FIELD_NAME_RECYCLE_ID <<
                           BSON( "$gt" <<
                                 (INT64)( candidate.getRecycleID() ) ) ) ;
               matcherUID =
                     BSON( FIELD_NAME_ORIGIN_ID << (INT64)originID <<
                           FIELD_NAME_RECYCLE_ID <<
                           BSON( "$gt" <<
                                 (INT64)( candidate.getRecycleID() ) ) ) ;

               // the recycle ID is increased with time
               orderBy = BSON( FIELD_NAME_RECYCLE_ID << 1 ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to build recycle item matcher, "
                       "occur exception %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }

            // get oldest versions by name
            rc = _getItems( matcherName, orderBy, _hintOrigName,
                            CAT_RECYCLE_MAX_RETRY, cb, candNameItemList ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get oldest items by name [%s] "
                         "after item [origin %s, recycle %s], rc: %d",
                         originName, candidate.getOriginName(),
                         candidate.getRecycleName(), rc ) ;

            // get oldest versions by unique ID
            rc = _getItems( matcherUID, orderBy, _hintOrigID,
                            CAT_RECYCLE_MAX_RETRY, cb, candUIDItemList ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get oldest items by "
                         "origin ID [%llu] after item [origin %s, recycle %s], "
                         "rc: %d", originID, candidate.getOriginName(),
                         candidate.getRecycleName(), rc ) ;

            iterName = candNameItemList.begin() ;
            iterUID = candUIDItemList.begin() ;

            // merge 2 lists to find a candidate
            while ( retryCount < CAT_RECYCLE_MAX_RETRY )
            {
               utilRecycleItem tempItem ;
               if ( iterName != candNameItemList.end() )
               {
                  tempItem = *iterName ;
               }
               if ( iterUID != candUIDItemList.end() )
               {
                  if ( ( !tempItem.isValid() ) ||
                       ( tempItem.getRecycleID() > iterUID->getRecycleID() ) )
                  {
                     tempItem = *iterUID ;
                  }
               }
               if ( !tempItem.isValid() )
               {
                  // end loop
                  break ;
               }
               rc = tryLockItem( tempItem, cb, EXCLUSIVE, lockMgr ) ;
               if ( SDB_OK == rc )
               {
                  // found candidate
                  PD_LOG( PDDEBUG, "Found later available item "
                          "[origin %s, recycle %s]",
                          tempItem.getOriginName(),
                          tempItem.getRecycleName() ) ;
                  candidate = tempItem ;
                  foundCandidate = TRUE ;
                  break ;
               }
               else if ( SDB_LOCK_FAILED == rc )
               {
                  // still failed to lock, look for the next one
                  rc = SDB_OK ;
                  if ( ( iterName != candNameItemList.end() ) &&
                       ( tempItem.getRecycleID() == iterName->getRecycleID() ) )
                  {
                     ++ iterName ;
                  }
                  if ( ( iterUID != candUIDItemList.end() ) &&
                       ( tempItem.getRecycleID() == iterUID->getRecycleID() ) )
                  {
                     ++ iterUID ;
                  }
                  ++ retryCount ;
               }
               else
               {
                  PD_RC_CHECK( rc, PDERROR, "Failed to lock temp item "
                               "[origin %s, recycle %s], rc: %d",
                               tempItem.getOriginName(),
                               tempItem.getRecycleName(), rc ) ;
               }
            }

            PD_CHECK( foundCandidate, SDB_LOCK_FAILED, error, PDERROR,
                      "Failed to lock dropping item [origin %s, recycle %s]",
                      candidate.getOriginName(),
                      candidate.getRecycleName() ) ;
         }
         else if ( SDB_OK != rc )
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to lock oldest item "
                         "[origin %s, recycle %s], rc: %d",
                         candidate.getOriginName(),
                         candidate.getRecycleName(), rc ) ;
         }
      }
      oldestItem = candidate ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__TRYFINDOLDESTITEM_VER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__GETOLDITEM_UID, "_catRecycleBinManager::_getOldestItem" )
   INT32 _catRecycleBinManager::_getOldestItem( utilCLUniqueID originID,
                                                pmdEDUCB *cb,
                                                utilRecycleItem &item )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__GETOLDITEM_UID ) ;

      BSONObj matcher ;

      SDB_ASSERT( UTIL_UNIQUEID_NULL != originID, "origin ID is invalid" ) ;

      try
      {
         matcher = BSON( FIELD_NAME_ORIGIN_ID << (INT64)originID ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build query, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         return rc ;
      }

      rc = _getOldestItem( matcher, _hintOrigID, cb, item ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the oldest recycle item, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__GETOLDITEM_UID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__GETOLDITEM_NAME, "_catRecycleBinManager::_getOldestItem" )
   INT32 _catRecycleBinManager::_getOldestItem( const CHAR *originName,
                                                pmdEDUCB *cb,
                                                utilRecycleItem &item )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__GETOLDITEM_NAME ) ;

      BSONObj matcher ;

      SDB_ASSERT( NULL != originName, "origin name is invalid" ) ;

      try
      {
         matcher = BSON( FIELD_NAME_ORIGIN_NAME << originName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build query, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         return rc ;
      }

      rc = _getOldestItem( matcher, _hintOrigName, cb, item ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the oldest recycle item, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__GETOLDITEM_NAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__GETOLDITEM, "_catRecycleBinManager::_getOldestItem" )
   INT32 _catRecycleBinManager::_getOldestItem( const BSONObj &matcher,
                                                const BSONObj &hint,
                                                pmdEDUCB *cb,
                                                utilRecycleItem &item )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__GETOLDITEM ) ;

      UTIL_RECY_ITEM_LIST itemList ;
      BSONObj orderBy ;

      try
      {
         // the recycle ID is increased with time
         orderBy = BSON( FIELD_NAME_RECYCLE_ID << 1 ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build query, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         return rc ;
      }

      rc = _getItems( matcher, orderBy, hint, 1, cb, itemList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the oldest recycle item, "
                   "rc: %d", rc ) ;

      if ( !itemList.empty() )
      {
         item = itemList.front() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__GETOLDITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__SAVEOBJ, "_catRecycleBinManager::_saveObject" )
   INT32 _catRecycleBinManager::_saveObject( const CHAR *collection,
                                             const BSONObj &object,
                                             BOOLEAN canReplace,
                                             pmdEDUCB *cb,
                                             INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__SAVEOBJ ) ;

      rc = rtnInsert( collection, object, 1,
                      canReplace ? FLG_INSERT_REPLACEONDUP : 0, cb, _dmsCB,
                      _dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert object to collection [%s], "
                   "rc: %d", collection, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__SAVEOBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR_PROCESSOBJS, "_catRecycleBinManager::processObjects" )
   INT32 _catRecycleBinManager::processObjects( catRecycleBinProcessor &processor,
                                                pmdEDUCB *cb,
                                                INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR_PROCESSOBJS ) ;

      ossPoolList< BSONObj > matcherList ;

      rc = processor.getMatcher( matcherList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get matcher from processor [%s], "
                   "rc: %d", processor.getName(), rc ) ;

      for ( ossPoolList< BSONObj >::iterator iter = matcherList.begin() ;
            iter != matcherList.end() ;
            ++ iter )
      {
         const BSONObj &matcher = (*iter) ;
         rc = _processObjects( processor, matcher, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to process objects with "
                      "processor [%s], rc: %d", processor.getName(), rc ) ;
      }

      if ( processor.getExpectedCount() > 0 )
      {
         PD_CHECK( processor.getMatchedCount() ==
                                     (UINT32)( processor.getExpectedCount() ),
                   SDB_CAT_CORRUPTION, error, PDERROR,
                   "Failed to check processor [%s], should have [%d] "
                   "matched objects, only found [%u]",
                   processor.getName(), processor.getExpectedCount(),
                   processor.getMatchedCount() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR_PROCESSOBJS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__PROCESSOBJS, "_catRecycleBinManager::_processObjects" )
   INT32 _catRecycleBinManager::_processObjects( catRecycleBinProcessor &processor,
                                                 const BSONObj &matcher,
                                                 pmdEDUCB *cb,
                                                 INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__PROCESSOBJS ) ;

      rtnQueryOptions options ;
      const CHAR *collection = NULL ;
      INT64 contextID = -1 ;

      collection = processor.getCollection() ;
      SDB_ASSERT( NULL != collection, "collection is invalid" ) ;

      options.setCLFullName( collection ) ;
      options.setQuery( matcher ) ;

      rc = rtnQuery( options, cb, _dmsCB, _rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to query collection [%s], rc: %d",
                   collection, rc ) ;

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         BSONObj object ;

         rc = rtnGetMore( contextID, 1, buffObj, cb, _rtnCB ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               contextID = -1 ;
               goto done ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to get objects from collection [%s], "
                         "rc: %d", collection, rc ) ;
         }

         try
         {
            object = BSONObj( buffObj.data() ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to parse origin object, "
                    "occur exception %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

         processor.increaseMatchedCount() ;

         rc = processor.processObject( object, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to process object, rc: %d", rc ) ;

         processor.increaseProcessedCount() ;
      }

   done:
      if ( -1 != contextID )
      {
         _rtnCB->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__PROCESSOBJS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATRECYBINMGR__CHECKDELAYLOCKFAILED, "_catRecycleBinManager::_checkDelayLockFailed" )
   INT32 _catRecycleBinManager::_checkDelayLockFailed( const utilRecycleBinConf &conf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATRECYBINMGR__CHECKDELAYLOCKFAILED ) ;

      // retry exceeded times, only pass the oldest item
      if ( !sdbGetCatalogueCB()->getMainController()->canDelayed() )
      {
         SDB_ASSERT( conf.getAutoDrop(), "conf can be AutoDrop" ) ;
         enableLimitCheck() ;
      }
      else
      {
         rc = SDB_LOCK_FAILED ;
      }

      PD_TRACE_EXITRC( SDB__CATRECYBINMGR__CHECKDELAYLOCKFAILED, rc ) ;
      return rc ;
   }

}
