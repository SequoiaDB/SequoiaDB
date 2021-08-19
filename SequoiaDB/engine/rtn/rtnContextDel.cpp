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

   Source File Name = rtnContextDel.hpp

   Descriptive Name = RunTime Delete Operation Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          5/26/2017   David Li  Split from rtnContext.cpp

   Last Changed =

*******************************************************************************/
#include "rtnContextDel.hpp"
#include "rtn.hpp"
#include "dpsOp2Record.hpp"
#include "clsMgr.hpp"
#include "rtnTrace.hpp"
#include "ossMemPool.hpp"
#include "rtnLocalTask.hpp"
#include "clsUniqueIDCheckJob.hpp"

namespace engine
{
   RTN_CTX_AUTO_REGISTER(_rtnContextDelCS, RTN_CONTEXT_DELCS, "DELCS")

   _rtnContextDelCS::_rtnContextDelCS( SINT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID )
   {
      _status = DELCSPHASE_0 ;
      _pDmsCB = pmdGetKRCB()->getDMSCB() ;
      _pDpsCB = pmdGetKRCB()->getDPSCB() ;
      _pCatAgent = pmdGetKRCB()->getClsCB ()->getCatAgent () ;
      _pTransCB = pmdGetKRCB()->getTransCB();
      _gotDmsCBWrite = FALSE ;
      _gotLogSize = 0 ;
      _logicCSID = DMS_INVALID_LOGICCSID ;
      _hitEnd     = FALSE ;
      ossMemset( _name, 0, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
   }

   _rtnContextDelCS::~_rtnContextDelCS()
   {
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      pmdEDUCB *cb = eduMgr->getEDUByID( eduID() ) ;
      if ( DELCSPHASE_1 == _status )
      {
         INT32 rcTmp = SDB_OK;
         rcTmp = rtnDropCollectionSpaceP1Cancel( _name, cb, _pDmsCB,
                                                 getDPSCB() );
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "failed to cancel drop cs(name:%s, rc=%d)",
                    _name, rcTmp );
         }
         _status = DELCSPHASE_0;
      }
      _clean( cb );
   }

   const CHAR* _rtnContextDelCS::name() const
   {
      return "DELCS" ;
   }

   RTN_CONTEXT_TYPE _rtnContextDelCS::getType () const
   {
      return RTN_CONTEXT_DELCS;
   }

   INT32 _rtnContextDelCS::open( const CHAR *pCollectionName,
                                 _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record();

      SDB_ASSERT( pCollectionName, "pCollectionName can't be null!" );
      PD_CHECK( pCollectionName, SDB_INVALIDARG, error, PDERROR,
                "pCollectionName is null!" );
      rc = dmsCheckCSName( pCollectionName );
      PD_RC_CHECK( rc, PDERROR, "Invalid cs name(name:%s)",
                   pCollectionName );

      ossStrncpy( _name, pCollectionName, DMS_COLLECTION_SPACE_NAME_SZ ) ;

      /// test collection space exist
      rc = rtnTestCollectionSpaceCommand( pCollectionName, _pDmsCB ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         /// ignore collection space not exist
         PD_LOG( PDINFO, "Ignored error[%d] when drop collection space[%s]",
                 rc, pCollectionName ) ;
         rc = SDB_OK ;
         _isOpened = TRUE ;
         goto done ;
      }

      if ( NULL != getDPSCB() )
      {
         // reserved log-size
         UINT32 logRecSize = 0;
         rc = dpsCSDel2Record( pCollectionName, record ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to build record:%d",rc ) ;

         rc = getDPSCB()->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = _pTransCB->reservedLogSpace( logRecSize, cb );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to reserved log space(length=%u)",
                      logRecSize );
         _gotLogSize = logRecSize ;
      }

      rc = _pDmsCB->writable ( cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "dms is not writable, rc = %d", rc ) ;
      _gotDmsCBWrite = TRUE;

      rc = _tryLock( pCollectionName, cb );
      PD_RC_CHECK( rc, PDERROR, "Failed to lock, rc: %d", rc ) ;

      rc = rtnDropCollectionSpaceP1( _name, cb, _pDmsCB, getDPSCB() );
      PD_RC_CHECK( rc, PDERROR, "Failed to drop cs in phase1, rc: %d", rc );
      _status = DELCSPHASE_1 ;
      _isOpened = TRUE ;

   done:
      return rc;
   error:
      _clean( cb );
      goto done;
   }

   void _rtnContextDelCS::_toString( stringstream &ss )
   {
      ss << ",Name:" << _name
         << ",GotLogSize:" << _gotLogSize
         << ",GotDMSWrite:" << _gotDmsCBWrite
         << ",LogicalID:" << _logicCSID
         << ",Step:" << _status ;
   }

   INT32 _rtnContextDelCS::_prepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      SDB_RTNCB *pRtnCB = sdbGetRTNCB() ;
      clsCB *pClsCB = sdbGetClsCB() ;
      shardCB *pShdMgr = pClsCB->getShardCB() ;
      clsTaskMgr *pTaskMgr = pmdGetKRCB()->getClsCB()->getTaskMgr() ;
      CLS_SUBCL_LIST subCLs ;
      CLS_SUBCL_LIST_IT it ;
      ossPoolSet< string > mainCLs ;
      ossPoolSet< string >::iterator mainIter ;

      _pCatAgent->lock_w() ;
      _pCatAgent->clearBySpaceName( _name, &subCLs, &mainCLs ) ;
      _pCatAgent->release_w() ;

      it = subCLs.begin() ;
      while( it != subCLs.end() )
      {
         if ( SDB_OK != pShdMgr->syncUpdateCatalog( (*it).c_str() ) )
         {
            _pCatAgent->lock_w() ;
            _pCatAgent->clear( (*it).c_str() ) ;
            _pCatAgent->release_w() ;
         }
         pClsCB->invalidateCata( (*it).c_str() ) ;
         ++it ;
      }
      pClsCB->invalidateCata( _name ) ;

      // Clear main collection plans
      mainIter = mainCLs.begin() ;
      while ( mainIter != mainCLs.end() )
      {
         const CHAR * mainCLName = ( *mainIter ).c_str() ;
         // Clear plan cache in self
         pRtnCB->getAPM()->invalidateCLPlans( mainCLName ) ;
         // Clear plan cache in secondary nodes
         pClsCB->invalidatePlan( mainCLName ) ;
         ++ mainIter ;
      }

      /// already drop phrase1
      if ( DELCSPHASE_1 == _status )
      {
         rc = rtnDropCollectionSpaceP2( _name, cb, _pDmsCB, getDPSCB() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to drop cs in phase2(%d)", rc ) ;
         _status = DELCSPHASE_0 ;
         _clean( cb ) ;
      }
      else
      {
         //It is main cs, which hasn't data file, we should invalidate plan here.
         //If it is normal cs which its data file exists on data node,
         //invalidate plan will be executed in dms by calling onDropCS().
         pRtnCB->getAPM()->invalidateSUPlans( _name ) ;
         pClsCB->invalidateCache( _name, DPS_LOG_INVALIDCATA_TYPE_PLAN ) ;
      }

      rc = SDB_DMS_EOC ;

      /// wait all collection space's task finished
      cb->writingDB( FALSE ) ;
      while( pTaskMgr->taskCountByCS( _name ) > 0 )
      {
         pTaskMgr->waitTaskEvent() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextDelCS::_tryLock( const CHAR *pCollectionName,
                                     _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;

      if ( getDPSCB() )
      {
         UINT32 logicCSID = DMS_INVALID_LOGICCSID ;
         dmsStorageUnit *su = NULL ;
         dpsTransRetInfo lockConflict ;

         UINT32 length = ossStrlen ( pCollectionName ) ;
         PD_CHECK( (length > 0 && length <= DMS_SU_NAME_SZ), SDB_INVALIDARG,
                   error, PDERROR, "Invalid length of collectionspace name:%s",
                   pCollectionName ) ;

         rc = _pDmsCB->nameToSUAndLock( pCollectionName, suID, &su ) ;
         PD_RC_CHECK(rc, PDERROR, "lock collection space(%s) failed(rc=%d)",
                     pCollectionName, rc ) ;

         logicCSID = su->LogicalCSID() ;

         _pDmsCB->suUnlock ( suID ) ;
         suID = DMS_INVALID_CS ;
         su = NULL ;

         rc = _pTransCB->transLockTryX( cb, logicCSID, DMS_INVALID_MBID,
                                        NULL, &lockConflict ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Get transaction-lock of CS(%s) failed(rc=%d)"OSS_NEWLINE
                      "Conflict( representative ):"OSS_NEWLINE
                      "   EDUID:  %llu"OSS_NEWLINE
                      "   TID:    %u"OSS_NEWLINE
                      "   LockId: %s"OSS_NEWLINE
                      "   Mode:   %s"OSS_NEWLINE,
                      pCollectionName, rc,
                      lockConflict._eduID,
                      lockConflict._tid,
                      lockConflict._lockID.toString().c_str(),
                      lockModeToString( lockConflict._lockType ) ) ;

         _logicCSID = logicCSID ;
      }

   done:
      if ( suID != DMS_INVALID_CS )
      {
         _pDmsCB->suUnlock ( suID ) ;
         suID = DMS_INVALID_CS ;
      }
      return rc;
   error:
      goto done;
   }

   INT32 _rtnContextDelCS::_releaseLock( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      if ( cb && getDPSCB() && ( _logicCSID != DMS_INVALID_LOGICCSID ) )
      {
         _pTransCB->transLockRelease( cb, _logicCSID );
         _logicCSID = DMS_INVALID_LOGICCSID;
      }
      return rc;
   }

   void _rtnContextDelCS::_clean( _pmdEDUCB *cb )
   {
      INT32 rcTmp = SDB_OK;
      rcTmp = _releaseLock( cb );
      if ( rcTmp )
      {
         PD_LOG( PDERROR, "releas lock failed, rc: %d", rcTmp );
      }
      if ( _gotDmsCBWrite )
      {
         _pDmsCB->writeDown ( cb ) ;
         _gotDmsCBWrite = FALSE;
      }
      if ( _gotLogSize > 0 )
      {
         _pTransCB->releaseLogSpace( _gotLogSize, cb );
         _gotLogSize = 0;
      }
   }

   RTN_CTX_AUTO_REGISTER(_rtnContextDelCL, RTN_CONTEXT_DELCL, "DELCL")

   _rtnContextDelCL::_rtnContextDelCL( SINT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID )
   {
      _pDmsCB        = pmdGetKRCB()->getDMSCB() ;
      _pDpsCB        = pmdGetKRCB()->getDPSCB() ;
      _pCatAgent     = pmdGetKRCB()->getClsCB()->getCatAgent () ;
      _pTransCB      = pmdGetKRCB()->getTransCB();
      _gotDmsCBWrite = FALSE ;
      _hasLock       = FALSE ;
      _hasDropped    = FALSE ;
      _mbContext     = NULL ;
      _su            = NULL ;
      _clShortName   = NULL ;
      _hitEnd = FALSE ;
      ossMemset( _collectionName, 0, sizeof( _collectionName ) ) ;
   }

   _rtnContextDelCL::~_rtnContextDelCL()
   {
      pmdEDUMgr *eduMgr    = pmdGetKRCB()->getEDUMgr() ;
      pmdEDUCB *cb         = eduMgr->getEDUByID( eduID() ) ;
      _clean( cb ) ;
   }

   INT32 _rtnContextDelCL::_tryLock( const CHAR *pCollectionName,
                                     _pmdEDUCB *cb )
   {
      INT32 rc                = SDB_OK ;
      dmsStorageUnitID suID   = DMS_INVALID_CS ;
      IDmsExtDataHandler *extHandler = NULL ;

      ossStrncpy( _collectionName, pCollectionName,
                  DMS_COLLECTION_FULL_NAME_SZ ) ;

      rc = rtnResolveCollectionNameAndLock ( _collectionName, _pDmsCB,
                                             &_su, &_clShortName,
                                             suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name"
                   "(collection:%s, rc: %d)", _collectionName, rc ) ;

      // lock collection
      if ( getDPSCB() )
      {
         dpsTransRetInfo lockConflict ;
         rc = _su->data()->getMBContext( &_mbContext, _clShortName,
                                         EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pCollectionName, rc ) ;

         rc = _pTransCB->transLockTryX( cb, _su->LogicalCSID(),
                                        _mbContext->mbID(),
                                        NULL, &lockConflict ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Get transaction-lock of collection(%s) failed(rc=%d)"
                      OSS_NEWLINE
                      "Conflict( representative ):"OSS_NEWLINE
                      "   EDUID:  %llu"OSS_NEWLINE
                      "   TID:    %u"OSS_NEWLINE
                      "   LockId: %s"OSS_NEWLINE
                      "   Mode:   %s"OSS_NEWLINE,
                      pCollectionName, rc,
                      lockConflict._eduID,
                      lockConflict._tid,
                      lockConflict._lockID.toString().c_str(),
                      lockModeToString( lockConflict._lockType ) ) ;

         _hasLock = TRUE ;
      }

      extHandler = _su->data()->getExtDataHandler() ;
      if ( extHandler )
      {
         if ( !_mbContext )
         {
            rc = _su->data()->getMBContext( &_mbContext, _clShortName,
                                            EXCLUSIVE ) ;
            PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                         "rc: %d", pCollectionName, rc ) ;
         }

         rc = extHandler->onDelCL( _su->CSName(), _clShortName, cb ) ;
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            // if capped cs doesn't exist, we just ignor error
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "External operation on delete cl failed, "
                      "rc: %d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextDelCL::_releaseLock( _pmdEDUCB *cb )
   {
      if ( cb && _hasLock )
      {
         _pTransCB->transLockRelease( cb, _su->LogicalCSID(),
                                      _mbContext->mbID() ) ;
         _hasLock = FALSE ;
      }
      return SDB_OK ;
   }

   const CHAR* _rtnContextDelCL::name() const
   {
      return "DELCL" ;
   }

   RTN_CONTEXT_TYPE _rtnContextDelCL::getType () const
   {
      return RTN_CONTEXT_DELCL ;
   }

   INT32 _rtnContextDelCL::open( const CHAR *pCollectionName,
                                 _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      /// set w info
      _w = w ;

      SDB_ASSERT( pCollectionName, "pCollectionName can't be null!" );
      PD_CHECK( pCollectionName, SDB_INVALIDARG, error, PDERROR,
               "pCollectionName is null!" );
      rc = dmsCheckFullCLName( pCollectionName );
      PD_RC_CHECK( rc, PDERROR, "Invalid collection name(name:%s)",
                   pCollectionName ) ;

      rc = _pDmsCB->writable ( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;
      _gotDmsCBWrite = TRUE ;

      rc = _tryLock( pCollectionName, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock(rc=%d)", rc ) ;
      _isOpened = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextDelCL::_prepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      SDB_RTNCB * pRtnCB = pmdGetKRCB()->getRTNCB() ;
      clsCB * pClsCB = pmdGetKRCB()->getClsCB() ;
      clsTaskMgr * pTaskMgr = pClsCB->getTaskMgr() ;
      CHAR mainCL[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { '\0' } ;

      _pCatAgent->lock_w () ;
      _pCatAgent->clear ( _collectionName, mainCL, sizeof( mainCL ) ) ;
      _pCatAgent->release_w () ;
      pClsCB->invalidateCata( _collectionName ) ;

      // Clear catalog info and cached plans of main-collection if needed
      if ( '\0' != mainCL[ 0 ] )
      {
         _pCatAgent->lock_w() ;
         _pCatAgent->clear( mainCL ) ;
         _pCatAgent->release_w() ;
         pRtnCB->getAPM()->invalidateCLPlans( mainCL ) ;
         pClsCB->invalidateCache( mainCL, DPS_LOG_INVALIDCATA_TYPE_CATA |
                                          DPS_LOG_INVALIDCATA_TYPE_PLAN ) ;
      }

      // drop collection
      rc = _su->data()->dropCollection ( _clShortName, cb, getDPSCB(),
                                         TRUE, _mbContext ) ;
      if ( rc )
      {
         // Ignore SDB_DMS_NOTEXIST, which means the CL mignt be deleted already
         if ( SDB_DMS_NOTEXIST == rc )
         {
            PD_LOG ( PDWARNING, "Collection %s doesn't exist, ignored in drop "
                     "collection, rc: %d", _collectionName, rc ) ;
            rc = SDB_OK ;
         }
         else
         {
            PD_LOG ( PDERROR, "Failed to drop collection %s, rc: %d",
                     _collectionName, rc ) ;
            goto error ;
         }
      }

      _hasDropped = TRUE ;

      _clean( cb ) ;
      rc = SDB_DMS_EOC ;

      /// wait all collection's task finished
      cb->writingDB( FALSE ) ;

      {
         UINT32 waitCnt = 0 ;
         while( pTaskMgr->taskCountByCL( _collectionName ) > 0 )
         {
            pTaskMgr->waitTaskEvent() ;
            waitCnt ++ ;

            // Log the task list after waiting over 10 minutes
            if ( waitCnt > 600 )
            {
               PD_LOG( PDDEBUG, "DropCL [%s] is waiting for split tasks:\n%s",
                       _collectionName,
                       pTaskMgr->dumpTasks( CLS_TASK_UNKNOW ).c_str() ) ;
               waitCnt = 0 ;
            }

         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextDelCL::_toString( stringstream &ss )
   {
      ss << ",Name:" << _collectionName
         << ",GotDMSWrite:" << _gotDmsCBWrite
         << ",HasLock:" << _hasLock
         << ",HasDropped:" << _hasDropped ;
   }

   void _rtnContextDelCL::_clean( _pmdEDUCB *cb )
   {
      INT32 rcTmp = SDB_OK;
      IDmsExtDataHandler *extHandler = NULL ;

      if ( _su && _su->data() )
      {
         extHandler = _su->data()->getExtDataHandler() ;
         if ( extHandler )
         {
            if ( _hasDropped )
            {
               extHandler->done( DMS_EXTOPR_TYPE_DROPCL, cb ) ;
            }
            else
            {
               extHandler->abortOperation( DMS_EXTOPR_TYPE_DROPCL, cb ) ;
            }
         }
      }

      rcTmp = _releaseLock( cb ) ;
      if ( rcTmp )
      {
         PD_LOG( PDERROR, "release lock failed, rc: %d", rcTmp ) ;
      }
      if ( _su && _mbContext )
      {
         _su->data()->releaseMBContext( _mbContext ) ;
      }

      // unlock su
      if ( _pDmsCB && _su )
      {
         string csname = _su->CSName() ;
         _pDmsCB->suUnlock ( _su->CSID() ) ;
         _su = NULL ;

         if ( _hasDropped )
         {
            // ignore errors
            _pDmsCB->dropEmptyCollectionSpace( csname.c_str(),
                                               cb, getDPSCB() ) ;
         }
      }

      if ( _gotDmsCBWrite )
      {
         _pDmsCB->writeDown( cb ) ;
         _gotDmsCBWrite = FALSE ;
      }
      _isOpened = FALSE ;
   }

   RTN_CTX_AUTO_REGISTER(_rtnContextDelMainCL, RTN_CONTEXT_DELMAINCL, "DELMAINCL")

   _rtnContextDelMainCL::_rtnContextDelMainCL( SINT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID )
   {
      _pCatAgent     = pmdGetKRCB()->getClsCB()->getCatAgent() ;
      _pRtncb        = pmdGetKRCB()->getRTNCB();
      _lockDms       = FALSE ;
      _hitEnd        = FALSE ;
      ossMemset( _name, 0, DMS_COLLECTION_FULL_NAME_SZ + 1 );
   }

   _rtnContextDelMainCL::~_rtnContextDelMainCL()
   {
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      pmdEDUCB *cb = eduMgr->getEDUByID( eduID() ) ;
      _clean( cb );
   }

   void _rtnContextDelMainCL::_clean( _pmdEDUCB *cb )
   {
      if ( _lockDms )
      {
         pmdGetKRCB()->getDMSCB()->writeDown( cb ) ;
         _lockDms = FALSE ;
      }

      SUBCL_CONTEXT_LIST::iterator iter = _subContextList.begin();
      while( iter != _subContextList.end() )
      {
         if ( iter->second != -1 )
         {
            _pRtncb->contextDelete( iter->second, cb );
         }
         _subContextList.erase( iter++ );
      }
   }

   const CHAR* _rtnContextDelMainCL::name() const
   {
      return "DELMAINCL" ;
   }

   RTN_CONTEXT_TYPE _rtnContextDelMainCL::getType () const
   {
      return RTN_CONTEXT_DELMAINCL ;
   }

   INT32 _rtnContextDelMainCL::open( const CHAR *pCollectionName,
                                     CLS_SUBCL_LIST &subCLList,
                                     _pmdEDUCB *cb,
                                     INT16 w )
   {
      INT32 rc = SDB_OK ;
      CLS_SUBCL_LIST_IT iter ;
      rtnContextDelCL *delContext   = NULL ;
      SINT64 contextID              = -1 ;

      SDB_ASSERT( pCollectionName, "pCollectionName can't be null!" ) ;
      PD_CHECK( pCollectionName, SDB_INVALIDARG, error, PDERROR,
                "pCollectionName is null!" ) ;

      rc = dmsCheckFullCLName( pCollectionName ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid collection name[%s])",
                   pCollectionName ) ;

      // we need to check dms writable when invalidate cata/plan/statistics
      rc = pmdGetKRCB()->getDMSCB()->writable ( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      _lockDms = TRUE ;

      /// open sub collection context
      iter = subCLList.begin() ;
      while( iter != subCLList.end() )
      {
         rc = _pRtncb->contextNew( RTN_CONTEXT_DELCL,
                                   (rtnContext **)&delContext,
                                   contextID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create sub-context of sub-"
                      "collection[%s] in drop collection[%s], rc: %d",
                      (*iter).c_str(), pCollectionName, rc ) ;

         rc = delContext->open( (*iter).c_str(), cb, w ) ;
         if ( rc != SDB_OK )
         {
            _pRtncb->contextDelete( contextID, cb ) ;
            if ( SDB_DMS_NOTEXIST == rc )
            {
               ++iter;
               continue;
            }
            PD_LOG( PDERROR, "Failed to open sub-context of sub-"
                    "collection[%s] in drop collection[%s], rc: %d",
                    (*iter).c_str(), pCollectionName, rc ) ;
            goto error;
         }
         _subContextList[ *iter ] = contextID ;
         ++iter ;
      }

      ossStrcpy( _name, pCollectionName ) ;
      _isOpened = TRUE;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _rtnContextDelMainCL::_prepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      SUBCL_CONTEXT_LIST::iterator iterCtx ;

      /// drop sub collections
      iterCtx = _subContextList.begin() ;
      while( iterCtx != _subContextList.end() )
      {
         rtnContextBuf buffObj;
         rc = rtnGetMore( iterCtx->second, -1, buffObj, cb, _pRtncb ) ;
         PD_CHECK( SDB_DMS_EOC == rc || SDB_DMS_NOTEXIST == rc,
                   rc, error, PDERROR,
                   "Failed to del sub-collection, rc: %d",
                   rc ) ;
         rc = SDB_OK ;
         _subContextList.erase( iterCtx++ ) ;
      }

      /// clear main collection's catalog info
      _pCatAgent->lock_w () ;
      _pCatAgent->clear ( _name ) ;
      _pCatAgent->release_w () ;

      // Clear cached main-collection plans
      _pRtncb->getAPM()->invalidateCLPlans( _name ) ;

      // Tell secondary nodes to clear catalog and plan caches
      sdbGetClsCB()->invalidateCache( _name,
                                      DPS_LOG_INVALIDCATA_TYPE_CATA |
                                      DPS_LOG_INVALIDCATA_TYPE_PLAN ) ;

      _clean( cb ) ;
      rc = SDB_DMS_EOC ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextDelMainCL::_toString( stringstream &ss )
   {
      ss << ",Name:" << _name ;
   }

   RTN_CTX_AUTO_REGISTER(_rtnContextRenameCS, RTN_CONTEXT_RENAMECS, "RENAMECS")

   _rtnContextRenameCS::_rtnContextRenameCS( SINT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID )
   {
      _pDmsCB     = pmdGetKRCB()->getDMSCB() ;
      _pDpsCB     = pmdGetKRCB()->getDPSCB() ;
      _pCatAgent  = pmdGetKRCB()->getClsCB()->getCatAgent() ;
      _pFreezingWnd = pmdGetKRCB()->getClsCB()->getShardCB()->getFreezingWindow() ;
      _pLTMgr     = pmdGetKRCB()->getRTNCB()->getLTMgr() ;
      _pTransCB   = pmdGetKRCB()->getTransCB();
      _blockID    = 0 ;
      _lockDms    = FALSE ;
      _logicCSID  = DMS_INVALID_LOGICCSID ;
      _status     = RENAMECSPHASE_0 ;
      _skipGetMore = FALSE ;
      _hitEnd     = FALSE ;
      ossMemset( _oldName, 0, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
      ossMemset( _newName, 0, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
   }

   _rtnContextRenameCS::~_rtnContextRenameCS()
   {
      pmdEDUMgr *eduMgr    = pmdGetKRCB()->getEDUMgr() ;
      pmdEDUCB *cb         = eduMgr->getEDUByID( eduID() ) ;

#ifdef _WINDOWS
      if ( RENAMECSPHASE_1 == _status )
      {
         INT32 rcTmp = SDB_OK;
         rcTmp = _pDmsCB->renameCollectionSpaceP1Cancel( _oldName, _newName,
                                                         cb, getDPSCB() );
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "failed to cancel rename cs[%s], rc: %d",
                    _oldName, rcTmp );
         }
         _status = RENAMECSPHASE_0;
      }
#endif

      if ( _taskPtr.get() && _taskPtr->isTaskValid() )
      {
         /// context is killed by interrupted
         if ( cb->isInterrupted() )
         {
            _pCatAgent->lock_w() ;
            _pCatAgent->clearBySpaceName( _oldName ) ;
            _pCatAgent->release_w() ;

            if ( SDB_OK == clsStartRenameCheckJob( _taskPtr, _blockID ) )
            {
               _blockID = 0 ;
            }
            else
            {
               _pLTMgr->removeTask( _taskPtr, cb, _pDpsCB ) ;
            }
         }
         else
         {
            _pLTMgr->removeTask( _taskPtr, cb, _pDpsCB ) ;
         }
      }

      _releaseLock( cb ) ;
      _logger.clear() ;
   }

   const CHAR* _rtnContextRenameCS::name() const
   {
      return "RENAMECS" ;
   }

   RTN_CONTEXT_TYPE _rtnContextRenameCS::getType () const
   {
      return RTN_CONTEXT_RENAMECS ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRENAMECS_OPEN, "_rtnContextRenameCS::open" )
   INT32 _rtnContextRenameCS::open( const CHAR *pCSName,
                                    const CHAR *pNewCSName,
                                    _pmdEDUCB *cb,
                                    BOOLEAN useLocalTask )
   {
      INT32 rc = SDB_OK ;
      INT32 rcNew = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCTXRENAMECS_OPEN ) ;

      rtnLTRename *pRenameTask = NULL ;
      utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;

      /// check cs name
      SDB_ASSERT( pCSName, "cs name can't be null!" );
      PD_CHECK( pCSName, SDB_INVALIDARG, error, PDERROR,
                "cs name is null!" );

      SDB_ASSERT( pNewCSName, "new cs name can't be null!" );
      PD_CHECK( pNewCSName, SDB_INVALIDARG, error, PDERROR,
                "new cs name is null!" );

      rc = dmsCheckCSName( pCSName );
      PD_RC_CHECK( rc, PDERROR, "Invalid cs name[%s]", pCSName );

      rc = dmsCheckCSName( pNewCSName );
      PD_RC_CHECK( rc, PDERROR, "Invalid cs name[%s]", pNewCSName );

      ossStrncpy( _oldName, pCSName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
      ossStrncpy( _newName, pNewCSName, DMS_COLLECTION_SPACE_NAME_SZ ) ;

      /// test collection space exist
      rc = rtnTestCollectionSpaceCommand( pCSName, _pDmsCB, NULL, &csUniqueID ) ;

      rcNew = rtnTestCollectionSpaceCommand( pNewCSName, _pDmsCB ) ;

      if ( SDB_DMS_CS_NOTEXIST == rc && SDB_OK == rcNew )
      {
         // If old cs does not exist, but new cs already exists, then we just
         // goto done, and skip get more stage.
         _skipGetMore = TRUE ;
         _isOpened = TRUE ;
         rc = SDB_OK ;
         PD_LOG( PDINFO, "Old cs[%s] does not exist, but new cs[%s] "
                 "already exists, ignore error", pCSName, pNewCSName ) ;
         goto done ;
      }
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         // Ignore collection space not exist, because it may be a cs of maincl.
         // And do not set _status to phase_1
         PD_LOG( PDINFO, "Ignored error[%d] when rename collection space[%s]",
                 rc, pCSName ) ;
         rc = SDB_OK ;
         _isOpened = TRUE ;
         goto done ;
      }
      if ( SDB_OK == rcNew )
      {
         rc = SDB_DMS_CS_EXIST ;
         PD_LOG( PDERROR, "Collection space[%s] already exists, rc: %d",
                 pNewCSName, rc ) ;
         goto error ;
      }

      if ( UTIL_UNIQUEID_NULL == csUniqueID )
      {
         rc = SDB_DMS_UNQIUEID_UPGRADE ;
         PD_LOG( PDERROR, "Rename cs[%s] not ready: "
                 "upgrade for unique id is not finished, rc: %d",
                 pCSName, rc ) ;
         goto error ;
      }

      /// lock
      rc = _tryLock( pCSName, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock, rc: %d", rc ) ;

      /// log to .SEQUOIADB_RENAME_INFO
      {
         utilRenameLog aLog ( _oldName, _newName ) ;

         rc = _logger.init() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to init rename logger, rc: %d", rc ) ;

         rc = _logger.log( aLog ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to log rename info to file, rc: %d", rc ) ;
      }

      /// rename cs at phase 1
#ifdef _WINDOWS
         rc = _pDmsCB->renameCollectionSpaceP1( _oldName, _newName,
                                                cb, _pDpsCB );
         PD_RC_CHECK( rc, PDERROR, "Failed to rename cs "
                      "from [%s] to [%s] at phase 1, rc: %d",
                      _oldName, _newName, rc ) ;
#endif

      /// add task
      if ( useLocalTask )
      {
         INT16 i = 0 ;

         rc = rtnGetLTFactory()->create( RTN_LOCAL_TASK_RENAMECS, _taskPtr ) ;
         PD_RC_CHECK( rc, PDERROR, "Alloc rename task failed, rc: %d", rc ) ;

         pRenameTask = ( rtnLTRename* )_taskPtr.get() ;
         pRenameTask->setInfo( pCSName, pNewCSName ) ;

         while( TRUE )
         {
            rc = _pLTMgr->addTask( _taskPtr, cb, _pDpsCB ) ;
            if ( SDB_CLS_MUTEX_TASK_EXIST == rc &&
                 i++ < RTN_RENAME_BLOCKWRITE_TIMES )
            {
               // When two threads concurrently do rename, addTask() will report
               // -175. We retry multiple times to reduce the error.
               _pLTMgr->waitTaskEvent( OSS_ONE_SEC ) ;
               continue ;
            }

            if ( rc )
            {
               if ( SDB_CLS_MUTEX_TASK_EXIST == rc )
               {
                  PD_LOG_MSG( PDERROR, "Rename cs is mutually exclusive with "
                              "other rename cs/cl" ) ;
               }
               else
               {
                  PD_LOG( PDERROR, "Add task(%s) failed, rc: %d",
                          _taskPtr->toPrintString().c_str(), rc ) ;
               }
               goto error ;
            }
            break ;
         }
      }

      _status   = RENAMECSPHASE_1 ;
      _isOpened = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCTXRENAMECS_OPEN, rc ) ;
      return rc;
   error:
      _releaseLock( cb ) ;
      if ( _taskPtr.get() && _taskPtr->isTaskValid() )
      {
         _pLTMgr->removeTask( _taskPtr, cb, _pDpsCB ) ;
      }
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRENAMECS_PREPAREDATA, "_rtnContextRenameCS::_prepareData" )
   INT32 _rtnContextRenameCS::_prepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCTXRENAMECS_PREPAREDATA ) ;

      SDB_RTNCB *pRtnCB = sdbGetRTNCB() ;
      clsCB *pClsCB = sdbGetClsCB() ;
      shardCB *pShdMgr = pClsCB->getShardCB() ;
      CLS_SUBCL_LIST subCLs ;
      CLS_SUBCL_LIST_IT it ;
      ossPoolSet< string > mainCLs ;
      ossPoolSet< string >::iterator mainIter ;

      if ( _skipGetMore )
      {
         _isOpened = FALSE ;
         rc = SDB_DMS_EOC ;
         goto done ;
      }

      _pCatAgent->lock_w() ;
      _pCatAgent->clearBySpaceName( _oldName, &subCLs, &mainCLs ) ;
      _pCatAgent->release_w() ;

      it = subCLs.begin() ;
      while( it != subCLs.end() )
      {
         if ( SDB_OK != pShdMgr->syncUpdateCatalog( (*it).c_str() ) )
         {
            _pCatAgent->lock_w() ;
            _pCatAgent->clear( (*it).c_str() ) ;
            _pCatAgent->release_w() ;
         }
         pClsCB->invalidateCata( (*it).c_str() ) ;
         ++it ;
      }
      pClsCB->invalidateCata( _oldName ) ;

      // Clear main collection plans
      mainIter = mainCLs.begin() ;
      while ( mainIter != mainCLs.end() )
      {
         const CHAR * mainCLName = ( *mainIter ).c_str() ;
         // Clear plan cache in self
         pRtnCB->getAPM()->invalidateCLPlans( mainCLName ) ;
         // Clear plan cache in secondary nodes
         pClsCB->invalidatePlan( mainCLName ) ;
         ++ mainIter ;
      }

      if ( _status == RENAMECSPHASE_1 )
      {
#ifdef _WINDOWS
         rc = _pDmsCB->renameCollectionSpaceP2( _oldName, _newName,
                                                cb, _pDpsCB );
#else
         rc = _pDmsCB->renameCollectionSpace( _oldName, _newName,
                                              cb, _pDpsCB );
#endif
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to rename cs from [%s] to [%s], rc: %d",
                      _oldName, _newName, rc ) ;

         rc = _logger.clear() ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Failed to clear rename info, rc: %d", rc ) ;
         }

         _status = RENAMECSPHASE_0 ;
         _releaseLock( cb ) ;
      }
      else
      {
         //It is main cs, which hasn't data file, we should invalidate plan here.
         //If it is normal cs which its data file exists on data node,
         //invalidate plan will be executed in dms by calling onRenameCS().
         pRtnCB->getAPM()->invalidateSUPlans( _oldName ) ;
         pClsCB->invalidateCache( _oldName, DPS_LOG_INVALIDCATA_TYPE_PLAN ) ;
      }

      rc = SDB_DMS_EOC ;

   done:
      if ( SDB_DMS_EOC == rc && _taskPtr.get() &&
           _taskPtr->isTaskValid() )
      {
         _pLTMgr->removeTask( _taskPtr, cb, _pDpsCB ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNCTXRENAMECS_PREPAREDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextRenameCS::_toString( stringstream &ss )
   {
      ss << ",Name:" << _oldName
         << ",NewName:" << _newName
         << ",BlockID:" << _blockID
         << ",LockDMS:" << _lockDms
         << ",LogicalID:" << _logicCSID
         << ",Step:" << _status ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRENAMECS__TRYLOCK, "_rtnContextRenameCS::_tryLock" )
   INT32 _rtnContextRenameCS::_tryLock( const CHAR *pCSName, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCTXRENAMECS__TRYLOCK ) ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      UINT32 logicCSID = DMS_INVALID_LOGICCSID ;
      dmsStorageUnit *su = NULL ;
      pmdEDUMgr *eduMgr = cb->getEDUMgr() ;
      UINT32 i = 0 ;

      /// get cs logical id
      rc = _pDmsCB->nameToSUAndLock( pCSName, suID, &su ) ;
      PD_RC_CHECK(rc, PDERROR, "lock collection space[%s] failed, rc: %d",
                  pCSName, rc ) ;

      logicCSID = su->LogicalCSID() ;

      _pDmsCB->suUnlock ( suID ) ;
      suID = DMS_INVALID_CS ;
      su = NULL ;

      /// block cs write
      rc = _pFreezingWnd->registerCS( _oldName, _blockID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Block all write operations of "
                 "collectionspace[%s] failed, rc: %d",
                 _oldName, rc ) ;
         goto error ;
      }
      else
      {
         PD_LOG( PDEVENT, "Begin to block all write operations "
                 "of collectionspace[%s], ID: %llu", _oldName,
                 _blockID ) ;
      }

      /// wait write done
      i = 0 ;
      while( TRUE )
      {
         ++i ;
         UINT32 cnt = 0 ;
         UINT32 transCnt = 0 ;
         dpsTransLockId lockID( logicCSID, DMS_INVALID_MBID, NULL ) ;
         cnt = eduMgr->getWritingEDUCount( -1, _blockID, EDU_BLOCK_FREEZING_WND,
                                          lockID, &transCnt ) ;
         if ( cnt > 0 )
         {
            if ( i < RTN_RENAME_BLOCKWRITE_TIMES )
            {
               ossSleep( OSS_ONE_SEC ) ;
               continue ;
            }
            else if ( cb->isInterrupted() )
            {
               rc = SDB_APP_INTERRUPT ;
            }
            else
            {
               rc = ( cnt == transCnt ) ?
                    SDB_DPS_TRANS_LOCK_INCOMPATIBLE : SDB_LOCK_FAILED ;
               PD_LOG_MSG( PDERROR, "Failed to wait for other write "
                           "operations to finish" ) ;
            }
         }

         break ;
      }

      if ( rc )
      {
         goto error ;
      }

      rc = _pDmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      _lockDms = TRUE ;

      if ( getDPSCB() )
      {
         dpsTransRetInfo lockConflict ;
         rc = _pTransCB->transLockTryS( cb, logicCSID, DMS_INVALID_MBID,
                                        NULL, &lockConflict ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Get transaction-lock of CS[%s] failed, rc: %d"OSS_NEWLINE
                      "Conflict( representative ):"OSS_NEWLINE
                      "   EDUID:  %llu"OSS_NEWLINE
                      "   TID:    %u"OSS_NEWLINE
                      "   LockId: %s"OSS_NEWLINE
                      "   Mode:   %s"OSS_NEWLINE,
                      pCSName, rc,
                      lockConflict._eduID,
                      lockConflict._tid,
                      lockConflict._lockID.toString().c_str(),
                      lockModeToString( lockConflict._lockType ) ) ;

         _logicCSID = logicCSID ;
      }

   done:
      if ( suID != DMS_INVALID_CS )
      {
         _pDmsCB->suUnlock ( suID ) ;
         suID = DMS_INVALID_CS ;
      }
      PD_TRACE_EXITRC( SDB__RTNCTXRENAMECS__TRYLOCK, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRENAMECS__RELLOCK, "_rtnContextRenameCS::_releaseLock" )
   INT32 _rtnContextRenameCS::_releaseLock( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__RTNCTXRENAMECS__RELLOCK ) ;

      if ( cb && getDPSCB() && ( _logicCSID != DMS_INVALID_LOGICCSID ) )
      {
         _pTransCB->transLockRelease( cb, _logicCSID );
         _logicCSID = DMS_INVALID_LOGICCSID;
      }

      if ( 0 != _blockID )
      {
         _pFreezingWnd->unregisterCS( _oldName, _blockID ) ;
         PD_LOG( PDEVENT, "End to block all write operations of "
                 "collectionspace(%s), ID: %llu",
                 _oldName, _blockID ) ;
         _blockID = 0 ;
      }

      if ( _lockDms )
      {
         _pDmsCB->writeDown( cb ) ;
         _lockDms = FALSE ;
      }

      PD_TRACE_EXIT( SDB__RTNCTXRENAMECS__RELLOCK ) ;
      return SDB_OK ;
   }

   RTN_CTX_AUTO_REGISTER(_rtnContextRenameCL, RTN_CONTEXT_RENAMECL, "RENAMECL")

   _rtnContextRenameCL::_rtnContextRenameCL( SINT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID )
   {
      _pDmsCB        = pmdGetKRCB()->getDMSCB() ;
      _pDpsCB        = pmdGetKRCB()->getDPSCB() ;
      _pCatAgent     = pmdGetKRCB()->getClsCB()->getCatAgent () ;
      _pFreezingWnd = pmdGetKRCB()->getClsCB()->getShardCB()->getFreezingWindow() ;
      _pLTMgr        = pmdGetKRCB()->getRTNCB()->getLTMgr() ;
      _pTransCB      = pmdGetKRCB()->getTransCB() ;
      _blockID       = 0 ;
      _lockDms       = FALSE ;
      _mbID          = DMS_INVALID_MBID ;
      _su            = NULL ;
      _skipGetMore   = FALSE ;
      _hitEnd        = FALSE ;
      ossMemset( _clShortName, 0, sizeof( _clShortName ) ) ;
      ossMemset( _newCLShortName, 0, sizeof( _newCLShortName ) ) ;
      ossMemset( _clFullName, 0, sizeof( _clFullName ) ) ;
   }

   _rtnContextRenameCL::~_rtnContextRenameCL()
   {
      pmdEDUMgr *eduMgr    = pmdGetKRCB()->getEDUMgr() ;
      pmdEDUCB *cb         = eduMgr->getEDUByID( eduID() ) ;

      if ( _taskPtr.get() && _taskPtr->isTaskValid() )
      {
         /// context is killed by interrupted
         if ( cb->isInterrupted() )
         {
            _pCatAgent->lock_w() ;
            _pCatAgent->clear( _clFullName ) ;
            _pCatAgent->release_w() ;

            if ( SDB_OK == clsStartRenameCheckJob( _taskPtr, _blockID ) )
            {
               _blockID = 0 ;
            }
            else
            {
               _pLTMgr->removeTask( _taskPtr, cb, _pDpsCB ) ;
            }
         }
         else
         {
            _pLTMgr->removeTask( _taskPtr, cb, _pDpsCB ) ;
         }
      }

      _releaseLock( cb ) ;
   }

   const CHAR* _rtnContextRenameCL::name() const
   {
      return "RENAMECL" ;
   }

   RTN_CONTEXT_TYPE _rtnContextRenameCL::getType () const
   {
      return RTN_CONTEXT_RENAMECL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRENAMECL_OPEN, "_rtnContextRenameCL::open" )
   INT32 _rtnContextRenameCL::open( const CHAR *csName, const CHAR *clShortName,
                                    const CHAR *newCLShortName,
                                    _pmdEDUCB *cb, INT16 w,
                                    BOOLEAN useLocalTask )
   {
      INT32 rc = SDB_OK ;
      INT32 rcNew = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCTXRENAMECL_OPEN ) ;

      CHAR newCLFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      rtnLTRename *pRenameTask = NULL ;
      utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;

      /// set w info
      _w = w ;

      /// check cs name
      SDB_ASSERT( csName, "cs name can't be null!" );
      PD_CHECK( csName, SDB_INVALIDARG, error, PDERROR,
                "cs name is null!" );

      rc = dmsCheckCSName( csName );
      PD_RC_CHECK( rc, PDERROR, "Invalid cs name[%s]", csName ) ;

      /// check cl name
      SDB_ASSERT( clShortName, "collection name can't be null!" );
      PD_CHECK( clShortName, SDB_INVALIDARG, error, PDERROR,
                "collection name is null!" );

      SDB_ASSERT( newCLShortName, "new collection name can't be null!" );
      PD_CHECK( newCLShortName, SDB_INVALIDARG, error, PDERROR,
                "new collection name is null!" );

      rc = dmsCheckCLName( clShortName );
      PD_RC_CHECK( rc, PDERROR, "Invalid collection name[%s]", clShortName ) ;

      rc = dmsCheckCLName( newCLShortName );
      PD_RC_CHECK( rc, PDERROR, "Invalid collection name[%s]", newCLShortName );

      ossStrncpy( _clShortName, clShortName, DMS_COLLECTION_NAME_SZ ) ;
      ossStrncpy( _newCLShortName, newCLShortName, DMS_COLLECTION_NAME_SZ ) ;
      ossSnprintf( _clFullName, sizeof( _clFullName ),
                   "%s.%s", csName, clShortName ) ;
      ossSnprintf( newCLFullName, sizeof( newCLFullName ),
                   "%s.%s", csName, newCLShortName ) ;

      /// test collection space exist
      rc = rtnTestCollectionCommand( _clFullName, _pDmsCB, NULL, &clUniqueID ) ;

      rcNew = rtnTestCollectionCommand( newCLFullName, _pDmsCB ) ;

      if ( SDB_DMS_NOTEXIST == rc && SDB_OK == rcNew )
      {
         // If old cl does not exist, but new cl already exists,then we just
         // goto done, and skip get more stage.
         _skipGetMore = TRUE ;
         _isOpened = TRUE ;
         rc = SDB_OK ;
         PD_LOG( PDINFO, "Old cl[%s] does not exist, but new cl[%s] "
                 "already exists, ignore error", _clFullName, newCLFullName ) ;
         goto done ;
      }
      if ( rc )
      {
         PD_LOG( PDERROR, "Collection[%s] does not exist, rc: %d",
                 _clFullName, rc ) ;
         goto error ;
      }
      if ( SDB_OK == rcNew )
      {
         rc = SDB_DMS_EXIST ;
         PD_LOG( PDERROR, "Collection[%s] already exists, rc: %d",
                 newCLFullName, rc ) ;
         goto error ;
      }

      if ( UTIL_UNIQUEID_NULL == clUniqueID )
      {
         rc = SDB_DMS_UNQIUEID_UPGRADE ;
         PD_LOG( PDERROR, "Rename cl[%s] not ready: "
                 "upgrade for unique id is not finished, rc: %d",
                 _clFullName, rc ) ;
         goto error ;
      }

      /// lock
      rc = _tryLock( csName, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock, rc: %d", rc ) ;
      _lockDms = TRUE ;

      /// add task
      if ( useLocalTask )
      {
         INT16 i = 0 ;

         rc = rtnGetLTFactory()->create( RTN_LOCAL_TASK_RENAMECL, _taskPtr ) ;
         PD_RC_CHECK( rc, PDERROR, "Alloc rename task failed, rc: %d", rc ) ;

         pRenameTask = ( rtnLTRename* )_taskPtr.get() ;
         pRenameTask->setInfo( _clFullName, newCLFullName ) ;

         while( TRUE )
         {
            rc = _pLTMgr->addTask( _taskPtr, cb, _pDpsCB ) ;
            if ( SDB_CLS_MUTEX_TASK_EXIST == rc &&
                 i++ < RTN_RENAME_BLOCKWRITE_TIMES )
            {
               // When two threads concurrently do rename, addTask() will report
               // -175. We retry multiple times to reduce the error.
               _pLTMgr->waitTaskEvent( OSS_ONE_SEC ) ;
               continue ;
            }

            if ( rc )
            {
               if ( SDB_CLS_MUTEX_TASK_EXIST == rc )
               {
                  PD_LOG_MSG( PDERROR, "Rename cl is mutually exclusive with "
                              "other rename cs/cl" ) ;
               }
               else
               {
                  PD_LOG( PDERROR, "Add task(%s) failed, rc: %d",
                          _taskPtr->toPrintString().c_str(), rc ) ;
               }
               goto error ;
            }
            break ;
         }
      }

      _isOpened = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCTXRENAMECL_OPEN, rc ) ;
      return rc ;
   error:
      _releaseLock( cb ) ;
      if ( _taskPtr.get() && _taskPtr->isTaskValid() )
      {
         _pLTMgr->removeTask( _taskPtr, cb, _pDpsCB ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRENAMECL_PREPAREDATA, "_rtnContextRenameCL::_prepareData" )
   INT32 _rtnContextRenameCL::_prepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCTXRENAMECL_PREPAREDATA ) ;

      SDB_RTNCB * pRtnCB = pmdGetKRCB()->getRTNCB() ;
      clsCB * pClsCB = pmdGetKRCB()->getClsCB() ;
      CHAR mainCL[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { '\0' } ;

      if ( _skipGetMore )
      {
         rc = SDB_DMS_EOC ;
         goto done ;
      }

      _pCatAgent->lock_w () ;
      _pCatAgent->clear( _clFullName, mainCL, sizeof( mainCL ) ) ;
      _pCatAgent->release_w () ;
      pClsCB->invalidateCata( _clFullName ) ;

      // Clear catalog info and cached plans of main-collection if needed
      if ( '\0' != mainCL[ 0 ] )
      {
         _pCatAgent->lock_w() ;
         _pCatAgent->clear( mainCL ) ;
         _pCatAgent->release_w() ;
         pRtnCB->getAPM()->invalidateCLPlans( mainCL ) ;
         pClsCB->invalidateCache( mainCL, DPS_LOG_INVALIDCATA_TYPE_CATA |
                                          DPS_LOG_INVALIDCATA_TYPE_PLAN ) ;
      }

      // rename collection
      rc = _su->data()->renameCollection ( _clShortName, _newCLShortName,
                                           cb, _pDpsCB ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to rename collection from [%s] to [%s], rc: %d",
                   _clShortName, _newCLShortName, rc ) ;

      _releaseLock( cb ) ;

      rc = SDB_DMS_EOC ;

   done:
      if ( SDB_DMS_EOC == rc && _taskPtr.get() &&
           _taskPtr->isTaskValid() )
      {
         _pLTMgr->removeTask( _taskPtr, cb, _pDpsCB ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNCTXRENAMECL_PREPAREDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextRenameCL::_toString( stringstream &ss )
   {
      ss << ",Name:" << _clFullName
         << ",NewCLName:" << _newCLShortName
         << ",BlockID:" << _blockID
         << ",LockDMS:" << _lockDms
         << ",MBID:" << _mbID ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRENAMECL__TRYLOCK, "_rtnContextRenameCL::_tryLock" )
   INT32 _rtnContextRenameCL::_tryLock( const CHAR *csName, _pmdEDUCB *cb )
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCTXRENAMECL__TRYLOCK ) ;

      dmsStorageUnitID suID   = DMS_INVALID_CS ;
      dmsMBContext* mbContext = NULL ;
      UINT16 mbID             = DMS_INVALID_MBID ;
      pmdEDUMgr *eduMgr = cb->getEDUMgr() ;
      UINT32 i = 0 ;

      // get collection info
      rc = _pDmsCB->nameToSUAndLock( csName, suID, &_su, SHARED );
      PD_RC_CHECK( rc, PDERROR, "lock collection space[%s] failed, rc: %d",
                   csName, rc );

      rc = _su->data()->getMBContext( &mbContext, _clShortName, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                   "rc: %d", _clShortName, rc ) ;

      mbID = mbContext->mbID() ;

      _su->data()->releaseMBContext( mbContext ) ;
      mbContext = NULL ;

      /// block collection write
      rc = _pFreezingWnd->registerCL( _clFullName, _blockID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Block all write operations of "
                 "collection[%s] failed, rc: %d",
                 _clFullName, rc ) ;
         goto error ;
      }
      else
      {
         PD_LOG( PDEVENT, "Begin to block all write operations "
                 "of collection[%s], ID: %llu", _clFullName,
                 _blockID ) ;
      }

      /// wait write done
      i = 0 ;
      while( TRUE )
      {
         ++i ;
         UINT32 cnt = 0 ;
         UINT32 transCnt = 0 ;
         dpsTransLockId lockID( _su->LogicalCSID(), DMS_INVALID_MBID, NULL ) ;
         cnt = eduMgr->getWritingEDUCount( -1, _blockID, EDU_BLOCK_FREEZING_WND,
                                          lockID, &transCnt ) ;
         if ( cnt > 0 )
         {
            if ( i < RTN_RENAME_BLOCKWRITE_TIMES )
            {
               ossSleep( OSS_ONE_SEC ) ;
               continue ;
            }
            else if ( cb->isInterrupted() )
            {
               rc = SDB_APP_INTERRUPT ;
            }
            else
            {
               rc = ( cnt == transCnt ) ?
                    SDB_DPS_TRANS_LOCK_INCOMPATIBLE : SDB_LOCK_FAILED ;
               PD_LOG_MSG( PDERROR, "Failed to wait for other write "
                           "operations to finish" ) ;
            }
         }

         break ;
      }

      if ( rc )
      {
         goto error ;
      }

      rc = _pDmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      _lockDms = TRUE ;

      if ( getDPSCB() )
      {
         dpsTransRetInfo lockConflict ;

         rc = _pTransCB->transLockTryS( cb, _su->LogicalCSID(), mbID,
                                        NULL, &lockConflict ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Get transaction-lock of collection[%s] failed, rc: %d"
                      OSS_NEWLINE
                      "Conflict( representative ):"OSS_NEWLINE
                      "   EDUID:  %llu"OSS_NEWLINE
                      "   TID:    %u"OSS_NEWLINE
                      "   LockId: %s"OSS_NEWLINE
                      "   Mode:   %s"OSS_NEWLINE,
                      _clFullName, rc,
                      lockConflict._eduID,
                      lockConflict._tid,
                      lockConflict._lockID.toString().c_str(),
                      lockModeToString( lockConflict._lockType ) ) ;

         _mbID = mbID ;
      }

   done:
      if ( _su && mbContext )
      {
         _su->data()->releaseMBContext( mbContext ) ;
         mbContext = NULL ;
      }
      PD_TRACE_EXITRC( SDB__RTNCTXRENAMECL__TRYLOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRENAMECL__RELLOCK, "_rtnContextRenameCL::_releaseLock" )
   INT32 _rtnContextRenameCL::_releaseLock( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__RTNCTXRENAMECL__RELLOCK ) ;

      if ( cb && _mbID != DMS_INVALID_MBID )
      {
         _pTransCB->transLockRelease( cb, _su->LogicalCSID(), _mbID ) ;
         _mbID = DMS_INVALID_MBID ;
      }

      if ( _pDmsCB && _su )
      {
         _pDmsCB->suUnlock( _su->CSID() ) ;
         _su = NULL ;
      }

      if ( 0 != _blockID )
      {
         _pFreezingWnd->unregisterCL( _clFullName, _blockID ) ;
         PD_LOG( PDEVENT, "End to block all write operations of "
                 "collection(%s), ID: %llu",
                 _clFullName, _blockID ) ;
         _blockID = 0 ;
      }

      if ( _lockDms )
      {
         _pDmsCB->writeDown( cb ) ;
         _lockDms = FALSE ;
      }

      PD_TRACE_EXIT( SDB__RTNCTXRENAMECL__RELLOCK ) ;
      return SDB_OK ;
   }

   RTN_CTX_AUTO_REGISTER(_rtnContextRenameMainCL, RTN_CONTEXT_RENAMEMAINCL, "RENAMEMAINCL")

   _rtnContextRenameMainCL::_rtnContextRenameMainCL( SINT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID )
   {
      _pDmsCB        = pmdGetKRCB()->getDMSCB() ;
      _pCatAgent     = pmdGetKRCB()->getClsCB()->getCatAgent() ;
      _lockDms       = FALSE ;
      _hitEnd        = FALSE ;
      ossMemset( _name, 0, DMS_COLLECTION_FULL_NAME_SZ + 1 );
   }

   _rtnContextRenameMainCL::~_rtnContextRenameMainCL()
   {
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      pmdEDUCB *cb = eduMgr->getEDUByID( eduID() ) ;

      /// context is killed by interrupted
      if ( _lockDms && cb->isInterrupted() )
      {
         SDB_RTNCB *pRtnCB = sdbGetRTNCB() ;

         /// clear main collection's catalog info
         _pCatAgent->lock_w () ;
         _pCatAgent->clear ( _name ) ;
         _pCatAgent->release_w () ;

         /// clear main collection's access plan
         pRtnCB->getAPM()->invalidateCLPlans( _name ) ;

         if ( sdbGetClsCB()->isPrimary() )
         {
            /// tell secondary nodes to clear catalog info and access plan
            sdbGetClsCB()->invalidateCache( _name,
                                            DPS_LOG_INVALIDCATA_TYPE_CATA |
                                            DPS_LOG_INVALIDCATA_TYPE_PLAN ) ;
         }
      }

      _clean( cb ) ;
   }

   void _rtnContextRenameMainCL::_clean( _pmdEDUCB *cb )
   {
      if ( _lockDms )
      {
         _pDmsCB->writeDown( cb ) ;
         _lockDms = FALSE ;
      }
   }

   const CHAR* _rtnContextRenameMainCL::name() const
   {
      return "RENAMEMAINCL" ;
   }

   RTN_CONTEXT_TYPE _rtnContextRenameMainCL::getType () const
   {
      return RTN_CONTEXT_RENAMEMAINCL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRENAMEMCL_OPEN, "_rtnContextRenameMainCL::open" )
   INT32 _rtnContextRenameMainCL::open( const CHAR *pCollectionName,
                                        _pmdEDUCB *cb,
                                        INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCTXRENAMEMCL_OPEN ) ;

      SDB_ASSERT( pCollectionName, "pCollectionName can't be null!" ) ;
      PD_CHECK( pCollectionName, SDB_INVALIDARG, error, PDERROR,
                "pCollectionName is null!" ) ;

      rc = dmsCheckFullCLName( pCollectionName ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid collection name[%s])",
                   pCollectionName ) ;

      // For renaming maincl, we only need to write a dps log to clear cata
      // info and access plan. This doesn't require setting ReplSize,
      // Because ReplSize is for data reliability.
      // _w = w ;

      // we need to check dms writable when invalidate cata/plan/statistics
      rc = _pDmsCB->writable ( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      _lockDms = TRUE ;

      ossStrcpy( _name, pCollectionName ) ;
      _isOpened = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCTXRENAMEMCL_OPEN, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRENAMEMCL_PREPAREDATA, "_rtnContextRenameMainCL::_prepareData" )
   INT32 _rtnContextRenameMainCL::_prepareData( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__RTNCTXRENAMEMCL_PREPAREDATA ) ;
      SDB_RTNCB *pRtnCB = sdbGetRTNCB() ;

      /// clear main collection's catalog info
      _pCatAgent->lock_w () ;
      _pCatAgent->clear ( _name ) ;
      _pCatAgent->release_w () ;

      /// clear main collection's access plan
      pRtnCB->getAPM()->invalidateCLPlans( _name ) ;

      /// tell secondary nodes to clear catalog info and access plan
      sdbGetClsCB()->invalidateCache( _name,
                                      DPS_LOG_INVALIDCATA_TYPE_CATA |
                                      DPS_LOG_INVALIDCATA_TYPE_PLAN ) ;

      _clean( cb ) ;
      PD_TRACE_EXIT( SDB__RTNCTXRENAMEMCL_PREPAREDATA ) ;
      return SDB_DMS_EOC ;
   }

   void _rtnContextRenameMainCL::_toString( stringstream &ss )
   {
      ss << ",Name:" << _name ;
   }
}

