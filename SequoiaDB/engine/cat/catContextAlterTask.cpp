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

   Source File Name = catContextAlterTask.cpp

   Descriptive Name = Alter-Tasks for Catalog Runtime Context

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

#include "catContextAlterTask.hpp"
#include "catCommon.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   /*
      _catCtxAlterTask implement
    */
   _catCtxAlterTask::_catCtxAlterTask ( const string & dataName,
                                        const rtnAlterTask * task )
   : _catCtxDataTask( dataName ),
     _task( task )
   {
   }

   _catCtxAlterTask::~_catCtxAlterTask ()
   {
   }

   /*
      _catCtxAlterCLTask implement
    */
   _catCtxAlterCLTask::_catCtxAlterCLTask ( const string & collection,
                                            const rtnAlterTask * task )
   : _catCtxAlterTask( collection, task ),
     _postAutoSplit( FALSE ),
     _subCLOFMainCL( FALSE )
   {
   }

   _catCtxAlterCLTask::~_catCtxAlterCLTask ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK_STARTPOSTTASK, "_catCtxAlterCLTask::startPostTasks" )
   INT32 _catCtxAlterCLTask::startPostTasks ( _pmdEDUCB * cb,
                                              SDB_DMSCB * pDmsCB,
                                              SDB_DPSCB * pDpsCB,
                                              INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK_STARTPOSTTASK ) ;

      BSONObj boCollection ;
      clsCatalogSet cataSet( _dataName.c_str() ) ;
      BSONObj setObject, unsetObject ;

      rc = catGetCollection( _dataName, boCollection, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the Collection [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

      rc = cataSet.updateCatSet ( boCollection ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to parse catalog info [%s], rc: %d",
                   boCollection.toString().c_str(), rc ) ;

      rc = _buildPostTasks( cataSet, cb, pDmsCB, pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build post tasks, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK_STARTPOSTTASK, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK_CLEARPOSTTASK, "_catCtxAlterCLTask::clearPostTasks" )
   INT32 _catCtxAlterCLTask::clearPostTasks ( _pmdEDUCB * cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      BSONObj taskObj ;
      INT32 type = CLS_TASK_UNKNOW ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK_CLEARPOSTTASK ) ;
      for ( ossPoolList<UINT64>::const_iterator iterTask = _postTasks.begin() ;
            iterTask != _postTasks.end() ;
            iterTask ++ )
      {
         UINT64 taskID =  *iterTask ;
         rc = catGetTask( taskID, taskObj, cb);
         PD_RC_CHECK( rc, PDWARNING, "Failed to get task[%ul], rc: %d",
                     taskID, rc ) ;

         rc = rtnGetIntElement( taskObj, CAT_TASKTYPE_NAME, type ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                      CAT_TASKTYPE_NAME, rc ) ;

         if( CLS_TASK_SEQUENCE == type )
         {
            rc = catRemoveTask( taskID, FALSE, cb, w ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to remove task[%ul], rc: %d",
                         taskID, rc ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK_CLEARPOSTTASK, rc ) ;
      return rc ;

   error :
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK_CHECK_INT, "_catCtxAlterCLTask::_checkInternal" )
   INT32 _catCtxAlterCLTask::_checkInternal ( _pmdEDUCB * cb,
                                              catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK_CHECK_INT ) ;

      INT64 taskCount = 0 ;
      clsCatalogSet cataSet( _dataName.c_str() );

      rc = catGetAndLockCollection( _dataName, _boData, cb,
                                    _needLocks ? &lockMgr : NULL, SHARED ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get the collection [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

      rc = cataSet.updateCatSet ( _boData ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to parse catalog info [%s], rc: %d",
                   _boData.toString().c_str(), rc ) ;

      switch ( _task->getActionType() )
      {
         case RTN_ALTER_CL_CREATE_ID_INDEX :
         {
            rc = _checkCreateIDIndex( cataSet, cb, lockMgr ) ;
            break ;
         }
         case RTN_ALTER_CL_DROP_ID_INDEX :
         {
            rc = _checkDropIDIndex( cataSet, cb, lockMgr ) ;
            break ;
         }
         case RTN_ALTER_CL_ENABLE_SHARDING :
         {
            const rtnCLEnableShardingTask * localTask =
                     dynamic_cast< const rtnCLEnableShardingTask * >( _task ) ;
            PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                      "Failed to get task" ) ;

            rc = _checkEnableShard( cataSet, localTask->getShardingArgument(),
                                    cb, lockMgr ) ;
            break ;
         }
         case RTN_ALTER_CL_DISABLE_SHARDING :
         {
            rc = _checkDisableShard( cataSet, cb, lockMgr ) ;
            break ;
         }
         case RTN_ALTER_CL_ENABLE_COMPRESS :
         {
            rc = _checkEnableCompress( cataSet, cb, lockMgr ) ;
            break ;
         }
         case RTN_ALTER_CL_DISABLE_COMPRESS :
         {
            rc = _checkDisableCompress( cataSet, cb, lockMgr ) ;
            break ;
         }
         case RTN_ALTER_CL_SET_ATTRIBUTES :
         {
            rc = _checkSetAttributes( cataSet, cb, lockMgr ) ;
            break ;
         }
         case RTN_ALTER_CL_CREATE_AUTOINC_FLD :
         {
            rc = _checkCreateAutoIncField( cataSet, cb, lockMgr ) ;
            break ;
         }
         case RTN_ALTER_CL_DROP_AUTOINC_FLD :
         {
            rc = _checkDropAutoIncField( cataSet, cb, lockMgr ) ;
            break ;
         }
         default :
         {
            rc = SDB_INVALIDARG ;
            break ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to check task, rc: %d", rc ) ;

      if ( _task->testFlags( RTN_ALTER_TASK_FLAG_SHARDLOCK ) )
      {
         rc = catGetCLTaskCountByType( _dataName.c_str(), cb, CLS_TASK_SPLIT,
                                       taskCount ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get task count of collection [%s], rc: %d",
                      _dataName.c_str(), rc ) ;

         PD_CHECK( 0 == taskCount, SDB_OPERATION_CONFLICT, error, PDERROR,
                   "Failed to [%s]: should have no split tasks",
                   _task->getActionName() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK_CHECK_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK_EXECUTE_INT, "_catCtxAlterCLTask::_executeInternal" )
   INT32 _catCtxAlterCLTask::_executeInternal ( _pmdEDUCB * cb,
                                                SDB_DMSCB * pDmsCB,
                                                SDB_DPSCB * pDpsCB,
                                                INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK_EXECUTE_INT ) ;

      BSONObj boCollection ;
      clsCatalogSet cataSet( _dataName.c_str() ) ;
      BSONObj setObject, unsetObject ;

      rc = catGetCollection( _dataName, boCollection, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the Collection [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

      rc = cataSet.updateCatSet( boCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse catalog info [%s], rc: %d",
                   boCollection.toString().c_str(), rc ) ;

      rc = _buildFields( cataSet, setObject, unsetObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build set fields for alter "
                   "task [%s] on collection [%s], rc: %d",
                   _task->getActionName(), _dataName.c_str(), rc ) ;

      if ( !setObject.isEmpty() || !unsetObject.isEmpty() )
      {
         rc = catAlterCLStep( _dataName, setObject, unsetObject,
                              cb, pDmsCB, pDpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to alter collection [%s], rc: %d",
                      _dataName.c_str(), rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK_EXECUTE_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK_ROLLBACK_INT, "_catCtxAlterCLTask::_rollbackInternal" )
   INT32 _catCtxAlterCLTask::_rollbackInternal ( _pmdEDUCB * cb,
                                                 SDB_DMSCB * pDmsCB,
                                                 SDB_DPSCB * pDpsCB,
                                                 INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK_ROLLBACK_INT ) ;

      BSONObj boCollection ;
      clsCatalogSet cataSet( _dataName.c_str() ) ;
      BSONObj setObject, unsetObject ;

      for ( ossPoolList< UINT64 >::const_iterator iter = _postTasks.begin() ;
            iter != _postTasks.end() ;
            iter ++ )
      {
         catRemoveTask( *iter, FALSE, cb, w ) ;
      }

      _postTasks.clear() ;

      rc = catGetCollection( _dataName, boCollection, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the Collection [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

      rc = cataSet.updateCatSet( boCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse catalog info [%s], rc: %d",
                   boCollection.toString().c_str(), rc ) ;

      rc = _buildRollbackFields( cataSet, setObject, unsetObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build fields for alter "
                   "task [%s] on collection [%s], rc: %d",
                   _task->getActionName(), _dataName.c_str(), rc ) ;

      if ( !setObject.isEmpty() || !unsetObject.isEmpty() )
      {
         rc = catAlterCLStep( _dataName, setObject, unsetObject,
                              cb, pDmsCB, pDpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to alter collection [%s], rc: %d",
                      _dataName.c_str(), rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK_ROLLBACK_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__CHKCRTIDIDX, "_catCtxAlterCLTask::_checkCreateIDIndex" )
   INT32 _catCtxAlterCLTask::_checkCreateIDIndex ( const clsCatalogSet & cataSet,
                                                   _pmdEDUCB * cb,
                                                   catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__CHKCRTIDIDX ) ;

      PD_CHECK( !OSS_BIT_TEST( cataSet.getAttribute(), DMS_MB_ATTR_CAPPED ),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to check [%s]: collection [%s] is capped",
                _task->getActionName(), _dataName.c_str() ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__CHKCRTIDIDX, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__CHKDROPIDIDX, "_catCtxAlterCLTask::_checkDropIDIndex" )
   INT32 _catCtxAlterCLTask::_checkDropIDIndex ( const clsCatalogSet & cataSet,
                                                 _pmdEDUCB * cb,
                                                 catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;
      INT64 splitTaskNum = 0 ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__CHKDROPIDIDX ) ;

      PD_CHECK( !OSS_BIT_TEST( cataSet.getAttribute(), DMS_MB_ATTR_CAPPED ),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to check [%s]: collection [%s] is capped",
                _task->getActionName(), _dataName.c_str() ) ;

      rc = catGetCLTaskCountByType( _dataName.c_str(), cb, CLS_TASK_SPLIT,
                                    splitTaskNum ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get split task number for "
                   "collection [%s]", _dataName.c_str() ) ;
      if ( splitTaskNum > 0 )
      {
         rc = SDB_OPERATION_CONFLICT ;
         PD_LOG( PDERROR, "Can not change AutoIndexId to false when collection "
                 "[%s] is being splitted", _dataName.c_str() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__CHKDROPIDIDX, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__CHKENABLESHD, "_catCtxAlterCLTask::_checkEnableShard" )
   INT32 _catCtxAlterCLTask::_checkEnableShard ( const clsCatalogSet & cataSet,
                                                 const rtnCLShardingArgument & argument,
                                                 _pmdEDUCB * cb,
                                                 catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__CHKENABLESHD ) ;

      PD_CHECK( !OSS_BIT_TEST( cataSet.getAttribute(), DMS_MB_ATTR_CAPPED ),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to check [%s]: collection [%s] is capped",
                _task->getActionName(), _dataName.c_str() ) ;

      if ( cataSet.isMainCL() &&
           _task->testFlags( RTN_ALTER_TASK_FLAG_MAINCLALLOW ) &&
           argument.testArgumentMask( UTIL_CL_SHDKEY_FIELD ) )
      {
         // Special case to alter sharding key of main-collection without any
         // sub-collections
         PD_CHECK( 0 == cataSet.getItemNum(),
                   SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                   "Failed to [%s]: Could not enable sharding in "
                   "main-collection", _task->getActionName() ) ;
      }
      else
      {
         PD_CHECK( !cataSet.isMainCL(),
                   SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                   "Failed to [%s]: Could not enable sharding in "
                   "main-collection", _task->getActionName() ) ;

         rc = _checkAutoSplit( cataSet, argument, cb, lockMgr ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to [%s]: Failed to check AutoSplit, rc: %d",
                      _task->getActionName(), rc ) ;

         if ( UTIL_CL_AUTOSPLIT_FIELD != argument.getArgumentMask() )
         {
            // Could be only executed on one group
            PD_CHECK( 1 == cataSet.groupCount(),
                      SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                      "Failed to [%s]: should have one group",
                      _task->getActionName() ) ;
         }
         else
         {
            // either the same auto split value or only one group
            PD_CHECK( ( ( cataSet.hasAutoSplit() &&
                          cataSet.isAutoSplit() == argument.isAutoSplit() ) ||
                        1 == cataSet.groupCount() ),
                        SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                        "Failed to [%s]: should have one group",
                        _task->getActionName() ) ;

         }
         if ( cataSet.isSharding() )
         {
            PD_LOG( PDWARNING, "Sharding is already enabled" ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__CHKENABLESHD, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__CHKDISABLESHD, "_catCtxAlterCLTask::_checkDisableShard" )
   INT32 _catCtxAlterCLTask::_checkDisableShard ( const clsCatalogSet & cataSet,
                                                  _pmdEDUCB * cb,
                                                  catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__CHKDISABLESHD ) ;

      PD_CHECK( !cataSet.isMainCL(), SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to [%s]: Could not disable sharding in main-collection",
                _task->getActionName() ) ;
      PD_CHECK( 1 == cataSet.groupCount(),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to [%s]: should have one group",
                _task->getActionName() ) ;
      PD_CHECK( !OSS_BIT_TEST( cataSet.getAttribute(), DMS_MB_ATTR_CAPPED ),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to check [%s]: collection [%s] is capped",
                _task->getActionName(), _dataName.c_str() ) ;

      if ( !cataSet.isSharding() )
      {
         PD_LOG( PDWARNING, "Sharding is already disabled" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__CHKDISABLESHD, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__CHKENABLECOMPRESS, "_catCtxAlterCLTask::_checkEnableCompress" )
   INT32 _catCtxAlterCLTask::_checkEnableCompress ( const clsCatalogSet & cataSet,
                                                    _pmdEDUCB * cb,
                                                    catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__CHKENABLECOMPRESS ) ;

      PD_CHECK( !OSS_BIT_TEST( cataSet.getAttribute(), DMS_MB_ATTR_CAPPED ),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to check [%s]: collection [%s] is capped",
                _task->getActionName(), _dataName.c_str() ) ;

      if ( UTIL_COMPRESSOR_INVALID != cataSet.getCompressType() )
      {
         PD_LOG( PDWARNING, "Compression already enabled" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__CHKENABLECOMPRESS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__CHKDISABLECOMPRESS, "_catCtxAlterCLTask::_checkDisableCompress" )
   INT32 _catCtxAlterCLTask::_checkDisableCompress ( const clsCatalogSet & cataSet,
                                                     _pmdEDUCB * cb,
                                                     catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__CHKDISABLECOMPRESS ) ;

      PD_CHECK( !OSS_BIT_TEST( cataSet.getAttribute(), DMS_MB_ATTR_CAPPED ),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to check [%s]: collection [%s] is capped",
                _task->getActionName(), _dataName.c_str() ) ;

      if ( UTIL_COMPRESSOR_INVALID == cataSet.getCompressType() )
      {
         PD_LOG( PDWARNING, "Compression already disabled" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__CHKDISABLECOMPRESS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__CHKSETATTR, "_catCtxAlterCLTask::_checkSetAttributes" )
   INT32 _catCtxAlterCLTask::_checkSetAttributes ( const clsCatalogSet & cataSet,
                                                   _pmdEDUCB * cb,
                                                   catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__CHKSETATTR ) ;

      const rtnCLSetAttributeTask * localTask =
                  dynamic_cast< const rtnCLSetAttributeTask * >( _task ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error,
                PDERROR, "Failed to get task" ) ;

      if ( localTask->containShardingArgument() )
      {
         rc = _checkEnableShard( cataSet, localTask->getShardingArgument(),
                                 cb, lockMgr ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check shard arguments, rc: %d", rc ) ;
      }

      if ( localTask->containCompressArgument() )
      {
         rc = _checkEnableCompress( cataSet, cb, lockMgr ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check compress arguments, rc: %d", rc ) ;
      }

      if ( localTask->containExtOptionArgument() )
      {
         PD_CHECK( OSS_BIT_TEST( cataSet.getAttribute(), DMS_MB_ATTR_CAPPED ),
                   SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                   "Failed to check capped arguments: collection [%s] is not "
                   "capped", _dataName.c_str() ) ;
      }

      if ( localTask->testArgumentMask( UTIL_CL_AUTOIDXID_FIELD ) )
      {
         PD_CHECK( !OSS_BIT_TEST( cataSet.getAttribute(), DMS_MB_ATTR_CAPPED ),
                   SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                   "Failed to check [%s]: collection [%s] is capped",
                   _task->getActionName(), _dataName.c_str() ) ;

         // Can not change AutoIndexId from true to false when split is in
         // progress.
         if ( !localTask->isAutoIndexID() )
         {
            INT64 splitTaskNum = 0 ;
            rc = catGetCLTaskCountByType( _dataName.c_str(), cb, CLS_TASK_SPLIT,
                                          splitTaskNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get split task number for "
                         "collection [%s]", _dataName.c_str() ) ;
            if ( splitTaskNum > 0 )
            {
               rc = SDB_OPERATION_CONFLICT ;
               PD_LOG( PDERROR, "Can not change AutoIndexId to false when "
                       "collection [%s] is being splitted",
                       _dataName.c_str() ) ;
               goto error ;
            }
         }
      }

      if ( localTask->containAutoincArgument() && !_subCLOFMainCL )
      {
         autoIncFieldsList fldList ;
         const clsAutoIncSet* autoIncSet = NULL ;

         autoIncSet = cataSet.getAutoIncSet() ;
         fldList = localTask->getAutoincFieldArgument() ;
         for( UINT32 i = 0 ; i < fldList.size() ; i++ )
         {
            const CHAR *fieldName = NULL ;
            const clsAutoIncItem *field = NULL ;
            fieldName = fldList[i]->getFieldName() ;
            PD_CHECK( 0 != ossStrcmp( fieldName, "" ), SDB_SYS, error, PDERROR,
                      "Failed to get field name[%s]", fieldName ) ;
            field = autoIncSet->find( fieldName ) ;
            PD_CHECK( NULL != field, SDB_AUTOINCREMENT_FIELD_NOT_EXIST,
                     error, PDERROR, "Field[%s] does not exist on "
                      "collection", fieldName ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__CHKSETATTR, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__CHKCRTAUTOINCFLD, "_catCtxAlterCLTask::_checkCreateAutoIncField" )
   INT32 _catCtxAlterCLTask::_checkCreateAutoIncField ( const clsCatalogSet & cataSet,
                                                        _pmdEDUCB * cb,
                                                        catCtxLockMgr & lockMgr )
   {
      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__CHKCRTAUTOINCFLD ) ;
      INT32 rc = SDB_OK ;
      autoIncFieldsList fieldList ;

      const rtnCLCreateAutoincFieldTask* localTask =
                  dynamic_cast< const rtnCLCreateAutoincFieldTask * >( _task ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                "Failed to get task" ) ;
      PD_CHECK( localTask->containAutoincArgument(), SDB_SYS, error, PDERROR,
                "Failed to get autoinc argument" ) ;

      fieldList = localTask->getAutoincrementArgument() ;
      for( UINT32 i = 0 ; i < fieldList.size() ; i++ )
      {
         const CHAR *fieldName = NULL ;
         BSONObjBuilder newFieldBld ;
         fieldName = fieldList[i]->getFieldName() ;
         PD_CHECK( ossStrcmp( fieldName, "") != 0, SDB_SYS, error, PDERROR,
                   "Failed to get field name[%s]", fieldName ) ;
      }
   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__CHKCRTAUTOINCFLD, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__CHKDROPAUTOINCFLD, "_catCtxAlterCLTask::_checkDropAutoIncField" )
   INT32 _catCtxAlterCLTask::_checkDropAutoIncField ( const clsCatalogSet & cataSet,
                                                      _pmdEDUCB * cb,
                                                      catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;
      const clsAutoIncSet* autoIncSet = NULL ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__CHKDROPAUTOINCFLD ) ;

      autoIncFieldsList fieldList ;
      const rtnCLDropAutoincFieldTask* localTask =
                  dynamic_cast< const rtnCLDropAutoincFieldTask * >( _task ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error,
                PDERROR, "Failed to get task" ) ;
      PD_CHECK( localTask->containAutoincArgument(), SDB_SYS, error, PDERROR,
                "Failed to get autoinc argument" ) ;
      fieldList = localTask->getAutoincrementArgument() ;
      autoIncSet = cataSet.getAutoIncSet() ;
      for( UINT32 i = 0 ; i < fieldList.size() ; i++ )
      {
         const CHAR *fieldName = NULL ;
         const clsAutoIncItem *field = NULL ;
         fieldName = fieldList[i]->getFieldName() ;
         PD_CHECK( 0 != ossStrcmp( fieldName, "" ), SDB_SYS, error, PDERROR,
                   "Failed to get field name[%s]", fieldName ) ;
         field = autoIncSet->find( fieldName ) ;
         if( NULL == field )
         {
            rc = SDB_OK ;
            PD_LOG( PDWARNING, "Field[%s] does not exist on collection, no "
                    "need to drop", fieldName ) ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__CHKDROPAUTOINCFLD, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__BLDFIELDS, "_catCtxAlterCLTask::_buildFields" )
   INT32 _catCtxAlterCLTask::_buildFields ( clsCatalogSet & cataSet,
                                            BSONObj & setObject,
                                            BSONObj & unsetObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__BLDFIELDS ) ;

      BSONObjBuilder setBuilder, unsetBuilder ;
      UINT32 attribute = cataSet.getAttribute() ;

      switch ( _task->getActionType() )
      {
         case RTN_ALTER_CL_CREATE_ID_INDEX :
         {
            OSS_BIT_CLEAR( attribute, DMS_MB_ATTR_NOIDINDEX ) ;
            rc = SDB_OK ;
            break ;
         }
         case RTN_ALTER_CL_DROP_ID_INDEX :
         {
            OSS_BIT_SET( attribute, DMS_MB_ATTR_NOIDINDEX ) ;
            rc = SDB_OK ;
            break ;
         }
         case RTN_ALTER_CL_ENABLE_SHARDING :
         {
            const rtnCLEnableShardingTask * localTask =
                     dynamic_cast< const rtnCLEnableShardingTask * >( _task ) ;
            PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                      "Failed to get task" ) ;

            // Save old sharding arguments
            rc = _fillShardingArgument( cataSet, _rollbackShardArgument ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to set rollback argument, "
                         "rc: %d", rc ) ;

            _rollbackShardArgument.setArgumentMask(
                        localTask->getShardingArgument().getArgumentMask() ) ;

            rc = _buildEnableShardFields( cataSet,
                                          localTask->getShardingArgument(),
                                          _postAutoSplit, attribute, setBuilder,
                                          unsetBuilder ) ;
            break ;
         }
         case RTN_ALTER_CL_DISABLE_SHARDING :
         {
            rc = _buildDisableShardFields( cataSet, attribute,
                                           setBuilder, unsetBuilder ) ;
            break ;
         }
         case RTN_ALTER_CL_ENABLE_COMPRESS :
         {
            const rtnCLEnableCompressTask * localTask =
                        dynamic_cast<const rtnCLEnableCompressTask *>( _task ) ;
            PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                      "Failed to get enable compression task" ) ;

            rc = _buildEnableCompressFields( cataSet,
                                             localTask->getCompressArgument(),
                                             attribute,
                                             setBuilder, unsetBuilder ) ;
            break ;
         }
         case RTN_ALTER_CL_DISABLE_COMPRESS :
         {
            rc = _buildDisableCompressFields( cataSet, attribute,
                                              setBuilder, unsetBuilder ) ;
            break ;
         }
         case RTN_ALTER_CL_SET_ATTRIBUTES :
         {
            rc = _buildSetAttributeFields( cataSet, attribute, setBuilder,
                                           unsetBuilder ) ;
            break ;
         }
         case RTN_ALTER_CL_CREATE_AUTOINC_FLD :
         {
            autoIncFieldsList fieldList ;
            const rtnCLCreateAutoincFieldTask* localTask =
                  dynamic_cast< const rtnCLCreateAutoincFieldTask * >( _task ) ;
            PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                      "Failed to get task" ) ;
            fieldList = localTask->getAutoincrementArgument() ;
            rc = _buildCreateAutoincFields( cataSet, fieldList,
                                            setBuilder, unsetBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build fields, rc: %d", rc ) ;
            _rollbackAutoIncFields = fieldList ;
            break ;
         }
         case RTN_ALTER_CL_DROP_AUTOINC_FLD :
         {
            autoIncFieldsList fieldList ;
            const rtnCLDropAutoincFieldTask* localTask =
                   dynamic_cast< const rtnCLDropAutoincFieldTask * >( _task ) ;
            PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                      "Failed to get task" ) ;
            fieldList = localTask->getAutoincrementArgument() ;
            rc = _buildDropAutoincFields( cataSet, fieldList, setBuilder,
                                          unsetBuilder, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build fields, rc: %d", rc ) ;
            break ;
         }
         default :
         {
            rc = SDB_INVALIDARG ;
            break ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to build fields, rc: %d", rc ) ;

      // The attribute is changed
      if ( attribute != cataSet.getAttribute() )
      {
         CHAR attrString[ 100 ] = { 0 } ;
         mbAttr2String( attribute, attrString, sizeof( attrString ) - 1 ) ;
         setBuilder.append( CAT_ATTRIBUTE_NAME, attribute ) ;
         setBuilder.append( FIELD_NAME_ATTRIBUTE_DESC, attrString ) ;
      }

      setObject = setBuilder.obj() ;
      unsetObject = unsetBuilder.obj() ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__BLDFIELDS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__BLDROLLBACKFIELDS, "_catCtxAlterCLTask::_buildRollbackFields" )
   INT32 _catCtxAlterCLTask::_buildRollbackFields ( clsCatalogSet & cataSet,
                                                    BSONObj & setObject,
                                                    BSONObj & unsetObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__BLDROLLBACKFIELDS ) ;

      BSONObjBuilder setBuilder, unsetBuilder ;
      UINT32 attribute = cataSet.getAttribute() ;

      switch ( _task->getActionType() )
      {
         case RTN_ALTER_CL_CREATE_ID_INDEX :
         case RTN_ALTER_CL_DROP_ID_INDEX :
         case RTN_ALTER_CL_DISABLE_SHARDING :
         case RTN_ALTER_CL_ENABLE_COMPRESS :
         case RTN_ALTER_CL_DISABLE_COMPRESS :
         {
            rc = SDB_OK ;
            break ;
         }
         case RTN_ALTER_CL_ENABLE_SHARDING :
         {
            if ( _rollbackShardArgument.isSharding() )
            {
               rc = _buildEnableShardFields( cataSet, _rollbackShardArgument,
                                             FALSE, attribute, setBuilder,
                                             unsetBuilder ) ;
            }
            else
            {
               rc = _buildDisableShardFields( cataSet, attribute, setBuilder,
                                              unsetBuilder ) ;
            }
            break ;
         }
         case RTN_ALTER_CL_SET_ATTRIBUTES :
         {
            const rtnCLSetAttributeTask * localTask = NULL ;

            localTask = dynamic_cast<const rtnCLSetAttributeTask *>( _task ) ;
            PD_CHECK( NULL != localTask, SDB_INVALIDARG, error, PDERROR,
                      "Failed to get task" ) ;
            if ( _task->testArgumentMask( UTIL_CL_SHDKEY_FIELD ) )
            {
               if ( _rollbackShardArgument.isSharding() )
               {
                  rc = _buildEnableShardFields( cataSet, _rollbackShardArgument,
                                                FALSE, attribute, setBuilder,
                                                unsetBuilder ) ;
               }
               else
               {
                  rc = _buildDisableShardFields( cataSet, attribute, setBuilder,
                                                 unsetBuilder ) ;
               }
            }
            if ( _rollbackAutoIncFields.size() )
            {
               rc = _buildSetAutoincFields( cataSet, _rollbackAutoIncFields,
                                            setBuilder, unsetBuilder, FALSE ) ;
            }
            break ;
         }
         case RTN_ALTER_CL_CREATE_AUTOINC_FLD :
         {
            if ( _rollbackAutoIncFields.size() )
            {
               rc = _buildDropAutoincFields( cataSet, _rollbackAutoIncFields,
                                             setBuilder, unsetBuilder, FALSE ) ;
            }
            break ;
         }
         case RTN_ALTER_CL_DROP_AUTOINC_FLD :
         {
            if ( _rollbackAutoIncFields.size() )
            {
               rc = _buildCreateAutoincFields( cataSet, _rollbackAutoIncFields,
                                               setBuilder, unsetBuilder ) ;
            }
            break ;
         }
         default :
         {
            rc = SDB_INVALIDARG ;
            break ;
         }
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build rollback fields, rc: %d", rc ) ;

      // The attribute is changed
      if ( attribute != cataSet.getAttribute() )
      {
         CHAR attrString[ 100 ] = { 0 } ;
         mbAttr2String( attribute, attrString, sizeof( attrString ) - 1 ) ;
         setBuilder.append( CAT_ATTRIBUTE_NAME, attribute ) ;
         setBuilder.append( FIELD_NAME_ATTRIBUTE_DESC, attrString ) ;
      }

      setObject = setBuilder.obj() ;
      unsetObject = unsetBuilder.obj() ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__BLDROLLBACKFIELDS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__BLDENABLESHD, "_catCtxAlterCLTask::_buildEnableShardFields" )
   INT32 _catCtxAlterCLTask::_buildEnableShardFields ( clsCatalogSet & cataSet,
                                                       const rtnCLShardingArgument & argument,
                                                       BOOLEAN postAutoSplit,
                                                       UINT32 & attribute,
                                                       BSONObjBuilder & setBuilder,
                                                       BSONObjBuilder & unsetBuilder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__BLDENABLESHD ) ;

      PD_CHECK( argument.isSharding(), SDB_NO_SHARDINGKEY, error, PDERROR,
                "Failed to build sharding fields: no sharding key" ) ;

      if ( argument.testArgumentMask( UTIL_CL_AUTOSPLIT_FIELD ) )
      {
         setBuilder.appendBool( CAT_DOMAIN_AUTO_SPLIT, argument.isAutoSplit() ) ;
      }

      if ( 1 == cataSet.groupCount() && !cataSet.isMainCL() )
      {
         if ( argument.testArgumentMask( UTIL_CL_SHDKEY_FIELD |
                                         UTIL_CL_SHDTYPE_FIELD |
                                         UTIL_CL_PARTITION_FIELD ) )
         {
            BSONArrayBuilder cataInfoBuilder( setBuilder.subarrayStart( CAT_CATALOGINFO_NAME ) ) ;
            BSONObjBuilder cataItemBuilder( cataInfoBuilder.subobjStart() ) ;
            BSONObj lowBound, upBound ;

            clsCatalogSet::POSITION position = cataSet.getFirstItem() ;
            clsCatalogItem * cataItem = cataSet.getNextItem( position ) ;

            PD_CHECK( cataItem && NULL == cataSet.getNextItem( position ),
                      SDB_SYS, error, PDERROR, "Failed to [%s]: should "
                      "have one group", _task->getActionName() ) ;

            cataItemBuilder.append( CAT_CATALOGGROUPID_NAME,
                                    (INT32)cataItem->getGroupID() ) ;
            cataItemBuilder.append( CAT_GROUPNAME_NAME,
                                    cataItem->getGroupName() ) ;
            if ( argument.isHashSharding() )
            {
               rc = catBuildInitHashBound( lowBound, upBound,
                                           argument.getPartition() ) ;
            }
            else
            {
               Ordering order = Ordering::make( argument.getShardingKey() ) ;
               rc = catBuildInitRangeBound( argument.getShardingKey(), order,
                                            lowBound, upBound ) ;
            }
            PD_RC_CHECK( rc, PDWARNING,
                         "Failed to build cata info bound, rc: %d", rc ) ;

            cataItemBuilder.append ( CAT_LOWBOUND_NAME, lowBound ) ;
            cataItemBuilder.append ( CAT_UPBOUND_NAME, upBound ) ;
            cataItemBuilder.done() ;
            cataInfoBuilder.done() ;
         }
         setBuilder.append( CAT_SHARDINGKEY_NAME, argument.getShardingKey() ) ;
         setBuilder.append( CAT_SHARDING_TYPE, argument.isHashSharding() ?
                                               FIELD_NAME_SHARDTYPE_HASH :
                                               FIELD_NAME_SHARDTYPE_RANGE ) ;
         setBuilder.appendBool( CAT_ENSURE_SHDINDEX,
                                argument.isEnsureShardingIndex() ) ;
         if( argument.isHashSharding() )
         {
            setBuilder.append( CAT_SHARDING_PARTITION,
                               argument.getPartition() ) ;

            /// optimize query on hash-sharding only sdb's version >= 1.12
            /// update version since 1.12.4
            setBuilder.append( CAT_INTERNAL_VERSION, CAT_INTERNAL_VERSION_3 ) ;
         }
      }
      else if ( argument.getArgumentMask() == UTIL_CL_SHDKEY_FIELD &&
                cataSet.isMainCL() )
      {
         PD_CHECK( 0 == cataSet.getItemNum(),
                   SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                   "Failed to [%s]: Could not enable sharding in "
                   "main-collection", _task->getActionName() ) ;
         setBuilder.append( CAT_SHARDINGKEY_NAME, argument.getShardingKey() ) ;
      }
      else if ( argument.getArgumentMask() != UTIL_CL_AUTOSPLIT_FIELD )
      {
         PD_CHECK( !cataSet.isMainCL(), SDB_OPTION_NOT_SUPPORT, error,
                   PDERROR, "Failed to [%s]: should not be main-collection",
                   _task->getActionName() ) ;
         PD_CHECK( 1 == cataSet.groupCount(), SDB_OPTION_NOT_SUPPORT, error,
                   PDERROR, "Failed to [%s]: should have one group",
                   _task->getActionName() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__BLDENABLESHD, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__BLDDISABLESHD, "_catCtxAlterCLTask::_buildDisableShardFields" )
   INT32 _catCtxAlterCLTask::_buildDisableShardFields ( clsCatalogSet & cataSet,
                                                        UINT32 & attribute,
                                                        BSONObjBuilder & setBuilder,
                                                        BSONObjBuilder & unsetBuilder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__BLDDISABLESHD ) ;

      if ( 1 == cataSet.groupCount() &&
           !cataSet.isMainCL() )
      {
         BSONArrayBuilder cataInfoBuilder( setBuilder.subarrayStart( CAT_CATALOGINFO_NAME ) ) ;
         BSONObjBuilder cataItemBuilder( cataInfoBuilder.subobjStart() ) ;

         clsCatalogSet::POSITION position = cataSet.getFirstItem() ;
         clsCatalogItem * cataItem = cataSet.getNextItem( position ) ;

         PD_CHECK( NULL != cataItem && NULL == cataSet.getNextItem( position ),
                   SDB_SYS, error, PDERROR, "Failed to [%s]: should "
                   "have one group", _task->getActionName() ) ;

         cataItemBuilder.append( CAT_CATALOGGROUPID_NAME,
                                 (INT32)cataItem->getGroupID() ) ;
         cataItemBuilder.append( CAT_GROUPNAME_NAME,
                                 cataItem->getGroupName() ) ;

         cataItemBuilder.done() ;
         cataInfoBuilder.done() ;

         unsetBuilder.append( CAT_SHARDINGKEY_NAME, 1 ) ;
         unsetBuilder.append( CAT_ENSURE_SHDINDEX, 1 ) ;
         unsetBuilder.append( CAT_SHARDING_TYPE, 1 ) ;
         unsetBuilder.append( CAT_SHARDING_PARTITION, 1 ) ;
         unsetBuilder.append( CAT_INTERNAL_VERSION, 1 ) ;
         unsetBuilder.append( CAT_DOMAIN_AUTO_SPLIT, 1 ) ;

         // Use set fields to remove low and up bounds
         //unsetBuilder.append( CAT_CATALOGINFO_NAME ".0." CAT_LOWBOUND_NAME, 1 ) ;
         //unsetBuilder.append( CAT_CATALOGINFO_NAME ".0." CAT_UPBOUND_NAME, 1 ) ;
      }
      else
      {
         PD_CHECK( !cataSet.isMainCL(), SDB_OPTION_NOT_SUPPORT, error,
                   PDERROR, "Failed to [%s]: should not be main-collection",
                   _task->getActionName() ) ;
         PD_CHECK( 1 == cataSet.groupCount(), SDB_OPTION_NOT_SUPPORT, error,
                   PDERROR, "Failed to [%s]: should have one group",
                   _task->getActionName() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__BLDDISABLESHD, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__BLDENABLECOMPRESS, "_catCtxAlterCLTask::_buildEnableCompressFields" )
   INT32 _catCtxAlterCLTask::_buildEnableCompressFields ( clsCatalogSet & cataSet,
                                                          const rtnCLCompressArgument & argument,
                                                          UINT32 & attribute,
                                                          BSONObjBuilder & setBuilder,
                                                          BSONObjBuilder & unsetBuilder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__BLDENABLECOMPRESS ) ;

      // Alter the attribute only in below cases :
      // 1. collection is no compressed
      // 2. altering the compressor type, and old type of collection is
      //    different
      if ( !OSS_BIT_TEST( cataSet.getAttribute(), DMS_MB_ATTR_COMPRESSED ) ||
           ( argument.testArgumentMask( UTIL_CL_COMPRESSTYPE_FIELD ) &&
             cataSet.getCompressType() != argument.getCompressorType() ) )
      {
         OSS_BIT_SET( attribute, DMS_MB_ATTR_COMPRESSED ) ;

         setBuilder.append( CAT_COMPRESSIONTYPE,
                            argument.getCompressorType() ) ;
         setBuilder.append( FIELD_NAME_COMPRESSIONTYPE_DESC,
                            argument.getCompressionName() ) ;
      }

      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__BLDENABLECOMPRESS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__BLDDISABLECOMPRESS, "_catCtxAlterCLTask::_buildDisableCompressFields" )
   INT32 _catCtxAlterCLTask::_buildDisableCompressFields ( clsCatalogSet & cataSet,
                                                           UINT32 & attribute,
                                                           BSONObjBuilder & setBuilder,
                                                           BSONObjBuilder & unsetBuilder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__BLDDISABLECOMPRESS ) ;

      OSS_BIT_CLEAR( attribute, DMS_MB_ATTR_COMPRESSED ) ;

      unsetBuilder.append( CAT_COMPRESSIONTYPE, 1 ) ;
      unsetBuilder.append( FIELD_NAME_COMPRESSIONTYPE_DESC, 1 ) ;

      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__BLDDISABLECOMPRESS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__BLDEXTOPT, "_catCtxAlterCLTask::_buildExtOptionFields" )
   INT32 _catCtxAlterCLTask::_buildExtOptionFields ( clsCatalogSet & cataSet,
                                                     const rtnCLExtOptionArgument & argument,
                                                     UINT32 & attribute,
                                                     bson::BSONObjBuilder & setBuilder,
                                                     bson::BSONObjBuilder & unsetBuilder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__BLDEXTOPT ) ;

      PD_CHECK( OSS_BIT_TEST( cataSet.getAttribute(), DMS_MB_ATTR_CAPPED ),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to check capped arguments: collection [%s] is not "
                "capped", _dataName.c_str() ) ;

      if ( !cataSet.isMainCL() )
      {
         if ( argument.testArgumentMask( UTIL_CL_MAXSIZE_FIELD ) )
         {
            setBuilder.append( CAT_CL_MAX_SIZE, (INT64)argument.getMaxSize() ) ;
         }
         if ( argument.testArgumentMask( UTIL_CL_MAXREC_FIELD ) )
         {
            setBuilder.append( CAT_CL_MAX_RECNUM,
                               (INT64)argument.getMaxRec() ) ;
         }
         if ( argument.testArgumentMask( UTIL_CL_OVERWRITE_FIELD ) )
         {
            setBuilder.appendBool( CAT_CL_OVERWRITE, argument.isOverWrite() ) ;
         }
      }
      else
      {
         PD_CHECK( !cataSet.isMainCL(), SDB_OPTION_NOT_SUPPORT, error,
                   PDERROR, "Failed to [%s]: should not be main-collection",
                   _task->getActionName() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__BLDEXTOPT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__BLDSETATTR, "_catCtxAlterCLTask::_buildSetAttributeFields" )
   INT32 _catCtxAlterCLTask::_buildSetAttributeFields ( clsCatalogSet & cataSet,
                                                        UINT32 & attribute,
                                                        BSONObjBuilder & setBuilder,
                                                        BSONObjBuilder & unsetBuilder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__BLDSETATTR ) ;

      const rtnCLSetAttributeTask * localTask =
                  dynamic_cast< const rtnCLSetAttributeTask * >( _task ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error,
                PDERROR, "Failed to get task" ) ;

      if ( localTask->containShardingArgument() )
      {
         rtnCLShardingArgument argument = localTask->getShardingArgument() ;

         // Save old sharding arguments
         rc = _fillShardingArgument( cataSet, _rollbackShardArgument ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set rollback argument, "
                      "rc: %d", rc ) ;

         _rollbackShardArgument.setArgumentMask( argument.getArgumentMask() ) ;

         rc = _fillShardingArgument( cataSet, argument ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to fill sharding arguments, rc: %d", rc ) ;

         rc = _buildEnableShardFields( cataSet, argument, _postAutoSplit,
                                       attribute, setBuilder, unsetBuilder ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to build sharding fields, rc: %d", rc ) ;
      }

      if ( localTask->containCompressArgument() )
      {
         rtnCLCompressArgument argument = localTask->getCompressArgument() ;

         if ( argument.isCompressed() )
         {
            rc = _buildEnableCompressFields( cataSet, argument, attribute,
                                             setBuilder, unsetBuilder ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to build shard fields, rc: %d", rc ) ;
         }
         else
         {
            rc = _buildDisableCompressFields( cataSet, attribute,
                                              setBuilder, unsetBuilder ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to build shard fields, rc: %d", rc ) ;
         }
      }

      if ( localTask->containExtOptionArgument() )
      {
         rc = _buildExtOptionFields( cataSet, localTask->getExtOptionArgument(),
                                     attribute, setBuilder, unsetBuilder ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to build capped fields, rc: %d", rc ) ;
      }

      if ( localTask->containAutoincArgument() && !_subCLOFMainCL )
      {
         autoIncFieldsList fieldList ;
         fieldList = localTask->getAutoincFieldArgument() ;
         rc = _buildSetAutoincFields( cataSet, fieldList,
                                      setBuilder, unsetBuilder, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to build set autoinc fields, rc: %d", rc ) ;
         _rollbackAutoIncFields = fieldList ;
      }

      if ( localTask->testArgumentMask( UTIL_CL_REPLSIZE_FIELD ) )
      {
         setBuilder.append( CAT_CATALOG_W_NAME, localTask->getReplSize() ) ;
      }

      if ( localTask->testArgumentMask( UTIL_CL_STRICTDATAMODE_FIELD ) )
      {
         if ( localTask->isStrictDataMode() )
         {
            OSS_BIT_SET( attribute, DMS_MB_ATTR_STRICTDATAMODE ) ;
         }
         else
         {
            OSS_BIT_CLEAR( attribute, DMS_MB_ATTR_STRICTDATAMODE ) ;
         }
      }

      if ( localTask->testArgumentMask( UTIL_CL_AUTOIDXID_FIELD ) )
      {
         if ( localTask->isAutoIndexID() )
         {
            OSS_BIT_CLEAR( attribute, DMS_MB_ATTR_NOIDINDEX ) ;
         }
         else
         {
            OSS_BIT_SET( attribute, DMS_MB_ATTR_NOIDINDEX ) ;
         }
      }

      if ( localTask->testArgumentMask( UTIL_CL_AUTOREBALANCE_FIELD ) )
      {
         setBuilder.appendBool( CAT_DOMAIN_AUTO_REBALANCE,
                                localTask->isAutoRebalance() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__BLDSETATTR, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__BLDSETAUTOINCFIELDS, "_catCtxAlterCLTask::_buildSetAutoincFields" )
   INT32 _catCtxAlterCLTask::_buildSetAutoincFields ( clsCatalogSet & cataSet,
                                                     autoIncFieldsList &fieldList,
                                                     BSONObjBuilder & setBuilder,
                                                     BSONObjBuilder & unsetBuilder,
                                                     BOOLEAN addRbk )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasChanged = FALSE ;
      const CHAR *fieldName = NULL ;
      clsAutoIncSet *autoIncSet = NULL ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__BLDSETAUTOINCFIELDS ) ;

      try
      {
         autoIncSet = cataSet.getAutoIncSet() ;
         for( UINT32 i = 0 ; i < fieldList.size() ; i++ )
         {
            const clsAutoIncItem *field = NULL ;
            fieldName = fieldList[i]->getFieldName() ;
            PD_CHECK( fieldName, SDB_SYS, error,
                      PDERROR, "Failed to get field name" ) ;
            field = autoIncSet->find( fieldName ) ;
            PD_CHECK( NULL != field, SDB_AUTOINCREMENT_FIELD_NOT_EXIST,
                     error, PDERROR, "Field[%s] does not exist on "
                      "collection", fieldName ) ;
            if( fieldList[i]->testArgumentMask( UTIL_CL_AUTOINC_GENERATED_FIELD ) )
            {
               const CHAR *generated = NULL ;
               generated = field->generated() ;
               PD_CHECK( generated, SDB_SYS, error, PDERROR,
                         "Invalid generated type[%d] in catalog sequence field"
                         "[%s]", field->generatedType(), field->fieldName() ) ;
               if( generated &&
                   0 != ossStrcmp( generated, fieldList[i]->getGenerated() ) )
               {
                  BSONObjBuilder newFieldBld ;
                  newFieldBld.append( FIELD_NAME_AUTOINC_SEQ,
                                      field->sequenceName() ) ;
                  newFieldBld.append( FIELD_NAME_AUTOINC_FIELD, fieldName ) ;
                  newFieldBld.append( CAT_AUTOINC_GENERATED,
                                      fieldList[i]->getGenerated() ) ;
                  PD_CHECK( UTIL_SEQUENCEID_NULL != field->sequenceID(),
                            SDB_SYS, error, PDERROR,
                            "Catalog is invalid, failed to get field[%s]",
                            FIELD_NAME_AUTOINC_SEQ_ID ) ;
                  newFieldBld.append( FIELD_NAME_AUTOINC_SEQ_ID,
                                      (INT64)field->sequenceID() ) ;
                  autoIncSet->erase( fieldName ) ;
                  rc = autoIncSet->insert( newFieldBld.obj() ) ;
                  PD_RC_CHECK( rc, PDERROR,
                               "Failed to insert autoinc field[%s], rc: %d",
                               fieldName, rc ) ;
                  hasChanged = TRUE ;
               }
               if( addRbk )
               {
                  fieldList[i]->setGenerated( generated ) ;
                  fieldList[i]->setID( field->sequenceID() ) ;
               }
            }
         }
         if( hasChanged )
         {
            setBuilder.appendElements( autoIncSet->toBson() ) ;
         }
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build fields for alter collection[%s], "
                 "exception=%s", cataSet.name(), e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__BLDSETAUTOINCFIELDS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__BLDCREATEAUTOINCFIELDS, "_catCtxAlterCLTask::_buildCreateAutoincFields" )
   INT32 _catCtxAlterCLTask::_buildCreateAutoincFields ( clsCatalogSet & cataSet,
                                                         autoIncFieldsList & fieldList,
                                                         BSONObjBuilder & setBuilder,
                                                         BSONObjBuilder & unsetBuilder )
   {
      INT32 rc = SDB_OK ;
      const CHAR *fieldName = NULL ;
      utilCLUniqueID clUniqueID ;
      BOOLEAN hasSetGen = FALSE ;
      clsAutoIncSet * autoIncSet = cataSet.getAutoIncSet() ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__BLDCREATEAUTOINCFIELDS ) ;

      try
      {
         clUniqueID = cataSet.clUniqueID() ;
         for( UINT32 i = 0 ; i < fieldList.size() ; i++ )
         {
            string seqName ;
            BSONObjBuilder newFieldBld ;
            utilSequenceID ID = UTIL_SEQUENCEID_NULL ;
            fieldName = fieldList[i]->getFieldName() ;
            PD_CHECK( 0 != ossStrcmp( fieldName, "" ), SDB_SYS, error, PDERROR,
                      "Failed to get field name[%s]", fieldName ) ;
            seqName = catGetSeqName4AutoIncFld( clUniqueID, fieldName ) ;
            newFieldBld.append( FIELD_NAME_AUTOINC_SEQ, seqName ) ;
            newFieldBld.append( FIELD_NAME_AUTOINC_FIELD, fieldName ) ;
            hasSetGen = fieldList[i]->testArgumentMask( UTIL_CL_AUTOINC_GENERATED_FIELD ) ;
            newFieldBld.append( CAT_AUTOINC_GENERATED,
                                hasSetGen ? fieldList[i]->getGenerated() :
                                            CAT_GENERATED_DEFAULT ) ;
            ID = fieldList[i]->getID() ;
            PD_CHECK( ID != UTIL_SEQUENCEID_NULL, SDB_SYS, error, PDERROR,
                      "Failed to get field[%s]", FIELD_NAME_AUTOINC_SEQ_ID ) ;
            newFieldBld.append( FIELD_NAME_AUTOINC_SEQ_ID, (INT64)ID ) ;
            rc = autoIncSet->insert( newFieldBld.obj() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to insert autoinc field[%s], "
                         "rc: %d", fieldName, rc ) ;
         }
         setBuilder.appendElements( autoIncSet->toBson() ) ;
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build fields for on collection[%s], "
                 "exception=%s", cataSet.name(), e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__BLDCREATEAUTOINCFIELDS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__BLDDROPAUTOINCFIELDS, "_catCtxAlterCLTask::_buildDropAutoincFields" )
   INT32 _catCtxAlterCLTask::_buildDropAutoincFields ( clsCatalogSet & cataSet,
                                                       autoIncFieldsList & fieldList,
                                                       BSONObjBuilder & setBuilder,
                                                       BSONObjBuilder & unsetBuilder,
                                                       BOOLEAN addRbk )
   {
      INT32 rc = SDB_OK ;
      const CHAR *fieldName = NULL ;
      bson::BSONElement ele ;
      bson::BSONObjBuilder builder ;
      clsAutoIncSet *autoIncSet = cataSet.getAutoIncSet() ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__BLDDROPAUTOINCFIELDS ) ;

      try
      {
         for( UINT32 i = 0 ; i < fieldList.size() ; i++ )
         {
            const clsAutoIncItem *field = NULL ;
            fieldName = fieldList[i]->getFieldName() ;
            PD_CHECK( fieldName, SDB_SYS, error, PDERROR, "Failed to "
                      "get field name[%s]", fieldName ) ;
            field = autoIncSet->find( fieldName ) ;
            PD_CHECK( NULL != field, SDB_AUTOINCREMENT_FIELD_NOT_EXIST,
                     error, PDERROR, "Field[%s] does not exist on "
                      "collection", fieldName ) ;
            if( addRbk )
            {
               fieldList[i]->setID( field->sequenceID() ) ;
               if( AUTOINC_GEN_DEFAULT != field->generatedType() )
               {
                  fieldList[i]->setGenerated( field->generated() ) ;
                  fieldList[i]->parsedArgumentMask( UTIL_CL_AUTOINC_GENERATED_FIELD ) ;
               }
               _rollbackAutoIncFields.push_back( fieldList[i] ) ;
            }
            autoIncSet->erase( fieldName ) ;
         }
         setBuilder.appendElements( autoIncSet->toBson() ) ;
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build fields on collection[%s], "
                 "exception=%s", cataSet.name(), e.what() ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__BLDDROPAUTOINCFIELDS, rc ) ;
      return rc ;

   error :
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__FILLSHDARG, "_catCtxAlterCLTask::_fillShardingArgument" )
   INT32 _catCtxAlterCLTask::_fillShardingArgument ( clsCatalogSet & cataSet,
                                                     rtnCLShardingArgument & argument )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__FILLSHDARG ) ;

      if ( !argument.testArgumentMask( UTIL_CL_SHDKEY_FIELD ) )
      {
         argument.setShardingKey( cataSet.getShardingKey() ) ;
      }

      if ( argument.isSharding() )
      {
         if ( !argument.testArgumentMask( UTIL_CL_SHDTYPE_FIELD ) )
         {
            if ( cataSet.isSharding() )
            {
               argument.setHashSharding( cataSet.isHashSharding() ) ;
            }
            else
            {
               argument.setHashSharding( TRUE ) ;
            }
         }
         if ( !argument.testArgumentMask(UTIL_CL_PARTITION_FIELD ) &&
              argument.isHashSharding() )
         {
            argument.setPartition( cataSet.getHashPartition() ) ;
         }
         if ( !argument.testArgumentMask( UTIL_CL_ENSURESHDIDX_FIELD ) )
         {
            argument.setEnsureShardingIndex( cataSet.ensureShardingIndex() ) ;
         }

         if ( argument.isAutoSplit() )
         {
            PD_CHECK( argument.isHashSharding() &&
                      ( 1 == cataSet.groupCount() ||
                        cataSet.isAutoSplit() ),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to fill sharding argument: "
                      "AutoSplit should be used in hash sharding only" ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__FILLSHDARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__BLDPOSTTASKS, "_catCtxAlterCLTask::_buildPostTasks" )
   INT32 _catCtxAlterCLTask::_buildPostTasks ( clsCatalogSet & cataSet,
                                               _pmdEDUCB * cb,
                                               SDB_DMSCB * pDmsCB,
                                               SDB_DPSCB * pDpsCB,
                                               INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__BLDPOSTTASKS ) ;

      switch ( _task->getActionType() )
      {
         case RTN_ALTER_CL_CREATE_ID_INDEX :
         case RTN_ALTER_CL_DROP_ID_INDEX :
         case RTN_ALTER_CL_ENABLE_COMPRESS :
         case RTN_ALTER_CL_DISABLE_COMPRESS :
         case RTN_ALTER_CL_DISABLE_SHARDING :
         case RTN_ALTER_CL_CREATE_AUTOINC_FLD :
         {
            rc = SDB_OK ;
            break ;
         }
         case RTN_ALTER_CL_ENABLE_SHARDING :
         {
            if ( _postAutoSplit )
            {
               const rtnCLEnableShardingTask * localTask =
                           dynamic_cast< const rtnCLEnableShardingTask * >( _task ) ;
               PD_CHECK( NULL != localTask, SDB_SYS, error,
                         PDERROR, "Failed to get task" ) ;

               if ( cataSet.groupCount() == 1 &&
                    cataSet.isHashSharding() )
               {
                  rc = _buildAutoHashSplit( cataSet, cb, pDmsCB, pDpsCB, w ) ;
               }
               else
               {
                  PD_LOG( PDWARNING, "Failed to build auto hash split tasks: "
                          "more than one group" ) ;
               }
            }
            break ;
         }
         case RTN_ALTER_CL_SET_ATTRIBUTES :
         {
            const rtnCLSetAttributeTask * localTask =
                        dynamic_cast< const rtnCLSetAttributeTask * >( _task ) ;
            PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                      "Failed to get task" ) ;
            if ( _postAutoSplit )
            {
               if ( cataSet.groupCount() == 1 &&
                    cataSet.isHashSharding() )
               {
                  rc = _buildAutoHashSplit( cataSet, cb, pDmsCB, pDpsCB, w ) ;
               }
               else
               {
                  PD_LOG( PDWARNING, "Failed to build auto hash split tasks: "
                          "more than one group" ) ;
               }
            }
            if( localTask->containAutoincArgument() )
            {
               autoIncFieldsList fldList ;
               fldList = localTask->getAutoincFieldArgument() ;
               rc = _buildSequenceNames( cataSet, fldList, cb, w);
            }
            break ;
         }
         case RTN_ALTER_CL_DROP_AUTOINC_FLD :
         {
            const rtnCLDropAutoincFieldTask* localTask =
                    dynamic_cast< const rtnCLDropAutoincFieldTask * >( _task ) ;
            PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                      "Failed to get task" ) ;
            autoIncFieldsList fldList ;
            fldList = localTask->getAutoincrementArgument() ;
            rc = _buildSequenceNames( cataSet, fldList, cb, w);
            break ;
         }
         default :
         {
            rc = SDB_INVALIDARG ;
            break ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to build post tasks, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__BLDPOSTTASKS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__BLDSPLITTASKS, "_catCtxAlterCLTask::_buildAutoHashSplit" )
   INT32 _catCtxAlterCLTask::_buildAutoHashSplit ( clsCatalogSet & cataSet,
                                                   _pmdEDUCB * cb,
                                                   SDB_DMSCB * pDmsCB,
                                                   SDB_DPSCB * pDpsCB,
                                                   INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__BLDSPLITTASKS ) ;

      sdbCatalogueCB * catCB = pmdGetKRCB()->getCATLOGUECB() ;

      const CHAR * srcGroup = NULL ;
      CAT_DOMAIN_GROUP_MAP dstGroups ;

      UINT32 totalBound = (UINT32)cataSet.getHashPartition() ;
      const CHAR * collection = cataSet.name() ;

      clsCatalogSet::POSITION position = cataSet.getFirstItem() ;
      clsCatalogItem * cataItem = cataSet.getNextItem( position ) ;

      PD_CHECK( NULL != cataItem && NULL == cataSet.getNextItem( position ),
                SDB_SYS, error, PDERROR, "Failed to [%s]: should "
                "have one group", _task->getActionName() ) ;

      srcGroup = cataItem->getGroupName().c_str() ;

      rc = catGetSplitCandidateGroups( collection, dstGroups, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get split candidate groups, "
                   "rc: %d", rc ) ;

      if ( 1 < dstGroups.size() )
      {
         UINT32 avgBound = totalBound / dstGroups.size() ;
         UINT32 modBound = totalBound % dstGroups.size() ;
         UINT32 endBound = totalBound ;
         UINT32 beginBound = totalBound - avgBound ;

         for ( CAT_DOMAIN_GROUP_MAP::const_iterator it = dstGroups.begin() ;
               it != dstGroups.end();
               it ++ )
         {
            BSONObj splitInfo ;
            UINT32 dstGroupID = CAT_INVALID_GROUPID ;
            INT32 version = 0 ;
            UINT64 taskID = CLS_INVALID_TASKID ;
            const CHAR * dstGroup = it->first.c_str() ;

            if ( 0 == ossStrcmp( srcGroup, dstGroup ) )
            {
               continue ;
            }

            if ( modBound > 0 )
            {
               --modBound ;
               --beginBound ;
            }

            taskID = catCB->getCatlogueMgr()->assignTaskID() ;
            rc = catBuildHashSplitTask( collection, cataSet.clUniqueID(),
                                        srcGroup, dstGroup,
                                        beginBound, endBound, splitInfo ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to build split info, rc: %d", rc ) ;

            rc = catSplitReady( splitInfo, taskID, FALSE, cb, w,
                                dstGroupID, version ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to split collection [%s], rc: %d",
                         collection, rc ) ;

            endBound = beginBound ;
            beginBound = endBound - avgBound ;

            _postTasks.push_back( taskID ) ;
         }
      }
      else
      {
         PD_LOG( PDINFO,
                 "split range size: %d, do nothing.", dstGroups.size() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__BLDSPLITTASKS, rc ) ;
      return rc ;

   error :
      for ( ossPoolList< UINT64 >::const_iterator iter = _postTasks.begin() ;
            iter != _postTasks.end() ;
            iter ++ )
      {
         catRemoveTask( *iter, FALSE, cb, w ) ;
      }

      _postTasks.clear() ;

      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__BLDSEQNAMES, "_catCtxAlterCLTask::_buildSequenceNames" )
   INT32 _catCtxAlterCLTask::_buildSequenceNames ( clsCatalogSet & cataSet,
                                                   autoIncFieldsList &fldList,
                                                   _pmdEDUCB * cb,
                                                   INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__BLDSEQNAMES ) ;

      sdbCatalogueCB * catCB = pmdGetKRCB()->getCATLOGUECB() ;
      UINT64 taskID = CLS_INVALID_TASKID ;
      const clsAutoIncSet * autoIncSet = cataSet.getAutoIncSet() ;

      for( UINT32 i = 0 ; i < fldList.size() ; i++ )
      {
         BSONObj seqInfo ;
         BSONObjBuilder bld ;
         const CHAR *fieldName = NULL ;
         const CHAR *seqName = NULL ;
         const clsAutoIncItem *field = NULL ;

         fieldName = fldList[i]->getFieldName() ;
         PD_CHECK( fieldName, SDB_SYS, error, PDERROR, "Failed to get field "
                   "name[%s]", fieldName ) ;
         field = autoIncSet->find( fieldName ) ;
         if( NULL == field )
         {
            PD_LOG( PDWARNING, "Field[%s] does not exist on collection, "
                    "no need to build post task", fieldName ) ;
            rc = SDB_OK ;
            goto done ;
         }
         taskID = catCB->getCatlogueMgr()->assignTaskID() ;
         PD_CHECK( taskID != CLS_INVALID_TASKID,
                   SDB_INVALIDARG, error, PDERROR,
                   "Invalid task ID for sequence task" ) ;
         seqName = field->sequenceName() ;
         bld.append( CAT_TASKID_NAME, (INT64)taskID ) ;
         bld.append( CAT_TASKTYPE_NAME, CLS_TASK_SEQUENCE ) ;
         bld.append( CAT_COLLECTION_NAME, _dataName.c_str() ) ;
         bld.append( FIELD_NAME_AUTOINC_SEQ, seqName ) ;
         bld.append( CAT_AUTOINC_SEQ_ID , (INT64)field->sequenceID() ) ;
         seqInfo = bld.obj() ;
         rc = catAddTask( seqInfo, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Add sequence task failed, rc: %d", rc ) ;
         _postTasks.push_back( taskID ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__BLDSEQNAMES, rc ) ;
      return rc ;

   error :
      for ( ossPoolList< UINT64 >::const_iterator iter = _postTasks.begin() ;
            iter != _postTasks.end() ;
            iter ++ )
      {
         catRemoveTask( *iter, FALSE, cb, w ) ;
      }

      _postTasks.clear() ;

      goto done ;

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCLTASK__CHKAUTOSPLIT, "_catCtxAlterCLTask::_checkAutoSplit" )
   INT32 _catCtxAlterCLTask::_checkAutoSplit ( const clsCatalogSet & cataSet,
                                               const rtnCLShardingArgument & argument,
                                               _pmdEDUCB * cb,
                                               catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCLTASK__CHKAUTOSPLIT ) ;

      BOOLEAN autoSplit = FALSE ;

      if ( argument.testArgumentMask( UTIL_CL_AUTOSPLIT_FIELD ) )
      {
         // AutoSplit is given explicitly
         autoSplit = argument.isAutoSplit() ;
      }
      else if ( cataSet.isAutoSplit() )
      {
         // AutoSplit is specified already by collection itself
         autoSplit = TRUE ;
      }
      else if ( !cataSet.hasAutoSplit() && argument.isHashSharding() &&
                ( !cataSet.isSharding() ||
                  cataSet.isHashSharding() ||
                  ( cataSet.isRangeSharding() &&
                    argument.testArgumentMask( UTIL_CL_SHDTYPE_FIELD ) ) ) )
      {
         // AutoSplit is NOT given explicitly by alter command and collection
         // itself, in one of below cases:
         // 1. collection is NOT sharding before, and is hash sharding after
         // 2. collection is hash sharding before and after
         // 3. collection is range sharding before, but is hash sharding after
         // then, check with domain, use the AutoSplit of domain setting

         const CHAR * collection = cataSet.name() ;
         CHAR szSpace [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ]  = {0} ;
         CHAR szCollection [ DMS_COLLECTION_NAME_SZ + 1 ] = {0} ;

         BSONObj csObj ;
         BOOLEAN csExist = FALSE ;

         rc = rtnResolveCollectionName( collection, ossStrlen( collection ),
                                        szSpace, DMS_COLLECTION_SPACE_NAME_SZ,
                                        szCollection, DMS_COLLECTION_NAME_SZ ) ;
         PD_RC_CHECK( rc, PDERROR, "Resolve collection name[%s] failed, rc: %d",
                      collection, rc ) ;

         rc = catCheckSpaceExist( szSpace, csExist, csObj, cb ) ;
         PD_RC_CHECK( rc, PDWARNING, "Check collection space[%s] exist failed, "
                      "rc: %d", szSpace, rc ) ;
         PD_CHECK( csExist, SDB_DMS_CS_NOTEXIST, error, PDWARNING,
                   "Collection space[%s] is not exist", szSpace ) ;

         // get domain name
         const CHAR * domainName = NULL ;
         rc = rtnGetStringElement( csObj, CAT_DOMAIN_NAME, &domainName ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            // SYSTEM domain is not auto split
            autoSplit = FALSE ;
            rc = SDB_OK ;
         }
         else if ( SDB_OK == rc )
         {
            // Check domain exist
            BSONObj domainObj ;

            rc = catGetAndLockDomain( domainName, domainObj, cb, &lockMgr,
                                      SHARED ) ;
            PD_RC_CHECK( rc, PDERROR, "Get domain[%s] failed, rc: %d",
                         domainName, rc ) ;

            BSONElement split = domainObj.getField( CAT_DOMAIN_AUTO_SPLIT ) ;
            if ( Bool == split.type() )
            {
               autoSplit = split.Bool() ;
            }
         }
         else
         {
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field [%s] from collection space [%s], "
                         "rc: %d", CAT_DOMAIN_NAME, szSpace, rc ) ;
         }
      }

      if ( autoSplit )
      {
         // Should have sharding keys for auto split
         PD_CHECK( !cataSet.getShardingKey().isEmpty() ||
                   argument.testArgumentMask( UTIL_CL_SHDKEY_FIELD ),
                   SDB_NO_SHARDINGKEY, error, PDERROR,
                   "Failed to [%s]: should have sharding key",
                   _task->getActionName() ) ;

         // Should be hash sharding for auto split
         if ( argument.testArgumentMask( UTIL_CL_SHDTYPE_FIELD ) )
         {
            // Altering to hash sharding
            PD_CHECK( argument.isHashSharding(),
                      SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                      "Failed to [%s]: enable AutoSplit should be hash "
                      "sharding", _task->getActionName() ) ;
         }
         else
         {
            // Already hash sharding
            PD_CHECK( cataSet.isHashSharding() || argument.isHashSharding(),
                      SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                      "Failed to [%s]: enable AutoSplit should be hash "
                      "sharding", _task->getActionName() ) ;
         }

         PD_LOG( PDDEBUG, "[%s] on collection [%s]: AutoSplit is set to TRUE",
                 _task->getActionName(), cataSet.name() ) ;
      }

      _postAutoSplit = autoSplit ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCLTASK__CHKAUTOSPLIT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXSEQUENCETASK_GETCLUNIQUEID, "_catCtxSequenceTask::getCLUniqueID" )
   INT32 _catCtxSequenceTask::getCLUniqueID(_pmdEDUCB *cb, utilCLUniqueID *clUniqueID)
   {
      INT32 rc = SDB_OK ;
      BSONObj boCollection ;
      bson::BSONElement ele ;

      PD_TRACE_ENTRY( SDB_CATCTXSEQUENCETASK_GETCLUNIQUEID ) ;

      rc = catGetCollection( _collection, boCollection, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the Collection [%s], rc: %d",
                   _collection.c_str(), rc ) ;
      PD_CHECK( boCollection.hasField( CAT_CL_UNIQUEID ),
                SDB_CAT_CORRUPTION, error, PDERROR,
                "Failed to get uniqueid. Catalog [%s] type not supported "
                , boCollection.toString( false, false ).c_str() ) ;
      ele = boCollection.getField( CAT_CL_UNIQUEID ) ;
      if ( ele.eoo() )
      {
         *clUniqueID = 0;
      }
      else if ( ele.type() != NumberLong )
      {
         // if type is not Number, something wrong
         PD_RC_CHECK ( SDB_CAT_CORRUPTION, PDSEVERE,
                       "Catalog [%s] type error", CAT_CL_UNIQUEID ) ;
      }
      else
      {
         *clUniqueID = (UINT64)ele.Long();
      }
   done :
      PD_TRACE_EXITRC( SDB_CATCTXSEQUENCETASK_GETCLUNIQUEID, rc ) ;
      return rc ;

   error :
      goto done ;
   }
   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERSEQUENCETASK_CHECK_INT, "_catCtxAlterSequenceTask::_checkInternal" )
   INT32 _catCtxAlterSequenceTask::_checkInternal ( _pmdEDUCB *cb, catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;
      autoIncFieldsList fldList ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERSEQUENCETASK_CHECK_INT ) ;
      const rtnCLSetAttributeTask * localTask =
                   dynamic_cast< const rtnCLSetAttributeTask * >( getTask() ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error,
                PDERROR, "Failed to get task" ) ;
      PD_CHECK( localTask->containAutoincArgument(), SDB_SYS, error, PDERROR,
                "Failed to get autoinc argument" ) ;
      fldList = localTask->getAutoincFieldArgument() ;
      for( UINT32 i = 0 ; i < fldList.size() ; i++ )
      {
         rc = clsAutoIncItem::validAutoIncOption( fldList[i]->getArgument() ) ;
         PD_RC_CHECK( rc, PDWARNING, "Invalid autoIncrement options" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERSEQUENCETASK_CHECK_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERSEQUENCETASK_EXEC_INT, "_catCtxAlterSequenceTask::_executeInternal" )
   INT32 _catCtxAlterSequenceTask::_executeInternal ( _pmdEDUCB *cb,
                                                   SDB_DMSCB *pDmsCB,
                                                   SDB_DPSCB *pDpsCB,
                                                   INT16 w )
   {
      INT32 rc = SDB_OK ;
      BSONObj seqOpt ;
      utilCLUniqueID clUniqueID ;
      autoIncFieldsList fldList ;
      catSequenceManager *pSeqMgr = NULL ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERSEQUENCETASK_EXEC_INT ) ;

      const rtnCLSetAttributeTask * localTask =
                   dynamic_cast< const rtnCLSetAttributeTask * >( getTask() ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error,
                PDERROR, "Failed to get task" ) ;
      PD_CHECK( localTask->containAutoincArgument(), SDB_SYS, error, PDERROR,
                "Failed to get autoinc argument" ) ;

      pSeqMgr = sdbGetCatalogueCB()->getCatGTSMgr()->getSequenceMgr() ;
      PD_CHECK( NULL != pSeqMgr, SDB_SYS, error, PDERROR, "Failed to get "
                "sequence manager" ) ;

      rc = getCLUniqueID( cb, &clUniqueID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get cl uniqueid, rc: %d", rc ) ;

      fldList = localTask->getAutoincFieldArgument() ;
      for( UINT32 i = 0 ; i < fldList.size() ; i++ )
      {
         const CHAR *fieldName = fldList[i]->getFieldName() ;
         PD_CHECK( 0 != ossStrcmp( fieldName, "" ), SDB_SYS, error, PDERROR,
                   "Failed to get field name" ) ;

         string seqName = catGetSeqName4AutoIncFld( clUniqueID, fieldName ) ;
         BSONObj seqOpt = catBuildSequenceOptions( fldList[i]->getArgument(),
                                                   UTIL_SEQUENCEID_NULL,
                                                   fldList[i]->getArgumentMask()
                                                 ) ;

         BSONObj oldOptions ;
         UINT32 alterMask = UTIL_ARG_FIELD_EMPTY ;
         rc = pSeqMgr->alterSequence( seqName, seqOpt, cb, w, &oldOptions,
                                      &alterMask ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to alter sequence, rc: %d", rc ) ;

         if ( UTIL_ARG_FIELD_EMPTY != alterMask )
         {
            // Some fields had been changed, let's save the old options in
            // case of rollback
            rtnCLAutoincFieldArgument rollbackArgument( oldOptions ) ;
            rollbackArgument.setArgumentMask( alterMask ) ;
            _rollbackObj.push_back( rollbackArgument ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERSEQUENCETASK_EXEC_INT, rc ) ;
      return rc ;

   error :
      goto done ;

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERSEQUENCETASK_ROLLBACK_INT, "_catCtxAlterSequenceTask::_rollbackInternal" )
   INT32 _catCtxAlterSequenceTask::_rollbackInternal ( _pmdEDUCB *cb,
                                                       SDB_DMSCB *pDmsCB,
                                                       SDB_DPSCB *pDpsCB,
                                                       INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERSEQUENCETASK_ROLLBACK_INT ) ;

      BSONObj seqOpt ;
      bson::BSONElement ele ;
      catSequenceManager *pSeqMgr ;
      vector< rtnCLAutoincFieldArgument > & rollbackArgs = getRollbackObj() ;

      pSeqMgr = sdbGetCatalogueCB()->getCatGTSMgr()->getSequenceMgr() ;
      PD_CHECK( NULL != pSeqMgr, SDB_SYS, error, PDERROR, "Failed to get "
                "sequence manager" ) ;

      for( UINT32 i = 0 ; i < rollbackArgs.size() ; i++ )
      {
         BSONObj argument = rollbackArgs[i].getArgument() ;

         // get sequence name
         const CHAR *seqName = NULL ;
         PD_CHECK( argument.hasField( FIELD_NAME_SEQUENCE_NAME ),
                   SDB_SYS, error, PDERROR, "Failed to get field[%s]",
                   argument.toString( false, false ).c_str() ) ;
         ele = argument.getField( FIELD_NAME_SEQUENCE_NAME ) ;
         PD_CHECK( String == ele.type(), SDB_SYS, error, PDERROR,
                  "Failed to get field [%s]", FIELD_NAME_SEQUENCE_NAME ) ;
         seqName = ele.valuestrsafe() ;

         // build rollback object
         seqOpt = catBuildSequenceOptions( argument, UTIL_SEQUENCEID_NULL,
                                           rollbackArgs[i].getArgumentMask() ) ;

         // only rollback what had been changed by alter mask
         rc = pSeqMgr->alterSequence( seqName, seqOpt, cb, w, NULL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to alter sequence[%s] when rollback, rc: %d",
                      seqName, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERSEQUENCETASK_ROLLBACK_INT, rc ) ;
      return rc ;

   error :
      goto done ;

   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERSEQUENCETASK_GETCLUNIQUEID, "_catCtxAlterSequenceTask::getCLUniqueID" )
   INT32 _catCtxAlterSequenceTask::getCLUniqueID(_pmdEDUCB *cb, utilCLUniqueID *clUniqueID)
   {
      INT32 rc = SDB_OK ;
      BSONObj boCollection ;
      bson::BSONElement ele ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERSEQUENCETASK_GETCLUNIQUEID ) ;

      rc = catGetCollection( _dataName, boCollection, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the Collection [%s], rc: %d",
                   _dataName.c_str(), rc ) ;
      PD_CHECK( boCollection.hasField( CAT_CL_UNIQUEID ),
                SDB_CAT_CORRUPTION, error, PDERROR,
                "Failed to get uniqueid. Catalog [%s] type not supported "
                , boCollection.toString( false, false ).c_str() ) ;
      ele = boCollection.getField( CAT_CL_UNIQUEID ) ;
      if ( ele.eoo() )
      {
         *clUniqueID = 0;
      }
      else if ( ele.type() != NumberLong )
      {
         // if type is not Number, something wrong
         PD_RC_CHECK ( SDB_CAT_CORRUPTION, PDSEVERE,
                       "Catalog [%s] type error", CAT_CL_UNIQUEID ) ;
      }
      else
      {
         *clUniqueID = (UINT64)ele.Long();
      }
   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERSEQUENCETASK_GETCLUNIQUEID, rc ) ;
      return rc ;

   error :
      goto done ;

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATESEQUENCETASK_CHECK_INT, "_catCtxCreateSequenceTask::_checkInternal" )
   INT32 _catCtxCreateSequenceTask::_checkInternal ( _pmdEDUCB *cb,
                                                     catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;
      autoIncFieldsList fldList ;

      PD_TRACE_ENTRY( SDB_CATCTXCREATESEQUENCETASK_CHECK_INT ) ;
      const rtnCLCreateAutoincFieldTask * localTask =
           dynamic_cast< const rtnCLCreateAutoincFieldTask * >( getSeqTask() ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error,
                PDERROR, "Failed to get task" ) ;
      PD_CHECK( localTask->containAutoincArgument(), SDB_SYS, error, PDERROR,
                "Failed to get autoinc argument" ) ;

      fldList = localTask->getAutoincrementArgument() ;
      for( UINT32 i = 0 ; i < fldList.size() ; i++ )
      {
         rc = clsAutoIncItem::validAutoIncOption( fldList[i]->getArgument() ) ;
         PD_RC_CHECK( rc, PDWARNING, "Invalid autoIncrement options" ) ;
         rc = catValidSequenceOption( fldList[i]->getArgument() ) ;
         PD_RC_CHECK( rc, PDWARNING, "Invalid autoIncrement options" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXCREATESEQUENCETASK_CHECK_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATESEQUENCETASK_EXEC_INT, "_catCtxCreateSequenceTask::_executeInternal" )
   INT32 _catCtxCreateSequenceTask::_executeInternal ( _pmdEDUCB *cb,
                                                       SDB_DMSCB *pDmsCB,
                                                       SDB_DPSCB *pDpsCB,
                                                       INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXCREATESEQUENCETASK_EXEC_INT ) ;

      BSONObj seqOpt ;
      utilCLUniqueID clUniqueID ;
      autoIncFieldsList fldList ;
      catSequenceManager *pSeqMgr = NULL ;

      const rtnCLCreateAutoincFieldTask * localTask =
           dynamic_cast< const rtnCLCreateAutoincFieldTask * >( getSeqTask() ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                "Failed to get task" ) ;
      PD_CHECK( localTask->containAutoincArgument(), SDB_SYS, error, PDERROR,
                "Failed to get autoinc argument" ) ;

      pSeqMgr = sdbGetCatalogueCB()->getCatGTSMgr()->getSequenceMgr() ;
      PD_CHECK( NULL != pSeqMgr, SDB_SYS, error, PDERROR, "Failed to get "
                "sequence manager" ) ;

      rc = getCLUniqueID( cb, &clUniqueID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get cl uniqueid, rc: %d", rc ) ;

      fldList = localTask->getAutoincrementArgument() ;
      for( UINT32 i = 0 ; i < fldList.size() ; i++ )
      {
         BSONObjBuilder bld ;
         string seqName ;
         const CHAR *fieldName = NULL ;
         utilSequenceID ID = UTIL_SEQUENCEID_NULL ;
         fieldName = fldList[i]->getFieldName() ;
         PD_CHECK( 0 != ossStrcmp( fieldName, "" ), SDB_SYS, error, PDERROR,
                   "Failed to get field name[%s]", fieldName ) ;
         seqName = catGetSeqName4AutoIncFld( clUniqueID, fieldName ) ;
         bld.append( FIELD_NAME_SEQUENCE_NAME, seqName ) ;
         rc = catUpdateGlobalID( cb, w, ID );
         PD_RC_CHECK( rc, PDERROR, "Failed to get global ID for autoincrement "
                      "field[%s], rc: %d", fieldName ) ;
         fldList[i]->setID( ID ) ;
         seqOpt = catBuildSequenceOptions( fldList[i]->getArgument(), ID ) ;
         rc = pSeqMgr->createSequence( seqName, seqOpt, cb, w ) ;
         if( SDB_SEQUENCE_EXIST == rc )
         {
            rc = SDB_AUTOINCREMENT_FIELD_CONFLICT ;
            PD_LOG( PDERROR, "Failed to create sequence[%s], rc: %d",
                    seqName.c_str(), rc ) ;
            goto error ;
         }
         else if( rc )
         {
            PD_LOG( PDERROR, "Failed to create sequence[%s], rc: %d",
                      seqName.c_str(), rc ) ;
            goto error ;
         }
         _rollbackObj.push_back( bld.obj() );
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXCREATESEQUENCETASK_EXEC_INT, rc ) ;
      return rc ;

   error :
      goto done ;

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATESEQUENCETASK_ROLLBACK_INT, "_catCtxCreateSequenceTask::_rollbackInternal" )
   INT32 _catCtxCreateSequenceTask::_rollbackInternal ( _pmdEDUCB *cb,
                                                        SDB_DMSCB *pDmsCB,
                                                        SDB_DPSCB *pDpsCB,
                                                        INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXCREATESEQUENCETASK_ROLLBACK_INT ) ;

      BSONObj seqOpt ;
      bson::BSONElement ele ;
      catSequenceManager *pSeqMgr ;
      vector<BSONObj>& obj = getRollbackObj() ;

      pSeqMgr = sdbGetCatalogueCB()->getCatGTSMgr()->getSequenceMgr() ;
      PD_CHECK( NULL != pSeqMgr, SDB_SYS, error, PDERROR,
                "Failed to get sequence manager" ) ;

      PD_RC_CHECK( rc, PDERROR, "Failed to get cl uniqueid, rc: %d", rc ) ;

      for( UINT32 i = 0 ; i < obj.size() ; i++ )
      {
         const CHAR *seqName = NULL ;
         PD_CHECK( obj[i].hasField( FIELD_NAME_SEQUENCE_NAME ), SDB_SYS, error,
                   PDERROR, "Failed to get field[%s]",
                   obj[i].toString( false, false ).c_str() ) ;
         ele = obj[i].getField( FIELD_NAME_SEQUENCE_NAME ) ;
         PD_CHECK( String == ele.type(), SDB_SYS, error, PDERROR,
                  "Failed to get field [%s]", FIELD_NAME_SEQUENCE_NAME ) ;
         seqName = ele.valuestrsafe() ;

         // clear sequence tasks
         catRemoveSequenceTasks( seqName, cb, w ) ;

         rc = pSeqMgr->dropSequence( seqName, cb, w ) ;
         if ( SDB_SEQUENCE_NOT_EXIST == rc )
         {
            PD_LOG( PDWARNING, "Sequence[%s] not exist when rollback",
                  seqName ) ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to drop sequence[%s] when rollback, "
                      "rc: %d", seqName, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXCREATESEQUENCETASK_ROLLBACK_INT, rc ) ;
      return rc ;

   error :
      goto done ;

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPSEQUENCETASK_CHECK_INT, "_catCtxDropSequenceTask::_checkInternal" )
   INT32 _catCtxDropSequenceTask::_checkInternal ( _pmdEDUCB *cb,
                                                   catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;
      autoIncFieldsList fldList ;

      PD_TRACE_ENTRY( SDB_CATCTXDROPSEQUENCETASK_CHECK_INT ) ;
      const rtnCLDropAutoincFieldTask * localTask =
             dynamic_cast< const rtnCLDropAutoincFieldTask * >( getSeqTask() ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                "Failed to get task" ) ;
      PD_CHECK( localTask->containAutoincArgument(), SDB_SYS, error, PDERROR,
                "Failed to get autoinc argument" ) ;

      fldList = localTask->getAutoincrementArgument() ;
      for( UINT32 i = 0 ; i < fldList.size() ; i++ )
      {
         rc = clsAutoIncItem::validAutoIncOption( fldList[i]->getArgument() ) ;
         PD_RC_CHECK( rc, PDWARNING, "Invalid autoIncrement options" ) ;
         rc = catValidSequenceOption( fldList[i]->getArgument() ) ;
         PD_RC_CHECK( rc, PDWARNING, "Invalid autoIncrement options" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXDROPSEQUENCETASK_CHECK_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPSEQUENCETASK_EXEC_INT, "_catCtxDropSequenceTask::_executeInternal" )
   INT32 _catCtxDropSequenceTask::_executeInternal ( _pmdEDUCB *cb,
                                                     SDB_DMSCB *pDmsCB,
                                                     SDB_DPSCB *pDpsCB,
                                                     INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXDROPSEQUENCETASK_EXEC_INT ) ;

      BOOLEAN exist = TRUE ;
      utilCLUniqueID clUniqueID = 0 ;
      autoIncFieldsList fldList ;
      catSequenceManager *pSeqMgr = NULL ;

      const rtnCLDropAutoincFieldTask * localTask =
             dynamic_cast< const rtnCLDropAutoincFieldTask * >( getSeqTask() ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                "Failed to get task" ) ;
      PD_CHECK( localTask->containAutoincArgument(), SDB_SYS, error, PDERROR,
                "Failed to get autoinc argument" ) ;

      pSeqMgr = sdbGetCatalogueCB()->getCatGTSMgr()->getSequenceMgr() ;
      PD_CHECK( NULL != pSeqMgr, SDB_SYS, error, PDERROR, "Failed to get "
                "sequence manager" ) ;

      rc = getCLUniqueID( cb, &clUniqueID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get cl uniqueid, rc: %d", rc ) ;

      fldList = localTask->getAutoincrementArgument() ;
      for( UINT32 i = 0 ; i < fldList.size() ; i++ )
      {
         BSONObj seqOpt ;
         const CHAR *fldName = NULL ;
         string seqName ;
         fldName = fldList[i]->getFieldName() ;
         PD_CHECK( ossStrcmp( fldName, "" ), SDB_SYS, error, PDERROR,
                   "Failed to get field name" ) ;
         seqName = catGetSeqName4AutoIncFld( clUniqueID, fldName ) ;
         rc = pSeqMgr->getSequence( seqName, seqOpt, cb ) ;
         if ( SDB_SEQUENCE_NOT_EXIST == rc )
         {
            exist = FALSE ;
            rc = SDB_OK ;
         }

         // clear sequence tasks
         catRemoveSequenceTasks( seqName.c_str(), cb, w ) ;

         rc = pSeqMgr->dropSequence( seqName, cb, w ) ;
         if ( SDB_SEQUENCE_NOT_EXIST == rc )
         {
            PD_LOG( PDWARNING, "The sequence[%s] does not exist, no need to "
                    "drop", seqName.c_str()) ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to drop sequence[%s], rc: %d",
                      seqName.c_str(), rc ) ;
         if( exist )
         {
            _rollbackObj.push_back( seqOpt ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXDROPSEQUENCETASK_EXEC_INT, rc ) ;
      return rc ;

   error :
      goto done ;

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPSEQUENCETASK_ROLLBACK_INT, "_catCtxDropSequenceTask::_rollbackInternal" )
   INT32 _catCtxDropSequenceTask::_rollbackInternal ( _pmdEDUCB *cb,
                                                      SDB_DMSCB *pDmsCB,
                                                      SDB_DPSCB *pDpsCB,
                                                      INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXDROPSEQUENCETASK_ROLLBACK_INT ) ;

      catSequenceManager *pSeqMgr = NULL ;
      vector<BSONObj>& obj = getRollbackObj() ;

      pSeqMgr = sdbGetCatalogueCB()->getCatGTSMgr()->getSequenceMgr() ;
      PD_CHECK( NULL != pSeqMgr, SDB_SYS, error, PDERROR,
                "Failed to get sequence manager" ) ;

      for( UINT32 i = 0 ; i < obj.size() ; i++ )
      {
         BSONObj seqOpt = obj[ i ] ;
         BSONElement ele ;

         // get name field from rollback object
         const CHAR *seqName = NULL ;
         PD_CHECK( seqOpt.hasField( FIELD_NAME_SEQUENCE_NAME ), SDB_SYS, error,
                   PDERROR, "Failed to get field [%s] from %s",
                   FIELD_NAME_SEQUENCE_NAME, seqOpt.toString().c_str() ) ;
         ele = seqOpt.getField( FIELD_NAME_SEQUENCE_NAME ) ;
         PD_CHECK( String == ele.type(), SDB_SYS, error, PDERROR,
                  "Failed to get field [%s] from %s, type should be string",
                  FIELD_NAME_SEQUENCE_NAME, seqOpt.toString().c_str() ) ;
         seqName = ele.valuestrsafe() ;

         // insert rollback object
         rc = pSeqMgr->insertSequence( seqName, seqOpt, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create sequence [%s] when "
                      "rollback, rc: %d", seqName, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXDROPSEQUENCETASK_ROLLBACK_INT, rc ) ;
      return rc ;

   error :
      goto done ;

   }

   /*
      _catCtxAlterCSTask
    */
   _catCtxAlterCSTask::_catCtxAlterCSTask ( const std::string & collectionSpace,
                                            const rtnAlterTask * task )
   : _catCtxAlterTask( collectionSpace, task )
   {
   }

   _catCtxAlterCSTask::~_catCtxAlterCSTask ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCSTASK_CHECK_INT, "_catCtxAlterCSTask::_checkInternal" )
   INT32 _catCtxAlterCSTask::_checkInternal ( _pmdEDUCB * cb,
                                              catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCSTASK_CHECK_INT ) ;

      // get collection-space
      rc = catGetAndLockCollectionSpace( _dataName, _boData,
                                         cb, &lockMgr, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the collection space [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

      switch ( _task->getActionType() )
      {
         case RTN_ALTER_CS_SET_DOMAIN :
         {
            rc = _checkSetDomain( cb, lockMgr ) ;
            break ;
         }
         case RTN_ALTER_CS_REMOVE_DOMAIN :
         {
            rc = _checkRemoveDomain( cb, lockMgr ) ;
            break ;
         }
         case RTN_ALTER_CS_ENABLE_CAPPED :
         {
            rc = _checkEnableCapped( cb, lockMgr ) ;
            break ;
         }
         case RTN_ALTER_CS_DISABLE_CAPPED :
         {
            rc = _checkDisableCapped( cb, lockMgr ) ;
            break ;
         }
         case RTN_ALTER_CS_SET_ATTRIBUTES :
         {
            rc = _checkSetAttributes( cb, lockMgr ) ;
            break ;
         }
         default :
         {
            rc = SDB_INVALIDARG ;
            break ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to build set fields, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCSTASK_CHECK_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCSTASK_EXECUTE_INT, "_catCtxAlterCSTask::_executeInternal" )
   INT32 _catCtxAlterCSTask::_executeInternal ( _pmdEDUCB * cb,
                                                SDB_DMSCB * pDmsCB,
                                                SDB_DPSCB * pDpsCB,
                                                INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCSTASK_EXECUTE_INT ) ;

      BSONObj setObject, unsetObject ;

      /// we need to check something before execute
      if ( RTN_ALTER_CS_SET_ATTRIBUTES == _task->getActionType() )
      {
         const rtnCSSetAttributeTask * localTask =
                     dynamic_cast< const rtnCSSetAttributeTask * >( _task ) ;
         PD_CHECK( NULL != localTask, SDB_INVALIDARG, error, PDERROR,
                   "Failed to get alter task" ) ;

         if ( localTask->testArgumentMask( UTIL_CS_CAPPED_FIELD ) )
         {
            // get cs object: _boData
            BOOLEAN isExist = FALSE ;
            rc = catCheckSpaceExist( _dataName.c_str(), isExist, _boData, cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get info of collection space [%s], rc: %d",
                         _dataName.c_str(), rc ) ;
            PD_CHECK( isExist, SDB_DMS_CS_NOTEXIST, error, PDWARNING,
                      "Collection space [%s] does not exist!",
                      _dataName.c_str() ) ;

            // The collection space should be empty
            rc = _checkEmptyCollectionSpace( cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to check collection space [%s], rc: %d",
                         _dataName.c_str(), rc ) ;
         }

         if ( localTask->testArgumentMask( UTIL_CS_PAGESIZE_FIELD ) )
         {
            // get groups of cs : _groups
            rc = catGetCSGroupsFromCLs( _dataName.c_str(), cb, _groups ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get group list of cs[%s], rc: %d",
                         _dataName.c_str(), rc ) ;

            // The collection space should not have data
            PD_CHECK( 0 == _groups.size(), SDB_DMS_CS_NOT_EMPTY,
                     error, PDERROR, "Failed to check alter task [%s]: "
                     "collection space [%s] is not empty",
                     _task->getActionName(), _dataName.c_str() ) ;
         }
      }

      /// execute
      rc = _buildSetFields( setObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build set fields, rc: %d", rc ) ;

      rc = _buildUnsetFields( unsetObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build unset fields, rc: %d", rc ) ;

      rc = catUpdateCS( _dataName.c_str(), setObject, unsetObject, cb,
                        pDmsCB, pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to update collection space [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCSTASK_EXECUTE_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCSTASK__CHKSETDOMAIN, "_catCtxAlterCSTask::_checkSetDomain" )
   INT32 _catCtxAlterCSTask::_checkSetDomain ( _pmdEDUCB * cb,
                                               catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCSTASK__CHKSETDOMAIN ) ;

      const CHAR * domain = NULL ;
      const rtnCSSetDomainTask * localTask =
                  dynamic_cast< const rtnCSSetDomainTask * >( _task ) ;
      PD_CHECK( NULL != localTask, SDB_INVALIDARG, error, PDERROR,
                "Failed to get alter task" ) ;

      domain = localTask->getDomain() ;

      rc = _checkDomainGroups( domain, cb, lockMgr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check domain [%s], rc: %d",
                   domain, rc ) ;

      // No need to run on data groups
      _groups.clear() ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCSTASK__CHKSETDOMAIN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCSTASK__CHKRMDOMAIN, "_catCtxAlterCSTask::_checkRemoveDomain" )
   INT32 _catCtxAlterCSTask::_checkRemoveDomain ( _pmdEDUCB * cb,
                                                  catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCSTASK__CHKRMDOMAIN ) ;

      const rtnCSRemoveDomainTask * localTask =
                  dynamic_cast< const rtnCSRemoveDomainTask * >( _task ) ;
      PD_CHECK( NULL != localTask, SDB_INVALIDARG, error, PDERROR,
                      "Failed to get alter task" ) ;

      if ( _boData.hasElement( CAT_DOMAIN_NAME ) )
      {
         const CHAR * domain = NULL ;
         BSONElement domainElement = _boData.getField( CAT_DOMAIN_NAME ) ;
         PD_CHECK( String == domainElement.type(),
                   SDB_CAT_CORRUPTION, error, PDERROR,
                   "Failed to get domain" ) ;
         domain = domainElement.valuestr() ;

         PD_CHECK( lockMgr.tryLockDomain( domain, SHARED ),
                   SDB_LOCK_FAILED, error, PDERROR,
                   "Failed to lock domain [%s]", domain ) ;
      }

      // No need to run on data groups
      _groups.clear() ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCSTASK__CHKRMDOMAIN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCSTASK__CHKENABLECAP, "_catCtxAlterCSTask::_checkEnableCapped" )
   INT32 _catCtxAlterCSTask::_checkEnableCapped ( _pmdEDUCB * cb,
                                                  catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCSTASK__CHKENABLECAP ) ;

      const rtnCSEnableCappedTask * localTask =
                  dynamic_cast< const rtnCSEnableCappedTask * >( _task ) ;
      PD_CHECK( NULL != localTask, SDB_INVALIDARG, error, PDERROR,
                      "Failed to get alter task" ) ;

      rc = _checkEmptyCollectionSpace( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check collection space [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCSTASK__CHKENABLECAP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCSTASK__CHKDISABLECAP, "_catCtxAlterCSTask::_checkDisableCapped" )
   INT32 _catCtxAlterCSTask::_checkDisableCapped ( _pmdEDUCB * cb,
                                                   catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCSTASK__CHKDISABLECAP ) ;

      const rtnCSDisableCappedTask * localTask =
                  dynamic_cast< const rtnCSDisableCappedTask * >( _task ) ;
      PD_CHECK( NULL != localTask, SDB_INVALIDARG, error, PDERROR,
                      "Failed to get alter task" ) ;

      rc = _checkEmptyCollectionSpace( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check collection space [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCSTASK__CHKDISABLECAP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCSTASK__CHKSETATTR, "_catCtxAlterCSTask::_checkSetAttributes" )
   INT32 _catCtxAlterCSTask::_checkSetAttributes ( _pmdEDUCB * cb,
                                                   catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCSTASK__CHKSETATTR ) ;

      const rtnCSSetAttributeTask * localTask =
                  dynamic_cast< const rtnCSSetAttributeTask * >( _task ) ;
      PD_CHECK( NULL != localTask, SDB_INVALIDARG, error, PDERROR,
                      "Failed to get alter task" ) ;

      if ( localTask->testArgumentMask( UTIL_CS_DOMAIN_FIELD ) )
      {
         // The new domain should contain all groups of current collection space
         const CHAR * domain = localTask->getDomain() ;
         rc = _checkDomainGroups( domain, cb, lockMgr ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check domain groups, rc: %d", rc ) ;
      }
      else
      {
         rc = _checkGroups( cb, lockMgr ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check collection space [%s], rc: %d",
                      _dataName.c_str(), rc ) ;
      }

      if ( localTask->testArgumentMask( UTIL_CS_CAPPED_FIELD ) )
      {
         // The collection space should be empty
         rc = _checkEmptyCollectionSpace( cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check collection space [%s], rc: %d",
                      _dataName.c_str(), rc ) ;
      }

      if ( localTask->testArgumentMask( UTIL_CS_PAGESIZE_FIELD ) )
      {
         // The collection space should not have data
         PD_CHECK( 0 == _groups.size(), SDB_DMS_CS_NOT_EMPTY, error, PDERROR,
                   "Failed to check alter task [%s]: collection space [%s] is "
                   "not empty", _task->getActionName(), _dataName.c_str() ) ;
      }

      if ( localTask->getArgumentMask() == UTIL_CS_DOMAIN_FIELD )
      {
         // Only alter domain, no need to run on data groups
         _groups.clear() ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCSTASK__CHKSETATTR, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCSTASK__CHKDOMAINGRPS, "_catCtxAlterCSTask::_checkDomainGroups" )
   INT32 _catCtxAlterCSTask::_checkDomainGroups ( const CHAR * domain,
                                                  _pmdEDUCB * cb,
                                                  catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCSTASK__CHKDOMAINGRPS ) ;

      sdbCatalogueCB * catCB = pmdGetKRCB()->getCATLOGUECB() ;
      const CHAR * collectionSpace = _dataName.c_str() ;
      BSONObj boDomain ;
      ossPoolSet< UINT32 > occupiedGroups ;
      map< string, UINT32 > domainGroups ;

      rc = catGetAndLockDomain( domain, boDomain, cb, &lockMgr, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get domain [%s], rc: %d",
                   domain, rc ) ;

      rc = catGetDomainGroups( boDomain, domainGroups ) ;
      if ( SDB_CAT_NO_GROUP_IN_DOMAIN == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get groups of domain [%s], "
                   "rc: %d", domain, rc ) ;

      rc = catGetCSGroups( collectionSpace, cb, occupiedGroups, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get group list of collection "
                   "space [%s], rc: %d", collectionSpace, rc ) ;

      for ( ossPoolSet< UINT32 >::iterator iterGroup = occupiedGroups.begin() ;
            iterGroup != occupiedGroups.end() ;
            iterGroup ++ )
      {
         UINT32 groupID = ( *iterGroup ) ;
         const CHAR * groupName = catCB->groupID2Name( groupID ) ;

         PD_CHECK( domainGroups.end() != domainGroups.find( groupName ),
                   SDB_CAT_GROUP_NOT_IN_DOMAIN, error, PDERROR,
                   "Failed to check group [%s]: it is not in domain [%s]",
                   groupName, domain ) ;
         PD_CHECK( lockMgr.tryLockGroup( groupName, SHARED ),
                   SDB_LOCK_FAILED, error, PDWARNING,
                   "Failed to lock group [%s]",
                   groupName ) ;
         _groups.push_back( groupID ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCSTASK__CHKDOMAINGRPS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCSTASK__CHKGROUPS, "_catCtxAlterCSTask::_checkGroups" )
   INT32 _catCtxAlterCSTask::_checkGroups ( _pmdEDUCB * cb,
                                            catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCSTASK__CHKGROUPS ) ;

      sdbCatalogueCB * catCB = pmdGetKRCB()->getCATLOGUECB() ;

      BSONElement element ;
      BSONObj boCollections ;

      element = _boData.getField( CAT_COLLECTION ) ;

      if ( EOO == element.type() )
      {
         goto done ;
      }

      PD_CHECK( Array == element.type(), SDB_CAT_CORRUPTION, error, PDERROR,
                "Failed to get collections" ) ;
      boCollections = element.embeddedObject() ;

      if ( boCollections.nFields() > 0 )
      {
         BSONObjIterator iter ( boCollections ) ;
         while ( iter.more() )
         {
            string clFullName ;
            const CHAR * collection = NULL ;
            BSONElement beCollection = iter.next() ;
            BSONObj boCollection ;
            PD_CHECK( Object == beCollection.type(),
                      SDB_CAT_CORRUPTION, error, PDERROR,
                      "Invalid collection record field type: %d",
                      beCollection.type() ) ;
            boCollection = beCollection.embeddedObject() ;
            rc = rtnGetStringElement( boCollection, CAT_COLLECTION_NAME,
                                      &collection ) ;
            PD_CHECK( SDB_OK == rc, SDB_CAT_CORRUPTION, error, PDERROR,
                      "Get field [%s] failed, rc: %d",
                      CAT_COLLECTION_NAME, rc ) ;

            clFullName = _dataName ;
            clFullName += "." ;
            clFullName += collection ;

            rc = catGetAndLockCollection( clFullName, boCollection,
                                          cb, NULL, SHARED ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get the collection [%s]",
                         clFullName.c_str() ) ;

            catGetCollectionGroupSet( boCollection, _groups ) ;
         }
      }

      for ( CAT_GROUP_LIST::iterator iterGroup = _groups.begin() ;
            iterGroup != _groups.end() ;
            iterGroup ++ )
      {
         UINT32 groupID = ( *iterGroup ) ;
         const CHAR * groupName = catCB->groupID2Name( groupID ) ;
         PD_CHECK( lockMgr.tryLockGroup( groupName, SHARED ),
                   SDB_LOCK_FAILED, error, PDWARNING,
                   "Failed to lock group [%s]",
                   groupName ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCSTASK__CHKGROUPS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCSTASK__CHKEMPTYCS, "_catCtxAlterCSTask::_checkEmptyCollectionSpace" )
   INT32 _catCtxAlterCSTask::_checkEmptyCollectionSpace ( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCSTASK__CHKEMPTYCS ) ;

      BSONElement element ;
      BSONObj boCollections ;

      element = _boData.getField( CAT_COLLECTION ) ;

      if ( EOO == element.type() )
      {
         goto done ;
      }

      PD_CHECK( Array == element.type(), SDB_CAT_CORRUPTION, error, PDERROR,
                "Failed to get collections" ) ;
      boCollections = element.embeddedObject() ;

      PD_CHECK( boCollections.nFields() == 0,
                SDB_DMS_CS_NOT_EMPTY, error, PDERROR,
                "Failed to check collection space, the collection space is not "
                "empty" ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCSTASK__CHKEMPTYCS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCSTASK__BLDSETFIELDS, "_catCtxAlterCSTask::_buildSetFields" )
   INT32 _catCtxAlterCSTask::_buildSetFields ( BSONObj & setObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCSTASK__BLDSETFIELDS ) ;

      BSONObjBuilder setBuilder ;

      switch ( _task->getActionType() )
      {
         case RTN_ALTER_CS_SET_DOMAIN :
         {
            const rtnCSSetDomainTask * localTask =
                        dynamic_cast< const rtnCSSetDomainTask * >( _task ) ;
            PD_CHECK( NULL != localTask, SDB_INVALIDARG, error, PDERROR,
                      "Failed to get alter task" ) ;
            setBuilder.append( CAT_DOMAIN_NAME, localTask->getDomain() ) ;
            break ;
         }
         case RTN_ALTER_CS_ENABLE_CAPPED :
         {
            setBuilder.append( CAT_TYPE_NAME, DMS_STORAGE_CAPPED ) ;
            break ;
         }
         case RTN_ALTER_CS_DISABLE_CAPPED :
         {
            setBuilder.append( CAT_TYPE_NAME, DMS_STORAGE_NORMAL ) ;
            break ;
         }
         case RTN_ALTER_CS_SET_ATTRIBUTES :
         {
            const rtnCSSetAttributeTask * localTask =
                        dynamic_cast< const rtnCSSetAttributeTask * >( _task ) ;
            PD_CHECK( NULL != localTask, SDB_INVALIDARG, error, PDERROR,
                      "Failed to get alter task" ) ;
            if ( localTask->testArgumentMask( UTIL_CS_DOMAIN_FIELD ) )
            {
               setBuilder.append( CAT_DOMAIN_NAME, localTask->getDomain() ) ;
            }
            if ( localTask->testArgumentMask( UTIL_CS_PAGESIZE_FIELD ) )
            {
               setBuilder.append( CAT_PAGE_SIZE_NAME,
                                  localTask->getPageSize() ) ;
            }
            if ( localTask->testArgumentMask( UTIL_CS_LOBPAGESIZE_FIELD ) )
            {
               setBuilder.append( CAT_LOB_PAGE_SZ_NAME,
                                  localTask->getLobPageSize() ) ;
            }
            if ( localTask->testArgumentMask( UTIL_CS_CAPPED_FIELD ) )
            {
               setBuilder.append( CAT_TYPE_NAME,
                                  localTask->isCapped() ? DMS_STORAGE_CAPPED :
                                                          DMS_STORAGE_NORMAL ) ;
            }
            break ;
         }
         case RTN_ALTER_CS_REMOVE_DOMAIN :
         {
            break ;
         }
         default :
         {
            rc = SDB_INVALIDARG ;
            break ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to build set fields, rc: %d", rc ) ;

      setObject = setBuilder.obj() ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCSTASK__BLDSETFIELDS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERCSTASK__BLDUNSETFIELDS, "_catCtxAlterCSTask::_buildUnsetFields" )
   INT32 _catCtxAlterCSTask::_buildUnsetFields ( BSONObj & unsetObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERCSTASK__BLDUNSETFIELDS ) ;

      BSONObjBuilder unsetBuilder ;

      switch ( _task->getActionType() )
      {
         case RTN_ALTER_CS_REMOVE_DOMAIN :
         {
            unsetBuilder.append( CAT_DOMAIN_NAME, 1 ) ;
            break ;
         }
         case RTN_ALTER_CS_SET_DOMAIN :
         case RTN_ALTER_CS_ENABLE_CAPPED :
         case RTN_ALTER_CS_DISABLE_CAPPED :
         case RTN_ALTER_CS_SET_ATTRIBUTES :
         {
            break ;
         }
         default :
         {
            rc = SDB_INVALIDARG ;
            break ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to build set fields, rc: %d", rc ) ;

      unsetObject = unsetBuilder.obj() ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERCSTASK__BLDUNSETFIELDS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _catCtxAlterDomainTask define
    */
   _catCtxAlterDomainTask::_catCtxAlterDomainTask ( const string & domain,
                                                    const rtnAlterTask * task )
   : _catCtxAlterTask( domain, task )
   {
   }

   _catCtxAlterDomainTask::~_catCtxAlterDomainTask ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERDOMAINTASK_CHECK_INT, "_catCtxAlterDomainTask::_checkInternal" )
   INT32 _catCtxAlterDomainTask::_checkInternal ( _pmdEDUCB * cb,
                                                  catCtxLockMgr & lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERDOMAINTASK_CHECK_INT ) ;

      BOOLEAN checkGroups = _task->testArgumentMask( UTIL_DOMAIN_GROUPS_FIELD ) ;

      rc = catGetAndLockDomain( _dataName.c_str(), _boData,
                                cb, &lockMgr, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get domain [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

      if ( checkGroups )
      {
         rc = catGetDomainGroups( _boData, _groupMap ) ;
         if ( SDB_CAT_NO_GROUP_IN_DOMAIN == rc )
         {
            /// empty domain
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get groups of domain [%s], "
                      "rc: %d", _dataName.c_str(), rc ) ;
      }

      switch ( _task->getActionType() )
      {
         case RTN_ALTER_DOMAIN_ADD_GROUPS :
         {
            rc = _checkAddGroupTask( cb ) ;
            break ;
         }
         case RTN_ALTER_DOMAIN_REMOVE_GROUPS :
         {
            rc = _checkRemoveGroupTask( cb ) ;
            break ;
         }
         case RTN_ALTER_DOMAIN_SET_GROUPS :
         {
            rc = _checkSetGroupTask( cb ) ;
            break ;
         }
         case RTN_ALTER_DOMAIN_SET_ATTRIBUTES :
         {
            rc = _checkSetAttrTask( cb ) ;
            break ;
         }
         default :
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to check alter task: unsupported task "
                    "[%s]", _task->getActionName() ) ;
            goto error ;
         }
      }

      PD_RC_CHECK( rc, PDERROR, "Failed to check alter task [%s], rc: %d",
                   _task->getActionName(), rc ) ;

      if ( checkGroups )
      {
         for ( CAT_DOMAIN_GROUP_MAP::iterator iterGroup = _groupMap.begin() ;
               iterGroup != _groupMap.end() ;
               iterGroup ++ )
         {
            PD_CHECK( lockMgr.tryLockGroup( iterGroup->first, SHARED ),
                      SDB_LOCK_FAILED, error, PDERROR,
                      "Failed to lock group [%s]",
                      iterGroup->first.c_str() ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERDOMAINTASK_CHECK_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERDOMAINTASK_EXECUTE_INT, "_catCtxAlterDomainTask::_executeInternal" )
   INT32 _catCtxAlterDomainTask::_executeInternal ( _pmdEDUCB * cb,
                                                    SDB_DMSCB * pDmsCB,
                                                    SDB_DPSCB * pDpsCB,
                                                    INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERDOMAINTASK_EXECUTE_INT ) ;

      BSONObjBuilder builder ;
      rtnQueryOptions queryOptions ;

      if ( _task->testArgumentMask( UTIL_DOMAIN_GROUPS_FIELD ) )
      {
         rc = _toDomainGroups( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate new group list of "
                      "domain [%s], rc: %d", _dataName.c_str(), rc ) ;
      }

      if ( RTN_ALTER_DOMAIN_SET_ATTRIBUTES == _task->getActionType() )
      {
         const rtnDomainSetAttributeTask * localTask =
               dynamic_cast< const rtnDomainSetAttributeTask * >( _task ) ;
         if ( localTask->testArgumentMask( UTIL_DOMAIN_AUTOSPLIT_FIELD ) )
         {
            builder.appendBool( CAT_DOMAIN_AUTO_SPLIT,
                                localTask->isAutoSplit() ) ;
         }
         if ( localTask->testArgumentMask( UTIL_DOMAIN_AUTOREBALANCE_FIELD ) )
         {
            builder.appendBool( CAT_DOMAIN_AUTO_REBALANCE,
                                localTask->isAutoRebalance() ) ;
         }
      }

      rc = catUpdateDomain( _dataName.c_str(), builder.obj(),
                            cb, pDmsCB, pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update domain [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERDOMAINTASK_EXECUTE_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERDOMAINTASK__CHKADDGROUP, "_catCtxAlterDomainTask::_checkAddGroupTask" )
   INT32 _catCtxAlterDomainTask::_checkAddGroupTask ( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERDOMAINTASK__CHKADDGROUP ) ;

      const rtnDomainAddGroupTask * localTask =
                  dynamic_cast< const rtnDomainAddGroupTask * >( _task ) ;

      if ( NULL != localTask )
      {
         const RTN_DOMAIN_GROUP_LIST & groups = localTask->getGroups() ;

         PD_CHECK( groups.size() > 0,
                   SDB_CAT_NO_NODEGROUP_INFO, error, PDERROR,
                   "Failed to check groups: no group is specified" ) ;

         rc = _checkAddingGroups( localTask->getGroups(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check groups, rc: %d", rc ) ;
      }
      else
      {
         PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                   "Failed to get add group task" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERDOMAINTASK__CHKADDGROUP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERDOMAINTASK__CHKRMGROUP, "_catCtxAlterDomainTask::_checkRemoveGroupTask" )
   INT32 _catCtxAlterDomainTask::_checkRemoveGroupTask ( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERDOMAINTASK__CHKRMGROUP ) ;

      const rtnDomainRemoveGroupTask * localTask =
                  dynamic_cast< const rtnDomainRemoveGroupTask * >( _task ) ;

      if ( NULL != localTask )
      {
         const RTN_DOMAIN_GROUP_LIST & groups = localTask->getGroups() ;

         PD_CHECK( groups.size() > 0,
                   SDB_CAT_NO_NODEGROUP_INFO, error, PDERROR,
                   "Failed to check groups: no group is specified" ) ;

         rc = _checkRemovingGroups( localTask->getGroups(), cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check removing groups, rc: %d", rc ) ;
      }
      else
      {
         PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                   "Failed to get remove group task" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERDOMAINTASK__CHKRMGROUP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERDOMAINTASK__CHKSETGROUP, "_catCtxAlterDomainTask::_checkSetGroupTask" )
   INT32 _catCtxAlterDomainTask::_checkSetGroupTask ( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERDOMAINTASK__CHKSETGROUP ) ;

      const rtnDomainSetGroupTask * localTask =
                  dynamic_cast< const rtnDomainSetGroupTask * >( _task ) ;

      if ( NULL != localTask )
      {
         RTN_DOMAIN_GROUP_LIST addingGroups, removingGroups ;
         const RTN_DOMAIN_GROUP_LIST & groups = localTask->getGroups() ;

         _extractGroups( groups, addingGroups, removingGroups ) ;

         rc = _checkAddingGroups( addingGroups, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check adding groups, rc: %d", rc ) ;

         rc = _checkRemovingGroups( removingGroups, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check removing groups, rc: %d", rc ) ;
      }
      else
      {
         PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                   "Failed to get set group task" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERDOMAINTASK__CHKSETGROUP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERDOMAINTASK__CHKSETATTR, "_catCtxAlterDomainTask::_checkSetAttrTask" )
   INT32 _catCtxAlterDomainTask::_checkSetAttrTask ( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERDOMAINTASK__CHKSETATTR ) ;

      const rtnDomainSetAttributeTask * localTask =
                  dynamic_cast< const rtnDomainSetAttributeTask * >( _task ) ;

      if ( NULL != localTask )
      {
         RTN_DOMAIN_GROUP_LIST addingGroups, removingGroups ;
         const RTN_DOMAIN_GROUP_LIST & groups = localTask->getGroups() ;

         _extractGroups( groups, addingGroups, removingGroups ) ;

         rc = _checkAddingGroups( addingGroups, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check adding groups, rc: %d", rc ) ;

         rc = _checkRemovingGroups( removingGroups, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check removing groups, rc: %d", rc ) ;
      }
      else
      {
         PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                   "Failed to get add attribute task" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERDOMAINTASK__CHKSETATTR, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERDOMAINTASK__CHKADDGROUPS, "_catCtxAlterDomainTask::_checkAddingGroups" )
   INT32 _catCtxAlterDomainTask::_checkAddingGroups ( const RTN_DOMAIN_GROUP_LIST & groups,
                                                      _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERDOMAINTASK__CHKADDGROUPS ) ;

      sdbCatalogueCB * catCB = pmdGetKRCB()->getCATLOGUECB() ;

      for ( RTN_DOMAIN_GROUP_LIST::const_iterator iterGroup = groups.begin();
            iterGroup != groups.end();
            ++ iterGroup )
      {
         const CHAR * groupName = ( *iterGroup ) ;
         UINT32 groupID = CAT_INVALID_GROUPID ;
         BOOLEAN isExist = FALSE ;

         if ( _groupMap.find( groupName ) != _groupMap.end() )
         {
            continue ;
         }

         // Should be a valid data group
         groupID = catCB->groupName2ID( groupName ) ;
         PD_CHECK( CAT_INVALID_GROUPID != groupID,
                   SDB_CLS_NO_GROUP_INFO, error, PDERROR,
                   "Failed to check group [%s]: not exist", groupName ) ;
         PD_CHECK( DATA_GROUP_ID_BEGIN <= groupID &&
                   DATA_GROUP_ID_END >= groupID,
                   SDB_CAT_IS_NOT_DATAGROUP, error, PDERROR,
                   "Failed to check group [%s]: not data group", groupName ) ;

         if ( !catCB->checkGroupActived( groupName, isExist ) )
         {
            rc = SDB_REPL_GROUP_NOT_ACTIVE ;
            PD_CHECK( isExist, SDB_CLS_GRP_NOT_EXIST, error, PDWARNING,
                      "Failed to check group [%s]: not exist", groupName ) ;
            PD_RC_CHECK( rc, PDWARNING, "The group [%s] is not active, rc: %d",
                         groupName, rc ) ;
         }

         // the group that has no image can't be as the collection location
         PD_CHECK( !catCB->isImageEnabled() ||
                   catCB->getCatDCMgr()->groupInImage( groupName ),
                   SDB_CAT_GROUP_HASNOT_IMAGE, error, PDWARNING,
                   "Failed to check group [%s]: the group that has no image can"
                   " not be as the collection's location when image is enabled",
                   groupName ) ;

         _groupMap.insert( CAT_DOMAIN_GROUP_MAP::value_type( groupName,
                                                             groupID ) ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERDOMAINTASK__CHKADDGROUPS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERDOMAINTASK__CHKRMGROUPS, "_catCtxAlterDomainTask::_checkRemovingGroups" )
   INT32 _catCtxAlterDomainTask::_checkRemovingGroups ( const RTN_DOMAIN_GROUP_LIST & groups,
                                                        _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERDOMAINTASK__CHKRMGROUPS ) ;

      sdbCatalogueCB * catCB = pmdGetKRCB()->getCATLOGUECB() ;
      ossPoolSet< UINT32 > removingGroups ;

      for ( RTN_DOMAIN_GROUP_LIST::const_iterator iterGroup = groups.begin();
            iterGroup != groups.end();
            ++ iterGroup )
      {
         const CHAR * groupName = ( *iterGroup ) ;
         UINT32 groupID = CAT_INVALID_GROUPID ;

         if ( _groupMap.find( groupName ) == _groupMap.end() )
         {
            continue ;
         }

         groupID = catCB->groupName2ID( groupName ) ;
         removingGroups.insert( groupID ) ;
      }

      if ( !removingGroups.empty() )
      {
         ossPoolList< string > collectionSpaces ;
         ossPoolSet< UINT32 > occupiedGroups ;

         /// Get collection spaces for domain
         rc = catGetDomainCSs( _dataName.c_str(), cb, collectionSpaces ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get collection spaces for "
                      "domain [%s], rc: %d", _dataName.c_str(), rc ) ;

         for ( ossPoolList< string >::iterator itCS = collectionSpaces.begin() ;
               itCS != collectionSpaces.end() ;
               itCS ++ )
         {
            /// For each collection space:
            /// 1. Get groups from collections
            /// 2. Get groups under splitting tasks
            const CHAR * collectionSpace = itCS->c_str() ;
            rc = catGetCSGroups( collectionSpace, cb, occupiedGroups, FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get group list of collection "
                         "space [%s], rc: %d", collectionSpace, rc ) ;
            rc = catGetCSTaskGroups( collectionSpace, cb, occupiedGroups ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get splitting "
                         "group list of collection space [%s]: rc: %d",
                         collectionSpace, rc ) ;
         }

         for ( ossPoolSet< UINT32 >::iterator itGroup = removingGroups.begin() ;
               itGroup != removingGroups.end() ;
               itGroup ++ )
         {
            UINT32 groupID = ( *itGroup ) ;
            const CHAR * groupName = catCB->groupID2Name( groupID ) ;

            // The group should not be occupied
            PD_CHECK( occupiedGroups.end() == occupiedGroups.find( groupID ),
                      SDB_DOMAIN_IS_OCCUPIED, error, PDERROR,
                      "Failed to checkout removing groups from domain [%s]:"
                      "clear data(of this domain) before remove it "
                      "from domain. groups to be removed[%s]",
                      _dataName.c_str(), groupName ) ;

            _groupMap.erase( groupName ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXALTERDOMAINTASK__CHKRMGROUPS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERDOMAINTASK__EXTRAGRPS, "_catCtxAlterDomainTask::_extractGroups" )
   void _catCtxAlterDomainTask::_extractGroups ( const RTN_DOMAIN_GROUP_LIST & groups,
                                                 RTN_DOMAIN_GROUP_LIST & addingGroups,
                                                 RTN_DOMAIN_GROUP_LIST & removingGroups )
   {
      PD_TRACE_ENTRY( SDB_CATCTXALTERDOMAINTASK__EXTRAGRPS ) ;

      CAT_DOMAIN_GROUP_MAP keepingGroups ;

      for ( RTN_DOMAIN_GROUP_LIST::const_iterator iterGroup = groups.begin() ;
            iterGroup != groups.end() ;
            iterGroup ++ )
      {
         const CHAR * group = ( *iterGroup ) ;
         CAT_DOMAIN_GROUP_MAP::const_iterator iter = _groupMap.find( group ) ;

         if ( _groupMap.end() != iter )
         {
            keepingGroups[ iter->first ] = iter->second ;
         }
         else
         {
            addingGroups.push_back( group ) ;
         }
      }

      for ( CAT_DOMAIN_GROUP_MAP::const_iterator iter = _groupMap.begin() ;
            iter != _groupMap.end() ;
            iter ++ )
      {
         if ( keepingGroups.end() == keepingGroups.find( iter->first ) )
         {
            removingGroups.push_back( iter->first.c_str() ) ;
         }
      }

      PD_TRACE_EXIT( SDB_CATCTXALTERDOMAINTASK__EXTRAGRPS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERDOMAINTASK__TODOMAINGRPS, "_catCtxAlterDomainTask::_toDomainGroups" )
   INT32 _catCtxAlterDomainTask::_toDomainGroups ( BSONObjBuilder & builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXALTERDOMAINTASK__TODOMAINGRPS ) ;

      BSONArrayBuilder groupInfoBuilder( builder.subarrayStart( CAT_GROUPS_NAME ) ) ;

      for ( CAT_DOMAIN_GROUP_MAP::iterator iterGroup = _groupMap.begin() ;
            iterGroup != _groupMap.end() ;
            iterGroup ++ )
      {
         BSONObjBuilder groupBuilder( groupInfoBuilder.subobjStart() ) ;
         groupBuilder.append( CAT_GROUPNAME_NAME, iterGroup->first ) ;
         groupBuilder.append( CAT_GROUPID_NAME, (INT32)iterGroup->second ) ;
         groupBuilder.done() ;
      }

      groupInfoBuilder.done() ;

      PD_TRACE_EXITRC( SDB_CATCTXALTERDOMAINTASK__TODOMAINGRPS, rc ) ;

      return rc ;
   }

}
