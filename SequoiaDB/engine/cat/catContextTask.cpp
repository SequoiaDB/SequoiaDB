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

   Source File Name = catContextTask.cpp

   Descriptive Name = Sub-Tasks for Catalog Runtime Context

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

#include "catContextTask.hpp"
#include "catCommon.hpp"
#include "catSplit.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"

namespace engine
{
   _catCtxTaskBase::_catCtxTaskBase ()
   {
      _needLocks = TRUE ;
      _needUpdate = FALSE ;
      _hasUpdated = FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXTASK_CHECKTASK, "_catCtxTaskBase::checkTask" )
   INT32 _catCtxTaskBase::checkTask ( _pmdEDUCB *cb, catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXTASK_CHECKTASK ) ;

      try
      {
         rc = _checkInternal ( cb, lockMgr ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to check sub-task, %d",
                      rc ) ;

         _needUpdate = TRUE ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXTASK_CHECKTASK, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXTASK_PREEXECUTE, "_catCtxTaskBase::preExecute" )
   INT32 _catCtxTaskBase::preExecute ( _pmdEDUCB *cb,
                                       SDB_DMSCB *pDmsCB,
                                       SDB_DPSCB *pDpsCB,
                                       INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXTASK_PREEXECUTE ) ;

      if ( !_needUpdate )
      {
         goto done ;
      }

      try
      {
         rc = _preExecuteInternal( cb, pDmsCB, pDpsCB, w ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to pre-execute sub-task, %d",
                      rc ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXTASK_PREEXECUTE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXTASK_EXECUTE, "_catCtxTaskBase::execute" )
   INT32 _catCtxTaskBase::execute ( _pmdEDUCB *cb,
                                    SDB_DMSCB *pDmsCB,
                                    SDB_DPSCB *pDpsCB,
                                    INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXTASK_EXECUTE ) ;

      if ( !_needUpdate )
      {
         goto done ;
      }

      try
      {
         rc = _executeInternal( cb, pDmsCB, pDpsCB, w ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to execute sub-task, %d",
                      rc ) ;

         _hasUpdated = TRUE ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXTASK_EXECUTE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXTASK_ROLLBACK, "_catCtxTaskBase::rollback" )
   INT32 _catCtxTaskBase::rollback ( _pmdEDUCB *cb,
                                     SDB_DMSCB *pDmsCB,
                                     SDB_DPSCB *pDpsCB,
                                     INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXTASK_ROLLBACK ) ;

      if ( !_hasUpdated )
      {
         goto done ;
      }

      try
      {
         rc = _rollbackInternal( cb, pDmsCB, pDpsCB, w ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to rollback sub-task, rc: %d",
                      rc ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXTASK_ROLLBACK, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   void _catCtxTaskBase::addIgnoreRC ( INT32 rc )
   {
      _ignoreRC.insert( rc ) ;
   }

   BOOLEAN _catCtxTaskBase::isIgnoredRC ( INT32 rc )
   {
      return ( _ignoreRC.find( rc ) != _ignoreRC.end ( )) ;
   }

   /*
    * _catCtxDataTask implement
    */
   _catCtxDataTask::_catCtxDataTask ( const std::string &dataName )
   : _catCtxTaskBase ()
   {
      _dataName = dataName ;
   }

   /*
    * _catCtxDropCSTask implement
    */
   _catCtxDropCSTask::_catCtxDropCSTask ( const std::string &csName )
   : _catCtxDataTask( csName )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCSTASK_CHECK_INT, "_catCtxDropCSTask::_checkInternal" )
   INT32 _catCtxDropCSTask::_checkInternal ( _pmdEDUCB *cb,
                                             catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCSTASK_CHECK_INT ) ;

      rc = catGetAndLockCollectionSpace( _dataName, _boData, cb,
                                         &lockMgr, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get the collection space [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCSTASK_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCSTASK_EXECUTE_INT, "_catCtxDropCSTask::_executeInternal" )
   INT32 _catCtxDropCSTask::_executeInternal ( _pmdEDUCB *cb,
                                               SDB_DMSCB *pDmsCB,
                                               SDB_DPSCB *pDpsCB,
                                               INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCSTASK_EXECUTE_INT ) ;

      rc = catDropCSStep( _dataName, cb, pDmsCB, pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to drop collection space [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

      PD_LOG ( PDDEBUG,
               "Finished drop collection space [%s] task",
               _dataName.c_str() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCSTASK_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * _catCtxDropCLTask implement
    */
   _catCtxDropCLTask::_catCtxDropCLTask ( const string &clName, INT32 version )
   : _catCtxDataTask( clName )
   {
      _version = version ;
      _needUpdateCoord = FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCLTASK_CHECK_INT, "_catCtxDropCLTask::_checkInternal" )
   INT32 _catCtxDropCLTask::_checkInternal ( _pmdEDUCB *cb,
                                             catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCLTASK_CHECK_INT ) ;

      INT32 curVersion = -1 ;

      rc = catGetAndLockCollection( _dataName, _boData, cb,
                                    _needLocks ? &lockMgr : NULL, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get the collection [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

      rc = rtnGetIntElement( _boData, CAT_VERSION_NAME, curVersion ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get the field [%s], rc: %d",
                   CAT_VERSION_NAME, rc ) ;

      if ( -1 != _version )
      {
         if ( _version != curVersion )
         {
            PD_LOG( PDWARNING,
                    "Need update Coord version of [%s] "
                    "( curVer: %d, coordVer: %d )",
                  _dataName.c_str(), curVersion, _version ) ;
         }
         _needUpdateCoord = TRUE ;
      }

      _version = curVersion ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCLTASK_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCLTASK_PREEXECUTE_INT, "_catCtxDropCLTask::_preExecuteInternal" )
   INT32 _catCtxDropCLTask::_preExecuteInternal ( _pmdEDUCB *cb,
                                                  SDB_DMSCB *pDmsCB,
                                                  SDB_DPSCB *pDpsCB,
                                                  INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCLTASK_PREEXECUTE_INT ) ;

      rc = catRemoveCLTasks( _dataName, cb, w ) ;
      if ( SDB_CAT_TASK_NOTFOUND == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to remove tasks with the collection [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

      PD_LOG( PDDEBUG,
              "Finished pre-execute of drop collection [%s] task",
              _dataName.c_str() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCLTASK_PREEXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPCLTASK_EXECUTE_INT, "_catCtxDropCLTask::_executeInternal" )
   INT32 _catCtxDropCLTask::_executeInternal ( _pmdEDUCB *cb,
                                               SDB_DMSCB *pDmsCB,
                                               SDB_DPSCB *pDpsCB,
                                               INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPCLTASK_EXECUTE_INT ) ;

      rc = catDropCLStep( _dataName, _version, FALSE, cb, pDmsCB, pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to drop collection [%s], rc=%d",
                   _dataName.c_str(), rc ) ;

      PD_LOG( PDDEBUG,
              "Finished drop collection [%s] task",
              _dataName.c_str() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPCLTASK_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * _catCtxUnlinkMainCLTask implement
    */
   _catCtxUnlinkMainCLTask::_catCtxUnlinkMainCLTask ( const string &mainCLName,
                                                      const string &subCLName )
   : _catCtxDataTask( mainCLName )
   {
      _subCLName = subCLName ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXUNLINKMAINCLTASK_CHECK_INT, "_catCtxUnlinkMainCLTask::_checkInternal" )
   INT32 _catCtxUnlinkMainCLTask::_checkInternal ( _pmdEDUCB *cb,
                                                   catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXUNLINKMAINCLTASK_CHECK_INT ) ;

      rc = catGetAndLockCollection( _dataName, _boData, cb,
                                    _needLocks ? &lockMgr : NULL, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get the main-collection [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

      rc = catCheckMainCollection( _boData, TRUE ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Source collection [%s] must be partitioned-collection!",
                   _dataName.c_str() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXUNLINKMAINCLTASK_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXUNLINKMAINCLTASK_EXECUTE_INT, "_catCtxUnlinkMainCLTask::_executeInternal" )
   INT32 _catCtxUnlinkMainCLTask::_executeInternal ( _pmdEDUCB *cb,
                                                     SDB_DMSCB *pDmsCB,
                                                     SDB_DPSCB *pDpsCB,
                                                     INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXUNLINKMAINCLTASK_EXECUTE_INT ) ;

      BSONObj dummyLowBound, dummyUpBound ;

      rc = catUnlinkMainCLStep( _dataName, _subCLName,
                                FALSE, dummyLowBound, dummyUpBound,
                                cb, pDmsCB, pDpsCB, w ) ;
      if ( SDB_CAT_NO_MATCH_CATALOG == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to update the main-collection [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

      PD_LOG( PDDEBUG,
              "Finished unlink main collection [%s/%s]",
              _dataName.c_str(), _subCLName.c_str() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXUNLINKMAINCLTASK_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * _catCtxUnlinkSubCLTask implement
    */
   _catCtxUnlinkSubCLTask::_catCtxUnlinkSubCLTask ( const string &mainCLName,
                                                    const string &subCLName )
   : _catCtxDataTask( subCLName )
   {
      _needUnlink = FALSE ;
      _mainCLName = mainCLName ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXUNLINKSUBCLTASK_CHECK_INT, "_catCtxUnlinkSubCLTask::_checkInternal" )
   INT32 _catCtxUnlinkSubCLTask::_checkInternal ( _pmdEDUCB *cb,
                                                  catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXUNLINKSUBCLTASK_CHECK_INT ) ;

      string tmpMainCLName ;

      rc = catGetAndLockCollection( _dataName, _boData, cb,
                                    _needLocks ? &lockMgr : NULL, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get the sub-collection [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

      rc = catCheckMainCollection( _boData, FALSE ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Sub-collection [%s] could not be a main-collection!",
                   _dataName.c_str() ) ;

      rc = catCheckRelinkCollection ( _boData, tmpMainCLName ) ;
      if ( rc == SDB_RELINK_SUB_CL )
      {
         PD_CHECK( 0 == tmpMainCLName.compare( _mainCLName ),
                   SDB_INVALIDARG, error, PDWARNING,
                   "Failed to unlink sub-collection [%s], "
                   "the original main-collection is %s not %s",
                   _dataName.c_str(), tmpMainCLName.c_str(),
                   _mainCLName.c_str() ) ;
         rc = SDB_OK ;
         _needUnlink = TRUE ;
      }
      else
      {
         PD_LOG( PDWARNING,
                 "Sub-collection [%s] hasn't been linked",
                 _dataName.c_str() ) ;
         _needUnlink = FALSE ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXUNLINKSUBCLTASK_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXUNLINKSUBCLTASK_EXECUTE_INT, "_catCtxUnlinkSubCLTask::_executeInternal" )
   INT32 _catCtxUnlinkSubCLTask::_executeInternal ( _pmdEDUCB *cb,
                                                    SDB_DMSCB *pDmsCB,
                                                    SDB_DPSCB *pDpsCB,
                                                    INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXUNLINKSUBCLTASK_EXECUTE_INT ) ;

      if ( !_needUnlink )
      {
         goto done ;
      }

      rc = catUnlinkSubCLStep( _dataName, cb, pDmsCB, pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to update the sub-collection [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

      PD_LOG ( PDDEBUG,
               "Finished unlink sub-collection [%s/%s]",
               _mainCLName.c_str(), _dataName.c_str() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXUNLINKSUBCLTASK_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * _catCtxCreateIdxTask implement
    */
   _catCtxCreateIdxTask::_catCtxCreateIdxTask ( const std::string &clName,
                                                const std::string &idxName,
                                                const BSONObj &boIdx )
   : _catCtxDataTask ( clName )
   {
      _idxName = idxName ;
      _boIdx = boIdx ;
      _isUnique = FALSE ;
      _uniqueCheck = TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATEIDXTASK_CHECK_INT, "_catCtxCreateIdxTask::_checkInternal" )
   INT32 _catCtxCreateIdxTask::_checkInternal ( _pmdEDUCB *cb,
                                                catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATEIDXTASK_CHECK_INT ) ;

      std::set<UINT32> checkedSet ;
      clsCatalogSet cataSet( _dataName.c_str() );

      rc = catGetAndLockCollection( _dataName, _boData, cb,
                                    _needLocks ? &lockMgr : NULL, SHARED ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get the collection [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

      rc = cataSet.updateCatSet( _boData );
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to parse catalog info [%s], rc: %d",
                   _boData.toString().c_str(), rc );

      if ( cataSet.isSharding() )
      {
         rc = rtnGetObjElement ( _boIdx, IXM_KEY_FIELD, _boIdxKey ) ;
         PD_RC_CHECK ( rc, PDWARNING,
                       "Failed to get [%s] for index [%s], rc: %d",
                       IXM_KEY_FIELD, _boIdx.toString().c_str(), rc ) ;

         rc = rtnGetBooleanElement ( _boIdx, IXM_UNIQUE_FIELD, _isUnique ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            _isUnique = FALSE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK ( rc, PDWARNING,
                       "Failed to get [%s] for index [%s], rc: %d",
                       IXM_UNIQUE_FIELD, _boIdx.toString().c_str(), rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCTXCREATEIDXTASK_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATEIDXTASK_EXECUTE_INT, "_catCtxCreateIdxTask::_executeInternal" )
   INT32 _catCtxCreateIdxTask::_executeInternal ( _pmdEDUCB *cb,
                                                  SDB_DMSCB *pDmsCB,
                                                  SDB_DPSCB *pDpsCB,
                                                  INT16 w )
   {
      PD_TRACE_ENTRY ( SDB_CATCTXCREATEIDXTASK_EXECUTE_INT ) ;


      PD_TRACE_EXIT ( SDB_CATCTXCREATEIDXTASK_EXECUTE_INT ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATEIDXTASK_ROLLBACK_INT, "_catCtxCreateIdxTask::_rollbackInternal" )
   INT32 _catCtxCreateIdxTask::_rollbackInternal( _pmdEDUCB *cb,
                                                  SDB_DMSCB *pDmsCB,
                                                  SDB_DPSCB *pDpsCB,
                                                  INT16 w )
   {
      PD_TRACE_ENTRY ( SDB_CATCTXCREATEIDXTASK_ROLLBACK_INT ) ;


      PD_TRACE_EXIT ( SDB_CATCTXCREATEIDXTASK_ROLLBACK_INT ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATEIDXTASK_CHECKIDXKEY, "_catCtxCreateIdxTask::checkIndexKey" )
   INT32 _catCtxCreateIdxTask::checkIndexKey ( clsCatalogSet &cataSet,
                                               std::set<UINT32> &checkedKeyIDs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATEIDXTASK_CHECKIDXKEY ) ;

      UINT32 skSiteID = cataSet.getShardingKeySiteID() ;
      if ( skSiteID > 0 )
      {
         if ( checkedKeyIDs.count( skSiteID ) > 0 )
         {
            goto done ;
         }
         checkedKeyIDs.insert( skSiteID ) ;
      }

      try
      {
         const BSONObj &shardingKey = cataSet.getShardingKey() ;
         BSONObjIterator shardingItr ( shardingKey ) ;
         while ( shardingItr.more() )
         {
            BSONElement sk = shardingItr.next() ;
            if ( _boIdxKey.getField( sk.fieldName() ).eoo() )
            {
               PD_LOG( PDWARNING,
                       "All fields in sharding key must "
                       "be included in unique index, missing field: %s,"
                       "shardingKey: %s, indexKey: %s, collection: %s",
                       sk.fieldName(), shardingKey.toString().c_str(),
                       _boIdxKey.toString().c_str(), cataSet.name() ) ;
               rc = SDB_SHARD_KEY_NOT_IN_UNIQUE_KEY ;
               goto error ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCREATEIDXTASK_CHECKIDXKEY, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * _catCtxDropIdxTask implement
    */
   _catCtxDropIdxTask::_catCtxDropIdxTask ( const std::string &clName,
                                            const std::string &idxName )
   : _catCtxDataTask ( clName )
   {
      _idxName = idxName ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPIDXTASK_CHECK_INT, "_catCtxDropIdxTask::_checkInternal" )
   INT32 _catCtxDropIdxTask::_checkInternal( _pmdEDUCB *cb,
                                             catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDROPIDXTASK_CHECK_INT ) ;

      rc = catGetAndLockCollection( _dataName, _boData, cb,
                                    _needLocks ? &lockMgr : NULL, SHARED ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get the collection [%s], rc: %d",
                   _dataName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDROPIDXTASK_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDROPIDXTASK_EXECUTE_INT, "_catCtxDropIdxTask::_executeInternal" )
   INT32 _catCtxDropIdxTask::_executeInternal ( _pmdEDUCB *cb,
                                                SDB_DMSCB *pDmsCB,
                                                SDB_DPSCB *pDpsCB,
                                                INT16 w )
   {
      PD_TRACE_ENTRY ( SDB_CATCTXDROPIDXTASK_EXECUTE_INT ) ;


      PD_TRACE_EXIT ( SDB_CATCTXDROPIDXTASK_EXECUTE_INT ) ;

      return SDB_OK ;
   }

   /*
    * _catCtxDelCLsFromCSTask implement
    */
   _catCtxDelCLsFromCSTask::_catCtxDelCLsFromCSTask ()
   : _catCtxDataTask( "" )
   {
   }

   INT32 _catCtxDelCLsFromCSTask::deleteCL ( const std::string &clFullName )
   {
      INT32 rc = SDB_OK ;

      CHAR szCLName[ DMS_COLLECTION_NAME_SZ + 1 ] = {0} ;
      CHAR szCSName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;

      rc = rtnResolveCollectionName( clFullName.c_str(), clFullName.size(),
                                     szCSName, DMS_COLLECTION_SPACE_NAME_SZ,
                                     szCLName, DMS_COLLECTION_NAME_SZ ) ;
      PD_RC_CHECK( rc, PDWARNING, "Resolve collection name[%s] failed, rc: %d",
                   clFullName.c_str(), rc ) ;

      {
         COLLECTION_MAP::iterator iter = _deleteCLMap.find( szCSName ) ;
         if ( iter == _deleteCLMap.end() )
         {
            vector<string> collections ;
            collections.push_back( szCLName ) ;
            _deleteCLMap.insert( make_pair( szCSName, collections ) ) ;
         }
         else
         {
            iter->second.push_back( szCLName ) ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDELCLCSTASK_CHECK_INT, "_catCtxDelCLsFromCSTask::_checkInternal" )
   INT32 _catCtxDelCLsFromCSTask::_checkInternal ( _pmdEDUCB *cb,
                                                   catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDELCLCSTASK_CHECK_INT ) ;


      PD_TRACE_EXITRC ( SDB_CATCTXDELCLCSTASK_CHECK_INT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXDELCLCSTASK_EXECUTE_INT, "_catCtxDelCLsFromCSTask::_executeInternal" )
   INT32 _catCtxDelCLsFromCSTask::_executeInternal ( _pmdEDUCB *cb,
                                                     SDB_DMSCB *pDmsCB,
                                                     SDB_DPSCB *pDpsCB,
                                                     INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXDELCLCSTASK_EXECUTE_INT ) ;

      for ( COLLECTION_MAP::iterator iterCS = _deleteCLMap.begin();
            iterCS != _deleteCLMap.end() ;
            ++ iterCS )
      {
         const string &csName = iterCS->first ;
         const vector<string> &deleteCLLst = iterCS->second ;

         rc = catDelCLsFromCS( csName, deleteCLLst, cb, pDmsCB, pDpsCB, w ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to delete collections from collection space [%s], "
                      "rc: %d",
                      csName.c_str(), rc ) ;

         PD_LOG ( PDDEBUG,
                  "Finished delete collections from collection space [%s] task",
                  csName.c_str() ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXDELCLCSTASK_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * _catCtxUnlinkCSTask implement
    */
   _catCtxUnlinkCSTask::_catCtxUnlinkCSTask ( const std::string &csName )
   : _catCtxDataTask( csName )
   {
   }

   INT32 _catCtxUnlinkCSTask::unlinkCS ( const std::string &mainCLName )
   {
      INT32 rc = SDB_OK ;

      _mainCLLst.insert( mainCLName ) ;

      return rc ;
   }

   INT32 _catCtxUnlinkCSTask::unlinkCS ( const std::set<std::string> &mainCLLst )
   {
      INT32 rc = SDB_OK ;

      _mainCLLst.insert( mainCLLst.begin(), mainCLLst.end() ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXUNLINKCSTASK_CHECK_INT, "_catCtxUnlinkCSTask::_checkInternal" )
   INT32 _catCtxUnlinkCSTask::_checkInternal ( _pmdEDUCB *cb,
                                               catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXUNLINKCSTASK_CHECK_INT ) ;

      std::set<std::string>::iterator iterMainCL = _mainCLLst.begin() ;

      while ( iterMainCL != _mainCLLst.end() )
      {
         const string &mainCLName = (*iterMainCL) ;
         BSONObj boMainCL ;

         rc = catGetAndLockCollection( mainCLName, boMainCL, cb,
                                       _needLocks ? &lockMgr : NULL, EXCLUSIVE ) ;
         if ( SDB_DMS_NOTEXIST == rc )
         {
            rc = SDB_OK ;
            _mainCLLst.erase( iterMainCL++ ) ;
            continue ;
         }
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to get the main-collection [%s], rc: %d",
                      mainCLName.c_str(), rc ) ;

         rc = catCheckMainCollection( boMainCL, TRUE ) ;
         if ( SDB_INVALID_MAIN_CL == rc )
         {
            rc = SDB_OK ;
            _mainCLLst.erase( iterMainCL++ ) ;
            continue ;
         }
         PD_RC_CHECK( rc, PDWARNING,
                      "Source collection [%s] must be partitioned-collection!",
                      mainCLName.c_str() ) ;

         iterMainCL ++ ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXUNLINKCSTASK_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXUNLINKCSTASK_EXECUTE_INT, "_catCtxUnlinkCSTask::_executeInternal" )
   INT32 _catCtxUnlinkCSTask::_executeInternal ( _pmdEDUCB *cb,
                                                 SDB_DMSCB *pDmsCB,
                                                 SDB_DPSCB *pDpsCB,
                                                 INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXUNLINKCSTASK_EXECUTE_INT ) ;

      for ( std::set<std::string>::iterator iterMainCL = _mainCLLst.begin();
            iterMainCL != _mainCLLst.end() ;
            ++ iterMainCL )
      {
         const string &mainCLName = (*iterMainCL) ;

         rc = catUnlinkCSStep( mainCLName, _dataName, cb, pDmsCB, pDpsCB, w ) ;
         if ( SDB_DMS_NOTEXIST == rc ||
              SDB_INVALID_MAIN_CL == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to unlink collections of space [%s] from "
                      "main collection [%s], rc: %d",
                      _dataName.c_str(), mainCLName.c_str(), rc ) ;

         PD_LOG ( PDDEBUG,
                  "Finished unlink collections from main collection [%s] task",
                  mainCLName.c_str() ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXUNLINKCSTASK_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }
}
