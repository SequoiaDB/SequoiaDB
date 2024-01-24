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

#include "catCommon.hpp"
#include "catContextData.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"
#include "ossMemPool.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
    * _catCtxDataBase implement
    */
   _catCtxDataBase::_catCtxDataBase ( INT64 contextID, UINT64 eduID )
   : _catContextBase( contextID, eduID ),
     _groupHandler( _lockMgr )
   {
   }

   _catCtxDataBase::~_catCtxDataBase ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDATA__REGEVENTHANDLERS, "_catCtxDataBase::_regEventHandlers" )
   INT32 _catCtxDataBase::_regEventHandlers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXDATA__REGEVENTHANDLERS ) ;

      rc = _regEventHandler( &_groupHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register group handler, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXDATA__REGEVENTHANDLERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
    * _catCtxDataMultiTaskBase implement
    */
   _catCtxDataMultiTaskBase::_catCtxDataMultiTaskBase ( INT64 contextID,
                                                        UINT64 eduID )
   : _catCtxDataBase( contextID, eduID )
   {
      // Always need rollback for finished sub-tasks
      _needRollbackAlways = TRUE ;
   }

   _catCtxDataMultiTaskBase::~_catCtxDataMultiTaskBase ()
   {
      // Clear sub tasks
      _catSubTasks::iterator iter = _subTasks.begin() ;
      while ( iter != _subTasks.end() )
      {
         _catCtxTaskBase *subTask = (*iter) ;
         iter = _subTasks.erase( iter ) ;
         SDB_OSS_DEL subTask ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDATAMULTITASK_PREEXECUTE_INT, "_catCtxDataMultiTaskBase::_preExecuteInternal" )
   INT32 _catCtxDataMultiTaskBase::_preExecuteInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDATAMULTITASK_PREEXECUTE_INT ) ;

      _catSubTasks::iterator iter = _execTasks.begin() ;
      while ( iter != _execTasks.end() )
      {
         _catCtxTaskBase *subTask = (*iter) ;
         rc = subTask->preExecute( cb, _pDmsCB, _pDpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to pre-execute sub-task, rc: %d",
                      rc ) ;
         ++iter ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDATAMULTITASK_PREEXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDATAMULTITASK_EXECUTE_INT, "_catCtxDataMultiTaskBase::_executeInternal" )
   INT32 _catCtxDataMultiTaskBase::_executeInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDATAMULTITASK_EXECUTE_INT ) ;

      _catSubTasks::iterator iter = _execTasks.begin() ;
      while ( iter != _execTasks.end() )
      {
         _catCtxTaskBase *subTask = (*iter) ;
         rc = subTask->recheckTask( cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to recheck sub-task, rc: %d",
                      rc ) ;

         ++ iter ;
      }

      iter = _execTasks.begin() ;
      while ( iter != _execTasks.end() )
      {
         _catCtxTaskBase *subTask = (*iter) ;
         rc = subTask->execute( cb, _pDmsCB, _pDpsCB, w ) ;
         if ( SDB_OK != rc && subTask->isIgnoredRC( rc ) )
         {
            PD_LOG( PDWARNING,
                    "Ignore sub-task executing error %d",
                    rc ) ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to execute sub-task, rc: %d",
                      rc ) ;
         ++iter ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDATAMULTITASK_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDATAMULTITASK_ROLLBACK_INT, "_catCtxDataMultiTaskBase::_rollbackInternal" )
   INT32 _catCtxDataMultiTaskBase::_rollbackInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDATAMULTITASK_ROLLBACK_INT ) ;

      _catSubTasks::iterator iter = _execTasks.begin() ;
      while ( iter != _execTasks.end() )
      {
         _catCtxTaskBase *subTask = (*iter) ;
         INT32 tmprc = subTask->rollback( cb, _pDmsCB, _pDpsCB, w ) ;
         if ( SDB_OK != tmprc)
         {
            rc = tmprc ;
            PD_LOG( PDERROR,
                    "Failed to rollback sub-task, rc: %d", rc ) ;
         }
         ++iter ;
      }

      PD_TRACE_EXITRC ( SDB_CATCTXDATAMULTITASK_ROLLBACK_INT, rc ) ;
      return rc ;
   }

   void _catCtxDataMultiTaskBase::_addTask ( _catCtxTaskBase *pCtx,
                                             BOOLEAN pushExec )
   {
      _subTasks.push_back( pCtx ) ;
      if ( pushExec )
      {
         _execTasks.push_back( pCtx ) ;
      }
   }

   INT32 _catCtxDataMultiTaskBase::_pushExecTask ( _catCtxTaskBase *pCtx )
   {
      _execTasks.push_back( pCtx ) ;
      return SDB_OK ;
   }

   /*
    * _catCtxCLMultiTask implement
    */
   _catCtxCLMultiTask::_catCtxCLMultiTask ( INT64 contextID, UINT64 eduID )
   : _catCtxDataMultiTaskBase( contextID, eduID )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCL_DROPCL_TASK, "_catCtxCLMultiTask::_addDropCLTask" )
   INT32 _catCtxCLMultiTask::_addDropCLTask ( const std::string &clName,
                                              INT32 version,
                                              _catCtxDropCLTask **ppCtx,
                                              BOOLEAN pushExec,
                                              BOOLEAN rmTaskAndIdx )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCL_DROPCL_TASK ) ;

      _catCtxDropCLTask *pCtx = NULL ;
      pCtx = SDB_OSS_NEW _catCtxDropCLTask( clName, version, rmTaskAndIdx ) ;
      PD_CHECK( pCtx, SDB_SYS, error, PDERROR,
                "Failed to create drop collection [%s] sub-task",
                clName.c_str() ) ;

      _addTask( pCtx, pushExec ) ;
      if ( ppCtx )
      {
         (*ppCtx) = pCtx ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCL_DROPCL_TASK, rc ) ;
      return rc ;
   error :
      SAFE_OSS_DELETE( pCtx ) ;
      goto done ;
   }

   /*
    * _catCtxDropCS implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxDropCS, RTN_CONTEXT_CAT_DROP_CS,
                          "CAT_DROP_CS" )

   _catCtxDropCS::_catCtxDropCS ( INT64 contextID, UINT64 eduID )
   : _catCtxCLMultiTask( contextID, eduID ),
     _globIdxHandler( _lockMgr ),
     _taskHandler( _lockMgr ),
     _recycleHandler( UTIL_RECYCLE_CS, UTIL_RECYCLE_OP_DROP, _taskHandler,
                      _lockMgr )
   {
      _executeOnP1 = FALSE ;
      _needRollback = FALSE ;
      _ensureEmpty = FALSE ;

      // may have recycled collections on dropped groups
      _groupHandler.setIgnoreNonExist( TRUE ) ;
   }

   _catCtxDropCS::~_catCtxDropCS ()
   {
      _onCtxDelete () ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCS_OPEN, "_catCtxDropCS::open" )
   INT32 _catCtxDropCS::open ( const BSONObj &query,
                               rtnContextBuf &buffObj,
                               _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXDROPCS_OPEN ) ;

      rc = _open( query, MSG_CAT_DROP_SPACE_REQ, buffObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open drop collection space context, "
                   "rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXDROPCS_OPEN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCS__REGEVENTHANDLERS, "_catCtxDropCS::_regEventHandlers" )
   INT32 _catCtxDropCS::_regEventHandlers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXDROPCS__REGEVENTHANDLERS ) ;

      rc = _BASE::_regEventHandlers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register base handlers, "
                   "rc: %d", rc ) ;

      rc = _regEventHandler( &_globIdxHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register global index handler, "
                   "rc: %d", rc ) ;

      rc = _regEventHandler( &_recycleHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register recycle handler, "
                   "rc: %d", rc ) ;

      // NOTE: behavior of task handler depends on recycle handler, so
      // must register task handler after recycle handler
      rc = _regEventHandler( &_taskHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register task handler, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXDROPCS__REGEVENTHANDLERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCS_PARSEQUERY, "_catCtxDropCS::_parseQuery" )
   INT32 _catCtxDropCS::_parseQuery ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCS_PARSEQUERY ) ;

      SDB_ASSERT( MSG_CAT_DROP_SPACE_REQ == _cmdType,
                  "Wrong command type" ) ;

      try
      {
         BSONObjIterator iter( _boQuery ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            if ( 0 == ossStrcmp(ele.fieldName(), CAT_COLLECTION_SPACE_NAME) )
            {
               PD_LOG_MSG_CHECK( _targetName.empty(), SDB_INVALIDARG, error,
                                 PDERROR, "More than one collection space "
                                 "name in options" ) ;
               PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field type is not String, type: %d, obj: %s, rc: %d",
                         ele.type(), _boQuery.toString().c_str(), rc ) ;
               _targetName = ele.valuestr() ;

               if ( _targetName.length() > DMS_SU_NAME_SZ )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG ( PDERROR, "Invalid length of collectionspace name: "
                           "%s, rc: %d", _targetName.c_str(), rc ) ;
                  goto error ;
               }
            }
            else if ( 0 == ossStrcmp(ele.fieldName(), CAT_ENSURE_CS_IS_EMPTY) )
            {
               PD_CHECK( Bool == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field type is not Bool, type: %d, obj: %s, rc: %d",
                         ele.type(), _boQuery.toString().c_str(), rc ) ;
               _ensureEmpty = ele.Bool() ;
            }
         }

         if ( _targetName.empty() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Collection space can't be empty, obj: %s",
                    _boQuery.toString().c_str() ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCS_PARSEQUERY, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCS_CHECK_INT, "_catCtxDropCS::_checkInternal" )
   INT32 _catCtxDropCS::_checkInternal ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCS_CHECK_INT ) ;

      _catCtxDropCSTask *pDropCSTask = NULL ;
      BSONElement ele ;

      rc = _addDropCSTask( _targetName, &pDropCSTask, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to create drop collection space [%s] task, rc: %d",
                   _targetName.c_str(), rc ) ;

      rc = pDropCSTask->checkTask( cb, _lockMgr ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check drop collection space [%s] task, rc: %d",
                   _targetName.c_str(), rc ) ;

      _boTarget = pDropCSTask->getDataObj() ;

      if ( !pDropCSTask->isDataSource() )
      {
         utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;

         rc = catParseCSUniqueID( _boTarget, csUniqueID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get unique ID of collection "
                      "space [%s], rc: %d", _targetName.c_str(), rc ) ;

         // dropping collection space need to drop recycle items inside
         // this collection space
         // get group list for recycle bin items
         rc = _groupHandler.addGroupsInRecycleBin( csUniqueID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get group list for recycle "
                      "collections from collection space [%s], rc: %d",
                      _targetName.c_str(), rc ) ;

         // set current collection space unique ID to global index handler
         _globIdxHandler.setExclusedCSUniqueID( csUniqueID ) ;
      }

      rc = _addDropCSSubTasks( pDropCSTask, cb ) ;
      PD_RC_CHECK( rc , PDERROR,
                   "Failed to add sub-tasks for drop collection space [%s], "
                   "rc: %d",
                   _targetName.c_str(), rc ) ;

      rc = _pushExecTask( pDropCSTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to push collection task, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCS_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCS_DROPCS_TASK, "_catCtxDropCS::_addDropCSTask" )
   INT32 _catCtxDropCS::_addDropCSTask ( const std::string &csName,
                                         _catCtxDropCSTask **ppCtx,
                                         BOOLEAN pushExec )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCS_DROPCS_TASK ) ;

      _catCtxDropCSTask *pCtx = NULL ;
      pCtx = SDB_OSS_NEW _catCtxDropCSTask( csName ) ;
      PD_CHECK( pCtx, SDB_SYS, error, PDERROR,
                "Failed to create drop collection space [%s] sub-task",
                csName.c_str() ) ;

      _addTask( pCtx, pushExec ) ;
      if ( ppCtx )
      {
         (*ppCtx) = pCtx ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCS_DROPCS_TASK, rc ) ;
      return rc ;
   error :
      SAFE_OSS_DELETE( pCtx ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCS_DROPCS_SUBTASK, "_catCtxDropCS::_addDropCSSubTasks" )
   INT32 _catCtxDropCS::_addDropCSSubTasks ( _catCtxDropCSTask *pDropCSTask,
                                             _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATCTXDROPCS_DROPCS_SUBTASK ) ;

      try
      {
         BSONElement ele ;

         ele = pDropCSTask->getDataObj().getField( CAT_COLLECTION ) ;

         if ( Array == ele.type() )
         {
            BSONObjIterator i ( ele.embeddedObject() ) ;
            std::set<std::string> externalMainCL ;
            while ( i.more() )
            {
               string clFullName ;
               BSONObj boTmp ;
               const CHAR *pCLName = NULL ;
               _catCtxDropCLTask *pDropCLTask = NULL ;
               BSONElement beTmp = i.next() ;
               PD_CHECK( Object == beTmp.type(),
                         SDB_CAT_CORRUPTION, error, PDERROR,
                         "Invalid collection record field type: %d",
                         beTmp.type() ) ;
               boTmp = beTmp.embeddedObject() ;
               rc = rtnGetStringElement( boTmp, CAT_COLLECTION_NAME, &pCLName ) ;
               PD_CHECK( SDB_OK == rc,
                         SDB_CAT_CORRUPTION, error, PDERROR,
                         "Get field [%s] failed, rc: %d",
                         CAT_COLLECTION_NAME, rc ) ;

               clFullName = _targetName ;
               clFullName += "." ;
               clFullName += pCLName ;

               if ( _ensureEmpty )
               {
                  rc = SDB_DMS_CS_NOT_EMPTY ;
                  PD_LOG( PDWARNING, "Could not drop while collectionspace"
                          " is not empty, rc: %d", rc ) ;
                  goto error ;
               }

               rc = _addDropCLTask( clFullName, -1, &pDropCLTask, TRUE, FALSE );

               // Space has been locked already
               pDropCLTask->disableLocks() ;

               rc = pDropCLTask->checkTask( cb, _lockMgr ) ;
               PD_RC_CHECK( rc, PDWARNING,
                            "Failed to check drop collection [%s] task, rc: %d",
                            clFullName.c_str(), rc ) ;

               if ( pDropCLTask->globalIndexCLList().size() > 0 )
               {
                  rc = _globIdxHandler.addGlobIdxs(
                                          pDropCLTask->globalIndexCLList() ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to add global indexes, "
                               "rc: %d", rc ) ;
               }

               rc = _addDropCLSubTasks( pDropCLTask, cb, externalMainCL ) ;
               PD_RC_CHECK( rc , PDERROR,
                            "Failed to add sub-tasks for drop collection [%s], "
                            "rc: %d",
                            clFullName.c_str(), rc ) ;

            }

            PD_LOG ( PDDEBUG,
                     "Found %d external main collections for drop collection space",
                     externalMainCL.size() ) ;

            if ( externalMainCL.size() > 0 )
            {
               _catCtxUnlinkCSTask *pUnlinkCS = NULL ;
               rc = _addUnlinkCSTask( _targetName, &pUnlinkCS ) ;
               PD_RC_CHECK( rc , PDERROR,
                            "Failed to add unlinkCS [%s] task, rc: %d",
                            _targetName.c_str(), rc ) ;

               pUnlinkCS->unlinkCS( externalMainCL ) ;

               pUnlinkCS->checkTask( cb, _lockMgr ) ;
               PD_RC_CHECK( rc, PDWARNING,
                            "Failed to check unlinkCS [%s] task, rc: %d",
                            _targetName.c_str(), rc ) ;

               pUnlinkCS->addIgnoreRC( SDB_DMS_NOTEXIST ) ;
               pUnlinkCS->addIgnoreRC( SDB_INVALID_MAIN_CL ) ;
            }

            if ( _groupHandler.isGroupEmpty() )
            {
               // If group size is 0, there are two possibilities:
               // (1) No collection is in the collection space.
               // (2) The collection space is using data source.
               // For the second scenario, we need to provide a group for it.
               BSONElement dsEle =
                     pDropCSTask->getDataObj().getField( FIELD_NAME_DATASOURCE_ID ) ;
               if ( !dsEle.eoo() )
               {
                  UTIL_DS_UID dsID = dsEle.numberInt() ;
                  SDB_ASSERT( UTIL_INVALID_DS_UID != dsID,
                              "Data source is invalid" ) ;
                  rc = _groupHandler.addGroup( SDB_DSID_2_GROUPID( dsID ) ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to add data source "
                               "group ID, rc: %d", rc ) ;
               }
            }
         }
         else if ( !ele.eoo() )
         {
            PD_LOG( PDERROR, "Invalid collection field[%s] type: %d",
                    CAT_COLLECTION, ele.type() ) ;
            rc = SDB_CAT_CORRUPTION ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCS_DROPCS_SUBTASK, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCS_DROPCL_SUBTASK, "_catCtxDropCS::_addDropCLSubTasks" )
   INT32 _catCtxDropCS::_addDropCLSubTasks ( _catCtxDropCLTask *pDropCLTask,
                                             _pmdEDUCB *cb,
                                             std::set<std::string> &externalMainCL )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCS_DROPCL_SUBTASK ) ;

      // For DropCL inside a DropCS:
      // 1. if cl is a sub-collection, need unlink it's main-collection
      // 2. if cl is a main-collection, need unlink it's sub-collections

      try
      {
         const std::string &clName = pDropCLTask->getDataName() ;
         clsCatalogSet cataSet( clName.c_str() );

         rc = cataSet.updateCatSet( pDropCLTask->getDataObj() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to parse catalog info [%s], rc: %d",
                      clName.c_str(), rc ) ;

         if ( cataSet.isMainCL() )
         {
            CLS_SUBCL_LIST subCLLst;
            CLS_SUBCL_LIST_IT iterSubCL;

            rc = cataSet.getSubCLList( subCLLst );
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get sub-collection list of collection [%s], "
                         "rc: %d",
                         clName.c_str(), rc ) ;
            iterSubCL = subCLLst.begin() ;
            while( iterSubCL != subCLLst.end() )
            {
               std::string subCLName = (*iterSubCL) ;

               // Do not delete sub-collections when dropping space
               BOOLEAN inSameSpace = FALSE ;
               _catCtxUnlinkSubCLTask *pUnlinkSubCLTask = NULL ;

               // Unlink the sub collection from deleting collection
               rc = _addUnlinkSubCLTask( clName, subCLName, &pUnlinkSubCLTask ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to create unlink collection task for "
                            "sub-collection [%s], rc: %d",
                            subCLName.c_str(), rc ) ;

               pUnlinkSubCLTask->addIgnoreRC( SDB_DMS_NOTEXIST ) ;

               rc = rtnCollectionsInSameSpace ( clName.c_str(),
                                                clName.size(),
                                                subCLName.c_str(),
                                                subCLName.size() ,
                                                inSameSpace ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to check whether main and sub collections "
                            "[%s] and [%s] are in the space, rc: %d",
                            clName.c_str(), subCLName.c_str(), rc ) ;
               if ( inSameSpace )
               {
                  // Collection Space has been already locked
                  pUnlinkSubCLTask->disableLocks() ;
               }
               rc = pUnlinkSubCLTask->checkTask( cb, _lockMgr ) ;
               if ( SDB_DMS_NOTEXIST == rc ||
                    SDB_INVALID_SUB_CL == rc )
               {
                  PD_LOG ( PDWARNING,
                           "Sub-collection [%s] have been changed",
                           subCLName.c_str() ) ;
                  rc = SDB_OK ;
                  ++iterSubCL ;
                  continue ;
               }

               rc = _groupHandler.addGroups( pUnlinkSubCLTask->getDataObj() ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to collect groups for sub-collection [%s], "
                            "rc: %d",
                            subCLName.c_str(), rc ) ;

               ++iterSubCL ;
            }
         }
         else
         {
            std::string mainCLName = cataSet.getMainCLName() ;
            if ( !mainCLName.empty() )
            {
               BOOLEAN inSameSpace = FALSE ;

               if ( externalMainCL.find( mainCLName ) == externalMainCL.end() )
               {
                  rc = rtnCollectionsInSameSpace ( mainCLName.c_str(),
                                                   mainCLName.size(),
                                                   clName.c_str(),
                                                   clName.size() ,
                                                   inSameSpace ) ;
                  PD_RC_CHECK( rc, PDERROR,
                               "Failed to check whether main and sub collections "
                               "[%s] and [%s] are in the space, rc: %d",
                               mainCLName.c_str(), clName.c_str(), rc ) ;
                  if ( !inSameSpace )
                  {
                     externalMainCL.insert( mainCLName ) ;
                  }
               }
            }
            rc = _groupHandler.addGroups( pDropCLTask->getDataObj() ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to collect groups for collection [%s], rc: %d",
                         clName.c_str(), rc ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCS_DROPCL_SUBTASK, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCS_UNLINKCS_TASK, "_catCtxDropCS::_addUnlinkCSTask" )
   INT32 _catCtxDropCS::_addUnlinkCSTask ( const std::string &csName,
                                           _catCtxUnlinkCSTask **ppCtx,
                                           BOOLEAN pushExec )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCS_UNLINKCS_TASK ) ;

      _catCtxUnlinkCSTask *pCtx = NULL ;
      pCtx = SDB_OSS_NEW _catCtxUnlinkCSTask( csName ) ;
      PD_CHECK( pCtx, SDB_SYS, error, PDERROR,
                "Failed to add unlinkCS [%s] task", csName.c_str() ) ;

      _addTask( pCtx, pushExec ) ;
      if ( ppCtx )
      {
         (*ppCtx) = pCtx ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCS_UNLINKCS_TASK, rc ) ;
      return rc ;
   error :
      SAFE_OSS_DELETE( pCtx ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCS_UNLINKSUBCL_TASK, "_catCtxDropCS::_addUnlinkSubCLTask" )
   INT32 _catCtxDropCS::_addUnlinkSubCLTask ( const std::string &mainCLName,
                                              const std::string &subCLName,
                                              _catCtxUnlinkSubCLTask **ppCtx,
                                              BOOLEAN pushExec )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCS_UNLINKSUBCL_TASK ) ;

      _catCtxUnlinkSubCLTask *pCtx = NULL ;
      pCtx = SDB_OSS_NEW _catCtxUnlinkSubCLTask( mainCLName, subCLName ) ;
      PD_CHECK( pCtx, SDB_SYS, error, PDERROR,
                "Failed to create link sub-collection [%s/%s] sub-task",
                mainCLName.c_str(), subCLName.c_str() ) ;

      _addTask( pCtx, pushExec ) ;
      if ( ppCtx )
      {
         (*ppCtx) = pCtx ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCS_UNLINKSUBCL_TASK, rc ) ;
      return rc ;
   error :
      SAFE_OSS_DELETE( pCtx ) ;
      goto done ;
   }

   /*
      _catCtxRenameCS implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxRenameCS, RTN_CONTEXT_CAT_RENAME_CS,
                          "CAT_RENAME_CS" )

   _catCtxRenameCS::_catCtxRenameCS ( INT64 contextID, UINT64 eduID )
   : _catCtxDataBase( contextID, eduID )
   {
      _executeOnP1 = FALSE ;
      _needRollback = FALSE ;

      // may have recycled collections on dropped groups
      _groupHandler.setIgnoreNonExist( TRUE ) ;
   }

   _catCtxRenameCS::~_catCtxRenameCS ()
   {
      _onCtxDelete () ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRENAMECS_PARSEQUERY, "_catCtxRenameCS::_parseQuery" )
   INT32 _catCtxRenameCS::_parseQuery ( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCTXRENAMECS_PARSEQUERY ) ;

      SDB_ASSERT( MSG_CAT_RENAME_CS_REQ == _cmdType, "Wrong command type" ) ;

      try
      {
         rc = rtnGetSTDStringElement( _boQuery, CAT_COLLECTION_SPACE_OLDNAME,
                                      _targetName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      CAT_COLLECTION_SPACE_NAME, rc ) ;

         rc = rtnGetSTDStringElement( _boQuery, CAT_COLLECTION_SPACE_NEWNAME,
                                      _newCSName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      CAT_COLLECTION_SPACE_NEWNAME, rc ) ;
      }
      catch ( exception & e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXRENAMECS_PARSEQUERY, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRENAMECS_CHECK_INT, "_catCtxRenameCS::_checkInternal" )
   INT32 _catCtxRenameCS::_checkInternal ( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCTXRENAMECS_CHECK_INT ) ;

      BOOLEAN newCSExist = FALSE ;
      BSONObj boCollectionspace ;
      ossPoolSet< UINT32 > subCLGroupSet ;
      INT64 taskCount = 0 ;
      utilCSUniqueID uniqueID = UTIL_UNIQUEID_NULL ;

      /// check new cs name whether invaild
      rc = dmsCheckCSName( _newCSName.c_str(), FALSE );
      PD_RC_CHECK( rc, PDERROR, "Invalid cs name[%s]", _newCSName.c_str() );

      try
      {
         // lock old cs
         rc = catGetAndLockCollectionSpace( _targetName, _boTarget,
                                            cb, &_lockMgr, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to lock the collection space[%s], rc: %d",
                      _targetName.c_str(), rc ) ;

         // check task count of old cs
         rc = catGetCSTaskCountByType( _targetName.c_str(), CLS_TASK_SPLIT,
                                       cb, taskCount ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get task count of cs[%s], rc: %d",
                      _targetName.c_str(), rc ) ;

         PD_CHECK( 0 == taskCount, SDB_OPERATION_CONFLICT, error, PDERROR,
                   "Failed to rename cs[%s]: should have no split tasks",
                   _targetName.c_str() ) ;

         // check new cs exists or not
         rc = catCheckSpaceExist( _newCSName.c_str(), newCSExist,
                                  boCollectionspace, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check collection space[%s] exists, rc: %d",
                      _newCSName.c_str(), rc ) ;
         PD_CHECK( FALSE == newCSExist, SDB_DMS_CS_EXIST, error, PDERROR,
                   "Collection space [%s] already exist!",
                   _newCSName.c_str() ) ;

         // lock new cs
         if ( !_lockMgr.tryLockCollectionSpace( _newCSName, EXCLUSIVE ) )
         {
            rc = SDB_LOCK_FAILED ;
            goto error ;
         }

         rc = catParseCSUniqueID( _boTarget, uniqueID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get unique ID of "
                      "collection space [%s], rc: %d",
                      _targetName.c_str(), rc ) ;

         // we need reply groups list to coord, so that coord can send msg to
         // corresponding data by groups list
         rc = catGetCSGroups( uniqueID, cb, TRUE, FALSE,
                              _groupHandler.getGroupIDSet() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get group list of cs[%s], rc: %d",
                      _targetName.c_str(), rc ) ;

         rc = catGetCSSubCLGroups( _targetName.c_str(), cb, subCLGroupSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get group list from "
                      "sub-collections of collection [%s], rc: %d",
                      _targetName.c_str(), rc ) ;

         rc = _groupHandler.addGroups( subCLGroupSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add sub-collection groups, "
                      "rc: %d", rc ) ;

         // check if from data source
         if ( _groupHandler.isGroupEmpty() &&
              _boTarget.hasField( FIELD_NAME_DATASOURCE_ID ) )
         {
            UTIL_DS_UID dsUID = UTIL_INVALID_DS_UID ;

            // check data source ID
            rc = catCheckDataSourceID( _boTarget, dsUID ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to check data source ID "
                         "from collection space [%s], rc: %d",
                         _targetName.c_str(), rc ) ;

            rc = _groupHandler.addGroup( SDB_DSID_2_GROUPID( dsUID ) ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add data source "
                         "group ID, rc: %d", rc ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXRENAMECS_CHECK_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRENAMECS_EXECUTE_INT, "_catCtxRenameCS::_executeInternal" )
   INT32 _catCtxRenameCS::_executeInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATCTXRENAMECS_EXECUTE_INT ) ;
      BSONElement ele ;

      rc = catRenameCSStep( _targetName, _newCSName, cb, _pDmsCB, _pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to rename collection space[%s], rc: %d",
                   _targetName.c_str(), rc ) ;

      ele = _boTarget.getField( CAT_COLLECTION ) ;
      if ( Array == ele.type() )
      {
         BSONObjIterator i ( ele.embeddedObject() ) ;
         while ( i.more() )
         {
            CHAR clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ]    = { 0 } ;
            CHAR newCLFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
            BSONObj boTmp ;
            const CHAR *pCLName = NULL ;

            BSONElement beTmp = i.next() ;
            PD_CHECK( Object == beTmp.type(),
                      SDB_CAT_CORRUPTION, error, PDERROR,
                      "Invalid field type: %d", beTmp.type() ) ;

            boTmp = beTmp.embeddedObject() ;
            rc = rtnGetStringElement( boTmp, CAT_COLLECTION_NAME, &pCLName ) ;
            PD_CHECK( SDB_OK == rc, SDB_CAT_CORRUPTION, error, PDERROR,
                      "Get field [%s] failed, rc: %d",
                      CAT_COLLECTION_NAME, rc ) ;

            ossSnprintf( clFullName, sizeof( clFullName ),
                         "%s.%s", _targetName.c_str(), pCLName ) ;
            ossSnprintf( newCLFullName, sizeof( newCLFullName ),
                         "%s.%s", _newCSName.c_str(), pCLName ) ;

            rc = catRenameCLStep( clFullName, newCLFullName,
                                  cb, _pDmsCB, _pDpsCB, w ) ;
            PD_RC_CHECK ( rc, PDERROR,
                          "Fail to rename cl from[%s] to [%s], rc: %d",
                          clFullName, newCLFullName, rc ) ;

         }
      }

      PD_LOG( PDDEBUG, "Rename collection space[%s] to [%s] succeed.",
              _targetName.c_str(), _newCSName.c_str() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXRENAMECS_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _catCtxAlterCS implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxAlterCS, RTN_CONTEXT_CAT_ALTER_CS,
                          "CAT_ALTER_CS" )

   _catCtxAlterCS::_catCtxAlterCS ( INT64 contextID, UINT64 eduID )
   : _catCtxDataMultiTaskBase( contextID, eduID )
   {
      // may have recycled collections on dropped groups
      _groupHandler.setIgnoreNonExist( TRUE ) ;
   }

   _catCtxAlterCS::~_catCtxAlterCS ()
   {
      _onCtxDelete () ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCS_PARSEQUERY, "_catCtxAlterCS::_parseQuery" )
   INT32 _catCtxAlterCS::_parseQuery ( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCS_PARSEQUERY ) ;

      BOOLEAN exist = FALSE ;

      SDB_ASSERT( MSG_CAT_ALTER_CS_REQ == _cmdType, "Wrong command type" ) ;

      try
      {
         BSONObj meta ;
         rc = rtnGetSTDStringElement( _boQuery, CAT_COLLECTION_SPACE_NAME,
                                      _targetName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      CAT_COLLECTION_SPACE_NAME, rc ) ;

         // Check if the collection space is using data source. If yes, it can't
         // be modified.
         rc = catCheckSpaceExist( _targetName.c_str(), exist, meta, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check collection space existence "
                      "failed[%d]", rc ) ;
         if ( !exist )
         {
            rc = SDB_DMS_CS_NOTEXIST ;
            PD_LOG( PDERROR, "Collection space[%s] dose not exist",
                    _targetName.c_str() ) ;
            goto error ;
         }

         if ( meta.hasField( FIELD_NAME_DATASOURCE_ID ) )
         {
            rc = SDB_OPERATION_INCOMPATIBLE ;
            PD_LOG_MSG( PDERROR, "Not allowed to alter a collection space "
                        "which is using data source[%d]", rc ) ;
            goto error ;
         }

         rc = _alterJob.initialize( _targetName.c_str(),
                                    RTN_ALTER_COLLECTION_SPACE,
                                    _boQuery ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to extract alter job, rc: %d",
                      rc ) ;

         PD_CHECK( RTN_ALTER_COLLECTION_SPACE == _alterJob.getObjectType(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Wrong type of alter job" ) ;
      }
      catch ( exception & e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCS_PARSEQUERY, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCS_CHECK_INT, "_catCtxAlterCS::_checkInternal" )
   INT32 _catCtxAlterCS::_checkInternal ( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCS_CHECK_INT ) ;

      if ( _alterJob.isEmpty() )
      {
         goto done ;
      }
      else
      {
         const rtnAlterTask * task = NULL ;

         PD_CHECK( 1 == _alterJob.getAlterTasks().size(),
                   SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                   "Failed to check alter job: should have only one task" ) ;

         task = _alterJob.getAlterTasks().front() ;
         rc = _checkAlterTask( task, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check alter collection space command [%s], "
                      "rc: %d", task->getActionName(), rc ) ;

         if ( task->testFlags( RTN_ALTER_TASK_FLAG_3PHASE ) )
         {
            _executeOnP1 = FALSE ;
         }
         else
         {
            _executeOnP1 = TRUE ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCS_CHECK_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCS__ADDALTERTASK, "_catCtxAlterCS::_addAlterTask" )
   INT32 _catCtxAlterCS::_addAlterTask ( const string & collectionSpace,
                                         const rtnAlterTask * task,
                                         catCtxAlterCSTask ** catTask,
                                         BOOLEAN pushExec )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCS__ADDALTERTASK ) ;

      SDB_ASSERT( NULL != task, "task is invalid" ) ;
      SDB_ASSERT( NULL != catTask, "alter task is invalid" ) ;

      catCtxAlterCSTask * tempTask = SDB_OSS_NEW catCtxAlterCSTask( collectionSpace, task ) ;
      PD_CHECK( tempTask, SDB_OOM, error, PDERROR,
                "Failed to create alter task [%s] on collection space [%s]",
                task->getActionName(), collectionSpace.c_str() ) ;

      _addTask( tempTask, pushExec ) ;
      if ( catTask )
      {
         (*catTask) = tempTask ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCS__ADDALTERTASK, rc ) ;
      return rc ;

   error :
      SAFE_OSS_DELETE( tempTask ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCS__CHKALTERTASK, "_catCtxAlterCS::_checkAlterTask" )
   INT32 _catCtxAlterCS::_checkAlterTask ( const rtnAlterTask * task, _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCS__CHKALTERTASK ) ;

      catCtxLockMgr localLockMgr ;
      catCtxLockMgr * lockMgr = &localLockMgr ;
      catCtxAlterCSTask * catTask = NULL ;
      SET_UINT32 groupSet ;

      if ( task->testFlags( RTN_ALTER_TASK_FLAG_CONTEXTLOCK ) )
      {
         lockMgr = &_lockMgr ;
      }

      SDB_ASSERT( NULL != lockMgr, "lock manager is invalid" ) ;

      rc = _addAlterTask( _targetName, task, &catTask, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add alter task [%s] on "
                   "collection space [%s], rc: %d", task->getActionName(),
                   _targetName.c_str(), rc ) ;

      rc = catTask->checkTask( cb, *lockMgr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check alter task [%s] on "
                   "collection space [%s], rc: %d", task->getActionName(),
                   _targetName.c_str(), rc ) ;

      rc = _pushExecTask( catTask ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to push collection task, "
                   "rc: %d", rc ) ;

      {
         const CAT_GROUP_LIST & groups = catTask->getGroups() ;
         for ( CAT_GROUP_LIST::const_iterator iterGroup = groups.begin() ;
               iterGroup != groups.end() ;
               iterGroup ++ )
         {
            groupSet.insert( (*iterGroup ) ) ;
         }
      }

      rc = _groupHandler.addGroups( groupSet ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add groups, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCS__CHKALTERTASK, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
    * _catCtxCreateCL implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxCreateCL, RTN_CONTEXT_CAT_CREATE_CL,
                          "CAT_CREATE_CL" )

   _catCtxCreateCL::_catCtxCreateCL ( INT64 contextID, UINT64 eduID )
   : _catCtxDataBase( contextID, eduID )
   {
      _executeOnP1 = TRUE ;
      _needRollback = TRUE ;
      _clUniqueID = UTIL_UNIQUEID_NULL ;
      _fieldMask = UTIL_ARG_FIELD_EMPTY ;
   }

   _catCtxCreateCL::~_catCtxCreateCL ()
   {
      _splitList.clear() ;
      _onCtxDelete () ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCRTCL_OPEN, "_catCtxCreateCL::open" )
   INT32 _catCtxCreateCL::open ( const bson::BSONObj &query,
                                 rtnContextBuf &buffObj,
                                 _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXCRTCL_OPEN ) ;

      rc = _open( query, MSG_CAT_CREATE_COLLECTION_REQ, buffObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open create collection context, "
                   "rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXCRTCL_OPEN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATECL_PARSEQUERY, "_catCtxCreateCL::_parseQuery" )
   INT32 _catCtxCreateCL::_parseQuery ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATECL_PARSEQUERY ) ;

      SDB_ASSERT( MSG_CAT_CREATE_COLLECTION_REQ == _cmdType,
                  "Wrong command type" ) ;

      try
      {
         rc = rtnGetSTDStringElement( _boQuery, CAT_COLLECTION_NAME,
                                      _targetName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_COLLECTION_NAME, rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCREATECL_PARSEQUERY, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATECL_CHECK_INT, "_catCtxCreateCL::_checkInternal" )
   INT32 _catCtxCreateCL::_checkInternal ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATECL_CHECK_INT ) ;

      CHAR szSpace[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;
      CHAR szCollection[ DMS_COLLECTION_NAME_SZ + 1 ] = {0} ;
      BSONObj boSpace, boDomain, boDummy ;
      utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
      UTIL_DS_UID dsUID = UTIL_INVALID_DS_UID ;
      CAT_GROUP_LIST groupList ;

      // Just check the existence of collection, no lock is needed
      rc = catGetCollection( _targetName, boDummy, cb ) ;
      PD_CHECK( SDB_DMS_NOTEXIST == rc,
                SDB_DMS_EXIST, error, PDERROR,
                "Create failed, the collection [%s] exists",
                _targetName.c_str() ) ;

      // split collection full name to csname and clname
      rc = rtnResolveCollectionName( _targetName.c_str(),
                                     _targetName.size(),
                                     szSpace, DMS_COLLECTION_SPACE_NAME_SZ,
                                     szCollection, DMS_COLLECTION_NAME_SZ );
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to resolve collection name [%s], rc: %d",
                    _targetName.c_str(), rc ) ;

      // make sure the name is valid
      rc = dmsCheckCLName( szCollection, FALSE ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to check collection name [%s], rc: %d",
                    szCollection, rc ) ;

      // get collection-space
      rc = catGetAndLockCollectionSpace( szSpace, boSpace, cb, NULL, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the collection space [%s], rc: %d",
                   szSpace, rc ) ;

      // Check if the collection space is mapping to another collection
      // space on a data source(pure mapping collection space). If yes, it's
      // not allowed to create collection in this collection space.
      rc = catCheckDataSourceID( boSpace, dsUID ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to check data source ID from "
                   "collection [%s], rc: %d", szSpace, rc ) ;
      if ( UTIL_INVALID_DS_UID != dsUID )
      {
         rc = SDB_OPERATION_INCOMPATIBLE ;
         PD_LOG( PDERROR, "Can not create collection on a data source "
                 "mapping collection space[%s]", szSpace ) ;
         goto error ;
      }

      // here we do not care what the values are
      // we care how many records in the specified collection space
      rc = catParseCSUniqueID( boSpace, csUniqueID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get unique ID of "
                   "collection space [%s], rc: %d",
                   szSpace, rc ) ;
      rc = catCheckCSCapacity( csUniqueID, 1, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check capacity for "
                   "collection space [%s], rc: %d", szSpace, rc ) ;

      // Lock the collection name
      PD_CHECK( _lockMgr.tryLockCollection( szSpace, _targetName, EXCLUSIVE ),
                SDB_LOCK_FAILED, error, PDERROR,
                "Failed to lock collection [%s]",
                _targetName.c_str() ) ;

      // try to get domain obj of cl.
      {
         BSONElement eleDomain = boSpace.getField( CAT_DOMAIN_NAME ) ;
         if ( String == eleDomain.type() )
         {
            std::string domainName = eleDomain.str() ;
            // get Domain
            rc = catGetAndLockDomain( domainName, boDomain, cb,
                                      &_lockMgr, SHARED ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get the domain [%s], rc: %d",
                         domainName.c_str(), rc ) ;
         }
      }

      // Build the new object
      rc = catCheckAndBuildCataRecord( _boQuery, _fieldMask, _clInfo ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check create collection obj [%s], rc: %d",
                   _boQuery.toString().c_str(), rc ) ;

      {
         // Capped collection check.
         BSONElement eleType = boSpace.getField( CAT_TYPE_NAME ) ;
         if ( NumberInt == eleType.type() )
         {
            INT32 type = eleType.numberInt() ;
            if ( ( DMS_STORAGE_NORMAL == type ) && _clInfo._capped )
            {
               PD_LOG( PDERROR, "Capped colleciton can only be created on "
                       "Capped collection space" ) ;
               rc = SDB_OPERATION_INCOMPATIBLE ;
               goto error ;
            }
            // If the user create a collection on a Capped CS, without specify
            // "Capped" for collection, it's also OK.
            if ( ( DMS_STORAGE_CAPPED == type ) && !_clInfo._capped )
            {
               _clInfo._capped = TRUE ;
               _fieldMask |= UTIL_CL_CAPPED_FIELD ;
            }
         }
      }

      // Get last history version of collection name
      _version = catGetBucketVersion( _targetName.c_str(), cb ) ;
      _clInfo._version = _version ;

      rc = _combineOptions( boDomain, boSpace, _fieldMask, _clInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to combine options, domainObj [%s], "
                 "create cl options[%s], rc: %d",
                 boDomain.toString().c_str(), _boQuery.toString().c_str(),
                 rc ) ;
         goto error ;
      }

      // get unique id
      {
         BSONElement ele = boSpace.getField( CAT_CS_CLUNIQUEHWM ) ;
         PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field[%s], type: %d",
                   CAT_CS_CLUNIQUEHWM, ele.type() );

         _clInfo._clUniqueID = (INT64)ele.numberLong() + 1 ;
         _clUniqueID = _clInfo._clUniqueID ;

         if ( utilGetCLInnerID(_clUniqueID) > (utilCLInnerID)UTIL_CLINNERID_MAX )
         {
            rc = SDB_CAT_CL_UNIQUEID_EXCEEDED ;
            PD_LOG( PDERROR,
                    "CL inner id can't exceed %u, cl unique id: %llu, rc: %d",
                    UTIL_CLINNERID_MAX, _clUniqueID, rc ) ;
            goto error ;
         }
      }

      /// choose a group to create cl
      rc = _chooseGroupOfCl( boDomain, boSpace, _clInfo, cb,
                             groupList, _splitList ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to choose groups for new collection [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

      rc = _groupHandler.addGroups( groupList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add group list, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCREATECL_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATECL_EXECUTE_INT, "_catCtxCreateCL::_executeInternal" )
   INT32 _catCtxCreateCL::_executeInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      BSONObj boNewObj ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATECL_EXECUTE_INT ) ;

      // build new collection record for meta data.
      rc = catBuildCatalogRecord ( cb, _clInfo, _fieldMask, 0,
                                   _groupHandler.getGroupIDSet(),
                                   _splitList, boNewObj, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Build new collection catalog record failed, rc: %d",
                   rc ) ;

      _boTarget = boNewObj.getOwned() ;

      // create sequence
      if( _fieldMask & UTIL_CL_AUTOINC_FIELD )
      {
         rc = catCreateAutoIncSequences( _clInfo, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to create system sequences of collection[%s], "
                      "rc: %d", _targetName.c_str(), rc ) ;
      }

      // create cl
      rc = catCreateCLStep( _targetName, _clUniqueID, _boTarget,
                            cb, _pDmsCB, _pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to create collection[name: %s, id: %llu], rc: %d",
                   _targetName.c_str(), _clUniqueID, rc ) ;
      PD_LOG( PDDEBUG, "Create collection[name: %s, id: %llu] succeed.",
              _targetName.c_str(), _clUniqueID ) ;

      // create index: $id, $shard
      rc = _createSysIndex( _clInfo, _indexList, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to create system index for collection[%s], "
                   "rc: %d", _targetName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCREATECL_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _catCtxCreateCL::_createSysIndex( const catCollectionInfo& clInfo,
                                           ossPoolVector<BSONObj>& indexList,
                                           pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN addNewIdx = FALSE ;
      BSONObj indexObj ;
      UINT64 idxUniqID = 0 ;

      // main-collection doesn't have $id or $shard index
      if ( clInfo._isMainCL )
      {
         goto done ;
      }
      // the collection which mapped to data source doesn't have index
      if ( clInfo._dsUID != UTIL_INVALID_DS_UID )
      {
         goto done ;
      }

      // build $id index
      if ( clInfo._autoIndexId )
      {
         rc = catGetAndIncIdxUniqID( clInfo._pCLName, cb, w, idxUniqID ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get index unique id, collection: %s, rc: %d",
                      clInfo._pCLName, rc ) ;

         BSONObj idIdx = BSON( IXM_FIELD_NAME_UNIQUEID << (INT64)idxUniqID <<
                               IXM_FIELD_NAME_KEY << BSON( DMS_ID_KEY_NAME << 1 ) <<
                               IXM_FIELD_NAME_NAME << IXM_ID_KEY_NAME <<
                               IXM_FIELD_NAME_UNIQUE << true <<
                               IXM_FIELD_NAME_ENFORCED << true ) ;

         rc = catAddIndex( clInfo._pCLName, idIdx, cb, w,
                           &addNewIdx, &indexObj ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to add index, rc: %d",
                       rc ) ;

         indexList.push_back( indexObj ) ;
      }

      if ( clInfo._isSharding && clInfo._enSureShardIndex )
      {
         if ( clInfo._autoIndexId &&
              0 == clInfo._shardingKey.woCompare( BSON( DMS_ID_KEY_NAME << 1 ) ) )
         {
            PD_LOG( PDWARNING, "$shard index and $id index are the same "
                    "definition. Skip creating $shard index" ) ;
         }
         else
         {
            rc = catGetAndIncIdxUniqID( clInfo._pCLName, cb, w, idxUniqID ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get index unique id, collection: %s, rc: %d",
                         clInfo._pCLName, rc ) ;

            BSONObj shardIdx = BSON( IXM_FIELD_NAME_UNIQUEID << (INT64)idxUniqID <<
                                     IXM_FIELD_NAME_KEY << clInfo._shardingKey <<
                                     IXM_FIELD_NAME_NAME << IXM_SHARD_KEY_NAME ) ;

            rc = catAddIndex( clInfo._pCLName, shardIdx, cb, w,
                              &addNewIdx, &indexObj ) ;
            PD_RC_CHECK ( rc, PDERROR,
                          "Failed to add index, rc: %d",
                          rc ) ;

            indexList.push_back( indexObj ) ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _catCtxCreateCL::_buildP1Reply( BSONObjBuilder &builder )
   {
      // reply eg:
      // { Index: [ { Name: "cs.cl", IndexName: "$id", IndexDef: xxx, ... },
      //            { Name: "cs.cl", IndexName: "$shard", IndexDef: xx, ... } ],

      INT32 rc = SDB_OK ;

      try
      {
         ossPoolVector<BSONObj>::iterator it ;
         BSONArrayBuilder arr( builder.subarrayStart( FIELD_NAME_INDEX ) ) ;
         for ( it = _indexList.begin() ; it != _indexList.end() ; it++ )
         {
            arr.append ( *it ) ;
         }
         arr.done() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATECL_ROLLBACK_INT, "_catCtxCreateCL::_rollbackInternal" )
   INT32 _catCtxCreateCL::_rollbackInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATECL_ROLLBACK_INT ) ;

      rc = catDropCLStep( _targetName, _version, TRUE,
                          cb, _pDmsCB, _pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to drop collection [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCREATECL_ROLLBACK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATECL_COMBINE_OPTS, "_catCtxCreateCL::_combineOptions" )
   INT32 _catCtxCreateCL::_combineOptions( const BSONObj &boDomain,
                                           const BSONObj &boSpace,
                                           UINT32 &fieldMask,
                                           catCollectionInfo &clInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATECL_COMBINE_OPTS ) ;

      /// it is a sysdomain.
      if ( boDomain.isEmpty() )
      {
         goto done ;
      }

      // If the AutoSplit field is not specified, we will use the value of
      // AutoSplit in domain. In that case, AutoSplit field will not stored
      // in record of SYSCAT.SYSCOLLECTIONS
      if ( !( UTIL_CL_AUTOSPLIT_FIELD & fieldMask ) )
      {
         if ( clInfo._isSharding && clInfo._isHash )
         {
            BSONElement split = boDomain.getField( CAT_DOMAIN_AUTO_SPLIT ) ;
            if ( Bool == split.type() )
            {
               clInfo._autoSplit = split.Bool() ;
               // NOTE: no need to store the AutoSplit filed
               // fieldMask |= UTIL_CL_AUTOSPLIT_FIELD ;
            }
         }
      }

      if ( !( UTIL_CL_AUTOREBALANCE_FIELD & fieldMask ) )
      {
         if ( clInfo._isSharding && clInfo._isHash )
         {
            BSONElement rebalance = boDomain.getField( CAT_DOMAIN_AUTO_REBALANCE ) ;
            if ( Bool == rebalance.type() )
            {
               clInfo._autoRebalance = rebalance.Bool() ;
               fieldMask |= UTIL_CL_AUTOREBALANCE_FIELD ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCREATECL_COMBINE_OPTS, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATECL_CHOOSE_GRP, "_catCtxCreateCL::_chooseGroupOfCl" )
   INT32 _catCtxCreateCL::_chooseGroupOfCl( const BSONObj &domainObj,
                                            const BSONObj &csObj,
                                            const catCollectionInfo &clInfo,
                                            _pmdEDUCB *cb,
                                            std::vector<UINT32> &groupIDList,
                                            std::map<std::string, UINT32> &splitRange )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATECL_CHOOSE_GRP ) ;

      if ( NULL != clInfo._gpSpecified )
      {
         rc = _chooseCLGroupBySpec( clInfo._gpSpecified, domainObj, cb,
                                    groupIDList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get group for collection by "
                      "specified, rc: %d", rc ) ;
      }
      else if ( clInfo._autoSplit )
      {
         rc = _chooseCLGroupAutoSplit( domainObj, groupIDList, splitRange ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get groups for auto-split "
                      "collection, rc: %d", rc ) ;
      }
      else if ( UTIL_INVALID_DS_UID == clInfo._dsUID )
      {
         rc = _chooseCLGroupDefault( domainObj, csObj, clInfo._assignType,
                                     cb, groupIDList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get groups for collection, "
                      "rc: %d", rc ) ;
      }

      if ( UTIL_INVALID_DS_UID == clInfo._dsUID )
      {
         rc = catCheckGroupsByID( groupIDList ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to assign group for collection [%s], rc: %d",
                      clInfo._pCLName, rc ) ;
      }

      if ( clInfo._isMainCL )
      {
         /// For main CL only test for available group
         groupIDList.clear() ;
         splitRange.clear() ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCREATECL_CHOOSE_GRP, rc ) ;
      return rc ;

   error :
      groupIDList.clear() ;
      splitRange.clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATECL__CHOOSECLGRPSPEC, "_catCtxCreateCL::_chooseCLGroupBySpec" )
   INT32 _catCtxCreateCL::_chooseCLGroupBySpec ( const CHAR * groupName,
                                                 const BSONObj & domainObj,
                                                 _pmdEDUCB * cb,
                                                 std::vector<UINT32> & groupIDList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXCREATECL__CHOOSECLGRPSPEC ) ;

      BOOLEAN isSysDomain = domainObj.isEmpty() ;
      INT32 tmpGrpID = CAT_INVALID_GROUPID ;

      /// if the group is specified.
      /// 1) whether the group exists.
      /// 2) whether the group is one of the groups of domain.

      // test group first
      rc = catGroupName2ID( groupName, (UINT32 &)tmpGrpID, TRUE, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to convert group name [%s] to id, "
                   "rc: %d", groupName, rc ) ;

      if ( isSysDomain )
      {
         groupIDList.push_back( (UINT32)tmpGrpID ) ;
      }
      else
      {
         std::map<string, UINT32> groupsOfDomain ;
         std::map<string, UINT32>::iterator itr ;

         rc = catGetDomainGroups( domainObj, groupsOfDomain ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get groups from domain "
                      "info [%s], rc: %d", domainObj.toString().c_str(), rc ) ;

         itr = groupsOfDomain.find( groupName ) ;
         PD_CHECK( groupsOfDomain.end() != itr, SDB_CAT_GROUP_NOT_IN_DOMAIN,
                   error, PDERROR, "[%s] is not a group of given domain",
                   groupName ) ;

         groupIDList.push_back( itr->second ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXCREATECL__CHOOSECLGRPSPEC, rc ) ;
      return rc ;

   error :
      groupIDList.clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATECL__CHOOSECLGRPAUTOSPLIT, "_catCtxCreateCL::_chooseCLGroupAutoSplit" )
   INT32 _catCtxCreateCL::_chooseCLGroupAutoSplit ( const BSONObj & domainObj,
                                                    std::vector<UINT32> & groupIDList,
                                                    std::map<std::string, UINT32> & splitRange )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXCREATECL__CHOOSECLGRPAUTOSPLIT ) ;

      BOOLEAN isSysDomain = domainObj.isEmpty() ;

      if ( isSysDomain )
      {
         // Split to all SYS domain groups
         sdbGetCatalogueCB()->getGroupsID( groupIDList, TRUE ) ;
         sdbGetCatalogueCB()->getGroupNameMap( splitRange, TRUE ) ;
      }
      else
      {
         // Split to all domain groups
         rc = catGetDomainGroups( domainObj, groupIDList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get groups from domain info "
                      "[%s], rc: %d", domainObj.toString().c_str(), rc ) ;
         rc = catGetDomainGroups( domainObj, splitRange ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get groups from domain info "
                      "[%s], rc: %d", domainObj.toString().c_str(), rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXCREATECL__CHOOSECLGRPAUTOSPLIT, rc ) ;
      return rc ;

   error :
      splitRange.clear() ;
      groupIDList.clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATECL__CHOOSECLGRPDEF, "_catCtxCreateCL::_chooseCLGroupDefault" )
   INT32 _catCtxCreateCL::_chooseCLGroupDefault ( const BSONObj & domainObj,
                                                  const BSONObj & csObj,
                                                  INT32 assignType,
                                                  _pmdEDUCB * cb,
                                                  std::vector<UINT32> & groupIDList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXCREATECL__CHOOSECLGRPDEF ) ;

      std::vector<UINT32> candidateGroupList ;
      UINT32 tmpGrpID = CAT_INVALID_GROUPID ;
      BOOLEAN isSysDomain = domainObj.isEmpty() ;

      // FOLLOW is given
      if ( ASSIGN_FOLLOW == assignType )
      {
         utilCSUniqueID uniqueID = UTIL_UNIQUEID_NULL ;

         rc = catParseCSUniqueID( csObj, uniqueID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get unique ID of "
                      "collection space [%s], rc: %d",
                      csObj.toPoolString().c_str(), rc ) ;

         rc = catGetCSGroups( uniqueID, cb, FALSE, FALSE, candidateGroupList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get collection space [%s]"
                      " groups, rc: %d", csObj.toString().c_str(), rc ) ;
      }

      // Collection space has domain
      if ( candidateGroupList.empty() && !isSysDomain )
      {
         /// Randomly choose one group in the domain
         rc = catGetDomainGroups( domainObj, candidateGroupList ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get groups from domain info [%s], rc: %d",
                      domainObj.toString().c_str(), rc ) ;
      }

      if ( candidateGroupList.empty() )
      {
         // No candidate groups, choose one from SYS domain
         tmpGrpID = CAT_INVALID_GROUPID ;
         rc = sdbGetCatalogueCB()->getAGroupRand( tmpGrpID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get group from SYS, rc: %d",
                      rc ) ;
      }
      else if ( 1 == candidateGroupList.size() )
      {
         // Got a single group, assign directly
         tmpGrpID = candidateGroupList[ 0 ] ;
      }
      else
      {
         // Got multiple groups, randomly choose one
         tmpGrpID = candidateGroupList[ ossRand() %
                                        candidateGroupList.size() ] ;
      }

      groupIDList.push_back( tmpGrpID ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXCREATECL__CHOOSECLGRPDEF, rc ) ;
      return rc ;

   error :
      groupIDList.clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATECL_GETBOUND, "_catCtxCreateCL::_getBoundFromClObj" )
   INT32 _catCtxCreateCL::_getBoundFromClObj( const BSONObj &clObj,
                                              UINT32 &totalBound )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATECL_GETBOUND ) ;

      BSONElement upBound =
            clObj.getFieldDotted( CAT_CATALOGINFO_NAME ".0." CAT_UPBOUND_NAME ) ;

      if ( Object != upBound.type() )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      {
         BSONElement first = upBound.embeddedObject().firstElement() ;
         if ( NumberInt != first.type() )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         totalBound = first.Int() ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCREATECL_GETBOUND, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * _catCtxDropCL implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxDropCL, RTN_CONTEXT_CAT_DROP_CL,
                          "CAT_DROP_CL" )

   _catCtxDropCL::_catCtxDropCL ( INT64 contextID, UINT64 eduID )
   : _catCtxCLMultiTask( contextID, eduID ),
     _globIdxHandler( _lockMgr ),
     _taskHandler( _lockMgr ),
     _recycleHandler( UTIL_RECYCLE_CL, UTIL_RECYCLE_OP_DROP, _taskHandler,
                      _lockMgr )
   {
      _executeOnP1 = FALSE ;
      _needRollback = FALSE ;
   }

   _catCtxDropCL::~_catCtxDropCL ()
   {
      _onCtxDelete () ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCL_OPEN, "_catCtxDropCL::open" )
   INT32 _catCtxDropCL::open ( const BSONObj &query,
                               rtnContextBuf &buffObj,
                               _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXDROPCL_OPEN ) ;

      rc = _open( query, MSG_CAT_DROP_COLLECTION_REQ, buffObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open drop collection context, "
                   "rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXDROPCL_OPEN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCL__REGEVENTHANDLERS, "_catCtxDropCL::_regEventHandlers" )
   INT32 _catCtxDropCL::_regEventHandlers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXDROPCL__REGEVENTHANDLERS ) ;

      rc = _BASE::_regEventHandlers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register base handlers, "
                   "rc: %d", rc ) ;

      rc = _regEventHandler( &_globIdxHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register global index handler, "
                   "rc: %d", rc ) ;

      rc = _regEventHandler( &_recycleHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register recycle handler, "
                   "rc: %d", rc ) ;

      // NOTE: behavior of task handler depends on recycle handler, so
      // must register task handler after recycle handler
      rc = _regEventHandler( &_taskHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register task handler, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXDROPCL__REGEVENTHANDLERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCL_PARSEQUERY, "_catCtxDropCL::_parseQuery" )
   INT32 _catCtxDropCL::_parseQuery ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCL_PARSEQUERY ) ;

      SDB_ASSERT( MSG_CAT_DROP_COLLECTION_REQ == _cmdType,
                  "Wrong command type" ) ;

      try
      {
         rc = rtnGetSTDStringElement( _boQuery, CAT_COLLECTION_NAME,
                                      _targetName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field %s, rc: %d",
                      CAT_COLLECTION_NAME, rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCL_PARSEQUERY, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCL_CHECK_INT, "_catCtxDropCL::_checkInternal" )
   INT32 _catCtxDropCL::_checkInternal ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCL_CHECK_INT ) ;

      _catCtxDropCLTask *pDropCLTask = NULL ;

      // Add the dropCL task after removing CL from CS
      // If one of them is interupted by multiple times of system crashes, we
      // could use the same dropCL command to continue the drop process.
      rc = _addDropCLTask( _targetName, _version, &pDropCLTask, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to create drop collection task, rc: %d", rc ) ;

      rc = pDropCLTask->checkTask ( cb, _lockMgr ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check collection task, rc: %d", rc ) ;

      _boTarget = pDropCLTask->getDataObj() ;

      rc = _globIdxHandler.addGlobIdxs( pDropCLTask->globalIndexCLList() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add global indexes, "
                   "rc: %d", rc ) ;

      rc = _addDropCLSubTasks ( pDropCLTask, cb ) ;
      PD_RC_CHECK( rc , PDERROR,
                   "Failed to add sub-tasks for drop collection") ;

      rc = _pushExecTask( pDropCLTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to push drop collection task, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCL_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCL__BLDP1REPLY, "_catCtxDropCL::_buildP1Reply" )
   INT32 _catCtxDropCL::_buildP1Reply( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCL__BLDP1REPLY ) ;

      try
      {
         // Version of collection in Coord need to be updated
         builder.append( CAT_COLLECTION, _boTarget ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build reply, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCL__BLDP1REPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCL_DROPCL_SUBTASK, "_catCtxDropCL::_addDropCLSubTasks" )
   INT32 _catCtxDropCL::_addDropCLSubTasks ( _catCtxDropCLTask *pDropCLTask,
                                             _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCL_DROPCL_SUBTASK ) ;

      // For DropCL:
      // 1. if cl is a sub-collection, need unlink it's main-collection
      // 2. if cl is a main-collection, need delete it's sub-collections

      try
      {
         const std::string &clName = pDropCLTask->getDataName() ;
         clsCatalogSet cataSet( clName.c_str() );

         rc = cataSet.updateCatSet( pDropCLTask->getDataObj() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to parse catalog info [%s], rc: %d",
                      clName.c_str(), rc ) ;

         // Since we might need to drop a batch of collections, we use
         // DelCLsFromCSTask to handle the changes to each "Collection" array
         // of the spaces which the collections belong
         _catCtxDelCLsFromCSTask *pDelCLsFromCSTask = NULL ;
         rc = _addDelCLsFromCSTask ( &pDelCLsFromCSTask, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to create delCLsFromCS task, "
                      "rc: %d", rc ) ;
         // Add the dropping collection to DelCLsFromCSTask first
         rc = pDelCLsFromCSTask->deleteCL( clName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to add collection [%s] into delCLsFromCS task, "
                      "rc: %d", clName.c_str(), rc ) ;

         if ( cataSet.isMainCL() )
         {
            // For main-collection
            CLS_SUBCL_LIST subCLLst;
            CLS_SUBCL_LIST_IT iterSubCL;

            rc = cataSet.getSubCLList( subCLLst );
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get sub-collection list of collection [%s], "
                         "rc: %d",
                         clName.c_str(), rc ) ;
            iterSubCL = subCLLst.begin() ;
            while( iterSubCL != subCLLst.end() )
            {
               std::string subCLName = (*iterSubCL) ;

               // Drop sub-collection
               _catCtxDropCLTask *pDropSubCLTask = NULL ;
               rc = _addDropCLTask( subCLName, -1, &pDropSubCLTask ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to create drop sub-collection [%s] task, "
                            "rc: %d",
                            subCLName.c_str(), rc ) ;

               pDropSubCLTask->addIgnoreRC( SDB_DMS_NOTEXIST ) ;

               rc = pDropSubCLTask->checkTask( cb, _lockMgr ) ;
               if ( SDB_DMS_NOTEXIST == rc ||
                    SDB_INVALID_SUB_CL == rc )
               {
                  PD_LOG ( PDWARNING,
                           "Sub-collection [%s] have been changed",
                           subCLName.c_str() ) ;
                  rc = SDB_OK ;
                  ++iterSubCL ;
                  continue;
               }
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to check drop sub-collection [%s] task, "
                            "rc: %d",
                            subCLName.c_str(), rc ) ;

               rc = pDelCLsFromCSTask->deleteCL( subCLName ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to add collection [%s] into delCLsFromCS task, "
                            "rc: %d", subCLName.c_str(), rc ) ;

               rc = _groupHandler.addGroups( pDropSubCLTask->getDataObj() ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to collect groups for sub-collection [%s], "
                            "rc: %d",
                            subCLName.c_str(), rc ) ;

               ++iterSubCL ;
            }
         }
         else
         {
            std::string mainCLName = cataSet.getMainCLName() ;
            if ( !mainCLName.empty() )
            {
               _catCtxUnlinkMainCLTask *pUnlinkMainCLTask = NULL ;
               rc = _addUnlinkMainCLTask( mainCLName, clName, &pUnlinkMainCLTask ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to create unlink main-collection [%s] task, "
                            "rc: %d",
                            mainCLName.c_str(), rc ) ;

               rc = pUnlinkMainCLTask->checkTask( cb, _lockMgr ) ;
               if ( SDB_DMS_NOTEXIST == rc ||
                    SDB_INVALID_MAIN_CL == rc )
               {
                  PD_LOG ( PDWARNING,
                           "Main-collection [%s] have been changed",
                           mainCLName.c_str() ) ;
                  rc = SDB_OK ;
               }
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to check main-collection [%s] task, rc: %d",
                            mainCLName.c_str(), rc ) ;
               pUnlinkMainCLTask->addIgnoreRC( SDB_DMS_NOTEXIST ) ;
               pUnlinkMainCLTask->addIgnoreRC( SDB_INVALID_MAIN_CL ) ;
            }
            rc = _groupHandler.addGroups( pDropCLTask->getDataObj() ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to collect groups for collection [%s], rc: %d",
                         clName.c_str(), rc ) ;
         }

         if ( pDelCLsFromCSTask )
         {
            pDelCLsFromCSTask->checkTask( cb, _lockMgr ) ;
            _pushExecTask( pDelCLsFromCSTask ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCL_DROPCL_SUBTASK, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCL_UNLINKMAINCL_TASK, "_catCtxDropCL::_addUnlinkMainCLTask" )
   INT32 _catCtxDropCL::_addUnlinkMainCLTask ( const std::string &mainCLName,
                                               const std::string &subCLName,
                                               _catCtxUnlinkMainCLTask **ppCtx,
                                               BOOLEAN pushExec )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCL_UNLINKMAINCL_TASK ) ;

      _catCtxUnlinkMainCLTask *pCtx = NULL ;
      pCtx = SDB_OSS_NEW _catCtxUnlinkMainCLTask( mainCLName, subCLName ) ;
      PD_CHECK( pCtx, SDB_SYS, error, PDERROR,
                "Failed to create unlink main-collection [%s/%s] sub-task",
                mainCLName.c_str(), subCLName.c_str() ) ;

      _addTask( pCtx, pushExec ) ;
      if ( ppCtx )
      {
         (*ppCtx) = pCtx ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCL_UNLINKMAINCL_TASK, rc ) ;
      return rc ;
   error :
      SAFE_OSS_DELETE( pCtx ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCL_DELCLCS_TASK, "_catCtxDropCL::_addDelCLsFromCSTask" )
   INT32 _catCtxDropCL::_addDelCLsFromCSTask ( _catCtxDelCLsFromCSTask **ppCtx,
                                               BOOLEAN pushExec )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCL_DELCLCS_TASK ) ;

      _catCtxDelCLsFromCSTask *pCtx = NULL ;
      pCtx = SDB_OSS_NEW _catCtxDelCLsFromCSTask() ;
      PD_CHECK( pCtx, SDB_SYS, error, PDERROR,
                "Failed to add delClsFromCS task" ) ;

      _addTask( pCtx, pushExec ) ;
      if ( ppCtx )
      {
         (*ppCtx) = pCtx ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCL_DELCLCS_TASK, rc ) ;
      return rc ;
   error :
      SAFE_OSS_DELETE( pCtx ) ;
      goto done ;
   }

   /*
      _catCtxRenameCL implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxRenameCL, RTN_CONTEXT_CAT_RENAME_CL,
                          "CAT_RENAME_CL" )

   _catCtxRenameCL::_catCtxRenameCL ( INT64 contextID, UINT64 eduID )
   : _catCtxDataBase( contextID, eduID )
   {
      _executeOnP1 = FALSE ;
      _needRollback = FALSE ;
   }

   _catCtxRenameCL::~_catCtxRenameCL ()
   {
      _onCtxDelete () ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRENAMECL_PARSEQUERY, "_catCtxRenameCL::_parseQuery" )
   INT32 _catCtxRenameCL::_parseQuery ( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCTXRENAMECL_PARSEQUERY ) ;

      SDB_ASSERT( MSG_CAT_RENAME_CL_REQ == _cmdType, "Wrong command type" ) ;

      const CHAR* csName = NULL ;
      const CHAR* oldCLName = NULL ;
      const CHAR* newCLName = NULL ;

      try
      {
         rc = rtnGetStringElement ( _boQuery, CAT_COLLECTIONSPACE, &csName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      CAT_COLLECTIONSPACE, rc ) ;

         rc = rtnGetStringElement( _boQuery, CAT_COLLECTION_OLDNAME,
                                   &oldCLName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      CAT_COLLECTION_OLDNAME, rc ) ;

         rc = rtnGetStringElement( _boQuery, CAT_COLLECTION_NEWNAME,
                                   &newCLName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      CAT_COLLECTION_NEWNAME, rc ) ;

         _targetName = csName ;
         _targetName += "." ;
         _targetName += oldCLName ;

         _newCLFullName = csName ;
         _newCLFullName += "." ;
         _newCLFullName += newCLName ;

         _newCLShortName = newCLName ;
      }
      catch ( exception & e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXRENAMECL_PARSEQUERY, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRENAMECL_CHECK_INT, "_catCtxRenameCL::_checkInternal" )
   INT32 _catCtxRenameCL::_checkInternal ( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCTXRENAMECL_CHECK_INT ) ;

      BOOLEAN newCLExist = FALSE ;
      BSONObj boOldCL, boNewCL ;
      INT64 taskCount = 0 ;
      clsCatalogSet cataSet( _targetName.c_str() );

      try
      {
         // make sure new collection name is valid
         rc = dmsCheckCLName( _newCLShortName.c_str(), FALSE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check collection name [%s], rc: %d",
                      _newCLShortName.c_str(), rc ) ;

         // lock old cl
         rc = catGetAndLockCollection( _targetName, boOldCL,
                                       cb, &_lockMgr, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to lock the collection[%s], rc: %d",
                      _targetName.c_str(), rc ) ;

         // check split task count of old cl
         rc = catGetCLTaskCountByType( _targetName.c_str(), CLS_TASK_SPLIT, cb,
                                       taskCount ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get task count of cl[%s], rc: %d",
                      _targetName.c_str(), rc ) ;

         PD_CHECK( 0 == taskCount, SDB_OPERATION_CONFLICT, error, PDERROR,
                   "Failed to rename cl[%s]: should have no split tasks",
                   _targetName.c_str() ) ;

         // check new cl exists or not
         rc = catCheckCollectionExist( _newCLFullName.c_str(), newCLExist,
                                       boNewCL, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check collection[%s] exists, rc: %d",
                      _newCLFullName.c_str(), rc ) ;
         PD_CHECK( FALSE == newCLExist, SDB_DMS_EXIST, error, PDERROR,
                   "Collection[%s] already exist!",
                   _newCLFullName.c_str() ) ;

         // lock new cl
         if ( !_lockMgr.tryLockCollection( _newCLFullName, EXCLUSIVE ) )
         {
            rc = SDB_LOCK_FAILED ;
            goto error ;
         }

         // we need reply groups list to coord, so that coord can send msg to
         // corresponding data by groups list
         rc = cataSet.updateCatSet( boOldCL ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to parse catalog info[%s], rc: %d",
                      _targetName.c_str(), rc ) ;

         if ( cataSet.isMainCL() )
         {
            CLS_SUBCL_LIST subCLList ;
            CLS_SUBCL_LIST_IT iterSubCL ;

            rc = cataSet.getSubCLList( subCLList );
            PD_RC_CHECK( rc, PDERROR, "Failed to get sub-collection list of "
                         "collection[%s], rc: %d", _targetName.c_str(), rc ) ;

            for ( iterSubCL = subCLList.begin() ;
                  iterSubCL != subCLList.end() ;
                  iterSubCL ++ )
            {
               const string & subCLName = (*iterSubCL) ;
               BSONObj boCollection ;

               rc = catGetCollection( subCLName, boCollection, cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get the collection [%s], "
                            "rc: %d", _targetName.c_str() ) ;

               rc = _groupHandler.addGroups( boCollection ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to collect groups for "
                            "collection [%s], rc: %d", subCLName.c_str(), rc ) ;
            }
         }
         else
         {
            rc = _groupHandler.addGroups( boOldCL ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get group list of cl[%s], rc: %d",
                         _targetName.c_str(), rc ) ;
      }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXRENAMECL_CHECK_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRENAMECL_EXECUTE_INT, "_catCtxRenameCL::_executeInternal" )
   INT32 _catCtxRenameCL::_executeInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATCTXRENAMECL_EXECUTE_INT ) ;

      rc = catRenameCLStep( _targetName, _newCLFullName,
                            cb, _pDmsCB, _pDpsCB, w ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Fail to rename cl from[%s] to [%s], rc: %d",
                    _targetName.c_str(), _newCLFullName.c_str(), rc ) ;

      PD_LOG( PDDEBUG, "Rename collection[%s] to [%s] succeed.",
              _targetName.c_str(), _newCLFullName.c_str() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXRENAMECL_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * _catCtxAlterCL implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxAlterCL, RTN_CONTEXT_CAT_ALTER_CL,
                          "CAT_ALTER_CL" )

   _catCtxAlterCL::_catCtxAlterCL ( INT64 contextID, UINT64 eduID )
   : _catCtxDataMultiTaskBase( contextID, eduID )
   {
      _executeOnP1 = TRUE ;
      _needRollback = TRUE ;
   }

   _catCtxAlterCL::~_catCtxAlterCL ()
   {
      _onCtxDelete () ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL_PARSEQUERY, "_catCtxAlterCL::_parseQuery" )
   INT32 _catCtxAlterCL::_parseQuery ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL_PARSEQUERY ) ;

      BOOLEAN exist = FALSE ;

      SDB_ASSERT( MSG_CAT_ALTER_COLLECTION_REQ == _cmdType,
                  "Wrong command type" ) ;

      try
      {
         BSONObj meta ;
         rc = rtnGetSTDStringElement( _boQuery, CAT_COLLECTION_NAME,
                                      _targetName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_COLLECTION_NAME, rc ) ;

         // Check if the collection is using data source. If yes, it can't
         // be modified.
         rc = catCheckCollectionExist( _targetName.c_str(), exist, meta, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check collection existence failed[%d]",
                      rc ) ;
         if ( !exist )
         {
            rc = SDB_DMS_NOTEXIST ;
            PD_LOG( PDERROR, "Collection[%s] dose not exist",
                    _targetName.c_str() ) ;
            goto error ;
         }

         rc = _alterJob.initialize( _targetName.c_str(), RTN_ALTER_COLLECTION,
                                    _boQuery ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to extract alter job, rc: %d",
                      rc ) ;

         PD_CHECK( RTN_ALTER_COLLECTION == _alterJob.getObjectType(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to extract alter job: wrong type of alter job" ) ;

         PD_CHECK( 1 >= _alterJob.getAlterTasks().size(),
                   SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                   "Failed to extract alter job: not support multiple tasks" ) ;

         /*
          * If altering collection which is using data source, the only allowed
          * operation is to increase version of the table.
          */
         if ( meta.hasField( FIELD_NAME_DATASOURCE_ID ) &&
              ( 1 == _alterJob.getAlterTasks().size() ) )
         {
            rtnAlterTask *task = _alterJob.getAlterTasks().front() ;
            if ( RTN_ALTER_CL_INC_VERSION != task->getActionType() )
            {
               rc = SDB_OPERATION_INCOMPATIBLE ;
               PD_LOG_MSG( PDERROR, "Not allowed to alter a collection which is"
                           " using data source[%d]", rc ) ;
               goto error ;
            }
         }
      }
      catch ( exception & e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXALTERCL_PARSEQUERY, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL_CHECK_INT, "_catCtxAlterCL::_checkInternal" )
   INT32 _catCtxAlterCL::_checkInternal ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL_CHECK_INT ) ;

      if ( _alterJob.isEmpty() )
      {
         goto done ;
      }
      else
      {
         const rtnAlterTask * task = NULL ;

         PD_CHECK( 1 == _alterJob.getAlterTasks().size(),
                   SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                   "Failed to check alter job: should have only one task" ) ;

         task = _alterJob.getAlterTasks().front() ;
         rc = _checkAlterTask( task, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check alter collection command [%s], rc: %d",
                      task->getActionName(), rc ) ;

         if ( task->testFlags( RTN_ALTER_TASK_FLAG_3PHASE ) )
         {
            _executeOnP1 = FALSE ;
         }
         else
         {
            _executeOnP1 = TRUE ;
         }

         if( task->testFlags( RTN_ALTER_TASK_FLAG_SEQUENCE ) )
         {
            _needClearAfterDone = TRUE ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXALTERCL_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL_EXECUTE_INT, "_catCtxAlterCL::_executeInternal" )
   INT32 _catCtxAlterCL::_executeInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL_EXECUTE_INT ) ;

      if ( _alterJob.isEmpty() )
      {
         goto done ;
      }
      else
      {
         const rtnAlterTask * task = NULL ;

         PD_CHECK( 1 == _alterJob.getAlterTasks().size(),
                   SDB_SYS, error, PDERROR,
                   "Failed to check alter job: should have only one task" ) ;

         task = _alterJob.getAlterTasks().front() ;
         rc = _executeAlterTask( task, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to execute alter collection command [%s], rc: %d",
                      task->getActionName(), rc ) ;
      }

      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute Alter Collection command, rc: %d",
                   rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXALTERCL_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL_CLEAR_INT, "_catCtxAlterCL::_clearInternal" )
   INT32 _catCtxAlterCL::_clearInternal(  _pmdEDUCB *cb, INT16 w  )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL_CLEAR_INT ) ;

      if ( _alterJob.isEmpty() )
      {
         goto done ;
      }
      else
      {
         const rtnAlterTask * task = NULL ;

         PD_CHECK( 1 == _alterJob.getAlterTasks().size(), SDB_SYS, error, PDERROR,
                   "Failed to check alter job: should have only one task" ) ;

         task = _alterJob.getAlterTasks().front() ;
         rc = _clearAlterTask( task, cb, w ) ;
      }

      PD_RC_CHECK( rc, PDWARNING, "Failed to clear alter collection command,"
                   "rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXALTERCL_CLEAR_INT, rc ) ;
      return rc ;
   error :
      goto done ;

   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL_CHECK_ALTERTASK, "_catCtxAlterCL::_checkAlterTask" )
   INT32 _catCtxAlterCL::_checkAlterTask ( const rtnAlterTask * task,
                                           _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      INT32 actionType = RTN_ALTER_INVALID_ACTION ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL_CHECK_ALTERTASK ) ;

      set<string> collectionSet ;
      catCtxLockMgr localLockMgr ;
      catCtxAlterCLTask * catTask = NULL ;
      catCtxTaskBase * catAutoIncTask = NULL ;
      catCtxLockMgr * lockMgr = NULL ;

      if ( task->testFlags( RTN_ALTER_TASK_FLAG_CONTEXTLOCK ) )
      {
         lockMgr = &_lockMgr ;
      }
      else
      {
         lockMgr = &localLockMgr ;
      }

      SDB_ASSERT( NULL != lockMgr, "lock manager is invalid" ) ;

      rc = _addSequenceTask( _targetName, task, &catAutoIncTask );
      PD_RC_CHECK( rc, PDERROR, "Failed to add sequence task [%s] on "
                   "collection [%s], rc: %d", task->getActionName(),
                   _targetName.c_str(), rc ) ;

      // do not push task to _execTasks here, because we should push this task
      // after subcl's task
      rc = _addAlterTask( _targetName, task, &catTask, FALSE, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add alter task [%s] on "
                   "collection [%s], rc: %d", task->getActionName(),
                   _targetName.c_str(), rc ) ;

      if( catAutoIncTask )
      {
         rc = catAutoIncTask->checkTask( cb, *lockMgr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check alter task [%s] on "
                      "collection [%s], rc: %d", task->getActionName(),
                      _targetName.c_str(), rc ) ;
      }
      // It will add into collectionSet with other cl.
      // So we can lock cl and lock sharding after.
      catTask->disableLocks() ;

      rc = catTask->checkTask( cb, *lockMgr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check alter task [%s] on "
                   "collection [%s], rc: %d", task->getActionName(),
                   _targetName.c_str(), rc ) ;

      actionType = task->getActionType() ;

      if( actionType != RTN_ALTER_CL_CREATE_AUTOINC_FLD &&
          actionType != RTN_ALTER_CL_DROP_AUTOINC_FLD &&
          actionType != RTN_ALTER_CL_INC_VERSION)
      {
         rc = _addAlterSubCLTask( catTask, cb, *lockMgr, collectionSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add alter sub tasks [%s] on "
                      "collection [%s], rc: %d", task->getActionName(),
                      _targetName.c_str(), rc ) ;
      }

      if( catAutoIncTask )
      {
         rc = _pushExecTask( catAutoIncTask ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to push collection task, "
                      "rc: %d", rc ) ;
      }

      rc = _pushExecTask( catTask ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to push collection task, "
                   "rc: %d", rc ) ;

      // Lock collections
      for ( set<string>::iterator iterCL = collectionSet.begin() ;
            iterCL != collectionSet.end() ;
            iterCL ++ )
      {
         const string & collectionName = (*iterCL) ;
         PD_CHECK( lockMgr->tryLockCollection( (*iterCL), SHARED ),
                   SDB_LOCK_FAILED, error, PDWARNING,
                   "Failed to lock collection [%s]", collectionName.c_str() ) ;

         // Lock collections for sharding
         if ( task->testFlags( RTN_ALTER_TASK_FLAG_SHARDLOCK ) )
         {
            PD_CHECK( _lockMgr.tryLockCollectionSharding( collectionName,
                                                          EXCLUSIVE ),
                      SDB_LOCK_FAILED, error, PDWARNING,
                      "Failed to lock collection [%s] for sharding",
                      collectionName.c_str() ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXALTERCL_CHECK_ALTERTASK, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL_EXECUTE_ALTERTASK, "_catCtxAlterCL::_executeAlterTask" )
   INT32 _catCtxAlterCL::_executeAlterTask ( const rtnAlterTask * task,
                                             _pmdEDUCB * cb,
                                             INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL_EXECUTE_ALTERTASK ) ;

      rc = _catCtxDataMultiTaskBase::_executeInternal( cb, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute alter collection [%s] job, rc: %d",
                   _targetName.c_str(), rc ) ;

      for( UINT32 i = 0 ; i < _execTasks.size() ; i++ )
      {
         catCtxAlterCLTask * task =
                           dynamic_cast<catCtxAlterCLTask *>( _execTasks[i] ) ;
         if( task )
         {
            rc = task->startPostTasks( cb, _pDmsCB, _pDpsCB, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to start post tasks, rc: %d", rc ) ;
         }

      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXALTERCL_EXECUTE_ALTERTASK, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL_CLEAR_ALTERTASK, "_catCtxAlterCL::_clearAlterTask" )
   INT32 _catCtxAlterCL::_clearAlterTask ( const rtnAlterTask * task,
                                             _pmdEDUCB * cb,
                                             INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL_CLEAR_ALTERTASK ) ;

      for( UINT32 i = 0 ; i < _execTasks.size() ; i++ )
      {
         catCtxAlterCLTask * task =
                           dynamic_cast<catCtxAlterCLTask *>( _execTasks[i] ) ;
         if( task )
         {
            rc = task->clearPostTasks( cb, w ) ;
            PD_RC_CHECK( rc, PDWARNING,
                         "Failed to remove post tasks, rc: %d", rc ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXALTERCL_CLEAR_ALTERTASK, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL__ADDALTERTASK, "_catCtxAlterCL::_addAlterTask" )
   INT32 _catCtxAlterCL::_addAlterTask ( const string & collection,
                                         const rtnAlterTask * task,
                                         catCtxAlterCLTask ** catTask,
                                         BOOLEAN pushExec,
                                         BOOLEAN isSubCL )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL__ADDALTERTASK ) ;

      SDB_ASSERT( NULL != task, "task is invalid" ) ;
      SDB_ASSERT( NULL != catTask, "alter task is invalid" ) ;
      catCtxAlterCLTask * tempTask = NULL ;

      tempTask = SDB_OSS_NEW catCtxAlterCLTask( collection, task ) ;
      PD_CHECK( tempTask, SDB_OOM, error, PDERROR,
                "Failed to create alter task [%s] on collection [%s]",
                task->getActionName(), collection.c_str() ) ;

      if( isSubCL )
      {
         tempTask->setSubCLFlag() ;
      }

      _addTask( tempTask, pushExec ) ;
      if ( catTask )
      {
         (*catTask) = tempTask ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCL__ADDALTERTASK, rc ) ;
      return rc ;

   error :
      SAFE_OSS_DELETE( tempTask ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL__ADDALTERSUBCLTASK, "_catCtxAlterCL::_addAlterSubCLTask" )
   INT32 _catCtxAlterCL::_addAlterSubCLTask ( catCtxAlterCLTask * catTask,
                                            pmdEDUCB * cb,
                                            catCtxLockMgr & lockMgr,
                                            set< string > & collectionSet )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCL__ADDALTERSUBCLTASK ) ;

      try
      {
         const string & collectionName = catTask->getDataName() ;
         clsCatalogSet cataSet( collectionName.c_str() ) ;

         rc = cataSet.updateCatSet( catTask->getDataObj() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse catalog info [%s], rc: %d",
                      collectionName.c_str(), rc ) ;

         if ( cataSet.isMainCL() )
         {
            CLS_SUBCL_LIST subCLList ;
            const rtnAlterTask * alterTask = catTask->getTask() ;
            CLS_SUBCL_LIST_IT iterSubCL ;

            PD_CHECK( alterTask->testFlags( RTN_ALTER_TASK_FLAG_MAINCLALLOW ),
                      SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                      "Failed to check alter task [%s]: not supported "
                      "in main-collection", alterTask->getActionName() ) ;

            rc = cataSet.getSubCLList( subCLList );
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get sub-collection list of collection [%s],"
                         " rc: %d", collectionName.c_str(), rc ) ;

            for ( iterSubCL = subCLList.begin() ;
                  iterSubCL != subCLList.end() ;
                  iterSubCL ++ )
            {
               const string & subCLName = (*iterSubCL) ;
               catCtxAlterCLTask * subCLTask = NULL ;
               catCtxTaskBase * subCLAutoIncTask = NULL ;

               rc = _addAlterTask( subCLName, alterTask, &subCLTask,
                                   TRUE, TRUE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to "
                            "add alter task [%s] on collection [%s], rc: %d",
                            alterTask->getActionName(), subCLName.c_str(), rc );

               if( subCLAutoIncTask )
               {
                  rc = subCLAutoIncTask->checkTask( cb, lockMgr ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to check "
                               "alter task [%s] on collection [%s], rc: %d",
                               alterTask->getActionName(),
                               subCLName.c_str(), rc ) ;
               }

               rc = subCLTask->checkTask( cb, lockMgr ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to check "
                            "alter task [%s] on collection [%s], rc: %d",
                            alterTask->getActionName(),
                            subCLName.c_str(), rc ) ;

               rc = _groupHandler.addGroups( subCLTask->getDataObj() ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to collect groups for collection [%s], "
                            "rc: %d",
                            subCLName.c_str(), rc ) ;

               collectionSet.insert( subCLName ) ;
            }
         }
         else
         {
            rc = _groupHandler.addGroups( catTask->getDataObj() ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get groups for collection [%s], rc: %d",
                         collectionName.c_str(), rc ) ;
         }

         collectionSet.insert( collectionName ) ;
      }
      catch( exception & e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCL__ADDALTERSUBCLTASK, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL__ADDSEQUENCETASK, "_catCtxAlterCL::_addSequenceTask" )
   INT32 _catCtxAlterCL::_addSequenceTask( const string & collection,
                                         const rtnAlterTask * task,
                                         catCtxTaskBase ** catAutoIncTask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL__ADDSEQUENCETASK ) ;

      SDB_ASSERT( NULL != task, "task is invalid" ) ;
      catCtxAlterCLTask * tempTask = NULL ;

      switch( task->getActionType() )
      {
         case RTN_ALTER_CL_CREATE_AUTOINC_FLD :
         {
            rc = _addCreateSeqenceTask( collection, task, catAutoIncTask ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add sequence task [%s] on "
                         "collection [%s], rc: %d", task->getActionName(),
                         _targetName.c_str(), rc ) ;
            break ;
         }
         case RTN_ALTER_CL_DROP_AUTOINC_FLD :
         {
            rc = _addDropSeqenceTask( collection, task, catAutoIncTask ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add sequence task [%s] on "
                         "collection [%s], rc: %d", task->getActionName(),
                         _targetName.c_str(), rc ) ;
            break ;
         }
         case RTN_ALTER_CL_SET_ATTRIBUTES :
         {
            const rtnCLSetAttributeTask *setTask =
                        dynamic_cast< const rtnCLSetAttributeTask * >( task ) ;
            if( setTask->containAutoincArgument() )
            {
               rc = _addAlterSeqenceTask( collection, task, catAutoIncTask ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to add sequence task [%s] on "
                            "collection [%s], rc: %d", task->getActionName(),
                            _targetName.c_str(), rc ) ;
            }
            break ;
         }
         default :
         {
            rc = SDB_OK ;
            break ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCL__ADDSEQUENCETASK, rc ) ;
      return rc ;

   error :
      SAFE_OSS_DELETE( tempTask ) ;
      goto done ;

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL_ADDCRTSEQ_TASK, "_catCtxAlterCL::_addCreateSeqenceTask" )
   INT32 _catCtxAlterCL::_addCreateSeqenceTask( const string & collection,
                                                const rtnAlterTask * task,
                                                catCtxTaskBase ** catAutoIncTask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL_ADDCRTSEQ_TASK ) ;

      SDB_ASSERT( NULL != task, "task is invalid" ) ;
      SDB_ASSERT( NULL != catAutoIncTask, "autoinc task is invalid" ) ;

      catCtxTaskBase * tempSeqTask = NULL ;

      tempSeqTask = SDB_OSS_NEW catCtxCreateSequenceTask( collection, task );
      PD_CHECK( tempSeqTask, SDB_OOM, error, PDERROR,
                "Failed to get create sequence task [%s] on collection [%s]",
                task->getActionName(), collection.c_str() ) ;
      _addTask( tempSeqTask, false ) ;

      *catAutoIncTask = tempSeqTask ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCL_ADDCRTSEQ_TASK, rc ) ;
      return rc ;

   error :
      SAFE_OSS_DELETE( tempSeqTask ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL_ADDDROPSEQ_TASK, "_catCtxAlterCL::_addDropSeqenceTask" )
   INT32 _catCtxAlterCL::_addDropSeqenceTask( const string & collection,
                                              const rtnAlterTask * task,
                                              catCtxTaskBase ** catAutoIncTask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL_ADDDROPSEQ_TASK ) ;

      SDB_ASSERT( NULL != task, "task is invalid" ) ;
      SDB_ASSERT( NULL != catAutoIncTask, "autoinc task is invalid" ) ;

      catCtxTaskBase * tempSeqTask = NULL ;

      tempSeqTask = SDB_OSS_NEW catCtxDropSequenceTask( collection, task );
      PD_CHECK( tempSeqTask, SDB_OOM, error, PDERROR,
                "Failed to get drop sequence task [%s] on collection [%s]",
                task->getActionName(), collection.c_str() ) ;
      _addTask( tempSeqTask, false ) ;

      *catAutoIncTask = tempSeqTask ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCL_ADDDROPSEQ_TASK, rc ) ;
      return rc ;

   error :
      SAFE_OSS_DELETE( tempSeqTask ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL_ADDALTERSEQ_TASK, "_catCtxAlterCL::_addAlterSeqenceTask" )
   INT32 _catCtxAlterCL::_addAlterSeqenceTask( const string &collection,
                                               const rtnAlterTask *task,
                                               catCtxTaskBase **catAutoIncTask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL_ADDALTERSEQ_TASK ) ;

      SDB_ASSERT( NULL != task, "task is invalid" ) ;
      SDB_ASSERT( NULL != catAutoIncTask, "autoinc task is invalid" ) ;

      catCtxTaskBase * tempSeqTask = NULL ;

      tempSeqTask = SDB_OSS_NEW catCtxAlterSequenceTask( collection, task );
      PD_CHECK( tempSeqTask, SDB_OOM, error, PDERROR,
                "Failed to get alter sequence task [%s] on collection [%s]",
                task->getActionName(), collection.c_str() ) ;
      _addTask( tempSeqTask, false ) ;

      *catAutoIncTask = tempSeqTask ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCL_ADDALTERSEQ_TASK, rc ) ;
      return rc ;

   error :
      SAFE_OSS_DELETE( tempSeqTask ) ;
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL__BUILDTASKREPLY, "_catCtxAlterCL::_buildTaskReply" )
   INT32 _catCtxAlterCL::_buildTaskReply( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCTXALTERCL__BUILDTASKREPLY ) ;

      if ( _execTasks.size() > 0 )
      {
         try
         {
            /* message format:
            *  { TaskID: [ 1, 2, ... ],
            *    Index: [ { Collection: "foo.bar", IndexDef: xxx },
            *             { Collection: "foo.bar", IndexDef: xxx },
            *             ...
            *           ]
            *  }
            */
            // Generate task list
            BSONArrayBuilder taskBuilder(
                                    builder.subarrayStart( CAT_TASKID_NAME ) ) ;
            for( UINT32 i = 0 ; i < _execTasks.size() ; i++ )
            {
               catCtxAlterCLTask * task =
                              dynamic_cast<catCtxAlterCLTask *>( _execTasks[i] ) ;
               if( task )
               {
                  const ossPoolList< UINT64 > & postTasks = task->getPostTasks() ;
                  for ( ossPoolList<UINT64>::const_iterator it = postTasks.begin();
                        it != postTasks.end() ;
                        it ++ )
                  {
                     taskBuilder.append( (INT64)( *it ) ) ;
                  }
               }
            }
            taskBuilder.done() ;
            // Generate index list
            BSONArrayBuilder idxBuilder(
                                    builder.subarrayStart( FIELD_NAME_INDEX ) ) ;
            for( UINT32 i = 0 ; i < _execTasks.size() ; i++ )
            {
               catCtxAlterCLTask * task =
                              dynamic_cast<catCtxAlterCLTask *>( _execTasks[i] ) ;
               if( task )
               {
                  const ossPoolList<BSONObj> & indexes = task->getIndexes() ;
                  for ( ossPoolList<BSONObj>::const_iterator it = indexes.begin() ;
                        it != indexes.end() ; it++ )
                  {
                     idxBuilder.append( BSON( FIELD_NAME_COLLECTION <<
                                              task->getDataName().c_str() <<
                                              IXM_FIELD_NAME_INDEX_DEF << *it ) ) ;
                  }
               }
            }
            idxBuilder.done() ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build task reply, "
                    "occur exception %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
         }
      }

      PD_TRACE_EXITRC( SDB_CATCTXALTERCL__BUILDTASKREPLY, rc ) ;
      return rc ;
   }

   /*
    * _catCtxLinkCL implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxLinkCL, RTN_CONTEXT_CAT_LINK_CL,
                          "CAT_LINK_CL" )

   _catCtxLinkCL::_catCtxLinkCL ( INT64 contextID, UINT64 eduID )
   : _catCtxDataBase( contextID, eduID )
   {
      _executeOnP1 = TRUE ;
      _needRollback = TRUE ;

      _needUpdateSubCL = FALSE ;
   }

   _catCtxLinkCL::~_catCtxLinkCL ()
   {
      _onCtxDelete () ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXLINKCL_PARSEQUERY, "_catCtxLinkCL::_parseQuery" )
   INT32 _catCtxLinkCL::_parseQuery ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXLINKCL_PARSEQUERY ) ;

      SDB_ASSERT( MSG_CAT_LINK_CL_REQ == _cmdType,
                  "Wrong command type" ) ;

      try
      {
         rc = rtnGetSTDStringElement( _boQuery, CAT_COLLECTION_NAME, _targetName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_COLLECTION_NAME, rc ) ;

         rc = rtnGetSTDStringElement( _boQuery, CAT_SUBCL_NAME, _subCLName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_SUBCL_NAME, rc ) ;

         rc = rtnGetObjElement( _boQuery, CAT_LOWBOUND_NAME, _lowBound ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_LOWBOUND_NAME, rc ) ;

         rc = rtnGetObjElement( _boQuery, CAT_UPBOUND_NAME, _upBound ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_UPBOUND_NAME, rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXLINKCL_PARSEQUERY, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXLINKCL_CHECK_INT, "_catCtxLinkCL::_checkInternal" )
   INT32 _catCtxLinkCL::_checkInternal ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXLINKCL_CHECK_INT ) ;

      string tmpMainCLName ;

      // 1. check main-collection
      rc = catGetAndLockCollection( _targetName, _boTarget, cb,
                                    &_lockMgr, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the main-collection [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

      // Main-collection should be main collection
      rc = catCheckMainCollection( _boTarget, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Collection [%s] is not a main-collection, rc: %d",
                   _targetName.c_str(), rc ) ;

      // 2. check sub-collection
      if ( 0 == _targetName.compare( _subCLName ) )
      {
         // Avoid repeating locked, report error eventually
         _boSubCL = _boTarget ;
      }
      else
      {
         rc = catGetAndLockCollection( _subCLName, _boSubCL, cb,
                                       &_lockMgr, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get the sub-collection [%s], rc: %d",
                      _subCLName.c_str(), rc ) ;
      }

      // Sub-collection should not be main collection
      rc = catCheckMainCollection( _boSubCL, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Collection [%s] could not be a main-collection, rc: %d",
                   _subCLName.c_str(), rc ) ;

      // Check if sub-collection already linked
      rc = catCheckRelinkCollection ( _boSubCL, tmpMainCLName ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Duplicate attach collection partition [%s], "
                   "its partitioned-collection is %s",
                   _subCLName.c_str(), tmpMainCLName.c_str() ) ;

      // Check if multiple collections on data source are being linked to the
      // main collection. Currently this is not supported.
      rc = catCheckLinkMultiDSCollection( _boTarget, _boSubCL, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Check multiple collections on data source "
                   "attached to main collection failed[%d]", rc ) ;

      rc = _groupHandler.addGroups( _boSubCL ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get group info from "
                   "collection record [%s], rc: %d",
                   _boSubCL.toPoolString().c_str(), rc ) ;

      PD_RC_CHECK( rc, PDERROR,
                   "Failed to lock groups, rc: %d",
                   rc ) ;

      _needUpdateSubCL = TRUE ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXLINKCL_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXLINKCL_EXECUTE_INT, "_catCtxLinkCL::_executeInternal" )
   INT32 _catCtxLinkCL::_executeInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXLINKCL_EXECUTE_INT ) ;

      BOOLEAN subCLUpdated = FALSE ;

      // Always update the sub-collection first, so if error happens between
      // these two steps, we could always clean the wrong status with unlink

      if ( _needUpdateSubCL )
      {
         // update sub-collection catalog info
         rc = catLinkSubCLStep( _targetName, _subCLName, cb, _pDmsCB, _pDpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update the sub-collection [%s], rc: %d",
                      _subCLName.c_str(), rc ) ;

         subCLUpdated = TRUE ;
      }

      // update main-collection catalog info
      rc = catLinkMainCLStep( _targetName, _subCLName, _lowBound, _upBound,
                              cb, _pDmsCB, _pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to update the main-collection [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXLINKCL_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      if ( subCLUpdated )
      {
         // Rollback sub-collection immediately
         INT32 tmpRC = SDB_OK ;
         tmpRC = catUnlinkSubCLStep( _subCLName,
                                     cb, _pDmsCB, _pDpsCB, w ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING,
                    "Failed to rollback the sub-collection [%s], rc: %d",
                    _subCLName.c_str(), tmpRC ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXLINKCL_ROLLBACK_INT, "_catCtxLinkCL::_rollbackInternal" )
   INT32 _catCtxLinkCL::_rollbackInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXLINKCL_ROLLBACK_INT ) ;

      INT32 tmpRC = SDB_OK ;
      BSONObj dummyLowBound, dummyUpBound ;

      // Always update the sub-collection first, so if error happens between
      // these two steps, we could always clean the wrong status with unlink

      if ( _needUpdateSubCL )
      {
         // Rollback sub-collection update
         tmpRC = catUnlinkSubCLStep( _subCLName, cb, _pDmsCB, _pDpsCB, w ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING,
                    "Failed to rollback the sub-collection [%s], rc: %d",
                    _subCLName.c_str(), tmpRC ) ;
            rc = tmpRC ;
         }
      }

      // Rollback main-collection update
      tmpRC = catUnlinkMainCLStep( _targetName, _subCLName,
                                   FALSE, dummyLowBound, dummyUpBound,
                                   cb, _pDmsCB, _pDpsCB, w ) ;
      if ( SDB_OK != tmpRC )
      {
         PD_LOG( PDWARNING,
                 "Failed to rollback the main-collection [%s], rc: %d",
                 _targetName.c_str(), tmpRC ) ;
         rc = tmpRC ;
      }

      PD_TRACE_EXITRC ( SDB_CATCTXLINKCL_ROLLBACK_INT, rc ) ;
      return rc ;
   }

   /*
    *  _catCtxUnlinkCL implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxUnlinkCL, RTN_CONTEXT_CAT_UNLINK_CL,
                          "CAT_UNLINK_CL" )

   _catCtxUnlinkCL::_catCtxUnlinkCL ( INT64 contextID, UINT64 eduID )
   : _catCtxDataBase( contextID, eduID )
   {
      _executeOnP1 = TRUE ;
      _needRollback = TRUE ;

      _needUpdateSubCL = FALSE ;
   }

   _catCtxUnlinkCL::~_catCtxUnlinkCL ()
   {
      _onCtxDelete () ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXUNLINKCL_PARSEQUERY, "_catCtxUnlinkCL::_parseQuery" )
   INT32 _catCtxUnlinkCL::_parseQuery ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXUNLINKCL_PARSEQUERY ) ;

      SDB_ASSERT( MSG_CAT_UNLINK_CL_REQ == _cmdType,
                  "Wrong command type" ) ;

      try
      {
         rc = rtnGetSTDStringElement( _boQuery, CAT_COLLECTION_NAME, _targetName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_COLLECTION_NAME, rc ) ;

         rc = rtnGetSTDStringElement( _boQuery, CAT_SUBCL_NAME, _subCLName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_SUBCL_NAME, rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_CATCTXUNLINKCL_PARSEQUERY, rc ) ;
      return rc;
   error :
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXUNLINKCL_CHECK_INT, "_catCtxUnlinkCL::_checkInternal" )
   INT32 _catCtxUnlinkCL::_checkInternal ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXUNLINKCL_CHECK_INT ) ;

      string tmpMainCLName ;

      // 1. check main-collection
      rc = catGetAndLockCollection( _targetName, _boTarget, cb,
                                    &_lockMgr, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the main-collection [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

      // Main-collection should be main collection
      rc = catCheckMainCollection( _boTarget, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Collection[%s] is not a main-collection, rc: %d",
                   _targetName.c_str(), rc ) ;

      // 2. check sub-collection
      if ( 0 == _targetName.compare( _subCLName ) )
      {
         // Avoid repeating locked, report error eventually
         _boSubCL = _boTarget ;
      }
      else
      {
         rc = catGetAndLockCollection( _subCLName, _boSubCL, cb,
                                       &_lockMgr, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get the sub-collection [%s], rc: %d",
                      _subCLName.c_str(), rc ) ;
      }

      // Sub-collection should not be main collection
      rc = catCheckMainCollection( _boSubCL, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Collection [%s] could not be a main-collection, rc: %d",
                   _subCLName.c_str(), rc ) ;

      // Check if sub-collection already linked
      rc = catCheckRelinkCollection ( _boSubCL, tmpMainCLName ) ;
      if ( rc == SDB_RELINK_SUB_CL )
      {
         PD_CHECK( 0 == tmpMainCLName.compare( _targetName ),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to unlink sub-collection [%s], "
                   "the original main-collection is %s not %s",
                   _subCLName.c_str(), tmpMainCLName.c_str(),
                   _targetName.c_str() ) ;
         _needUpdateSubCL = TRUE ;
      }
      else
      {
         PD_LOG( PDWARNING,
                 "Sub-collection [%s] hasn't been linked",
                 _subCLName.c_str() ) ;

         // Check if the mainCL contain the subCL, if so, still need
         // to update mainCL to remove the item in its the subCL list
         clsCatalogSet mainCLSet( _targetName.c_str() ) ;

         rc = mainCLSet.updateCatSet( _boTarget ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to parse catalog-info of main-collection(%s)",
                      _targetName.c_str() ) ;
         PD_CHECK( mainCLSet.isContainSubCL( _subCLName ),
                   SDB_INVALID_SUB_CL, error, PDERROR,
                   "Failed to unlink sub-collection, the main-collection"
                   "[%s] doesn't contain sub-collection [%s]",
                   _targetName.c_str(), _subCLName.c_str() ) ;

         _needUpdateSubCL = FALSE ;
      }

      rc = _groupHandler.addGroups( _boSubCL ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get group info from "
                   "collection record [%s], rc: %d",
                   _boSubCL.toPoolString().c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXUNLINKCL_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXUNLINKCL_EXECUTE_INT, "_catCtxUnlinkCL::_executeInternal" )
   INT32 _catCtxUnlinkCL::_executeInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXUNLINKCL_EXECUTE_INT ) ;

      BOOLEAN subCLUpdated = FALSE ;

      // Always update the sub-collection first, so if error happens between
      // these two steps, we could always clean the wrong status with unlink

      if ( _needUpdateSubCL )
      {
         // update main-collection catalog info
         rc = catUnlinkSubCLStep( _subCLName, cb, _pDmsCB, _pDpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update the sub-collection [%s], rc: %d",
                      _subCLName.c_str(), rc ) ;

         subCLUpdated = TRUE ;
      }

      // update main-collection catalog info
      rc = catUnlinkMainCLStep( _targetName, _subCLName,
                                TRUE, _lowBound, _upBound,
                                cb, _pDmsCB, _pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to update the main-collection [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXUNLINKCL_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      if ( subCLUpdated )
      {
         /// Rollback sub-collection immediately
         INT32 tmpRC = SDB_OK ;
         tmpRC = catLinkSubCLStep( _targetName, _subCLName,
                                   cb, _pDmsCB, _pDpsCB, w ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING,
                    "Failed to rollback the sub-collection [%s], rc: %d",
                    _subCLName.c_str(), tmpRC ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXUNLINKCL_ROLLBACK_INT, "_catCtxUnlinkCL::_rollbackInternal" )
   INT32 _catCtxUnlinkCL::_rollbackInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXUNLINKCL_ROLLBACK_INT ) ;

      INT32 tmpRC = SDB_OK ;

      // Always update the sub-collection first, so if error happens between
      // these two steps, we could always clean the wrong status with unlink

      if ( _needUpdateSubCL )
      {
         // Rollback sub-collection update
         tmpRC = catLinkSubCLStep( _targetName, _subCLName,
                                   cb, _pDmsCB, _pDpsCB, w ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING,
                    "Failed to rollback the sub-collection[%s], rc: %d",
                    _subCLName.c_str(), tmpRC ) ;
            rc = tmpRC ;
         }
      }

      // Make sure lowBound and upBound are valid
      if ( !_lowBound.isEmpty() && !_upBound.isEmpty() )
      {
         // Rollback main-collection update
         tmpRC = catLinkMainCLStep( _targetName, _subCLName,
                                    _lowBound, _upBound,
                                    cb, _pDmsCB, _pDpsCB, w ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING,
                    "Failed to rollback the main-collection[%s], rc: %d",
                    _targetName.c_str(), tmpRC ) ;
            rc = tmpRC ;
         }
      }


      PD_TRACE_EXITRC ( SDB_CATCTXUNLINKCL_ROLLBACK_INT, rc ) ;
      return rc ;
   }

   /*
      _catCtxTruncCL implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxTruncCL, RTN_CONTEXT_CAT_TRUNCATE_CL,
                          "CAT_TRUNCATE_CL" )

   _catCtxTruncCL::_catCtxTruncCL( INT64 contextID, UINT64 eduID )
   : _catCtxDataBase( contextID, eduID ),
     _globIdxHandler( _lockMgr ),
     _taskHandler( _lockMgr ),
     _recycleHandler( UTIL_RECYCLE_CL, UTIL_RECYCLE_OP_TRUNCATE, _taskHandler,
                      _lockMgr )
   {
      _executeOnP1 = FALSE ;
      _needRollback = FALSE ;
   }

   _catCtxTruncCL::~_catCtxTruncCL()
   {
      _onCtxDelete() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXTRUNCCL__REGEVENTHANDLERS, "_catCtxTruncCL::_regEventHandlers" )
   INT32 _catCtxTruncCL::_regEventHandlers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXTRUNCCL__REGEVENTHANDLERS ) ;

      rc = _BASE::_regEventHandlers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register base handlers, "
                   "rc: %d", rc ) ;

      rc = _regEventHandler( &_globIdxHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register global index handler, "
                   "rc: %d", rc ) ;

      rc = _regEventHandler( &_recycleHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register recycle handler, "
                   "rc: %d", rc ) ;

      // NOTE: behavior of task handler depends on recycle handler, so
      // must register task handler after recycle handler
      rc = _regEventHandler( &_taskHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register task handler, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXTRUNCCL__REGEVENTHANDLERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXTRUNCCL_PARSEQUERY, "_catCtxTruncCL::_parseQuery" )
   INT32 _catCtxTruncCL::_parseQuery ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXTRUNCCL_PARSEQUERY ) ;

      SDB_ASSERT( MSG_CAT_TRUNCATE_REQ == _cmdType,
                  "Wrong command type" ) ;

      try
      {
         rc = rtnGetSTDStringElement( _boQuery, CAT_COLLECTION, _targetName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field %s, rc: %d",
                      CAT_COLLECTION, rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse truncate command, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXTRUNCCL_PARSEQUERY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXTRUNCCL__CHKINT, "_catCtxTruncCL::_checkInternal" )
   INT32 _catCtxTruncCL::_checkInternal( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXTRUNCCL__CHKINT ) ;

      rc = catGetAndLockCollection( _targetName, _boTarget, cb, &_lockMgr,
                                    EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the collection [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

      if ( _boTarget.hasField( FIELD_NAME_DATASOURCE_ID ) )
      {
         UTIL_DS_UID dsUID = UTIL_INVALID_DS_UID ;

         // check data source ID
         rc = catCheckDataSourceID( _boTarget, dsUID ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to check data source ID from "
                      "collection space [%s], rc: %d", _targetName.c_str(),
                      rc ) ;

         rc = _groupHandler.addGroup( SDB_DSID_2_GROUPID( dsUID ) ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add data source "
                      "group ID, rc: %d", rc ) ;
      }

      try
      {
          clsCatalogSet cataSet( _targetName.c_str() ) ;

          rc = cataSet.updateCatSet( _boTarget ) ;
          PD_RC_CHECK( rc, PDERROR, "Failed to parse catalog for collection "
                       "[%s], rc: %d", _targetName.c_str(), rc ) ;

          if ( cataSet.isMainCL() )
          {
             // For main-collection
             CLS_SUBCL_LIST subCLList;

             rc = cataSet.getSubCLList( subCLList );
             PD_RC_CHECK( rc, PDERROR, "Failed to get sub-collection list of "
                          "collection [%s], rc: %d", _targetName.c_str(), rc ) ;
             for ( CLS_SUBCL_LIST_IT iterSubCL = subCLList.begin() ;
                   iterSubCL != subCLList.end() ;
                   ++ iterSubCL )
             {
                const string &subCLName = ( *iterSubCL ) ;
                BSONObj subCLObject ;

                rc = catGetAndLockCollection( subCLName, subCLObject, cb,
                                              &_lockMgr, EXCLUSIVE ) ;
                PD_RC_CHECK( rc, PDERROR, "Failed to get the collection [%s], "
                             "rc: %d", subCLName.c_str(), rc ) ;

                rc = _groupHandler.addGroups( subCLObject ) ;
                PD_RC_CHECK( rc, PDERROR, "Failed to collect groups for "
                             "sub-collection [%s], rc: %d", subCLName.c_str(),
                             rc ) ;

                // get collection's global index
                rc = _globIdxHandler.addGlobIdxs( subCLName.c_str() ) ;
                PD_RC_CHECK( rc, PDWARNING,
                             "Failed to get collection[%s]'s global indexes, "
                             "rc: %d", subCLName.c_str(), rc ) ;
             }
          }
          else
          {
             rc = _groupHandler.addGroups( _boTarget ) ;
             PD_RC_CHECK( rc, PDERROR, "Failed to collect groups for "
                          "collection [%s], rc: %d", _targetName.c_str(),
                          rc ) ;

             // get collection's global index
             rc = _globIdxHandler.addGlobIdxs( _targetName.c_str() ) ;
             PD_RC_CHECK( rc, PDWARNING,
                          "Failed to get collection[%s]'s global indexes, "
                          "rc: %d", _targetName.c_str(), rc ) ;
          }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to check collection, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXTRUNCCL__CHKINT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXTRUNCCL__BLDP1REPLY, "_catCtxTruncCL::_buildP1Reply" )
   INT32 _catCtxTruncCL::_buildP1Reply( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXTRUNCCL__BLDP1REPLY ) ;

      try
      {
         // Version of collection in Coord need to be updated
         builder.append( CAT_COLLECTION, _boTarget ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build reply, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXTRUNCCL__BLDP1REPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXTRUNCCL__EXEINT, "_catCtxTruncCL::_executeInternal" )
   INT32 _catCtxTruncCL::_executeInternal( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXTRUNCCL__EXEINT ) ;

      catSequenceManager *seqMgr = _pCatCB->getCatGTSMgr()->getSequenceMgr() ;

      try
      {
         BSONElement beAutoInc = _boTarget.getField( CAT_AUTOINCREMENT ) ;
         if ( EOO == beAutoInc.type() )
         {
            goto done ;
         }
         PD_CHECK( Array == beAutoInc.type(),
                   SDB_CAT_CORRUPTION, error, PDERROR,
                   "Failed to get auto-increment info, it is not array" ) ;

         {
            BSONObjIterator it( beAutoInc.embeddedObject() ) ;
            while ( it.more() )
            {
               BSONObj boAutoIncItem ;
               string seqName ;
               BSONElement ele = it.next() ;
               PD_CHECK( Object == ele.type(),
                         SDB_CAT_CORRUPTION, error, PDERROR,
                         "Failed to get auto-increment, it is not object" ) ;

               boAutoIncItem = ele.embeddedObject() ;
               rc = rtnGetSTDStringElement( boAutoIncItem, CAT_AUTOINC_SEQ,
                                            seqName ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get sequence name, "
                            "rc: %d", rc ) ;

               rc = seqMgr->resetSequence( seqName, cb, w ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to reset sequence[%s], "
                            "rc: %d", seqName.c_str(), rc ) ;
            }
         }

         PD_LOG( PDDEBUG, "truncated cl[%s]", _targetName.c_str() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to execute truncate, occur exception %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXTRUNCCL__EXEINT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
