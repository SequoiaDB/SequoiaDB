/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = catCommandRecycleBin.hpp

   Descriptive Name = Catalogue commands for recycle bin

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for catalog
   commands.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/01/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "catCommandRecycleBin.hpp"
#include "catCommon.hpp"
#include "catContextRecycleBin.hpp"
#include "rtnCB.hpp"
#include "catTrace.hpp"
#include "pdTrace.hpp"

namespace engine
{

   /*
      _catCMDGetRecycleBinDetail implement
    */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDGetRecycleBinDetail )

   _catCMDGetRecycleBinDetail::_catCMDGetRecycleBinDetail()
   : _recycleBinMgr( sdbGetCatalogueCB()->getRecycleBinMgr() )
   {
   }

   _catCMDGetRecycleBinDetail::~_catCMDGetRecycleBinDetail()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDGETRECYCLEBINDETAIL_DOIT, "_catCMDGetRecycleBinDetail::doit" )
   INT32 _catCMDGetRecycleBinDetail::doit( _pmdEDUCB *cb,
                                           rtnContextBuf &ctxBuf,
                                           INT64 &contextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDGETRECYCLEBINDETAIL_DOIT ) ;

      BSONObj resultObj ;
      const utilRecycleBinConf &info = _recycleBinMgr->getConf() ;

      rc = info.toBSON( resultObj, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for recycle bin detail, "
                   "rc: %d", rc ) ;

      ctxBuf = rtnContextBuf( resultObj ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATCMDGETRECYCLEBINDETAIL_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catCMDAlterRecycleBin implement
    */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDAlterRecycleBin )

   _catCMDAlterRecycleBin::_catCMDAlterRecycleBin()
   : _recycleBinMgr( sdbGetCatalogueCB()->getRecycleBinMgr() )
   {
   }

   _catCMDAlterRecycleBin::~_catCMDAlterRecycleBin()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDALTERRECYCLEBIN_INIT, "_catCMDAlterRecycleBin::init" )
   INT32 _catCMDAlterRecycleBin::init( const CHAR *pQuery,
                                       const CHAR *pSelector,
                                       const CHAR *pOrderBy,
                                       const CHAR *pHint,
                                       INT32 flags,
                                       INT64 numToSkip,
                                       INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDALTERRECYCLEBIN_INIT ) ;

      // copy from old conf
      _newConf = _recycleBinMgr->getConf() ;

      try
      {
         BSONObj infoObj( pQuery ) ;
         const CHAR *actionName = NULL ;

         BSONElement element = infoObj.getField( FIELD_NAME_ACTION ) ;
         PD_CHECK( String == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s] from alter options [%s]",
                   FIELD_NAME_ACTION, infoObj.toPoolString().c_str() ) ;
         actionName = element.valuestr() ;

         if ( 0 == ossStrcmp( CMD_VALUE_NAME_RECYCLEBIN_ENABLE,
                              actionName ) )
         {
            _newConf.setEnable( TRUE ) ;
         }
         else if ( 0 == ossStrcmp( CMD_VALUE_NAME_RECYCLEBIN_DISABLE,
                                   actionName ) )
         {
            _newConf.setEnable( FALSE ) ;
         }
         else if ( 0 == ossStrcmp( CMD_VALUE_NAME_RECYCLEBIN_SETATTR,
                                   actionName ) )
         {
            BSONObj options ;

            element = infoObj.getField( FIELD_NAME_OPTIONS ) ;
            PD_CHECK( Object == element.type(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to get field [%s] from alter options [%s]",
                      FIELD_NAME_ACTION, infoObj.toPoolString().c_str() ) ;
            options = element.embeddedObject() ;

            rc = _newConf.updateOptions( options ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to update options, rc: %d", rc ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to alter recycle bin by "
                    "unknown action [%s]", actionName ) ;
         }
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDALTERRECYCLEBIN_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDALTERRECYCLEBIN_DOIT, "_catCMDAlterRecycleBin::doit" )
   INT32 _catCMDAlterRecycleBin::doit( _pmdEDUCB *cb,
                                       rtnContextBuf &ctxBuf,
                                       INT64 &contextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDALTERRECYCLEBIN_DOIT ) ;

      sdbCatalogueCB *catCB = sdbGetCatalogueCB() ;
      INT16 w = catCB->majoritySize() ;

      rc = _recycleBinMgr->updateConf( _newConf, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update recycle bin conf, "
                   "rc: %d", rc ) ;

      // update catalog cache
      sdbGetCatalogueCB()->getCatDCMgr()->updateDCCache() ;

   done:
      PD_TRACE_EXITRC( SDB__CATCMDALTERRECYCLEBIN_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catCMDGetRecycleBinCount define
    */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDGetRecycleBinCount )

   _catCMDGetRecycleBinCount::_catCMDGetRecycleBinCount()
   {
   }

   _catCMDGetRecycleBinCount::~_catCMDGetRecycleBinCount()
   {
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDGETRECYBINCNT_INIT, "_catCMDGetRecycleBinCount::init" )
   INT32 _catCMDGetRecycleBinCount::init( const CHAR *pQuery,
                                          const CHAR *pSelector,
                                          const CHAR *pOrderBy,
                                          const CHAR *pHint,
                                          INT32 flags,
                                          INT64 numToSkip,
                                          INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDGETRECYBINCNT_INIT ) ;

      try
      {
         _queryObj = BSONObj( pQuery ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get query object, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDGETRECYBINCNT_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDGETRECYBINCNT_DOIT, "_catCMDGetRecycleBinCount::doit" )
   INT32 _catCMDGetRecycleBinCount::doit( _pmdEDUCB *cb,
                                          rtnContextBuf &ctxBuf,
                                          INT64 &contextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDGETRECYBINCNT_DOIT ) ;

      INT64 recycleCount = 0 ;

      rc = sdbGetCatalogueCB()->getRecycleBinMgr()->countItems(
                                             _queryObj, cb, recycleCount ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item count, rc: %d",
                   rc ) ;

      try
      {
         BSONObj resultObj = BSON( FIELD_NAME_TOTAL << recycleCount ) ;
         ctxBuf = rtnContextBuf( resultObj ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build result, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDGETRECYBINCNT_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catCMDDropRecycleBinBase implement
    */
   _catCMDDropRecycleBinBase::_catCMDDropRecycleBinBase()
   : _recycleBinMgr( sdbGetCatalogueCB()->getRecycleBinMgr() ),
     _recycleItemName( NULL ),
     _isEnforced( FALSE ),
     _isRecursive( FALSE ),
     _isIgnoreLock( FALSE )
   {
   }

   _catCMDDropRecycleBinBase::~_catCMDDropRecycleBinBase()
   {
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDDROPRECYCLEBIN_INIT, "_catCMDDropRecycleBinBase::init" )
   INT32 _catCMDDropRecycleBinBase::init( const CHAR *pQuery,
                                          const CHAR *pSelector,
                                          const CHAR *pOrderBy,
                                          const CHAR *pHint,
                                          INT32 flags,
                                          INT64 numToSkip,
                                          INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDDROPRECYCLEBIN_INIT ) ;

      try
      {
         _queryObj = BSONObj( pQuery ) ;
         _hintObj = BSONObj( pHint ) ;

         rc = rtnGetStringElement( _queryObj,
                                   FIELD_NAME_RECYCLE_NAME,
                                   &_recycleItemName ) ;
         if ( SDB_OK == rc )
         {
            utilRecycleItem dummyItem ;

            PD_CHECK( !_isDropAll(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse options, should not get "
                      "field [%s] in drop all command",
                      FIELD_NAME_RECYCLE_NAME ) ;

            rc = dummyItem.fromRecycleName( _recycleItemName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse recycle item "
                         "name [%s], rc: %d", _recycleItemName, rc ) ;
         }
         else if ( SDB_FIELD_NOT_EXIST == rc && _isDropAll() )
         {
            rc = SDB_OK ;
         }
         else
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                         FIELD_NAME_RECYCLE_NAME, rc ) ;
         }

         rc = rtnGetBooleanElement( _queryObj,
                                    FIELD_NAME_ENFORCED1,
                                    _isEnforced ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = rtnGetBooleanElement( _queryObj,
                                       FIELD_NAME_ENFORCED,
                                       _isEnforced ) ;
            if ( SDB_FIELD_NOT_EXIST == rc )
            {
               rc = SDB_OK ;
               _isEnforced = FALSE ;
            }
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      FIELD_NAME_ENFORCED1, rc ) ;

         if ( _isDropAll() )
         {
            // drop all items must enable recursive
            _isRecursive = TRUE ;
         }
         else
         {
            rc = rtnGetBooleanElement( _queryObj,
                                       FIELD_NAME_RECURSIVE,
                                       _isRecursive ) ;
            if ( SDB_FIELD_NOT_EXIST == rc )
            {
               _isRecursive = FALSE ;
               rc = SDB_OK ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                         FIELD_NAME_RECURSIVE, rc ) ;
         }

         // check ignore lock
         rc = rtnGetBooleanElement( _hintObj,
                                    FIELD_NAME_IGNORE_LOCK,
                                    _isIgnoreLock ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            // default is false
            _isIgnoreLock = FALSE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      FIELD_NAME_IGNORE_LOCK, rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get query object, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDDROPRECYCLEBIN_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDDROPRECYCLEBIN_DOIT, "_catCMDDropRecycleBinBase::doit" )
   INT32 _catCMDDropRecycleBinBase::doit( _pmdEDUCB *cb,
                                          rtnContextBuf &ctxBuf,
                                          INT64 &contextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDDROPRECYCLEBIN_DOIT ) ;

      BOOLEAN lockedRecycleBin = FALSE ;
      INT16 w = sdbGetCatalogueCB()->majoritySize() ;

      PD_CHECK( _recycleBinMgr->tryLockDropLatch(), SDB_LOCK_FAILED, error,
                PDERROR, "Failed to lock recycle bin to block drop expired "
                "background job" ) ;
      lockedRecycleBin = TRUE ;

      rc = _check( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check drop recycle bin items, "
                   "rc: %d", rc ) ;

      rc = _execute( cb, w, ctxBuf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute drop recycle bin items, "
                   "rc: %d", rc ) ;

   done:
      if ( lockedRecycleBin )
      {
         _recycleBinMgr->unlockDropLatch() ;
         lockedRecycleBin = FALSE ;
      }
      contextID = -1 ;
      PD_TRACE_EXITRC( SDB__CATCMDDROPRECYCLEBIN_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catCMDDropRecycleBinItem implement
    */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDDropRecycleBinItem )

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDDROPRECYCLEBINITEM__CHECK, "_catCMDDropRecycleBinItem::_check" )
   INT32 _catCMDDropRecycleBinItem::_check( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDDROPRECYCLEBINITEM__CHECK ) ;

      // NOTE: drop is long time job, only use short lock here
      catCtxLockMgr localLockMgr ;
      localLockMgr.setIgnoreLock( _isIgnoreLock ) ;

      rc = _recycleBinMgr->getItem( _recycleItemName, cb, _recycleItem ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item [%s], rc: %d",
                   _recycleItemName, rc ) ;

      if ( UTIL_RECYCLE_CS == _recycleItem.getType() )
      {
         utilCSUniqueID csUniqueID =
               (utilCSUniqueID)( _recycleItem.getOriginID() ) ;

         if ( !_isRecursive )
         {
            // if there are recursive recycled collections inside,
            // we can not drop it if not recursive
            rc = _checkRecycledCLInCS( _recycleItem, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check collection space "
                         "recycle item [origin %s, recycle %s], rc: %d",
                         _recycleItem.getOriginName(),
                         _recycleItem.getRecycleName(), rc ) ;
         }

         rc = catGetGroupsForRecycleCS( csUniqueID, cb, _groupIDSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get group list for "
                      "recycle collection space item [%s], rc: %d",
                      _recycleItemName, rc ) ;
      }
      else if ( UTIL_RECYCLE_CL == _recycleItem.getType() )
      {
         if ( _recycleItem.isCSRecycled() )
         {
            // the recycle item is in a recycled collection space
            // it should not be dropped if not enforced
            if ( _isEnforced )
            {
               // if it is enforced, force to dropped it
               PD_LOG( PDDEBUG, "Enforce to drop recycle item "
                       "[origin %s, recycle %s]",
                       _recycleItem.getOriginName(),
                       _recycleItem.getRecycleName() ) ;
            }
            else
            {
               // double check if a dropping recycle item of collection is
               // still in a recycled collection space
               // if so, we can not drop it, otherwise, we can drop it
               rc = _checkCLInRecycledCS( _recycleItem, cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to check collection "
                            "recycle item [origin %s, recycle %s], rc: %d",
                            _recycleItem.getOriginName(),
                            _recycleItem.getRecycleName(), rc ) ;
            }
         }

         if ( _recycleItem.isMainCL() && !_isEnforced )
         {
            // check if sub-collections in a dropping recycle item of
            // main-collection is in a recycled collection space
            // if so, we can not drop it
            rc = _checkSubCLInRecycledCS( _recycleItem, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check sub-collections in "
                         "recycle item [origin %s, recycle %s], rc: %d",
                         _recycleItem.getOriginName(),
                         _recycleItem.getRecycleName(), rc ) ;
         }

         rc = catGetGroupsForRecycleItem( _recycleItem.getRecycleID(), cb,
                                          _groupIDSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to group list for recycle "
                      "collection item [%s], rc: %d", _recycleItemName, rc ) ;
      }
      else
      {
         PD_LOG( PDWARNING, "Unknown recycle type [%s]",
                 utilGetRecycleTypeName( _recycleItem.getType() ) ) ;
         SDB_ASSERT( FALSE, "invalid recycle type" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _recycleBinMgr->tryLockItem( _recycleItem, cb, EXCLUSIVE, localLockMgr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock recycle item "
                   "[origin %s, recycle %s], rc: %d",
                   _recycleItem.getOriginName(),
                   _recycleItem.getRecycleName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATCMDDROPRECYCLEBINITEM__CHECK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDDROPRECYCLEBINITEM__CHKRECYCLCS, "_catCMDDropRecycleBinItem::_checkRecycledCLInCS" )
   INT32 _catCMDDropRecycleBinItem::_checkRecycledCLInCS( utilRecycleItem &item,
                                                          _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDDROPRECYCLEBINITEM__CHKRECYCLCS ) ;

      SDB_ASSERT( UTIL_RECYCLE_CS == item.getType(),
                  "should be recycle collection space" ) ;

      utilCSUniqueID csUniqueID = (utilCSUniqueID)( item.getOriginID() ) ;
      INT64 count = 0 ;

      rc = _recycleBinMgr->countItemsInCS( csUniqueID, cb, count ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get count of recycle items in "
                   "collection space recycle item [origin: %s, recycle: %s], "
                   "rc: %d", item.getOriginName(), item.getRecycleName(),
                   rc ) ;

      PD_LOG_MSG_CHECK( 0 == count, SDB_RECYCLE_CONFLICT, error, PDERROR,
                        "Failed to drop collection space recycle item "
                        "[origin %s, recycle %s], there are recursive "
                        "collection recycle items inside",
                        item.getOriginName(), item.getRecycleName() ) ;
      
      {
         catDropCSItemChecker checker( _recycleBinMgr, item ) ;

         rc = _recycleBinMgr->processObjects( checker, cb, 1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check collections for drop "
                      "recycle item [origin %s, recycle %s], rc: %d",
                      item.getOriginName(), item.getRecycleName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDDROPRECYCLEBINITEM__CHKRECYCLCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDDROPRECYCLEBINITEM__CHKRECYCS, "_catCMDDropRecycleBinItem::_checkCLInRecycledCS" )
   INT32 _catCMDDropRecycleBinItem::_checkCLInRecycledCS( utilRecycleItem &item,
                                                          _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDDROPRECYCLEBINITEM__CHKRECYCS ) ;

      SDB_ASSERT( UTIL_RECYCLE_CL == item.getType(),
                  "should be recycle collection" ) ;

      utilCLUniqueID clUniqueID = (utilCLUniqueID)( item.getOriginID() ) ;
      utilCSUniqueID csUniqueID = utilGetCSUniqueID( clUniqueID ) ;
      utilRecycleItem csItem ;

      rc = _recycleBinMgr->getItemByOrigID( csUniqueID, cb, csItem ) ;
      if ( SDB_RECYCLE_ITEM_NOTEXIST == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item of collection "
                   "space, rc: %d", rc ) ;

      PD_LOG_MSG_CHECK( FALSE, SDB_RECYCLE_CONFLICT, error, PDERROR,
                        "Failed to drop collection recycle item "
                        "[origin %s, recycle %s], collection space recycle "
                        "item [origin: %s, recycle: %s] is also "
                        "in recycle bin", item.getOriginName(),
                        item.getRecycleName(), csItem.getOriginName(),
                        csItem.getRecycleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATCMDDROPRECYCLEBINITEM__CHKRECYCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDDROPRECYCLEBINITEM__CHKSUBCLRECYCS, "_catCMDDropRecycleBinItem::_checkSubCLInRecycledCS" )
   INT32 _catCMDDropRecycleBinItem::_checkSubCLInRecycledCS( utilRecycleItem &item,
                                                             _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDDROPRECYCLEBINITEM__CHKSUBCLRECYCS ) ;

      SDB_ASSERT( UTIL_RECYCLE_CL == item.getType() && item.isMainCL(),
                  "should be recycle item for main collection" ) ;

      catDropItemSubCLChecker checker( _recycleBinMgr, item ) ;

      rc = _recycleBinMgr->processObjects( checker, cb, 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check sub collections for drop "
                   "recycle item [origin %s, recycle %s], rc: %d",
                   item.getOriginName(), item.getRecycleName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATCMDDROPRECYCLEBINITEM__CHKSUBCLRECYCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDDROPRECYCLEBINITEM__EXECUTE, "_catCMDDropRecycleBinItem::_execute" )
   INT32 _catCMDDropRecycleBinItem::_execute( _pmdEDUCB *cb,
                                              INT16 w,
                                              rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDDROPRECYCLEBINITEM__EXECUTE ) ;

      BSONObjBuilder retObjBuilder ;

      rc = _recycleBinMgr->dropItem( _recycleItem, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item [%s], "
                   "rc: %d", _recycleItemName, rc ) ;

      // ignore error in case that group had been removed
      rc = sdbGetCatalogueCB()->makeGroupsObj( retObjBuilder,
                                               _groupIDSet,
                                               TRUE, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to make group object, rc: %d", rc ) ;

      try
      {
         ctxBuf = rtnContextBuf( retObjBuilder.obj() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build return object, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDDROPRECYCLEBINITEM__EXECUTE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catCMDDropRecycleBinAll implement
    */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDDropRecycleBinAll )

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDDROPRECYCLEBINALL__CHECK, "_catCMDDropRecycleBinAll::_check" )
   INT32 _catCMDDropRecycleBinAll::_check( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDDROPRECYCLEBINALL__CHECK ) ;

      // there is no global lock for whole recycle bin, but the only
      // excluded operation against drop all items in recycle bin is returning
      // items from recycle bin, so we check number of returning items here
      PD_CHECK( !( _recycleBinMgr->hasReturningItems() ),
                SDB_LOCK_FAILED, error, PDERROR,
                "Failed to lock recycle bin, has returning items" ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATCMDDROPRECYCLEBINALL__CHECK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDDROPRECYCLEBINALL__EXECUTE, "_catCMDDropRecycleBinAll::_execute" )
   INT32 _catCMDDropRecycleBinAll::_execute( _pmdEDUCB *cb,
                                             INT16 w,
                                             rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDDROPRECYCLEBINALL__EXECUTE ) ;

      rc = _recycleBinMgr->dropAllItems( cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop all recycle items, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATCMDDROPRECYCLEBINALL__EXECUTE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catCMDReturnRecycleBinBase implement
    */
   _catCMDReturnRecycleBinBase::_catCMDReturnRecycleBinBase()
   : _recycleItemName( NULL )
   {
   }

   _catCMDReturnRecycleBinBase::~_catCMDReturnRecycleBinBase()
   {
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDRETURNRECYCLEBINBASE_INIT, "_catCMDReturnRecycleBinBase::init" )
   INT32 _catCMDReturnRecycleBinBase::init( const CHAR *pQuery,
                                            const CHAR *pSelector,
                                            const CHAR *pOrderBy,
                                            const CHAR *pHint,
                                            INT32 flags,
                                            INT64 numToSkip,
                                            INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDRETURNRECYCLEBINBASE_INIT ) ;

      try
      {
         _queryObj = BSONObj( pQuery ) ;

         BSONElement ele = _queryObj.getField( FIELD_NAME_RECYCLE_NAME ) ;
         PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s], it is not a string",
                   FIELD_NAME_RECYCLE_NAME ) ;
         _recycleItemName = ele.valuestr() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get query object, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDRETURNRECYCLEBINBASE_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDRETURNRECYCLEBINBASE_DOIT, "_catCMDReturnRecycleBinBase::doit" )
   INT32 _catCMDReturnRecycleBinBase::doit( _pmdEDUCB *cb,
                                            rtnContextBuf &ctxBuf,
                                            INT64 &contextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATCMDRETURNRECYCLEBINBASE_DOIT ) ;

      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      catCtxReturnRecycleBin::sharePtr context ;

      rc = rtnCB->contextNew( RTN_CONTEXT_CAT_RETURN_RECYCLEBIN,
                              context,
                              contextID,
                              cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create return recycle context, "
                   "rc: %d", rc ) ;

      rc = context->open( _queryObj, _isReturnToName(), ctxBuf, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open return recycle context, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATCMDRETURNRECYCLEBINBASE_DOIT, rc ) ;
      return rc ;

   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   /*
      _catCMDReturnRecycleBinItem
    */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDReturnRecycleBinItem )

   /*
      _catCMDReturnRecycleBinItemToName
    */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDReturnRecycleBinItemToName )

}
