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

using namespace bson ;

namespace engine
{
   /*
    * _catCtxDataBase implement
    */
   _catCtxDataBase::_catCtxDataBase ( INT64 contextID, UINT64 eduID )
   : _catContextBase( contextID, eduID )
   {
   }

   _catCtxDataBase::~_catCtxDataBase ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDATA_MAKEREPLY, "_catCtxDataBase::_makeReply" )
   INT32 _catCtxDataBase::_makeReply ( rtnContextBuf &buffObj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDATA_MAKEREPLY ) ;

      if ( !_groupList.empty() )
      {
         if ( ( CAT_CONTEXT_READY == _status && !_executeAfterLock ) ||
              ( CAT_CONTEXT_CAT_DONE == _status && _executeAfterLock ) )
         {
            BSONObjBuilder retObjBuilder ;
            _pCatCB->makeGroupsObj( retObjBuilder, _groupList, TRUE ) ;
            buffObj = rtnContextBuf( retObjBuilder.obj() ) ;
         }
         else if ( CAT_CONTEXT_END != _status )
         {
            BSONObj dummy ;
            buffObj = rtnContextBuf( dummy.getOwned() ) ;
         }
      }
      else
      {
      }

      PD_TRACE_EXITRC ( SDB_CATCTXDATA_MAKEREPLY, rc ) ;

      return rc ;
   }

   /*
    * _catCtxDataMultiTaskBase implement
    */
   _catCtxDataMultiTaskBase::_catCtxDataMultiTaskBase ( INT64 contextID,
                                                        UINT64 eduID )
   : _catCtxDataBase( contextID, eduID )
   {
      _needRollbackAlways = TRUE ;
   }

   _catCtxDataMultiTaskBase::~_catCtxDataMultiTaskBase ()
   {
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

   void _catCtxDataMultiTaskBase::_addTask ( _catCtxDataTask *pCtx,
                                             BOOLEAN pushExec )
   {
      _subTasks.push_back( pCtx ) ;
      if ( pushExec )
      {
         _execTasks.push_back( pCtx ) ;
      }
   }

   INT32 _catCtxDataMultiTaskBase::_pushExecTask ( _catCtxDataTask *pCtx )
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
                                              BOOLEAN pushExec )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCL_DROPCL_TASK ) ;

      _catCtxDropCLTask *pCtx = NULL ;
      pCtx = SDB_OSS_NEW _catCtxDropCLTask( clName, version ) ;
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
    * _catCtxIndexMultiTask implement
    */
   _catCtxIndexMultiTask::_catCtxIndexMultiTask ( INT64 contextID, UINT64 eduID )
   : _catCtxDataMultiTaskBase( contextID, eduID )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXINDEX_CREATEIDX_TASK, "_catCtxIndexMultiTask::_addCreateIdxTask" )
   INT32 _catCtxIndexMultiTask::_addCreateIdxTask ( const std::string &clName,
                                                    const std::string &idxName,
                                                    const BSONObj &boIdx,
                                                    _catCtxCreateIdxTask **ppCtx,
                                                    BOOLEAN pushExec )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXINDEX_CREATEIDX_TASK ) ;

      _catCtxCreateIdxTask *pCtx = NULL ;
      pCtx = SDB_OSS_NEW _catCtxCreateIdxTask( clName, idxName, boIdx ) ;
      PD_CHECK( pCtx, SDB_SYS, error, PDERROR,
                "Failed to add create index [%s/%s] task",
                clName.c_str(), idxName.c_str() ) ;

      _addTask( pCtx, pushExec ) ;
      if ( ppCtx )
      {
         (*ppCtx) = pCtx ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXINDEX_CREATEIDX_TASK, rc ) ;
      return rc ;
   error :
      SAFE_OSS_DELETE( pCtx ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXINDEX_DROPIDX_TASK, "_catCtxIndexMultiTask::_addDropIdxTask" )
   INT32 _catCtxIndexMultiTask::_addDropIdxTask ( const std::string &clName,
                                                  const std::string &idxName,
                                                  _catCtxDropIdxTask **ppCtx,
                                                  BOOLEAN pushExec )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXINDEX_DROPIDX_TASK ) ;

      _catCtxDropIdxTask *pCtx = NULL ;
      pCtx = SDB_OSS_NEW _catCtxDropIdxTask( clName, idxName ) ;
      PD_CHECK( pCtx, SDB_SYS, error, PDERROR,
                "Failed to add drop index [%s/%s] task",
                clName.c_str(), idxName.c_str() ) ;

      _addTask( pCtx, pushExec ) ;
      if ( ppCtx )
      {
         (*ppCtx) = pCtx ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXINDEX_DROPIDX_TASK, rc ) ;
      return rc ;
   error :
      SAFE_OSS_DELETE( pCtx ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXINDEX_CREATEIDX_SUBTASK, "_catCtxIndexMultiTask::_addCreateIdxSubTasks" )
   INT32 _catCtxIndexMultiTask::_addCreateIdxSubTasks ( _catCtxCreateIdxTask *pCreateIdxTask,
                                                        catCtxLockMgr &lockMgr,
                                                        _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXINDEX_CREATEIDX_SUBTASK ) ;

      try
      {
         std::set<UINT32> checkedKeyIDs ;
         BOOLEAN uniqueCheck = pCreateIdxTask->needUniqueCheck() ;
         const std::string &clName = pCreateIdxTask->getDataName() ;
         const std::string &idxName = pCreateIdxTask->getIdxName() ;
         const BSONObj &boIdx = pCreateIdxTask->getIdxObj() ;
         clsCatalogSet cataSet( clName.c_str() );

         rc = cataSet.updateCatSet( pCreateIdxTask->getDataObj() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to parse catalog info [%s], rc: %d",
                      clName.c_str(), rc ) ;

         if ( uniqueCheck )
         {
            rc = pCreateIdxTask->checkIndexKey( cataSet, checkedKeyIDs ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to check index key [%s] on collection [%s], "
                         "rc: %d",
                         boIdx.toString().c_str(),  clName.c_str(), rc ) ;
         }

         if ( cataSet.isMainCL() )
         {
            std::vector< std::string > subCLLst;
            std::vector< std::string >::iterator iterSubCL;

            rc = cataSet.getSubCLList( subCLLst );
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get sub-collection list of collection [%s], "
                         "rc: %d",
                         clName.c_str(), rc ) ;

            iterSubCL = subCLLst.begin() ;
            while ( iterSubCL != subCLLst.end() )
            {
               std::string subCLName = (*iterSubCL) ;
               _catCtxCreateIdxTask *pSubCLTask = NULL ;

               rc = _addCreateIdxTask( subCLName, idxName, boIdx, &pSubCLTask ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to add create index [%s/%s] sub-task, "
                            "rc: %d",
                            subCLName.c_str(), idxName.c_str(), rc ) ;

               rc = pSubCLTask->checkTask( cb, lockMgr ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to check create index [%s/%s] sub-task, "
                            "rc: %d",
                            subCLName.c_str(), idxName.c_str(), rc ) ;

               if ( uniqueCheck )
               {
                  clsCatalogSet subCataSet( subCLName.c_str() );

                  rc = subCataSet.updateCatSet( pSubCLTask->getDataObj() ) ;
                  PD_RC_CHECK( rc, PDERROR,
                               "Failed to parse catalog info [%s], rc: %d",
                               subCLName.c_str(), rc ) ;

                  rc = pSubCLTask->checkIndexKey( subCataSet, checkedKeyIDs ) ;
                  PD_RC_CHECK( rc, PDERROR,
                               "Failed to check index key [%s] on collection [%s], "
                               "rc: %d",
                               boIdx.toString().c_str(), subCLName.c_str(), rc ) ;
               }

               rc = catGetCollectionGroupSet( pSubCLTask->getDataObj(),
                                              _groupList ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to collect groups for collection [%s], "
                            "rc: %d",
                            subCLName.c_str(), rc ) ;

               ++iterSubCL ;
            }
         }
         else
         {
            rc = catGetCollectionGroupSet( pCreateIdxTask->getDataObj(),
                                           _groupList ) ;
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
      PD_TRACE_EXITRC ( SDB_CATCTXINDEX_CREATEIDX_SUBTASK, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXINDEX_CREATEIDX_TASKS, "_catCtxIndexMultiTask::_addCreateIdxTasks" )
   INT32 _catCtxIndexMultiTask::_addCreateIdxTasks ( const std::string &clName,
                                                     const std::string &idxName,
                                                     const BSONObj &boIdx,
                                                     BOOLEAN uniqueCheck,
                                                     _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXINDEX_CREATEIDX_TASKS ) ;

      catCtxLockMgr lockMgr ;
      _catCtxCreateIdxTask *pCreateIdxTask = NULL ;

      rc = _addCreateIdxTask( clName, idxName, boIdx,
                              &pCreateIdxTask, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to add create index [%s/%s] task, rc: %d",
                   clName.c_str(), idxName.c_str(), rc ) ;

      if ( uniqueCheck )
      {
         pCreateIdxTask->enableUniqueCheck() ;
      }
      else
      {
         pCreateIdxTask->disableUniqueCheck() ;
      }

      rc = pCreateIdxTask->checkTask( cb, lockMgr ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check create index [%s/%s] task, rc: %d",
                   clName.c_str(), idxName.c_str(), rc ) ;

      rc = _addCreateIdxSubTasks( pCreateIdxTask, lockMgr, cb ) ;
      PD_RC_CHECK( rc , PDERROR,
                   "Failed to add sub-tasks for create index [%s/%s] task, "
                   "rc: %d",
                   clName.c_str(), idxName.c_str(), rc ) ;

      rc = _pushExecTask( pCreateIdxTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to push collection task, rc: %d", rc ) ;

      rc = catLockGroups( _groupList, cb, lockMgr, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock groups, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXINDEX_CREATEIDX_TASKS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXINDEX_DROPIDX_SUBTASK, "_catCtxIndexMultiTask::_addDropIdxSubTasks" )
   INT32 _catCtxIndexMultiTask::_addDropIdxSubTasks ( _catCtxDropIdxTask *pDropIdxTask,
                                                      catCtxLockMgr &lockMgr,
                                                      _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXINDEX_DROPIDX_SUBTASK ) ;

      try
      {
         const std::string &clName = pDropIdxTask->getDataName() ;
         const std::string &idxName = pDropIdxTask->getIdxName() ;
         clsCatalogSet cataSet( clName.c_str() );

         rc = cataSet.updateCatSet( pDropIdxTask->getDataObj() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to parse catalog info [%s], rc: %d",
                      clName.c_str(), rc ) ;

         if ( cataSet.isMainCL() )
         {
            std::vector< std::string > subCLLst ;
            std::vector< std::string >::iterator iterSubCL ;

            rc = cataSet.getSubCLList( subCLLst );
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get sub-collection list of collection [%s], "
                         "rc: %d",
                         clName.c_str(), rc ) ;

            iterSubCL = subCLLst.begin() ;
            while ( iterSubCL != subCLLst.end() )
            {
               std::string subCLName = (*iterSubCL) ;
               _catCtxDropIdxTask *pSubCLTask = NULL ;

               rc = _addDropIdxTask( subCLName, idxName, &pSubCLTask ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to add drop index [%s/%s] sub-task, "
                            "rc: %d",
                            subCLName.c_str(), idxName.c_str(), rc ) ;

               rc = pSubCLTask->checkTask( cb, lockMgr ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to check drop index [%s/%s] sub-task, "
                            "rc: %d",
                            subCLName.c_str(), idxName.c_str(), rc ) ;

               rc = catGetCollectionGroupSet ( pSubCLTask->getDataObj(),
                                               _groupList ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to collect groups for collection [%s], "
                            "rc: %d",
                            subCLName.c_str(), rc ) ;

               ++iterSubCL ;
            }
         }
         else
         {
            rc = catGetCollectionGroupSet( pDropIdxTask->getDataObj(),
                                           _groupList ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to collect groups for collection [%s], "
                         "rc: %d",
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
      PD_TRACE_EXITRC ( SDB_CATCTXINDEX_DROPIDX_SUBTASK, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXINDEX_DROPIDX_TASKS, "_catCtxIndexMultiTask::_addDropIdxTasks" )
   INT32 _catCtxIndexMultiTask::_addDropIdxTasks ( const std::string &clName,
                                                   const std::string &idxName,
                                                   _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXINDEX_DROPIDX_TASKS ) ;

      catCtxLockMgr lockMgr ;
      _catCtxDropIdxTask *pDropIdxTask = NULL ;

      rc = _addDropIdxTask( clName, idxName, &pDropIdxTask, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to add drop index [%s/%s] task, rc: %d",
                   clName.c_str(), idxName.c_str(), rc ) ;

      rc = pDropIdxTask->checkTask( cb, lockMgr ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check drop index [%s/%s] task, rc: %d",
                   clName.c_str(), idxName.c_str(), rc ) ;

      rc = _addDropIdxSubTasks( pDropIdxTask, lockMgr, cb ) ;
      PD_RC_CHECK( rc , PDERROR,
                   "Failed to add sub-tasks for drop index [%s/%s], rc: %d",
                   clName.c_str(), idxName.c_str(), rc ) ;

      rc = _pushExecTask( pDropIdxTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to push collection task, rc: %d", rc ) ;

      rc = catLockGroups( _groupList, cb, lockMgr, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock groups, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXINDEX_DROPIDX_TASKS, rc ) ;
      return rc;
   error :
      goto done;
   }

   /*
    * _catCtxDropCS implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxDropCS, RTN_CONTEXT_CAT_DROP_CS,
                          "CAT_DROP_CS" )

   _catCtxDropCS::_catCtxDropCS ( INT64 contextID, UINT64 eduID )
   : _catCtxCLMultiTask( contextID, eduID )
   {
      _executeAfterLock = FALSE ;
      _commitAfterExecute = FALSE ;
      _needRollback = FALSE ;
   }

   _catCtxDropCS::~_catCtxDropCS ()
   {
      _onCtxDelete () ;
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
         rc = rtnGetSTDStringElement( _boQuery, CAT_COLLECTION_SPACE_NAME,
                                      _targetName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_COLLECTION_SPACE_NAME, rc ) ;
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

      rc = pDropCSTask->checkTask(cb, _lockMgr) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check drop collection space [%s] task, rc: %d",
                   _targetName.c_str(), rc ) ;

      rc = _addDropCSSubTasks( pDropCSTask, cb ) ;
      PD_RC_CHECK( rc , PDERROR,
                   "Failed to add sub-tasks for drop collection space [%s], "
                   "rc: %d",
                   _targetName.c_str(), rc ) ;

      rc = _pushExecTask( pDropCSTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to push collection task, rc: %d", rc ) ;

      rc = catLockGroups( _groupList, cb, _lockMgr, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to lock groups, rc: %d",
                   rc ) ;

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

               rc = _addDropCLTask( clFullName, -1, &pDropCLTask );

               pDropCLTask->disableLocks() ;

               rc = pDropCLTask->checkTask( cb, _lockMgr ) ;
               PD_RC_CHECK( rc, PDWARNING,
                            "Failed to check drop collection [%s] task, rc: %d",
                            clFullName.c_str(), rc ) ;

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
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
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
            std::vector< std::string > subCLLst;
            std::vector< std::string >::iterator iterSubCL;

            rc = cataSet.getSubCLList( subCLLst );
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get sub-collection list of collection [%s], "
                         "rc: %d",
                         clName.c_str(), rc ) ;
            iterSubCL = subCLLst.begin() ;
            while( iterSubCL != subCLLst.end() )
            {
               std::string subCLName = (*iterSubCL) ;

               BOOLEAN inSameSpace = FALSE ;
               _catCtxUnlinkSubCLTask *pUnlinkSubCLTask = NULL ;

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

               rc = catGetCollectionGroupSet( pUnlinkSubCLTask->getDataObj(),
                                              _groupList ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to collect groups for sub-collection [%s], "
                            "rc: %d",
                            subCLName.c_str(), rc ) ;

               ++iterSubCL ;
            }
         } else {
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
                  externalMainCL.insert( mainCLName ) ;
               }
            }
            rc = catGetCollectionGroupSet( pDropCLTask->getDataObj(),
                                           _groupList ) ;
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
    * _catCtxCreateCL implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxCreateCL, RTN_CONTEXT_CAT_CREATE_CL,
                          "CAT_CREATE_CL" )

   _catCtxCreateCL::_catCtxCreateCL ( INT64 contextID, UINT64 eduID )
   : _catCtxDataBase( contextID, eduID )
   {
      _executeAfterLock = TRUE ;
      _needRollback = TRUE ;
   }

   _catCtxCreateCL::~_catCtxCreateCL ()
   {
      _onCtxDelete () ;
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
      UINT32 fieldMask = 0 ;
      catCollectionInfo clInfo ;
      std::map<std::string, UINT32> splitList ;


      rc = catGetCollection( _targetName, boDummy, cb ) ;
      PD_CHECK( SDB_DMS_NOTEXIST == rc,
                SDB_DMS_EXIST, error, PDERROR,
                "Create failed, the collection [%s] exists",
                _targetName.c_str() ) ;

      rc = rtnResolveCollectionName( _targetName.c_str(),
                                     _targetName.size(),
                                     szSpace, DMS_COLLECTION_SPACE_NAME_SZ,
                                     szCollection, DMS_COLLECTION_NAME_SZ );
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to resolve collection name [%s], rc: %d",
                    _targetName.c_str(), rc ) ;

      rc = dmsCheckCLName( szCollection, FALSE ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to check collection name [%s], rc: %d",
                    szCollection, rc ) ;

      rc = catGetAndLockCollectionSpace( szSpace, boSpace, cb, NULL, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the collection space [%s], rc: %d",
                   szSpace, rc ) ;

      {
         BSONElement ele = boSpace.getField( CAT_COLLECTION ) ;
         if ( Array == ele.type() )
         {
            if ( ele.embeddedObject().nFields() >= DMS_MME_SLOTS )
            {
               PD_LOG( PDERROR,
                       "Collection Space [%s] cannot accept more collection",
                       szSpace ) ;
               rc = SDB_DMS_NOSPC ;
               goto error ;
            }
         }
      }

      PD_CHECK( _lockMgr.tryLockCollection( szSpace, _targetName, EXCLUSIVE ),
                SDB_LOCK_FAILED, error, PDERROR,
                "Failed to lock collection [%s]",
                _targetName.c_str() ) ;

      {
         BSONElement eleDomain = boSpace.getField( CAT_DOMAIN_NAME ) ;
         if ( String == eleDomain.type() )
         {
            std::string domainName = eleDomain.str() ;
            rc = catGetAndLockDomain( domainName, boDomain, cb,
                                      &_lockMgr, SHARED ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get the domain [%s], rc: %d",
                         domainName.c_str(), rc ) ;
         }
      }

      rc = catCheckAndBuildCataRecord( _boQuery, fieldMask, clInfo, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check create collection obj [%s], rc: %d",
                   _boQuery.toString().c_str(), rc ) ;

      {
         BSONElement eleType = boSpace.getField( CAT_TYPE_NAME ) ;
         if ( NumberInt == eleType.type() )
         {
            INT32 type = eleType.numberInt() ;
            if ( ( DMS_STORAGE_NORMAL == type ) && clInfo._capped )
            {
               PD_LOG( PDERROR, "Capped colleciton can only be created on "
                       "Capped collection space" ) ;
               rc = SDB_OPERATION_INCOMPATIBLE ;
               goto error ;
            }

            if ( ( DMS_STORAGE_CAPPED == type ) && !clInfo._capped )
            {
               clInfo._capped = TRUE ;
               fieldMask |= CAT_MASK_CAPPED ;
            }
         }
      }

      _version = catGetBucketVersion( _targetName.c_str(), cb ) ;
      clInfo._version = _version ;

      rc = _combineOptions( boDomain, boSpace, fieldMask, clInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to combine options, domainObj [%s], "
                 "create cl options[%s], rc: %d",
                 boDomain.toString().c_str(), _boQuery.toString().c_str(),
                 rc ) ;
         goto error ;
      }

      rc = _chooseGroupOfCl( boDomain, boSpace, clInfo, cb,
                             _groupList, splitList ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to choose groups for new collection [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

      rc = catLockGroups( _groupList, cb, _lockMgr, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to lock groups, rc: %d",
                   rc ) ;

      {
         BSONObj boNewObj ;
         rc = catBuildCatalogRecord ( clInfo, fieldMask, 0,
                                      _groupList, splitList,
                                      boNewObj ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Build new collection catalog record failed, rc: %d",
                      rc ) ;

         _boTarget = boNewObj.getOwned() ;
      }

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

      PD_TRACE_ENTRY ( SDB_CATCTXCREATECL_EXECUTE_INT ) ;

      rc = catCreateCLStep( _targetName, _boTarget,
                            cb, _pDmsCB, _pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to create collection [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCREATECL_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATECL_ROLLBACK_INT, "_catCtxCreateCL::_rollbackInternal" )
   INT32 _catCtxCreateCL::_rollbackInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATECL_ROLLBACK_INT ) ;

      rc = catDropCLStep( _targetName, _version, TRUE, cb, _pDmsCB, _pDpsCB, w ) ;
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

      if ( boDomain.isEmpty() )
      {
         goto done ;
      }

      if ( !( CAT_MASK_AUTOASPLIT & fieldMask ) )
      {
         if ( clInfo._isSharding && clInfo._isHash )
         {
            BSONElement split = boDomain.getField( CAT_DOMAIN_AUTO_SPLIT ) ;
            if ( Bool == split.type() )
            {
               clInfo._autoSplit = split.Bool() ;
               fieldMask |= CAT_MASK_AUTOASPLIT ;
            }
         }
      }

      if ( !( CAT_MASK_AUTOREBALAN & fieldMask ) )
      {
         if ( clInfo._isSharding && clInfo._isHash )
         {
            BSONElement rebalance = boDomain.getField( CAT_DOMAIN_AUTO_REBALANCE ) ;
            if ( Bool == rebalance.type() )
            {
               clInfo._autoRebalance = rebalance.Bool() ;
               fieldMask |= CAT_MASK_AUTOREBALAN ;
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
      else
      {
         rc = _chooseCLGroupDefault( domainObj, csObj, clInfo._assignType,
                                     cb, groupIDList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get groups for collection, "
                      "rc: %d", rc ) ;
      }

      rc = catCheckGroupsByID( groupIDList ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to assign group for collection [%s], rc: %d",
                   clInfo._pCLName, rc ) ;

      if ( clInfo._isMainCL )
      {
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
         sdbGetCatalogueCB()->getGroupsID( groupIDList, TRUE ) ;
         sdbGetCatalogueCB()->getGroupNameMap( splitRange, TRUE ) ;
      }
      else
      {
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

      if ( ASSIGN_FOLLOW == assignType )
      {
         BSONElement ele = csObj.getField( CAT_COLLECTION_SPACE_NAME ) ;
         rc = catGetCSGroupsFromCLs( ele.valuestrsafe(), cb,
                                     candidateGroupList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get collection space [%s]"
                      " groups, rc: %d", csObj.toString().c_str(), rc ) ;
      }

      if ( candidateGroupList.empty() && !isSysDomain )
      {
         rc = catGetDomainGroups( domainObj, candidateGroupList ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get groups from domain info [%s], rc: %d",
                      domainObj.toString().c_str(), rc ) ;
      }

      if ( candidateGroupList.empty() )
      {
         tmpGrpID = CAT_INVALID_GROUPID ;
         rc = sdbGetCatalogueCB()->getAGroupRand( tmpGrpID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get group from SYS, rc: %d",
                      rc ) ;
      }
      else if ( 1 == candidateGroupList.size() )
      {
         tmpGrpID = candidateGroupList[ 0 ] ;
      }
      else
      {
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
            clObj.getFieldDotted( CAT_CATALOGINFO_NAME".0."CAT_UPBOUND_NAME ) ;

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
   : _catCtxCLMultiTask( contextID, eduID )
   {
      _executeAfterLock = FALSE ;
      _commitAfterExecute = FALSE ;
      _needRollback = FALSE ;
      _needUpdateCoord = FALSE ;
   }

   _catCtxDropCL::~_catCtxDropCL ()
   {
      _onCtxDelete () ;
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

      rc = _addDropCLTask( _targetName, _version, &pDropCLTask, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to create drop collection task, rc: %d", rc ) ;

      rc = pDropCLTask->checkTask (cb, _lockMgr) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check collection task, rc: %d", rc ) ;

      rc = _addDropCLSubTasks ( pDropCLTask, cb ) ;
      PD_RC_CHECK( rc , PDERROR,
                   "Failed to add sub-tasks for drop collection") ;

      rc = _pushExecTask( pDropCLTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to push drop collection task, rc: %d", rc ) ;

      rc = catLockGroups( _groupList, cb, _lockMgr, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock groups, rc: %d", rc ) ;

      _needUpdateCoord = pDropCLTask->needUpdateCoord() ;
      _boTarget = pDropCLTask->getDataObj() ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCL_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCL_MAKEREPLY, "_catCtxDropCL::_makeReply" )
   INT32 _catCtxDropCL::_makeReply ( rtnContextBuf &buffObj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCL_MAKEREPLY ) ;

      if ( CAT_CONTEXT_READY == _status )
      {
         BSONObjBuilder retObjBuilder ;

         if ( _needUpdateCoord )
         {
            retObjBuilder.appendElements(
                  BSON( CAT_COLLECTION << _boTarget.getOwned() ) ) ;
            _pCatCB->makeGroupsObj( retObjBuilder, _groupList, TRUE ) ;
         }
         else if ( !_groupList.empty() )
         {
            _pCatCB->makeGroupsObj( retObjBuilder, _groupList, TRUE ) ;
         }

         BSONObj retObj = retObjBuilder.obj() ;
         if ( !retObj.isEmpty() )
         {
            buffObj = rtnContextBuf( retObj ) ;
         }
      }
      else
      {
         rc = _catCtxDataMultiTaskBase::_makeReply( buffObj ) ;
      }

      PD_TRACE_EXITRC ( SDB_CATCTXDROPCL_MAKEREPLY, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCL_DROPCL_SUBTASK, "_catCtxDropCL::_addDropCLSubTasks" )
   INT32 _catCtxDropCL::_addDropCLSubTasks ( _catCtxDropCLTask *pDropCLTask,
                                             _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCL_DROPCL_SUBTASK ) ;


      try
      {
         const std::string &clName = pDropCLTask->getDataName() ;
         clsCatalogSet cataSet( clName.c_str() );

         rc = cataSet.updateCatSet( pDropCLTask->getDataObj() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to parse catalog info [%s], rc: %d",
                      clName.c_str(), rc ) ;

         _catCtxDelCLsFromCSTask *pDelCLsFromCSTask = NULL ;
         rc = _addDelCLsFromCSTask ( &pDelCLsFromCSTask, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to create delCLsFromCS task, "
                      "rc: %d", rc ) ;
         rc = pDelCLsFromCSTask->deleteCL( clName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to add collection [%s] into delCLsFromCS task, "
                      "rc: %d", clName.c_str(), rc ) ;

         if ( cataSet.isMainCL() )
         {
            std::vector< std::string > subCLLst;
            std::vector< std::string >::iterator iterSubCL;

            rc = cataSet.getSubCLList( subCLLst );
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get sub-collection list of collection [%s], "
                         "rc: %d",
                         clName.c_str(), rc ) ;
            iterSubCL = subCLLst.begin() ;
            while( iterSubCL != subCLLst.end() )
            {
               std::string subCLName = (*iterSubCL) ;

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

               rc = catGetCollectionGroupSet( pDropSubCLTask->getDataObj(),
                                              _groupList ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to collect groups for sub-collection [%s], "
                            "rc: %d",
                            subCLName.c_str(), rc ) ;

               ++iterSubCL ;
            }
         } else {
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
            rc = catGetCollectionGroupSet( pDropCLTask->getDataObj(),
                                           _groupList ) ;
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
    * _catCtxAlterCL implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxAlterCL, RTN_CONTEXT_CAT_ALTER_CL,
                          "CAT_ALTER_CL" )

   _catCtxAlterCL::_catCtxAlterCL ( INT64 contextID, UINT64 eduID )
   : _catCtxIndexMultiTask( contextID, eduID )
   {
      _executeAfterLock = TRUE ;
      _needRollback = FALSE ;
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

      BOOLEAN isOld = FALSE ;

      SDB_ASSERT( MSG_CAT_ALTER_COLLECTION_REQ == _cmdType,
                  "Wrong command type" ) ;

      try
      {
         rc = rtnGetSTDStringElement( _boQuery, CAT_COLLECTION_NAME, _targetName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_COLLECTION_NAME, rc ) ;

         isOld = _boQuery.getField( FIELD_NAME_VERSION ).eoo() ;
         if ( isOld )
         {
            rc = rtnGetObjElement( _boQuery, CAT_OPTIONS_NAME, _alterFields ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field [%s], rc: %d",
                         CAT_OPTIONS_NAME, rc ) ;
         }
         else
         {
            rc = _alterJob.init( _boQuery ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to extract alter job, rc: %d",
                         rc ) ;

            PD_CHECK( _alterJob.getType() == RTN_ALTER_TYPE_CL,
                      SDB_INVALIDARG, error, PDERROR,
                      "Wrong type of alter job" ) ;
         }
      }
      catch ( std::exception &e )
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
         rc = _checkAlterCL( cb ) ;
      }
      else
      {
         rc = _checkAlterCLJob( cb ) ;
      }

      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check Alter Collection command, rc: %d",
                   rc ) ;

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
         rc = _executeAlterCL( cb, w ) ;
      }
      else
      {
         rc = _executeAlterCLJob( cb, w ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL_CHECK_ALTERCL, "_catCtxAlterCL::_checkAlterCL" )
   INT32 _catCtxAlterCL::_checkAlterCL ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL_CHECK_ALTERCL ) ;

      UINT32 fieldMask = 0 ;
      BSONObj boCollection ;
      catCollectionInfo newCLInfo ;
      clsCatalogSet cataSet( _targetName.c_str() );
      std::map<std::string, UINT32> splitList ;
      BSONObj tmpObj ;
      catCtxLockMgr lockMgr ;

      rc = catGetAndLockCollection( _targetName, boCollection, cb,
                                    &lockMgr, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the Collection [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

      rc = cataSet.updateCatSet( boCollection );
      PD_RC_CHECK( rc, PDERROR,
                  "Failed to parse catalog info [%s], rc: %d",
                  boCollection.toString().c_str(), rc );

      rc = catCheckAndBuildCataRecord( _alterFields, fieldMask,
                                       newCLInfo, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Check alter collection obj [%s] failed, rc: %d",
                   _alterFields.toString().c_str(), rc ) ;

      rc = _buildAlterFields( cataSet, fieldMask, newCLInfo, tmpObj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Build alter collection obj [%s] failed, rc: %d",
                   _alterFields.toString().c_str(), rc ) ;

      rc = catGetCollectionGroupsCascade( _targetName, boCollection,
                                          cb, _groupList ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to collect groups of collection [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

      rc = catLockGroups( _groupList, cb, lockMgr, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to lock groups, rc: %d", rc ) ;

      _boTarget = tmpObj.getOwned() ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCL_CHECK_ALTERCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL_CHECK_ALTERCLJOB, "_catCtxAlterCL::_checkAlterCLJob" )
   INT32 _catCtxAlterCL::_checkAlterCLJob ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL_CHECK_ALTERCLJOB ) ;

      BSONObj boTasks = _alterJob.getTasks() ;
      BSONObjIterator iterTask( boTasks ) ;

      while ( iterTask.more() )
      {
         BSONElement e = iterTask.next() ;
         if ( Object != e.type() )
         {
            PD_LOG( PDERROR,
                    "Invalid alter task: %s",
                    e.toString( FALSE, TRUE ).c_str() ) ;
            if ( _alterJob.getOptions().ignoreException )
            {
               continue ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         BSONObj boTask = e.embeddedObject() ;
         RTN_ALTER_FUNC_TYPE taskType = RTN_ALTER_FUNC_INVALID ;
         std::string taskName ;

         rc = rtnGetIntElement( boTask, FIELD_NAME_TASKTYPE,
                                (INT32 &)taskType ) ;
         PD_RC_CHECK( rc, PDERROR,
                       "Failed to get field [%s], rc: %d",
                       FIELD_NAME_TASKTYPE, rc ) ;

         rc = rtnGetSTDStringElement( boTask, FIELD_NAME_NAME, taskName ) ;
         PD_RC_CHECK( rc, PDERROR,
                       "Failed to get field [%s], rc: %d",
                       FIELD_NAME_NAME, rc ) ;

         switch ( taskType )
         {
         case RTN_ALTER_CL_CRT_ID_IDX :
         {
            BSONObj boIdx = BSON(
                  IXM_FIELD_NAME_KEY << BSON( DMS_ID_KEY_NAME << 1 ) <<
                  IXM_FIELD_NAME_NAME << IXM_ID_KEY_NAME <<
                  IXM_FIELD_NAME_UNIQUE << true <<
                  IXM_FIELD_NAME_V << 0 <<
                  IXM_FIELD_NAME_ENFORCED << true ) ;

            PD_CHECK( 0 == taskName.compare( SDB_ALTER_CRT_ID_INDEX ),
                      SDB_INVALIDARG, error, PDERROR,
                      "Task name [%s] not matched, expected %s",
                      taskName.c_str(), SDB_ALTER_CRT_ID_INDEX ) ;

            rc = _addCreateIdxTasks( _targetName, IXM_ID_KEY_NAME,
                                     boIdx, FALSE, cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to add create ID index [%s] tasks, rc: %d",
                         _targetName.c_str(), rc ) ;
            break ;
         }
         case RTN_ALTER_CL_DROP_ID_IDX :
            PD_CHECK( 0 == taskName.compare( SDB_ALTER_DROP_ID_INDEX ),
                      SDB_INVALIDARG, error, PDERROR,
                      "Task name [%s] not matched, expected %s",
                      taskName.c_str(), SDB_ALTER_CRT_ID_INDEX ) ;

            rc = _addDropIdxTasks ( _targetName, IXM_ID_KEY_NAME, cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to add drop ID index [%s] tasks, rc: %d",
                         _targetName.c_str(), rc ) ;
            break ;
         default :
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Unknown type of alter task %d", taskType ) ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXALTERCL_CHECK_ALTERCLJOB, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL_EXECUTE_ALTERCL, "_catCtxAlterCL::_executeAlterCL" )
   INT32 _catCtxAlterCL::_executeAlterCL ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL_EXECUTE_ALTERCL ) ;

      rc = catAlterCLStep( _targetName, _boTarget, cb, _pDmsCB, _pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to alter collection [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXALTERCL_EXECUTE_ALTERCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL_EXECUTE_ALTERCLJOB, "_catCtxAlterCL::_executeAlterCLJob" )
   INT32 _catCtxAlterCL::_executeAlterCLJob ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL_EXECUTE_ALTERCLJOB ) ;

      rc = _catCtxDataMultiTaskBase::_executeInternal( cb, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute alter collection [%s] job, rc: %d",
                   _targetName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXALTERCL_EXECUTE_ALTERCLJOB, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCL_BUILDALTERFIELD, "_catCtxAlterCL::_buildAlterFields" )
   INT32 _catCtxAlterCL::_buildAlterFields ( clsCatalogSet &cataSet,
                                             UINT32 mask,
                                             const catCollectionInfo &alterInfo,
                                             BSONObj &alterObj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXALTERCL_BUILDALTERFIELD ) ;

      UINT32 attribute = 0 ;
      clsCatalogSet::POSITION pos ;
      clsCatalogItem *item = NULL ;

      PD_CHECK( !( ( CAT_MASK_SHDKEY & mask ) && cataSet.isSharding() ),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Can not alter a sharding collection's shardingkey" ) ;

      PD_CHECK( !( CAT_MASK_ISMAINCL & mask ),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Can not change a collection to a main cl" ) ;

      PD_CHECK( !( CAT_MASK_COMPRESSED & mask ),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "can not alter attribute \"Compressed\"" ) ;

      PD_CHECK( !( CAT_MASK_CLNAME & mask ),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Can not alter attribute \"Name\"" ) ;

      if ( cataSet.isSharding() || cataSet.isMainCL() )
      {
         BSONObjBuilder builder ;
         if ( mask & CAT_MASK_REPLSIZE )
         {
            builder.append( CAT_CATALOG_W_NAME, alterInfo._replSize ) ;
         }
         if ( mask & CAT_MASK_AUTOREBALAN )
         {
            builder.appendBool( CAT_DOMAIN_AUTO_REBALANCE,
                                alterInfo._autoRebalance ) ;
         }

         alterObj = builder.obj() ;
         goto done ;
      }

      pos = cataSet.getFirstItem() ;
      item = cataSet.getNextItem( pos ) ;
      PD_CHECK ( item, SDB_SYS, error, PDERROR,
                 "Failed to get first item from catalog set" ) ;
      attribute = cataSet.getAttribute() ;

      if ( OSS_BIT_TEST( attribute, DMS_MB_ATTR_CAPPED )  )
      {
         PD_CHECK( !( ( CAT_MASK_SHDKEY & mask ) ||
                      ( CAT_MASK_SHDIDX & mask ) ||
                      ( CAT_MASK_SHDTYPE & mask ) ||
                      ( CAT_MASK_SHDPARTITION & mask ) ),
                   SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                   "The arguments are illegal for capped collection" ) ;
         if ( ( ( CAT_MASK_COMPRESSED & mask ) && alterInfo._isCompressed ) ||
              ( ( CAT_MASK_COMPRESSIONTYPE & mask ) &&
                ( UTIL_COMPRESSOR_INVALID != alterInfo._compressorType ) ) )
         {
            PD_LOG( PDERROR,
                    "Compression is not supported on capped collection" ) ;
            rc = SDB_OPTION_NOT_SUPPORT ;
            goto error ;
         }

         if ( ( ( CAT_MASK_AUTOASPLIT & mask ) && alterInfo._autoSplit ) ||
              ( ( CAT_MASK_AUTOINDEXID & mask ) && alterInfo._autoIndexId ) )
         {
            PD_LOG( PDERROR, "Autosplit|AutoIndexId is not supported "
                    "on capped collection" ) ;
            rc = SDB_OPTION_NOT_SUPPORT ;
            goto error ;
         }

         if ( ( CAT_MASK_CAPPED & mask ) && !alterInfo._capped )
         {
            PD_LOG( PDERROR, "Can't change from capped collection to normal "
                    "collection" ) ;
            rc = SDB_OPTION_NOT_SUPPORT ;
            goto error ;
         }

         if ( ( CAT_MASK_CLMAXSIZE & mask ) || ( CAT_MASK_CLMAXRECNUM & mask )
              || ( CAT_MASK_CLOVERWRITE & mask ) )
         {
            PD_LOG( PDERROR, "Can't change Size|Max|OverWrite" ) ;
            rc = SDB_OPTION_NOT_SUPPORT ;
            goto error ;
         }
      }
      else
      {
         if ( ( CAT_MASK_CAPPED & mask ) && alterInfo._capped )
         {
            PD_LOG( PDERROR, "Can't change from normal collection to capped "
                    "collection" ) ;
            rc = SDB_OPTION_NOT_SUPPORT ;
            goto error ;
         }
         if ( ( CAT_MASK_CLMAXRECNUM & mask ) || ( CAT_MASK_CLMAXSIZE & mask )
              || ( CAT_MASK_CLOVERWRITE & mask ) )
         {
            PD_LOG( PDERROR, "Max|Size|OverWrite is only supported on capped "
                    "collection" ) ;
            rc = SDB_OPTION_NOT_SUPPORT ;
            goto error ;
         }
      }

      {
         std::vector<UINT32> groupLst ;
         std::map<std::string, UINT32> splitLst ;

         groupLst.push_back( item->getGroupID() ) ;
         rc = catBuildCatalogRecord( alterInfo, mask, attribute,
                                     groupLst, splitLst, alterObj ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to build cata record, rc: %d",
                       rc ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXALTERCL_BUILDALTERFIELD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * _catCtxLinkCL implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxLinkCL, RTN_CONTEXT_CAT_LINK_CL,
                          "CAT_LINK_CL" )

   _catCtxLinkCL::_catCtxLinkCL ( INT64 contextID, UINT64 eduID )
   : _catCtxDataBase( contextID, eduID )
   {
      _executeAfterLock = TRUE ;
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

      rc = catGetAndLockCollection( _targetName, _boTarget, cb,
                                    &_lockMgr, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the main-collection [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

      rc = catCheckMainCollection( _boTarget, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Collection [%s] is not a main-collection, rc: %d",
                   _targetName.c_str(), rc ) ;

      if ( 0 == _targetName.compare( _subCLName ) )
      {
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

      rc = catCheckMainCollection( _boSubCL, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Collection [%s] could not be a main-collection, rc: %d",
                   _subCLName.c_str(), rc ) ;

      rc = catCheckRelinkCollection ( _boSubCL, tmpMainCLName ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Duplicate attach collection partition [%s], "
                   "its partitioned-collection is %s",
                   _subCLName.c_str(), tmpMainCLName.c_str() ) ;

      rc = catGetAndLockCollectionGroups( _boSubCL,
                                          _groupList,
                                          _lockMgr, SHARED ) ;
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


      if ( _needUpdateSubCL )
      {
         rc = catLinkSubCLStep( _targetName, _subCLName, cb, _pDmsCB, _pDpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update the sub-collection [%s], rc: %d",
                      _subCLName.c_str(), rc ) ;

         subCLUpdated = TRUE ;
      }

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


      if ( _needUpdateSubCL )
      {
         tmpRC = catUnlinkSubCLStep( _subCLName, cb, _pDmsCB, _pDpsCB, w ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING,
                    "Failed to rollback the sub-collection [%s], rc: %d",
                    _subCLName.c_str(), tmpRC ) ;
            rc = tmpRC ;
         }
      }

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
      _executeAfterLock = TRUE ;
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

      rc = catGetAndLockCollection( _targetName, _boTarget, cb,
                                    &_lockMgr, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the main-collection [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

      rc = catCheckMainCollection( _boTarget, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Collection[%s] is not a main-collection, rc: %d",
                   _targetName.c_str(), rc ) ;

      if ( 0 == _targetName.compare( _subCLName ) )
      {
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

      rc = catCheckMainCollection( _boSubCL, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Collection [%s] could not be a main-collection, rc: %d",
                   _subCLName.c_str(), rc ) ;

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

      rc = catGetAndLockCollectionGroups( _boSubCL,
                                          _groupList,
                                          _lockMgr, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to lock groups, rc: %d", rc ) ;

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


      if ( _needUpdateSubCL )
      {
         rc = catUnlinkSubCLStep( _subCLName, cb, _pDmsCB, _pDpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update the sub-collection [%s], rc: %d",
                      _subCLName.c_str(), rc ) ;

         subCLUpdated = TRUE ;
      }

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


      if ( _needUpdateSubCL )
      {
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

      if ( !_lowBound.isEmpty() && !_upBound.isEmpty() )
      {
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
    * _catCtxCreateIdx implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxCreateIdx, RTN_CONTEXT_CAT_CREATE_IDX,
                          "CAT_CREATE_IDX" )

   _catCtxCreateIdx::_catCtxCreateIdx ( INT64 contextID, UINT64 eduID )
   : _catCtxIndexMultiTask( contextID, eduID )
   {
      _executeAfterLock = TRUE ;
      _needRollback = TRUE ;
   }

   _catCtxCreateIdx::~_catCtxCreateIdx ()
   {
      _onCtxDelete() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATEIDX_PARSEQUERY, "_catCtxCreateIdx::_parseQuery" )
   INT32 _catCtxCreateIdx::_parseQuery ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATEIDX_PARSEQUERY ) ;

      SDB_ASSERT( MSG_CAT_CREATE_IDX_REQ == _cmdType,
                  "Wrong command type" ) ;

      try
      {
         rc = rtnGetSTDStringElement( _boQuery, CAT_COLLECTION, _targetName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_COLLECTION, rc ) ;

         rc = rtnGetObjElement( _boQuery, FIELD_NAME_INDEX, _boIdx ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      FIELD_NAME_INDEX, rc ) ;

         rc = rtnGetSTDStringElement( _boIdx, IXM_FIELD_NAME_NAME, _idxName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      IXM_FIELD_NAME_NAME, rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXCREATEIDX_PARSEQUERY, rc ) ;
      return rc;
   error :
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATEIDX_CHECK_INT, "_catCtxCreateIdx::_checkInternal" )
   INT32 _catCtxCreateIdx::_checkInternal ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATEIDX_CHECK_INT ) ;

      rc = _addCreateIdxTasks( _targetName, _idxName, _boIdx, TRUE, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to add create index [%s/%s] tasks, rc: %d",
                   _targetName.c_str(), _idxName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCREATEIDX_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * _catCtxDropIdx implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxDropIdx, RTN_CONTEXT_CAT_DROP_IDX,
                          "CAT_DROP_IDX" )

   _catCtxDropIdx::_catCtxDropIdx ( INT64 contextID, UINT64 eduID )
   : _catCtxIndexMultiTask( contextID, eduID )
   {
      _executeAfterLock = TRUE ;
      _needRollback = FALSE ;
   }

   _catCtxDropIdx::~_catCtxDropIdx ()
   {
      _onCtxDelete() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPIDX_PARSEQUERY, "_catCtxDropIdx::_parseQuery" )
   INT32 _catCtxDropIdx::_parseQuery ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPIDX_PARSEQUERY ) ;

      SDB_ASSERT( MSG_CAT_DROP_IDX_REQ == _cmdType,
                  "Wrong command type" ) ;

      try
      {
         BSONObj boIdx ;
         BSONElement beIdx ;

         rc = rtnGetSTDStringElement( _boQuery, CAT_COLLECTION, _targetName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_COLLECTION, rc ) ;

         rc = rtnGetObjElement( _boQuery, FIELD_NAME_INDEX, boIdx ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      FIELD_NAME_INDEX, rc ) ;

         beIdx = boIdx.firstElement() ;
         PD_CHECK( jstOID == beIdx.type() || String == beIdx.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Invalid index identifier type: %s", beIdx.toString().c_str() ) ;
         if ( String == beIdx.type() )
         {
            _idxName = beIdx.valuestr() ;
         }
         else
         {
            PD_LOG( PDDEBUG, "Deleting index by OID" ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPIDX_PARSEQUERY, rc ) ;
      return rc;
   error :
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPIDX_CHECK_INT, "_catCtxDropIdx::_checkInternal" )
   INT32 _catCtxDropIdx::_checkInternal ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPIDX_CHECK_INT ) ;

      rc = _addDropIdxTasks ( _targetName, _idxName, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to add drop index [%s/%s] tasks, rc: %d",
                   _targetName.c_str(), _idxName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPIDX_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }
}
