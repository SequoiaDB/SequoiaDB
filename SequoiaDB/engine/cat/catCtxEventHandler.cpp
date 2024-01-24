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

   Source File Name = catContextData.cpp

   Descriptive Name = Runtime Context of Catalog

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime Context of Catalog
   helper functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "catCtxEventHandler.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "rtn.hpp"
#include "catCommon.hpp"
#include "catRecycleBinManager.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _catCtxGroupHandler implement
    */
   _catCtxGroupHandler::_catCtxGroupHandler( catCtxLockMgr &lockMgr )
   : _catCtxEventHandler( lockMgr ),
     _ignoreNonExist( FALSE )
   {
   }

   _catCtxGroupHandler::~_catCtxGroupHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXGRPHANDLER_ADDGRP, "_catCtxGroupHandler::addGroup" )
   INT32 _catCtxGroupHandler::addGroup( UINT32 groupID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXGRPHANDLER_ADDGRP ) ;

      try
      {
         _groupIDSet.insert( groupID ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add group ID, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXGRPHANDLER_ADDGRP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXGRPHANDLER_ADDGRPS_CL, "_catCtxGroupHandler::addGroups" )
   INT32 _catCtxGroupHandler::addGroups( const BSONObj &boCollection )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXGRPHANDLER_ADDGRPS_CL ) ;

      rc = catGetCollectionGroupSet( boCollection, _groupIDSet ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to collect groups for collection [%s], "
                   "rc: %d", boCollection.toPoolString().c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXGRPHANDLER_ADDGRPS_CL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXGRPHANDLER_ADDGRPS_SET, "_catCtxGroupHandler::addGroups" )
   INT32 _catCtxGroupHandler::addGroups( const CAT_GROUP_SET &groupIDSet )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXGRPHANDLER_ADDGRPS_SET ) ;

      try
      {
         _groupIDSet.insert( groupIDSet.begin(), groupIDSet.end() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add groups, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXGRPHANDLER_ADDGRPS_SET, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXGRPHANDLER_ADDGRPS_LIST, "_catCtxGroupHandler::addGroups" )
   INT32 _catCtxGroupHandler::addGroups( const CAT_GROUP_LIST &groupIDList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXGRPHANDLER_ADDGRPS_LIST ) ;

      try
      {
         _groupIDSet.insert( groupIDList.begin(), groupIDList.end() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add groups, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXGRPHANDLER_ADDGRPS_LIST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXGRPHANDLER_ADDGRPS_VEC, "_catCtxGroupHandler::addGroups" )
   INT32 _catCtxGroupHandler::addGroups( const VEC_GROUP_ID &groupIDVec )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXGRPHANDLER_ADDGRPS_VEC ) ;

      try
      {
         _groupIDSet.insert( groupIDVec.begin(), groupIDVec.end() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add groups, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXGRPHANDLER_ADDGRPS_VEC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXGRPHANDLER_ADDGRPS_RECYCLEBIN, "_catCtxGroupHandler::addGroupsInRecycleBin" )
   INT32 _catCtxGroupHandler::addGroupsInRecycleBin( utilCSUniqueID csUniqueID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXGRPHANDLER_ADDGRPS_RECYCLEBIN ) ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      rc = catGetGroupsForRecycleCS( csUniqueID, cb, _groupIDSet ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get group list for recycle "
                   "collections from collection space [%llu], rc: %d",
                   csUniqueID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXGRPHANDLER_ADDGRPS_RECYCLEBIN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXGRPHANDLER_ONCHECKEVENT, "_catCtxGroupHandler::onCheckEvent" )
   INT32 _catCtxGroupHandler::onCheckEvent( SDB_EVENT_OCCUR_TYPE type,
                                            const CHAR *targetName,
                                            const bson::BSONObj &boTarget,
                                            _pmdEDUCB *cb,
                                            INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXGRPHANDLER_ONCHECKEVENT ) ;

      if ( SDB_EVT_OCCUR_AFTER == type )
      {
         // Lock groups in shared
         rc = catLockGroups( _groupIDSet, cb, _lockMgr, SHARED,
                             _ignoreNonExist ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to lock groups, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXGRPHANDLER_ONCHECKEVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXGRPHANDLER_BUILDP1REPLY, "_catCtxGroupHandler::buildP1Reply" )
   INT32 _catCtxGroupHandler::buildP1Reply( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXGRPHANDLER_BUILDP1REPLY ) ;

      // return group list to COORD, so COORD can send command to
      // specified groups
      rc = sdbGetCatalogueCB()->makeGroupsObj( builder, _groupIDSet, TRUE,
                                               _ignoreNonExist ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to make group object, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXGRPHANDLER_BUILDP1REPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catCtxGlobIdxHandler implement
    */
   _catCtxGlobIdxHandler::_catCtxGlobIdxHandler( catCtxLockMgr &lockMgr )
   : _catCtxEventHandler( lockMgr ),
     _excludedCSUniqueID( UTIL_UNIQUEID_NULL )
   {
   }

   _catCtxGlobIdxHandler::~_catCtxGlobIdxHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXGLOBIDXHANDLER_ADDGLOBIDXS, "_catCtxGlobIdxHandler::addGlobIdxs" )
   INT32 _catCtxGlobIdxHandler::addGlobIdxs( const CAT_PAIR_CLNAME_ID_LIST &globIdxList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXGLOBIDXHANDLER_ADDGLOBIDXS ) ;

      try
      {
         _globIdxList.insert( _globIdxList.end(),
                                globIdxList.begin(),
                                globIdxList.end() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add global index collections, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXGLOBIDXHANDLER_ADDGLOBIDXS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXGLOBIDXHANDLER_ADDGLOBIDXS_CL, "_catCtxGlobIdxHandler::addGlobIdxs" )
   INT32 _catCtxGlobIdxHandler::addGlobIdxs( const CHAR *collectionName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXGLOBIDXHANDLER_ADDGLOBIDXS_CL ) ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      // get collection's global index
      rc = catGetCLGlobalIndexesInfo( collectionName, cb, _globIdxList ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get collection[%s]'s "
                   "global indexes, rc: %d", collectionName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXGLOBIDXHANDLER_ADDGLOBIDXS_CL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXGLOBIDXHANDLER_BUILDP1REPLY, "_catCtxGlobIdxHandler::buildP1Reply" )
   INT32 _catCtxGlobIdxHandler::buildP1Reply( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXGLOBIDXHANDLER_BUILDP1REPLY ) ;

      if ( _globIdxList.empty() )
      {
         goto done ;
      }

      try
      {
         // build global index list
         BSONArrayBuilder indexArrBuilder(
                     builder.subarrayStart( CAT_GLOBAL_INDEX ) ) ;

         for ( CAT_PAIR_CLNAME_ID_LIST_IT it = _globIdxList.begin() ;
               it != _globIdxList.end() ;
               ++ it )
         {
            // if collection space is excluded, no need to return
            if ( ( UTIL_UNIQUEID_NULL == _excludedCSUniqueID ) ||
                 ( utilGetCSUniqueID( it->second ) != _excludedCSUniqueID ) )
            {
               indexArrBuilder.append(
                     BSON( CAT_COLLECTION << it->first <<
                           CAT_GIDX_CL_UNIQUEID << (INT64)( it->second ) ) ) ;
            }
         }

         indexArrBuilder.done() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build reply with global indexes, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXGLOBIDXHANDLER_BUILDP1REPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catCtxTaskHandler implement
    */
   _catCtxTaskHandler::_catCtxTaskHandler( catCtxLockMgr & lockMgr )
   : _catCtxEventHandler( lockMgr )
   {
   }

   _catCtxTaskHandler::~_catCtxTaskHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXTASKHANDLER__BUILDTASKREPLY, "_catCtxTaskHandler::_buildTaskReply" )
   INT32 _catCtxTaskHandler::_buildTaskReply( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXTASKHANDLER__BUILDTASKREPLY ) ;

      if ( !_taskSet.empty() )
      {
         try
         {
            BSONArrayBuilder taskBuilder(
                                    builder.subarrayStart( CAT_TASKID_NAME ) ) ;
            for ( ossPoolSet< UINT64 >::iterator iter = _taskSet.begin() ;
                  iter != _taskSet.end() ;
                  ++ iter )
            {
               taskBuilder.append( (INT64)( *iter ) ) ;
            }
            taskBuilder.doneFast() ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build reply for tasks, "
                    "occur exception %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXTASKHANDLER__BUILDTASKREPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXTASKHANDLER__CANCELTASKS, "_catCtxTaskHandler::_cancelTasks" )
   INT32 _catCtxTaskHandler::_cancelTasks( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXTASKHANDLER__CANCELTASKS ) ;

      for ( ossPoolSet< UINT64 >::iterator iter = _taskSet.begin() ;
            iter != _taskSet.end() ;
            ++ iter )
      {
         INT32 tmpRC = SDB_OK ;
         BSONObj query ;
         UINT32 returnGroupID = 0 ;

         UINT64 taskID = *iter ;

         try
         {
            query = BSON( CAT_TASKID_NAME << (INT64)taskID ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build cancel query, "
                    "occur exception: %s", e.what() ) ;
            continue ;
         }

         tmpRC = catTaskCancel( query, cb, w, returnGroupID ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to cancel task [%llu], rc: %d",
                    taskID, tmpRC ) ;
         }
      }

      PD_TRACE_EXITRC( SDB_CATCTXTASKHANDLER__CANCELTASKS, rc ) ;

      return rc ;
   }

   /*
      _catRecyCtxTaskHandler implement
    */
   _catRecyCtxTaskHandler::_catRecyCtxTaskHandler( catCtxLockMgr & lockMgr )
   : _catCtxTaskHandler( lockMgr )
   {
   }

   _catRecyCtxTaskHandler::~_catRecyCtxTaskHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYCTXTASKHANDLER_ONCHECKEVENT, "_catRecyCtxTaskHandler::onCheckEvent" )
   INT32 _catRecyCtxTaskHandler::onCheckEvent( SDB_EVENT_OCCUR_TYPE type,
                                               const CHAR *targetName,
                                               const BSONObj &boTarget,
                                               _pmdEDUCB *cb,
                                               INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYCTXTASKHANDLER_ONCHECKEVENT ) ;

      rc = _checkTasks( cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check tasks, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATRECYCTXTASKHANDLER_ONCHECKEVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYCTXTASKHANDLER__CHECKTASKS, "_catRecyCtxTaskHandler::_checkTasks" )
   INT32 _catRecyCtxTaskHandler::_checkTasks( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYCTXTASKHANDLER__CHECKTASKS ) ;

      if ( !_recycleItem.isValid() )
      {
         goto done ;
      }

      if ( UTIL_RECYCLE_CS == _recycleItem.getType() )
      {
         const CHAR *csName = _recycleItem.getOriginName() ;
         rc = catGetCSTasks( csName, cb, _taskSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get tasks on collection "
                      "space [%s], rc: %d", csName, rc ) ;
      }
      else if ( UTIL_RECYCLE_CL == _recycleItem.getType() )
      {
         const CHAR *clName = _recycleItem.getOriginName() ;
         if ( _recycleItem.isMainCL() )
         {
            clsCatalogSet catSet( clName ) ;
            BSONObj boCollection ;
            CLS_SUBCL_LIST subCLList ;

            rc = catGetCollection( clName, boCollection, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get collection [%s], rc: %d",
                         rc ) ;

            rc = catSet.updateCatSet( boCollection ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse catalog for "
                         "collection [%s], rc: %d", clName, rc ) ;

            rc = catSet.getSubCLList( subCLList ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get sub-collection list from "
                         "main collection [%s], rc: %d", clName, rc ) ;

            for ( CLS_SUBCL_LIST_IT iter = subCLList.begin() ;
                  iter != subCLList.end() ;
                  ++ iter )
            {
               const CHAR *subCLName = iter->c_str() ;

               rc = catGetCLTasks( subCLName, cb, _taskSet ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get split tasks on collection "
                            "space [%s], rc: %d", subCLName, rc ) ;
            }
         }
         else
         {
            rc = catGetCLTasks( clName, cb, _taskSet ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get split tasks on collection "
                         "space [%s], rc: %d", clName, rc ) ;
         }
      }

      // for both drop and truncate, we need to cancel tasks
      // - for split task
      // once collection or collection space recycled, the clean job of
      // split tasks will not be able to find the origin collection
      // - for index task
      // if the priamry nodes not finish task, while the secondary node finished,
      // which will cause the index not consistent
      // NOTE: the reason of truncate need cancel split tasks is because
      // once the split hangs by error (e.g. array sharding keys), the truncate
      // hangs also
      rc = _cancelTasks( cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to cancel tasks before "
                   "recycle item [origin: %s, recycle: %s], rc: %d",
                   _recycleItem.getOriginName(),
                   _recycleItem.getRecycleName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATRECYCTXTASKHANDLER__CHECKTASKS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catCtxRecycleHelper implement
    */
   _catCtxRecycleHelper::_catCtxRecycleHelper( UTIL_RECYCLE_TYPE type,
                                               UTIL_RECYCLE_OPTYPE opType )
   : _recycleBinMgr( sdbGetCatalogueCB()->getRecycleBinMgr() ),
     _recycleItem( type, opType )
   {
   }

   _catCtxRecycleHelper::~_catCtxRecycleHelper()
   {
   }

   /*
      _catCtxRecycleHandler implement
    */
   _catCtxRecycleHandler::_catCtxRecycleHandler( UTIL_RECYCLE_TYPE type,
                                                 UTIL_RECYCLE_OPTYPE opType,
                                                 catRecyCtxTaskHandler &taskHandler,
                                                 catCtxLockMgr &lockMgr )
   : _catCtxEventHandler( lockMgr ),
     _catCtxRecycleHelper( type, opType ),
     _taskHandler( taskHandler ),
     _isUseRecycleBin( TRUE ),
     _isReservedItem( FALSE )
   {
   }

   _catCtxRecycleHandler::~_catCtxRecycleHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRECYHANDLER_PARSEQUERY, "_catCtxRecycleHandler::parseQuery" )
   INT32 _catCtxRecycleHandler::parseQuery( const BSONObj &boQuery,
                                            _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRECYHANDLER_PARSEQUERY ) ;

      try
      {
         BOOLEAN isSkipRecycleBin = FALSE ;

         BSONElement ele = boQuery.getField( FIELD_NAME_SKIPRECYCLEBIN ) ;

         if ( EOO != ele.type() )
         {
            PD_CHECK( Bool == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to get field [%s], type is not Bool, "
                      "type: %d, obj: %s",
                      FIELD_NAME_SKIPRECYCLEBIN, ele.type(),
                      boQuery.toString().c_str() ) ;
            isSkipRecycleBin = ele.Bool() ;
         }

         // skip to use recycle bin
         if ( isSkipRecycleBin )
         {
            _isUseRecycleBin = FALSE ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse message, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXRECYHANDLER_PARSEQUERY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRECYHANDLER_ONCHECKEVENT, "_catCtxRecycleHandler::onCheckEvent" )
   INT32 _catCtxRecycleHandler::onCheckEvent( SDB_EVENT_OCCUR_TYPE type,
                                              const CHAR *targetName,
                                              const BSONObj &boTarget,
                                              _pmdEDUCB *cb,
                                              INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRECYHANDLER_ONCHECKEVENT ) ;

      if ( SDB_EVT_OCCUR_AFTER == type )
      {
         PD_CHECK( NULL != targetName, SDB_SYS, error, PDERROR,
                   "Failed to check target name, it is invalid" ) ;
         _recycleItem.setOriginName( targetName ) ;

         if ( UTIL_RECYCLE_CS == _recycleItem.getType() )
         {
            utilCSUniqueID originID = UTIL_UNIQUEID_NULL ;
            rc = catParseCSUniqueID( boTarget, originID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse unique ID from [%s], "
                         "rc: %d", boTarget.toPoolString().c_str(), rc ) ;
            _recycleItem.setOriginID( originID ) ;
         }
         else if ( UTIL_RECYCLE_CL == _recycleItem.getType() )
         {
            utilCLUniqueID originID = UTIL_UNIQUEID_NULL ;
            rc = catParseCLUniqueID( boTarget, originID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse unique ID from [%s], "
                         "rc: %d", boTarget.toPoolString().c_str(), rc ) ;
            _recycleItem.setOriginID( originID ) ;
         }

         try
         {
            // check data source
            if ( boTarget.hasField( FIELD_NAME_DATASOURCE_ID ) )
            {
               // it is from data source, not use recycle bin
               _isUseRecycleBin = FALSE ;
            }

            if ( UTIL_RECYCLE_CS == _recycleItem.getType() )
            {
               BSONElement ele = boTarget.getField( CAT_TYPE_NAME ) ;
               if ( ( NumberInt == ele.type() ) &&
                    ( DMS_STORAGE_CAPPED == ele.numberInt() ) )
               {
                  // for capped collection space, not use recycle bin
                  _isUseRecycleBin = FALSE ;
               }
            }
            else if ( UTIL_RECYCLE_CL == _recycleItem.getType() )
            {
               BSONElement ele = boTarget.getField( CAT_ATTRIBUTE_NAME ) ;
               if ( ( NumberInt == ele.type() ) &&
                    ( OSS_BIT_TEST( (UINT32)( ele.numberInt() ),
                                    DMS_MB_ATTR_CAPPED ) ) )
               {
                  // for capped collection space, not use recycle bin
                  _isUseRecycleBin = FALSE ;
               }
            }
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to parse target object, "
                    "occur exception: %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

         if ( _isUseRecycleBin )
         {
            rc = _checkRecycle( cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check recycle, rc: %d", rc ) ;
         }

         if ( ( !_isUseRecycleBin ) &&
              ( UTIL_RECYCLE_CS == _recycleItem.getType() ) )
         {
            // we need to drop recycle items inside this collection space
            // if we don't use recycle bin, so need to lock them
            rc = _recycleBinMgr->tryLockItem( _recycleItem, cb, EXCLUSIVE, _lockMgr ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to lock recycle items in collection space "
                         "[origin %s, recycle %s], rc: %d",
                         _recycleItem.getOriginName(),
                         _recycleItem.getRecycleName(), rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXRECYHANDLER_ONCHECKEVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRECYHANDLER__CHECKRECYCLE, "_catCtxRecycleHandler::_checkRecycle" )
   INT32 _catCtxRecycleHandler::_checkRecycle( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRECYHANDLER__CHECKRECYCLE ) ;

      rc = _recycleBinMgr->prepareItem( _recycleItem, _droppingItems, _lockMgr,
                                        cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare dropping items, "
                   "rc: %d", rc ) ;

      if ( !_recycleItem.isValid() )
      {
         _isUseRecycleBin = FALSE ;
      }
      else
      {
         _recycleBinMgr->reserveItem() ;
         _isReservedItem = TRUE ;
      }

      if ( _isUseRecycleBin )
      {
         // tell the task handler to handle recycle item
         _taskHandler.setRecycleItem( _recycleItem ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXRECYHANDLER__CHECKRECYCLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRECYHANDLER_ONEXECUTEEVENT, "_catCtxRecycleHandler::onExecuteEvent" )
   INT32 _catCtxRecycleHandler::onExecuteEvent( SDB_EVENT_OCCUR_TYPE type,
                                                _pmdEDUCB *cb,
                                                INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRECYHANDLER_ONEXECUTEEVENT ) ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         if ( _isUseRecycleBin )
         {
            rc = _executeRecycle( cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to execute recycle, "
                         "rc: %d", rc ) ;
         }
         else
         {
            rc = _executeWithoutRecycle( cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to execute without recycle, "
                         "rc: %d", rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXRECYHANDLER_ONEXECUTEEVENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRECYHANDLER__EXECRECYCKE, "_catCtxRecycleHandler::_executeRecycle" )
   INT32 _catCtxRecycleHandler::_executeRecycle( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRECYHANDLER__EXECRECYCKE ) ;

      SDB_ASSERT( _recycleItem.isValid(), "recycle item should be valid" ) ;

      rc = _recycleBinMgr->commitItem( _recycleItem, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to commit recycle item, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXRECYHANDLER__EXECRECYCKE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRECYHANDLER__EXECWITHOUTRECYCKE, "_catCtxRecycleHandler::_executeWithoutRecycle" )
   INT32 _catCtxRecycleHandler::_executeWithoutRecycle( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRECYHANDLER__EXECWITHOUTRECYCKE ) ;

      // if drop CS without recycle, drop recycled items inside
      // the collection space
      if ( UTIL_RECYCLE_CS == _recycleItem.getType() &&
           UTIL_RECYCLE_OP_DROP == _recycleItem.getOpType() &&
           UTIL_UNIQUEID_NULL != _recycleItem.getOriginID() )
      {
         utilCSUniqueID csUniqueID =
               (utilCSUniqueID)( _recycleItem.getOriginID() ) ;
         rc = _recycleBinMgr->dropItemsInCS( csUniqueID, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle items in "
                      "collection space [%s], rc: %d",
                      _recycleItem.getOriginName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXRECYHANDLER__EXECWITHOUTRECYCKE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRECYHANDLER_ONDELETEEVENT, "_catCtxRecycleHandler::onDeleteEvent" )
   void _catCtxRecycleHandler::onDeleteEvent()
   {
      PD_TRACE_ENTRY( SDB_CATCTXRECYHANDLER_ONDELETEEVENT ) ;

      if ( _isReservedItem )
      {
         _recycleBinMgr->unreserveItem() ;
      }

      PD_TRACE_EXIT( SDB_CATCTXRECYHANDLER_ONDELETEEVENT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRECYHANDLER_BUILDP1REPLY, "_catCtxRecycleHandler::buildP1Reply" )
   INT32 _catCtxRecycleHandler::buildP1Reply( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRECYHANDLER_BUILDP1REPLY ) ;

      if ( !_isUseRecycleBin )
      {
         goto done ;
      }

      if ( _recycleItem.isValid() )
      {
         rc = _recycleItem.toBSON( builder, FIELD_NAME_RECYCLE_ITEM ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build reply for "
                      "recycle item [origin %s, recycle %s], rc: %d",
                      _recycleItem.getOriginName(),
                      _recycleItem.getRecycleName(), rc ) ;
      }

      if ( !_droppingItems.empty() )
      {
         try
         {
            BSONArrayBuilder subBuilder(
                  builder.subarrayStart( FIELD_NAME_DROP_RECYCLE_ITEM ) ) ;

            for ( UTIL_RECY_ITEM_LIST_CIT iter = _droppingItems.begin() ;
                  iter != _droppingItems.end() ;
                  ++ iter )
            {
               subBuilder.append( iter->getRecycleName() ) ;
            }

            subBuilder.doneFast() ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build reply with dropping items, "
                    "occur exception %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            return rc ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXRECYHANDLER_BUILDP1REPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catRtrnCtxTaskHandler implement
    */
   _catRtrnCtxTaskHandler::_catRtrnCtxTaskHandler( catCtxLockMgr & lockMgr )
   : _catCtxTaskHandler( lockMgr )
   {
   }


   _catRtrnCtxTaskHandler::~_catRtrnCtxTaskHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRTRNCTXTASKHANDLER__ONROLLBACKEVENT, "_catRtrnCtxTaskHandler::onRollbackEvent" )
   INT32 _catRtrnCtxTaskHandler::onRollbackEvent( SDB_EVENT_OCCUR_TYPE type,
                                                  _pmdEDUCB *cb,
                                                  INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRTRNCTXTASKHANDLER__ONROLLBACKEVENT ) ;

      if ( SDB_EVT_OCCUR_AFTER == type )
      {
         _cancelTasks( cb, w ) ;
      }

      PD_TRACE_EXITRC( SDB_CATRTRNCTXTASKHANDLER__ONROLLBACKEVENT, rc ) ;

      return rc ;
   }

}
