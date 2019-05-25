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

   Source File Name = dmsCB.cpp

   Descriptive Name = Data Management Service Control Block

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   data management control block, which is the metatdata information for DMS
   component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsCB.hpp"
#include "dms.hpp"
#include "dmsStorageUnit.hpp"
#include "ossLatch.hpp"
#include "ossUtil.hpp"
#include "monDMS.hpp"
#include "dmsTempSUMgr.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "dpsOp2Record.hpp"
#include "rtn.hpp"
#include "ossLatch.hpp"
#include "rtnExtDataHandler.hpp"

#include <list>

using namespace std;
namespace engine
{

   enum DMS_LOCK_LEVEL
   {
      DMS_LOCK_NONE     = 0,
      DMS_LOCK_WRITE    = 1,     // for writable
      DMS_LOCK_WHOLE    = 2      // for backup or reorg
   } ;

   /*
      _SDB_DMS_CSCB implement
   */
   _SDB_DMS_CSCB::~_SDB_DMS_CSCB()
   {
      if ( _su )
      {
         SDB_OSS_DEL _su ;
      }
   }

   #define DMS_CS_MUTEX_BUCKET_SIZE          ( 128 )

   /*
      _SDB_DMSCB implement
   */

   _SDB_DMSCB::_SDB_DMSCB()
   :_writeCounter(0),
    _dmsCBState(DMS_STATE_NORMAL),
    _logicalSUID(0),
    _tempSUMgr( this ),
    _statSUMgr( this ),
    _ixmKeySorterCreator( NULL )
   {
      for ( UINT32 i = 0 ; i< DMS_MAX_CS_NUM ; ++i )
      {
         _cscbVec.push_back ( NULL ) ;
         _delCscbVec.push_back ( NULL ) ;
         _latchVec.push_back ( new(std::nothrow) ossRWMutex() ) ;
         _freeList.push_back ( i ) ;
      }

      for ( UINT32 i = 0 ; i < DMS_CS_MUTEX_BUCKET_SIZE ; ++i )
      {
         _vecCSMutex.push_back( new( std::nothrow ) ossSpinRecursiveXLatch() ) ;
      }

      _blockEvent.signal() ;
   }

   _SDB_DMSCB::~_SDB_DMSCB()
   {
   }

   INT32 _SDB_DMSCB::init ()
   {
      INT32 rc = SDB_OK ;

      if ( pmdGetKRCB()->isRestore() )
      {
         goto done ;
      }

      if ( SDB_ROLE_COORD != pmdGetDBRole() )
      {
         rc = rtnLoadCollectionSpaces ( pmdGetOptionCB()->getDbPath(),
                                        pmdGetOptionCB()->getIndexPath(),
                                        pmdGetOptionCB()->getLobPath(),
                                        pmdGetOptionCB()->getLobMetaPath(),
                                        this ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to load collectionspaces, rc: %d",
                      rc ) ;
      }

      rc = _tempSUMgr.init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init temp cb, rc: %d", rc ) ;

      if ( SDB_ROLE_DATA == pmdGetDBRole() ||
           SDB_ROLE_STANDALONE == pmdGetDBRole() )
      {
         rc = _statSUMgr.init() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to init stat cb, rc: %d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _SDB_DMSCB::active ()
   {
      INT32 rc = SDB_OK ;

      rc = _pageMapDispatcher.active() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Active page map dispatcher failed, rc: %d",
                 rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _SDB_DMSCB::deactive ()
   {
      return SDB_OK ;
   }

   INT32 _SDB_DMSCB::fini ()
   {
      _CSCBNameMapCleanup() ;

      for ( UINT32 i = 0 ; i < DMS_MAX_CS_NUM ; ++i )
      {
         if ( _latchVec[i] )
         {
            SDB_OSS_DEL _latchVec[i] ;
            _latchVec[i] = NULL ;
         }
      }
      for ( UINT32 i = 0 ; i < DMS_CS_MUTEX_BUCKET_SIZE ; ++i )
      {
         SDB_OSS_DEL _vecCSMutex[ i ] ;
         _vecCSMutex[ i ] = NULL ;
      }
      return SDB_OK ;
   }

   void _SDB_DMSCB::onConfigChange()
   {
      pmdOptionsCB *optCB = pmdGetKRCB()->getOptionCB() ;
      UINT32       syncInterval = optCB->getSyncInterval() ;
      UINT32       syncRecordNum = optCB->getSyncRecordNum() ;
      UINT32       syncDirtyRatio = optCB->getSyncDirtyRatio() ;
      BOOLEAN      syncDeep = optCB->isSyncDeep() ;

      ossScopedLock _lock( &_mutex, SHARED ) ;

      for ( vector<SDB_DMS_CSCB*>::iterator itr = _cscbVec.begin();
            itr != _cscbVec.end(); ++itr )
      {
         if ( NULL != (*itr) )
         {
            _dmsStorageUnit *su = (*itr)->_su ;
            su->setSyncConfig( syncInterval, syncRecordNum, syncDirtyRatio ) ;
            su->setSyncDeep( syncDeep ) ;

            dmsStorageInfo *pInfo = su->storageInfo() ;
            utilCacheUnit *pCache = su->cacheUnit() ;

            pInfo->_overflowRatio = optCB->getOverFlowRatio() ;
            pInfo->_extentThreshold = optCB->getExtendThreshold() << 20 ;
            pInfo->_enableSparse = optCB->sparseFile() ;
            pInfo->_cacheMergeSize = optCB->getCacheMergeSize() ;
            pInfo->_pageAllocTimeout = optCB->getPageAllocTimeout() ;

            pCache->setAllocTimeout( pInfo->_pageAllocTimeout ) ;
            pCache->updateMerge( pInfo->_directIO, pInfo->_cacheMergeSize ) ;

            su->lob()->getLobData()->enableSparse( pInfo->_enableSparse ) ;
         }
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__LGCSCBNMMAP, "_SDB_DMSCB::_logCSCBNameMap" )
   void _SDB_DMSCB::_logCSCBNameMap ()
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB__LGCSCBNMMAP );

      CSCB_MAP_CONST_ITER it ;

      for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end() ;
            it ++ )
      {
         PD_LOG ( PDDEBUG, "%s\n", it->first ) ;
      }
      PD_TRACE_EXIT ( SDB__SDB_DMSCB__LGCSCBNMMAP );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBNMINST, "_SDB_DMSCB::_CSCBNameInsert" )
   INT32 _SDB_DMSCB::_CSCBNameInsert ( const CHAR *pName,
                                       UINT32 topSequence,
                                       _dmsStorageUnit *su,
                                       dmsStorageUnitID &suID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB__CSCBNMINST );
      SDB_DMS_CSCB *cscb = NULL ;
      if ( 0 == _freeList.size() )
      {
         rc = SDB_DMS_SU_OUTRANGE ;
         goto error ;
      }
      cscb = SDB_OSS_NEW SDB_DMS_CSCB(pName, topSequence, su) ;
      if ( !cscb )
      {
         PD_LOG ( PDERROR, "Failed to allocate memory to insert cscb" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      suID = _freeList.back() ;
      su->_setCSID( suID ) ;
      su->_setLogicalCSID( _logicalSUID++ ) ;
      _freeList.pop_back() ;
      _cscbNameMap[cscb->_name] = suID ;
      _cscbVec[suID] = cscb ;

   done :
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB__CSCBNMINST, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _SDB_DMSCB::_CSCBNameLookup ( const CHAR *pName,
                                       SDB_DMS_CSCB **cscb,
                                       dmsStorageUnitID *suID,
                                       BOOLEAN exceptDeleting )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( cscb, "cscb can't be null!" ) ;

      CSCB_MAP_CONST_ITER it ;

      if ( _cscbNameMap.end() == (it = _cscbNameMap.find(pName)) )
      {
         rc = SDB_DMS_CS_NOTEXIST ;
         goto error;
      }
      if ( suID )
      {
         *suID = it->second ;
      }

      if ( _cscbVec[(*it).second] )
      {
         *cscb = _cscbVec[(*it).second] ;
      }
      else if ( _delCscbVec[(*it).second] )
      {
         if ( !exceptDeleting )
         {
            *cscb = _delCscbVec[(*it).second] ;
         }
         else
         {
            rc = SDB_DMS_CS_DELETING ;
            goto error ;
         }
      }
      else
      {
         SDB_ASSERT( FALSE, "This is impossible in this case" ) ;
         rc = SDB_DMS_CS_NOTEXIST ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _SDB_DMSCB::_CSCBNameLookupAndLock ( const CHAR *pName,
                                              dmsStorageUnitID &suID,
                                              SDB_DMS_CSCB **cscb,
                                              OSS_LATCH_MODE lockType,
                                              INT32 millisec )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( cscb, "cscb can't be null!" );

      rc = _CSCBNameLookup( pName, cscb, &suID, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( EXCLUSIVE == lockType )
      {
         rc = _latchVec[suID]->lock_w( millisec ) ;
      }
      else
      {
         rc = _latchVec[suID]->lock_r( millisec ) ;
      }
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      suID = DMS_INVALID_CS ;
      *cscb = NULL ;
      goto done ;
   }

   void _SDB_DMSCB::_CSCBRelease ( dmsStorageUnitID suID,
                                   OSS_LATCH_MODE lockType )
   {
      if ( EXCLUSIVE == lockType )
      {
         _latchVec[suID]->release_w();
      }
      else
      {
         _latchVec[suID]->release_r() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBNMREMV, "_SDB_DMSCB::_CSCBNameRemove" )
   INT32 _SDB_DMSCB::_CSCBNameRemove ( const CHAR *pName,
                                       _pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB,
                                       BOOLEAN onlyEmpty,
                                       SDB_DMS_CSCB *&pCSCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB__CSCBNMREMV );
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      UINT32 csLID = ~0 ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB();
      BOOLEAN isTransLocked = FALSE;
      BOOLEAN isReserved = FALSE;
      BOOLEAN isLocked = FALSE ;
      UINT32 logRecSize = 0;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;

      _mutex.get_shared () ;
      rc = _CSCBNameLookup( pName, &pCSCB, &suID, TRUE ) ;
      _mutex.release_shared () ;

      if ( rc )
      {
         goto error ;
      }

      if ( NULL != dpsCB )
      {
         rc = dpsCSDel2Record( pName, record ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build record:%d",rc ) ;
            goto error ;
         }
         rc = dpsCB->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = pTransCB->reservedLogSpace( logRecSize, cb );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to reserved log space(length=%u)",
                      logRecSize ) ;
         isReserved = TRUE ;
      }

   retry :
      if ( SDB_OK != _latchVec[suID]->lock_w( OSS_ONE_SEC ) )
      {
         rc = SDB_LOCK_FAILED ;
         goto error ;
      }
      if ( !_mutex.try_get() )
      {
         _latchVec[suID]->release_w () ;
         ossSleep( 50 ) ;
         goto retry ;
      }
      isLocked = TRUE ;
      _latchVec[suID]->release_w () ;

      {
         dmsStorageUnitID suTmpID = DMS_INVALID_SUID ;
         SDB_DMS_CSCB *tmpCSCB = NULL ;
         rc = _CSCBNameLookup( pName, &tmpCSCB, &suTmpID, TRUE ) ;
         if ( rc )
         {
            goto error ;
         }
         else if ( suTmpID != suID )
         {
            rc = SDB_DMS_CS_NOTEXIST ;
            goto error ;
         }
      }

      SDB_ASSERT ( pCSCB->_su, "su can't be null" ) ;
      if ( onlyEmpty && 0 != pCSCB->_su->data()->getCollectionNum() )
      {
         rc = SDB_DMS_CS_NOT_EMPTY ;
         goto error ;
      }
      csLID = pCSCB->_su->LogicalCSID() ;
      if ( cb )
      {
         rc = pTransCB->transLockTryX( cb, csLID ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to lock collection-space, rc:%d",
                     rc ) ;
            goto error ;
         }
         isTransLocked = TRUE ;
      }

      _cscbVec[ suID ] = NULL ;
      _cscbNameMap.erase( pName ) ;
      _freeList.push_back ( suID ) ;

      if ( dpsCB )
      {
         info.setInfoEx( csLID, ~0, DMS_INVALID_EXTENT, cb ) ;
         rc = dpsCB->prepare ( info ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to insert cscrt into log, rc = %d", rc ) ;
            goto error ;
         }
         _mutex.release() ;
         isLocked = FALSE ;

         dpsCB->writeData( info ) ;
      }

   done :
      if ( isLocked )
      {
         _mutex.release () ;
      }
      if ( isTransLocked )
      {
         pTransCB->transLockRelease( cb, csLID );
      }
      if ( isReserved )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB__CSCBNMREMV, rc );
      return rc ;
   error :
      pCSCB = NULL ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBRENAME, "_SDB_DMSCB::_CSCBRename" )
   INT32 _SDB_DMSCB::_CSCBRename( const CHAR *pName,
                                  const CHAR *pNewName,
                                  _pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB__CSCBRENAME );
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      UINT32 csLID = ~0 ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB();
      BOOLEAN isReserved = FALSE;
      BOOLEAN isLocked = FALSE, isLatchLocked = FALSE ;
      UINT32 logRecSize = 0;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;
      SDB_DMS_CSCB *pCSCB = NULL ;

      _mutex.get_shared () ;
      rc = _CSCBNameLookup( pName, &pCSCB, &suID, TRUE ) ;
      _mutex.release_shared () ;

      if ( rc )
      {
         goto error ;
      }

      if ( NULL != dpsCB )
      {
         rc = dpsCSRename2Record( pName, pNewName, record ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build record:%d",rc ) ;
            goto error ;
         }
         rc = dpsCB->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = pTransCB->reservedLogSpace( logRecSize, cb );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to reserved log space(length=%u)",
                      logRecSize ) ;
         isReserved = TRUE ;
      }

   retry :
      if ( SDB_OK != _latchVec[suID]->lock_w( OSS_ONE_SEC ) )
      {
         rc = SDB_LOCK_FAILED ;
         goto error ;
      }
      isLatchLocked = TRUE ;
      if ( !_mutex.try_get() )
      {
         _latchVec[suID]->release_w () ;
         isLatchLocked = FALSE ;
         ossSleep( 50 ) ;
         goto retry ;
      }
      isLocked = TRUE ;

      {
         dmsStorageUnitID suTmpID = DMS_INVALID_SUID ;
         SDB_DMS_CSCB *tmpCSCB = NULL ;
         rc = _CSCBNameLookup( pName, &tmpCSCB, &suTmpID, TRUE ) ;
         if ( rc )
         {
            goto error ;
         }
         else if ( suTmpID != suID )
         {
            rc = SDB_DMS_CS_NOTEXIST ;
            goto error ;
         }

         rc = _CSCBNameLookup( pNewName, &tmpCSCB, NULL, TRUE ) ;
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            rc = SDB_OK ;
         }
         else if ( SDB_OK == rc )
         {
            rc = SDB_DMS_CS_EXIST ;
            goto error ;
         }
         else
         {
            goto error ;
         }
      }

      SDB_ASSERT ( pCSCB->_su, "su can't be null" ) ;
      csLID = pCSCB->_su->LogicalCSID() ;

      rc = pCSCB->_su->renameCS( pNewName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rename collection space[%s] to [%s] failed, "
                 "rc: %d", pName, pNewName, rc ) ;
         goto error ;
      }

      _cscbNameMap.erase( pName ) ;
      ossStrncpy( pCSCB->_name, pNewName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
      pCSCB->_name[ DMS_COLLECTION_SPACE_NAME_SZ ] = 0 ;
      _cscbNameMap[ pCSCB->_name ] = suID ;

      if ( dpsCB )
      {
         info.setInfoEx( csLID, ~0, DMS_INVALID_EXTENT, cb ) ;
         rc = dpsCB->prepare ( info ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to insert cscrt into log, rc = %d", rc ) ;
            goto error ;
         }
         _mutex.release() ;
         isLocked = FALSE ;

         dpsCB->writeData( info ) ;
      }

      if ( isLocked )
      {
         _mutex.release () ;
         isLocked = FALSE ;
      }
      pCSCB->_su->getEventHolder()->onRenameCS( DMS_EVENT_MASK_ALL, pName,
                                                pNewName, cb, dpsCB ) ;

   done :
      if ( isLocked )
      {
         _mutex.release () ;
      }
      if ( isLatchLocked )
      {
         _latchVec[suID]->release_w() ;
      }
      if ( isReserved )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB__CSCBRENAME, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBNMREMVP1, "_SDB_DMSCB::_CSCBNameRemoveP1" )
   INT32 _SDB_DMSCB::_CSCBNameRemoveP1 ( const CHAR *pName,
                                         _pmdEDUCB *cb,
                                         SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB__CSCBNMREMVP1 );
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      SDB_DMS_CSCB *pCSCB = NULL ;
      BOOLEAN metaLock = FALSE ;

      _mutex.get_shared () ;
      rc = _CSCBNameLookup( pName, &pCSCB, &suID, TRUE ) ;
      _mutex.release_shared () ;

      if ( rc )
      {
         goto error ;
      }

   retry :
      if ( SDB_OK != _latchVec[suID]->lock_w( OSS_ONE_SEC ) )
      {
         rc = SDB_LOCK_FAILED ;
         goto error ;
      }
      if ( !_mutex.try_get() )
      {
         _latchVec[suID]->release_w () ;
         ossSleep( 50 ) ;
         goto retry ;
      }

      metaLock = TRUE ;
      _latchVec[suID]->release_w () ;

      {
         dmsStorageUnitID suTmpID = DMS_INVALID_SUID ;
         SDB_DMS_CSCB *tmpCSCB = NULL ;
         rc = _CSCBNameLookup( pName, &tmpCSCB, &suTmpID, TRUE ) ;
         if ( rc )
         {
            goto error ;
         }
         else if ( suTmpID != suID )
         {
            rc = SDB_DMS_CS_NOTEXIST ;
            goto error ;
         }
      }

      SDB_ASSERT ( pCSCB->_name, "cs-name can't be null" ) ;
      _delCscbVec[ suID ] = pCSCB ;
      _cscbVec[suID] = NULL ;

      _mutex.release () ;
      metaLock = FALSE ;

   done :
      if ( metaLock )
      {
         _mutex.release() ;
         metaLock = FALSE ;
      }
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB__CSCBNMREMVP1, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBNMREMVP1CANCEL, "_SDB_DMSCB::_CSCBNameRemoveP1Cancel" )
   INT32 _SDB_DMSCB::_CSCBNameRemoveP1Cancel ( const CHAR *pName,
                                               _pmdEDUCB *cb,
                                               SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB__CSCBNMREMVP1CANCEL );
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      SDB_DMS_CSCB *pCSCB = NULL ;

      ossScopedLock _lock( &_mutex, EXCLUSIVE ) ;
      rc = _CSCBNameLookup( pName, &pCSCB, &suID, FALSE ) ;
      if ( rc )
      {
         SDB_ASSERT( FALSE, "Impossible in the case" ) ;
         goto error ;
      }

      if ( _delCscbVec[suID] != pCSCB )
      {
         SDB_ASSERT( FALSE, "Impossible in this case" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else
      {
         _delCscbVec[ suID ] = NULL ;
         _cscbVec[ suID ] = pCSCB ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB__CSCBNMREMVP1CANCEL, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBNMREMVP2, "_SDB_DMSCB::_CSCBNameRemoveP2" )
   INT32 _SDB_DMSCB::_CSCBNameRemoveP2 ( const CHAR *pName,
                                         _pmdEDUCB *cb,
                                         SDB_DPSCB *dpsCB,
                                         SDB_DMS_CSCB *&pCSCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB__CSCBNMREMVP2 );
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      UINT32 csLID = ~0 ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      BOOLEAN isTransLocked = FALSE ;
      BOOLEAN isReserved = FALSE ;
      BOOLEAN isLocked = FALSE ;
      UINT32 logRecSize = 0 ;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;

      if ( NULL != dpsCB )
      {
         rc = dpsCSDel2Record( pName, record ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build record:%d",rc ) ;
            goto error ;
         }
         rc = dpsCB->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = pTransCB->reservedLogSpace( logRecSize, cb );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to reserved log space(length=%u)",
                      logRecSize ) ;
         isReserved = TRUE ;
      }

      _mutex.get() ;
      isLocked = TRUE ;

      rc = _CSCBNameLookup( pName, &pCSCB, &suID, FALSE ) ;
      if ( rc )
      {
         SDB_ASSERT( FALSE, "Impossible in this case" ) ;
         goto error ;
      }
      if ( pCSCB != _delCscbVec[suID] )
      {
         SDB_ASSERT( FALSE, "Impossible in this case" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      SDB_ASSERT ( pCSCB->_su, "su can't be null" ) ;
      csLID = pCSCB->_su->LogicalCSID() ;
      if ( cb )
      {
         rc = pTransCB->transLockTryX( cb, csLID ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to lock collection-space, rc:%d",
                     rc ) ;
            goto error ;
         }
         isTransLocked = TRUE ;
      }
      _delCscbVec[ suID ] = NULL ;
      _cscbNameMap.erase( pName ) ;
      _freeList.push_back ( suID ) ;

      if ( dpsCB )
      {
         info.setInfoEx( csLID, ~0, DMS_INVALID_EXTENT, cb ) ;
         rc = dpsCB->prepare ( info ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to insert cscrt into log, rc = %d",
                     rc ) ;
            goto error ;
         }
         _mutex.release() ;
         isLocked = FALSE ;

         dpsCB->writeData( info ) ;
      }

   done :
      if ( isLocked )
      {
         _mutex.release () ;
      }
      if ( isTransLocked )
      {
         pTransCB->transLockRelease( cb, csLID );
      }
      if ( isReserved )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB__CSCBNMREMVP2, rc );
      return rc ;
   error :
      pCSCB = NULL ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBNMMAPCLN, "_SDB_DMSCB::_CSCBNameMapCleanup" )
   void _SDB_DMSCB::_CSCBNameMapCleanup ()
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB__CSCBNMMAPCLN );

      CSCB_MAP_CONST_ITER it ;

      for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      {
         dmsStorageUnitID suID = (*it).second ;

         _freeList.push_back ( suID ) ;
         if ( _cscbVec[suID] )
         {
            SDB_OSS_DEL _cscbVec[suID] ;
            _cscbVec[suID] = NULL ;
         }
         if ( _delCscbVec[suID] )
         {
            SDB_OSS_DEL _delCscbVec[suID] ;
            _delCscbVec[suID] = NULL ;
         }
      }
      _cscbNameMap.clear() ;
      PD_TRACE_EXIT ( SDB__SDB_DMSCB__CSCBNMMAPCLN );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_WRITABLE, "_SDB_DMSCB::writable" )
   INT32 _SDB_DMSCB::writable( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_WRITABLE );

      BOOLEAN locked = FALSE ;

      if ( cb && cb->getLockItem(SDB_LOCK_DMS)->getMode() >= DMS_LOCK_WRITE )
      {
         cb->getLockItem(SDB_LOCK_DMS)->incCount() ;
         _stateMtx.get () ;
         ++_writeCounter ;
         _stateMtx.release() ;
         goto done ;
      }

   retry:
      _stateMtx.get () ;
      locked = TRUE ;

      switch ( _dmsCBState )
      {
      case DMS_STATE_READONLY :
         {
            if ( SDB_DB_OFFLINE_BK == PMD_DB_STATUS() )
            {
               rc = SDB_RTN_IN_BACKUP ;
            }
            else if ( SDB_DB_REBUILDING == PMD_DB_STATUS() )
            {
               rc = SDB_RTN_IN_REBUILD ;
               goto done ;
            }
            else
            {
               rc = SDB_DMS_STATE_NOT_COMPATIBLE ;
            }
         }
         break ;
      default :
         break ;
      }
      if ( SDB_OK == rc )
      {
         ++_writeCounter ;
         if ( cb )
         {
            cb->getLockItem(SDB_LOCK_DMS)->setMode( DMS_LOCK_WRITE ) ;
            cb->getLockItem(SDB_LOCK_DMS)->incCount() ;
         }
      }
      else if ( cb )
      {
         _stateMtx.release() ;
         locked = FALSE ;

         while ( TRUE )
         {
            if ( cb->isInterrupted() )
            {
               rc = SDB_APP_INTERRUPT ;
               break ;
            }
            rc = _blockEvent.wait( OSS_ONE_SEC ) ;
            if ( SDB_OK == rc )
            {
               goto retry ;
            }
         }
      }

   done:
      if ( locked )
      {
         _stateMtx.release() ;
      }
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB_WRITABLE, rc );
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_WRITEDOWN, "_SDB_DMSCB::writeDown" )
   void _SDB_DMSCB::writeDown( _pmdEDUCB * cb )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_WRITEDOWN );
      _stateMtx.get();
      --_writeCounter;
      SDB_ASSERT( 0 <= _writeCounter, "write counter should not < 0" ) ;
      _stateMtx.release();

      if ( cb && cb->getLockItem(SDB_LOCK_DMS)->getMode() >= DMS_LOCK_WRITE )
      {
         SDB_ASSERT( cb->getLockItem(SDB_LOCK_DMS)->lockCount() > 0,
                     "Dms lock count error" ) ;
         UINT32 count = cb->getLockItem(SDB_LOCK_DMS)->decCount() ;

         if ( 0 == count &&
              DMS_LOCK_WRITE == cb->getLockItem(SDB_LOCK_DMS)->getMode() )
         {
            cb->getLockItem(SDB_LOCK_DMS)->setMode(DMS_LOCK_NONE) ;
         }
      }
      PD_TRACE_EXIT ( SDB__SDB_DMSCB_WRITEDOWN );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_BLOCKWRITE, "_SDB_DMSCB::blockWrite" )
   INT32 _SDB_DMSCB::blockWrite( _pmdEDUCB *cb, SDB_DB_STATUS byStatus )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_BLOCKWRITE );

      _stateMtx.get();
      if ( DMS_STATE_NORMAL != _dmsCBState )
      {
         if ( SDB_DB_OFFLINE_BK == byStatus &&
              SDB_DB_OFFLINE_BK == PMD_DB_STATUS() )
         {
            rc = SDB_BACKUP_HAS_ALREADY_START ;
         }
         else if ( SDB_DB_REBUILDING == byStatus &&
                   SDB_DB_REBUILDING == PMD_DB_STATUS() )
         {
            rc = SDB_REBUILD_HAS_ALREADY_START ;
         }
         else
         {
            rc = SDB_DMS_STATE_NOT_COMPATIBLE ;
         }
         _stateMtx.release () ;
         goto done;
      }
      _dmsCBState = DMS_STATE_READONLY ;
      PMD_SET_DB_STATUS( byStatus ) ;
      _stateMtx.release () ;

      while ( TRUE )
      {
         if ( cb->isInterrupted() )
         {
            _dmsCBState = DMS_STATE_NORMAL ;
            PMD_SET_DB_STATUS( SDB_DB_NORMAL ) ;
            _blockEvent.signal() ;
            rc = SDB_APP_INTERRUPT ;
            break ;
         }
         _stateMtx.get();
         if ( 0 == _writeCounter )
         {
            _blockEvent.reset() ;
            _stateMtx.release();
            if ( cb )
            {
               cb->getLockItem(SDB_LOCK_DMS)->setMode( DMS_LOCK_WHOLE ) ;
            }
            goto done;
         }
         else
         {
            _stateMtx.release();
            ossSleepmillis( DMS_CHANGESTATE_WAIT_LOOP );
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB_BLOCKWRITE, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_UNBLOCKWRITE, "_SDB_DMSCB::unblockWrite" )
   void _SDB_DMSCB::unblockWrite( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_UNBLOCKWRITE );
      _stateMtx.get() ;
      _dmsCBState = DMS_STATE_NORMAL ;
      PMD_SET_DB_STATUS( SDB_DB_NORMAL ) ;
      _blockEvent.signalAll() ;
      _stateMtx.release() ;
      if ( cb )
      {
         cb->getLockItem(SDB_LOCK_DMS)->setMode( DMS_LOCK_NONE ) ;
      }
      PD_TRACE_EXIT ( SDB__SDB_DMSCB_UNBLOCKWRITE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_REGFULLSYNC, "_SDB_DMSCB::registerFullSync" )
   INT32 _SDB_DMSCB::registerFullSync( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_REGFULLSYNC );

   retry:
      _stateMtx.get() ;

      if ( DMS_STATE_NORMAL != _dmsCBState )
      {
         _stateMtx.release() ;

         rc = SDB_DMS_STATE_NOT_COMPATIBLE ;
         while ( cb )
         {
            if ( cb->isInterrupted() )
            {
               rc = SDB_APP_INTERRUPT ;
               break ;
            }
            rc = _blockEvent.wait( OSS_ONE_SEC ) ;
            if ( SDB_OK == rc )
            {
               goto retry ;
            }
         }
      }
      else
      {
         _dmsCBState = DMS_STATE_FULLSYNC ;
         PMD_SET_DB_STATUS( SDB_DB_FULLSYNC ) ;

         _stateMtx.release() ;
      }

      PD_TRACE_EXITRC ( SDB__SDB_DMSCB_REGFULLSYNC, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_FULLSYNCDOWN, "_SDB_DMSCB::fullSyncDown" )
   void _SDB_DMSCB::fullSyncDown( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_FULLSYNCDOWN );
      ossScopedLock lock( &_stateMtx ) ;
      _dmsCBState = DMS_STATE_NORMAL ;
      PMD_SET_DB_STATUS( SDB_DB_NORMAL ) ;
      PD_TRACE_EXIT ( SDB__SDB_DMSCB_FULLSYNCDOWN );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_REGBACKUP, "_SDB_DMSCB::registerBackup" )
   INT32 _SDB_DMSCB::registerBackup( _pmdEDUCB *cb, BOOLEAN offline )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_REGBACKUP );

      if ( offline )
      {
         rc = blockWrite( cb, SDB_DB_OFFLINE_BK ) ;
      }
      else
      {
         _stateMtx.get() ;
         if ( DMS_STATE_NORMAL != _dmsCBState )
         {
            if ( SDB_DB_OFFLINE_BK == PMD_DB_STATUS() ||
                 DMS_STATE_ONLINE_BACKUP == _dmsCBState )
            {
               rc = SDB_BACKUP_HAS_ALREADY_START ;
            }
            else
            {
               rc = SDB_DMS_STATE_NOT_COMPATIBLE ;
            }
         }
         else
         {
            _dmsCBState = DMS_STATE_ONLINE_BACKUP ;
         }
         _stateMtx.release () ;
      }

      PD_TRACE_EXITRC ( SDB__SDB_DMSCB_REGBACKUP, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_BACKUPDOWN, "_SDB_DMSCB::backupDown" )
   void _SDB_DMSCB::backupDown( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_BACKUPDOWN );
      if ( DMS_LOCK_WHOLE == cb->getLockItem(SDB_LOCK_DMS)->getMode() )
      {
         unblockWrite( cb ) ;
      }
      else
      {
         _stateMtx.get() ;
         _dmsCBState = DMS_STATE_NORMAL ;
         _stateMtx.release() ;
      }
      PD_TRACE_EXIT ( SDB__SDB_DMSCB_BACKUPDOWN );
   }

   INT32 _SDB_DMSCB::registerRebuild( _pmdEDUCB *cb )
   {
      return blockWrite( cb, SDB_DB_REBUILDING ) ;
   }

   void _SDB_DMSCB::rebuildDown( _pmdEDUCB *cb )
   {
      unblockWrite( cb ) ;
   }

   INT32 _SDB_DMSCB::nameToSUAndLock ( const CHAR *pName,
                                       dmsStorageUnitID &suID,
                                       _dmsStorageUnit **su,
                                       OSS_LATCH_MODE lockType,
                                       INT32 millisec )
   {
      INT32 rc = SDB_OK;

      SDB_DMS_CSCB *cscb = NULL;
      SDB_ASSERT( su, "su can't be null!" );
      if ( !pName )
      {
         return SDB_INVALIDARG ;
      }
      ossScopedLock _lock(&_mutex, SHARED) ;
      rc = _CSCBNameLookupAndLock( pName, suID,
                                   &cscb, lockType,
                                   millisec ) ;
      if ( SDB_OK == rc )
      {
         *su = cscb->_su ;
      }
      return rc ;
   }

   INT32 _SDB_DMSCB::verifySUAndLock ( const dmsEventSUItem *pSUItem,
                                       _dmsStorageUnit **ppSU,
                                       OSS_LATCH_MODE lockType,
                                       INT32 millisec )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( pSUItem, "pSUItem is invalid" ) ;
      SDB_ASSERT( ppSU, "ppSU is invalid" ) ;

      const CHAR *pCSName = pSUItem->_pCSName ;
      dmsStorageUnitID origSUID = pSUItem->_suID ;
      UINT32 origSULID = pSUItem->_suLID ;

      dmsStorageUnit *pSU = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      UINT32 suLID = DMS_INVALID_LOGICCSID ;

      rc = nameToSUAndLock( pCSName, suID, &pSU, lockType, millisec ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get collection space [%s], rc: %d",
                   pCSName, rc ) ;

      suLID = pSU->LogicalCSID() ;

      PD_CHECK( suID == origSUID && suLID == origSULID,
                SDB_DMS_CS_NOTEXIST, error, PDWARNING,
                "Collection space [%s] had been updated, "
                "original [ ID: %d, LID: %u ], new [ ID: %d, LID: %u ]",
                pCSName, origSUID, origSULID, suID, suLID ) ;

  done :
     (*ppSU) = pSU ;
     return rc ;

  error :
     if ( DMS_INVALID_SUID != suID )
     {
        suUnlock( suID, lockType ) ;
     }
     pSU = NULL ;
     goto done ;
   }

   _dmsStorageUnit *_SDB_DMSCB::suLock ( dmsStorageUnitID suID )
   {
      ossScopedLock _lock(&_mutex, SHARED) ;
      if ( NULL == _cscbVec[suID] )
      {
         return NULL ;
      }
      _latchVec[suID]->lock_r() ;
      return _cscbVec[suID]->_su ;
   }

   void _SDB_DMSCB::suUnlock ( dmsStorageUnitID suID,
                               OSS_LATCH_MODE lockType )
   {
      _CSCBRelease( suID, lockType ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_ADDCS, "_SDB_DMSCB::addCollectionSpace" )
   INT32 _SDB_DMSCB::addCollectionSpace( const CHAR * pName,
                                         UINT32 topSequence,
                                         _dmsStorageUnit * su,
                                         _pmdEDUCB *cb,
                                         SDB_DPSCB *dpsCB,
                                         BOOLEAN isCreate )
   {
      INT32 rc = SDB_OK ;
      dmsStorageUnitID suID ;
      SDB_DMS_CSCB *cscb = NULL ;
      BOOLEAN isReserved = FALSE;
      BOOLEAN isLocked = FALSE ;
      UINT32 logRecSize = 0;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record();
      INT32 pageSize = 0 ;
      INT32 lobPageSz = 0 ;
      INT32 type = 0 ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB();
      _SDB_RTNCB *pRtnCB = pmdGetKRCB()->getRTNCB() ;

      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_ADDCS );

      if ( !pName || !su )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      pageSize = su->getPageSize() ;
      lobPageSz = su->getLobPageSize() ;
      type = su->type() ;

      if ( NULL != dpsCB )
      {
         rc = dpsCSCrt2Record( pName, pageSize, lobPageSz, type, record ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to build record:%d", rc ) ;
            goto error ;
         }
         rc = dpsCB->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = pTransCB->reservedLogSpace( logRecSize, cb );
         PD_RC_CHECK( rc, PDERROR,
                     "failed to reserved log space(length=%u)",
                     logRecSize );
         isReserved = TRUE ;
      }

      _mutex.get() ;
      isLocked = TRUE ;

      rc = _CSCBNameLookup( pName, &cscb ) ;
      if ( SDB_OK == rc )
      {
         rc = SDB_DMS_CS_EXIST;
         goto error;
      }
      else if ( rc != SDB_DMS_CS_NOTEXIST )
      {
         goto error;
      }

      rc = _CSCBNameInsert ( pName, topSequence, su, suID ) ;
      if ( SDB_OK == rc && dpsCB )
      {
         UINT32 suLID = su->LogicalCSID() ;
         info.setInfoEx( suLID, ~0, DMS_INVALID_EXTENT, cb ) ;
         rc = dpsCB->prepare ( info ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to insert cscrt into log, rc = %d", rc ) ;
            goto error ;
         }
         _mutex.release() ;
         isLocked = FALSE ;
         dpsCB->writeData( info ) ;
      }

      su->regEventHandler( &_statSUMgr ) ;
      su->regEventHandler( pRtnCB->getAPM() ) ;

      if ( isLocked )
      {
         _mutex.release() ;
         isLocked = FALSE ;
      }
      if ( isCreate )
      {
         su->getEventHolder()->onCreateCS( DMS_EVENT_MASK_ALL, cb, dpsCB ) ;
      }
      else
      {
         su->getEventHolder()->onLoadCS( DMS_EVENT_MASK_ALL, cb, dpsCB ) ;
      }

   done :
      if ( isLocked )
      {
         _mutex.release() ;
      }
      if ( isReserved )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB_ADDCS, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DELCS, "_SDB_DMSCB::_delCollectionSpace" )
   INT32 _SDB_DMSCB::_delCollectionSpace( const CHAR * pName, _pmdEDUCB * cb,
                                          SDB_DPSCB * dpsCB,
                                          BOOLEAN removeFile,
                                          BOOLEAN onlyEmpty )
   {
      INT32 rc = SDB_OK ;
      SDB_DMS_CSCB *pCSCB = NULL ;
      IDmsExtDataHandler *extHandler = NULL ;
      BOOLEAN extOprFinish = FALSE ;

      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_DELCS ) ;

      if ( !pName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _mutex.get_shared () ;
      rc = _CSCBNameLookup( pName, &pCSCB ) ;
      _mutex.release_shared () ;
      if ( rc )
      {
         goto error ;
      }

      extHandler = pCSCB->_su->data()->getExtDataHandler() ;
      if ( extHandler )
      {
         rc = extHandler->onDelCS( pCSCB->_su->CSName(), cb,
                                   removeFile, dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "External operation on delete cs failed, "
                      "rc: %d", rc ) ;
      }

      rc = _CSCBNameRemove ( pName, cb, dpsCB, onlyEmpty, pCSCB ) ;
      if ( rc )
      {
         goto error ;
      }
      else if ( !pCSCB )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      if ( extHandler )
      {
         rc = extHandler->done( cb, dpsCB ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "External done operation failed, rc: %d", rc ) ;
         }
         else
         {
            extOprFinish = TRUE ;
         }
      }

      if ( removeFile )
      {
         pCSCB->_su->getEventHolder()->onDropCS( DMS_EVENT_MASK_ALL, cb, dpsCB ) ;
         rc = pCSCB->_su->remove() ;
      }
      else
      {
         pCSCB->_su->getEventHolder()->onUnloadCS( DMS_EVENT_MASK_ALL, cb, dpsCB ) ;
         pCSCB->_su->close() ;
      }

      SDB_OSS_DEL pCSCB ;

      if ( rc )
      {
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB_DELCS, rc );
      return rc ;
   error :
      if ( extHandler && !extOprFinish )
      {
         extHandler->abortOperation( cb ) ;
      }
      goto done ;
   }

   INT32 _SDB_DMSCB::dropCollectionSpace ( const CHAR *pName, _pmdEDUCB *cb,
                                           SDB_DPSCB *dpsCB )
   {
      aquireCSMutex( pName ) ;
      INT32 rc = _delCollectionSpace( pName, cb, dpsCB, TRUE, FALSE ) ;
      releaseCSMutex( pName ) ;

      return rc ;
   }

   INT32 _SDB_DMSCB::unloadCollectonSpace( const CHAR *pName, _pmdEDUCB *cb )
   {
      return _delCollectionSpace( pName, cb, NULL, FALSE, FALSE ) ;
   }

   INT32 _SDB_DMSCB::dropEmptyCollectionSpace( const CHAR *pName,
                                               _pmdEDUCB *cb,
                                               SDB_DPSCB *dpsCB )
   {
      aquireCSMutex( pName ) ;
      INT32 rc = _delCollectionSpace( pName, cb, dpsCB, TRUE, TRUE ) ;
      releaseCSMutex( pName ) ;
      if ( SDB_LOCK_FAILED == rc )
      {
         rc = SDB_DMS_CS_NOT_EMPTY ;
      }
      return rc ;
   }

   INT32 _SDB_DMSCB::renameCollectionSpace( const CHAR *pName,
                                            const CHAR *pNewName,
                                            _pmdEDUCB *cb,
                                            SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      SDB_DMS_CSCB *pCSCB = NULL ;
      BOOLEAN aquired = FALSE ;

      if ( !pName || !pNewName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      aquireCSMutex( pName ) ;
      aquired = TRUE ;

      _mutex.get_shared() ;
      rc = _CSCBNameLookup( pNewName, &pCSCB, NULL, TRUE ) ;
      _mutex.release_shared() ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         rc = SDB_OK ;
      }
      else if ( SDB_OK == rc )
      {
         rc = SDB_DMS_CS_EXIST ;
         goto error ;
      }
      else
      {
         goto error ;
      }

      rc = _CSCBRename( pName, pNewName, cb, dpsCB ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      if ( aquired )
      {
         releaseCSMutex( pName ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DROPCSP1, "_SDB_DMSCB::dropCollectionSpaceP1" )
   INT32 _SDB_DMSCB::dropCollectionSpaceP1 ( const CHAR *pName, _pmdEDUCB *cb,
                                             SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_DROPCSP1 ) ;
      IDmsExtDataHandler *extHandler = NULL ;
      SDB_DMS_CSCB *pCSCB = NULL ;

      if ( !pName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _mutex.get_shared () ;
      rc = _CSCBNameLookup( pName, &pCSCB ) ;
      _mutex.release_shared () ;
      extHandler = pCSCB->_su->data()->getExtDataHandler() ;
      if ( extHandler )
      {
         rc = extHandler->onDelCS( pCSCB->_su->CSName(),
                                   cb, TRUE, dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "External operation on drop CS[ %s ] failed,"
                      " rc: %d", pName, rc ) ;
      }

      rc = _CSCBNameRemoveP1( pName, cb, dpsCB ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to drop cs[%s], rc: %d",
                 pName, rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB_DROPCSP1, rc );
      return rc ;
   error :
      if ( extHandler )
      {
         extHandler->abortOperation( cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DROPCSP1CANCEL, "_SDB_DMSCB::dropCollectionSpaceP1Cancel" )
   INT32 _SDB_DMSCB::dropCollectionSpaceP1Cancel ( const CHAR *pName, _pmdEDUCB *cb,
                                                   SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      IDmsExtDataHandler *extHandler = NULL ;
      SDB_DMS_CSCB *pCSCB = NULL ;

      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_DROPCSP1CANCEL ) ;
      if ( !pName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = _CSCBNameRemoveP1Cancel( pName, cb, dpsCB );
      PD_RC_CHECK( rc, PDERROR,
                   "failed to cancel remove cs(rc=%d)",
                   rc );

      _mutex.get_shared () ;
      rc = _CSCBNameLookup( pName, &pCSCB ) ;
      _mutex.release_shared () ;
      if ( rc )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Find collection space[ %s ] failed, rc: %d",
                 pName, rc ) ;
         goto error ;
      }
      SDB_ASSERT( pCSCB, "Collection space CB is NULL" ) ;
      if ( pCSCB )
      {
         extHandler = pCSCB->_su->data()->getExtDataHandler() ;
         if ( extHandler )
         {
            rc = extHandler->abortOperation( cb ) ;
            PD_RC_CHECK( rc, PDERROR, "External operation on drop CS[ %s ] "
                         "failed, rc: %d", pName, rc ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB_DROPCSP1CANCEL, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DROPCSP2, "_SDB_DMSCB::dropCollectionSpaceP2" )
   INT32 _SDB_DMSCB::dropCollectionSpaceP2 ( const CHAR *pName, _pmdEDUCB *cb,
                                             SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      SDB_DMS_CSCB *pCSCB = NULL ;
      IDmsExtDataHandler *extHandler = NULL ;

      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_DROPCSP2 ) ;
      if ( !pName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _CSCBNameRemoveP2( pName, cb, dpsCB, pCSCB ) ;
      if ( rc )
      {
         goto error ;
      }
      else if ( !pCSCB )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      extHandler = pCSCB->_su->data()->getExtDataHandler() ;
      if ( extHandler )
      {
         rc = extHandler->done( cb, dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "External operation on drop CS[ %s ] failed,"
                      " rc: %d", pName, rc ) ;
      }

      pCSCB->_su->getEventHolder()->onDropCS( DMS_EVENT_MASK_ALL, cb, dpsCB ) ;
      rc = pCSCB->_su->remove() ;

      SDB_OSS_DEL pCSCB ;
      PD_RC_CHECK( rc, PDERROR,
                   "remove failed(rc=%d)", rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB_DROPCSP2, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPCLSIMPLE, "_SDB_DMSCB::dumpInfo" )
   void _SDB_DMSCB::dumpInfo( MON_CL_SIM_LIST &collectionList,
                              BOOLEAN sys )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_DUMPCLSIMPLE );

      ossScopedLock _lock(&_mutex, SHARED) ;

      CSCB_MAP_CONST_ITER it ;

      for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      {
         dmsStorageUnit *su = NULL ;
         dmsStorageUnitID suID = (*it).second ;

         SDB_DMS_CSCB *cscb = _cscbVec[suID] ;
         if ( !cscb )
         {
            continue ;
         }
         su = cscb->_su ;
         SDB_ASSERT ( su, "storage unit pointer can't be NULL" ) ;

         if ( ( !sys && dmsIsSysCSName(su->CSName()) ) ||
              ( ossStrcmp ( su->CSName(), SDB_DMSTEMP_NAME ) == 0 ) )
         {
            continue ;
         }
         su->dumpInfo ( collectionList, sys ) ;
      } // for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      PD_TRACE_EXIT ( SDB__SDB_DMSCB_DUMPCLSIMPLE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPCSSIMPLE, "_SDB_DMSCB::dumpInfo" )
   void _SDB_DMSCB::dumpInfo( MON_CS_SIM_LIST &csList,
                              BOOLEAN sys, BOOLEAN dumpCL, BOOLEAN dumpIdx )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_DUMPCSSIMPLE );

      ossScopedLock _lock(&_mutex, SHARED) ;

      CSCB_MAP_CONST_ITER it ;

      for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); ++it )
      {
         dmsStorageUnit *su = NULL ;
         dmsStorageUnitID suID = (*it).second ;
         SDB_DMS_CSCB *cscb = _cscbVec[suID] ;
         if ( !cscb )
         {
            continue ;
         }

         su = cscb->_su ;
         SDB_ASSERT ( su, "storage unit pointer can't be NULL" ) ;
         if ( ( !sys && dmsIsSysCSName(su->CSName()) ) ||
              ( ossStrcmp ( su->CSName(), SDB_DMSTEMP_NAME ) == 0 ) )
         {
            continue ;
         }

         monCSSimple cs ;
         su->dumpInfo( cs, sys, dumpCL, dumpIdx ) ;
         csList.insert ( cs ) ;
      }
      PD_TRACE_EXIT ( SDB__SDB_DMSCB_DUMPCSSIMPLE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPINFO, "_SDB_DMSCB::dumpInfo" )
   void _SDB_DMSCB::dumpInfo ( MON_CL_LIST &collectionList,
                               BOOLEAN sys )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_DUMPINFO );

      ossScopedLock _lock(&_mutex, SHARED) ;

      CSCB_MAP_CONST_ITER it ;

      for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      {
         dmsStorageUnit *su = NULL ;
         dmsStorageUnitID suID = (*it).second ;

         SDB_DMS_CSCB *cscb = _cscbVec[suID] ;
         if ( !cscb )
         {
            continue ;
         }
         su = cscb->_su ;
         SDB_ASSERT ( su, "storage unit pointer can't be NULL" ) ;

         if ( ( !sys && dmsIsSysCSName(su->CSName()) ) ||
              ( ossStrcmp ( su->CSName(), SDB_DMSTEMP_NAME ) == 0 ) )
         {
            continue ;
         }
         su->dumpInfo ( collectionList, sys ) ;
      } // for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      PD_TRACE_EXIT ( SDB__SDB_DMSCB_DUMPINFO );
   }  // void dumpInfo

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPINFO2, "_SDB_DMSCB::dumpInfo" )
   void _SDB_DMSCB::dumpInfo ( MON_CS_LIST &csList,
                               BOOLEAN sys )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_DUMPINFO2 );

      ossScopedLock _lock(&_mutex, SHARED) ;

      CSCB_MAP_CONST_ITER it ;
      for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      {
         dmsStorageUnit *su = NULL ;
         dmsStorageUnitID suID = (*it).second ;
         SDB_DMS_CSCB *cscb = _cscbVec[suID] ;
         if ( !cscb )
         {
            continue ;
         }
         su = cscb->_su ;
         SDB_ASSERT ( su, "storage unit pointer can't be NULL" ) ;
         if ( !sys && dmsIsSysCSName(cscb->_name) )
         {
            continue ;
         }
         else if ( dmsIsSysCSName(cscb->_name) &&
                   0 == ossStrcmp(cscb->_name, SDB_DMSTEMP_NAME ) )
         {
            continue ;
         }
         monCollectionSpace cs ;
         su->dumpInfo ( cs, sys ) ;
         csList.insert ( cs ) ;
      }
      PD_TRACE_EXIT ( SDB__SDB_DMSCB_DUMPINFO2 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPINFO3, "_SDB_DMSCB::dumpInfo" )
   void _SDB_DMSCB::dumpInfo ( MON_SU_LIST &storageUnitList,
                               BOOLEAN sys )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_DUMPINFO3 );

      ossScopedLock _lock(&_mutex, SHARED) ;

      CSCB_MAP_CONST_ITER it ;

      for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      {
         dmsStorageUnit *su = NULL ;
         dmsStorageUnitID suID = (*it).second ;
         SDB_DMS_CSCB *cscb = _cscbVec[suID] ;
         monStorageUnit storageUnit ;
         if ( !cscb )
         {
            continue ;
         }
         su = cscb->_su ;
         SDB_ASSERT ( su, "storage unit pointer can't be NULL" ) ;

         if ( ( !sys && dmsIsSysCSName(su->CSName()) ) ||
              ( ossStrcmp ( su->CSName(), SDB_DMSTEMP_NAME ) == 0 ) )
         {
            continue ;
         }

         su->dumpInfo ( storageUnit ) ;

         storageUnitList.insert( storageUnit ) ;
      } // for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      PD_TRACE_EXIT ( SDB__SDB_DMSCB_DUMPINFO3 );
   }  // void dumpInfo

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPINFO4, "_SDB_DMSCB::dumpInfo" )
   void _SDB_DMSCB::dumpInfo ( INT64 &totalFileSize )
   {
      totalFileSize = 0;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_DUMPINFO4 );

      ossScopedLock _lock(&_mutex, SHARED) ;

      CSCB_MAP_CONST_ITER it ;

      for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      {
         dmsStorageUnit *su = NULL ;
         dmsStorageUnitID suID = (*it).second ;

         SDB_DMS_CSCB *cscb = _cscbVec[suID] ;
         if ( !cscb )
         {
            continue ;
         }
         su = cscb->_su ;
         SDB_ASSERT ( su, "storage unit pointer can't be NULL" );
         totalFileSize += su->totalSize();
      }
      PD_TRACE_EXIT ( SDB__SDB_DMSCB_DUMPINFO4 );
   }

   void _SDB_DMSCB::dumpPageMapCSInfo( MON_CSNAME_VEC &vecCS )
   {
      ossScopedLock _lock( &_mutex, SHARED ) ;

      SDB_DMS_CSCB *cscb      = NULL ;
      for ( CSCB_MAP_CONST_ITER it = _cscbNameMap.begin() ;
            it != _cscbNameMap.end() ;
            ++it )
      {
         cscb = _cscbVec[ (*it).second ] ;
         if ( NULL == cscb || NULL == cscb->_su )
         {
            continue ;
         }
         else if ( cscb->_su->index()->getPageMapUnit()->isEmpty() )
         {
            continue ;
         }
         vecCS.push_back( monCSName( cscb->_name ) ) ;
      }
   }

   dmsTempSUMgr *_SDB_DMSCB::getTempSUMgr ()
   {
      return &_tempSUMgr ;
   }

   dmsStatSUMgr *_SDB_DMSCB::getStatSUMgr ()
   {
      return &_statSUMgr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CLRSUCACHES, "_SDB_DMSCB::clearSUCaches" )
   void _SDB_DMSCB::clearSUCaches ( UINT32 mask )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_CLRSUCACHES ) ;

      MON_CS_SIM_LIST monCSList ;
      dumpInfo( monCSList, TRUE, FALSE, FALSE ) ;
      clearSUCaches( monCSList, mask ) ;

      PD_TRACE_EXIT ( SDB__SDB_DMSCB_CLRSUCACHES ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CLRSUCACHES_CSLIST, "_SDB_DMSCB::clearSUCaches" )
   void _SDB_DMSCB::clearSUCaches ( const MON_CS_SIM_LIST &monCSList,
                                    UINT32 mask )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_CLRSUCACHES_CSLIST ) ;

      for ( MON_CS_SIM_LIST::const_iterator csIter = monCSList.begin() ;
            csIter != monCSList.end() ;
            csIter ++ )
      {
         INT32 rc = SDB_OK ;
         dmsStorageUnit *pSU = NULL ;
         const monCSSimple &monCS = (*csIter) ;
         const CHAR *pCSName = monCS._name ;
         dmsStorageUnitID suID = monCS._suID ;
         dmsEventSUItem suItem( pCSName, suID, monCS._logicalID ) ;

         rc = verifySUAndLock( &suItem, &pSU, EXCLUSIVE, OSS_ONE_SEC ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDDEBUG, "Failed to get storage unit [%s], rc: %d",
                    pCSName, rc ) ;
            continue ;
         }

         pSU->getEventHolder()->onClearSUCaches( mask ) ;

         suUnlock( suID, EXCLUSIVE ) ;
      }

      if ( OSS_BIT_TEST( mask, DMS_EVENT_MASK_PLAN ) )
      {
         sdbGetRTNCB()->getAPM()->invalidateAllPlans() ;
      }

      PD_TRACE_EXIT ( SDB__SDB_DMSCB_CLRSUCACHES_CSLIST ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CHGSUCACHES, "_SDB_DMSCB::changeSUCaches" )
   void _SDB_DMSCB::changeSUCaches ( UINT32 mask )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_CHGSUCACHES ) ;

      MON_CS_SIM_LIST monCSList ;
      dumpInfo( monCSList, TRUE, FALSE, FALSE ) ;
      changeSUCaches( monCSList, mask ) ;

      PD_TRACE_EXIT ( SDB__SDB_DMSCB_CHGSUCACHES ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CHGSUCACHES_CSLIST, "_SDB_DMSCB::changeSUCaches" )
   void _SDB_DMSCB::changeSUCaches ( const MON_CS_SIM_LIST &monCSList,
                                     UINT32 mask )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_CHGSUCACHES_CSLIST ) ;

      for ( MON_CS_SIM_LIST::const_iterator csIter = monCSList.begin() ;
            csIter != monCSList.end() ;
            csIter ++ )
      {
         INT32 rc = SDB_OK ;
         dmsStorageUnit *pSU = NULL ;
         const monCSSimple &monCS = (*csIter) ;
         const CHAR *pCSName = monCS._name ;
         dmsStorageUnitID suID = monCS._suID ;
         dmsEventSUItem suItem( pCSName, suID, monCS._logicalID ) ;

         rc = verifySUAndLock( &suItem, &pSU, EXCLUSIVE, OSS_ONE_SEC ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDDEBUG, "Failed to get storage unit [%s], rc: %d",
                    pCSName, rc ) ;
            continue ;
         }

         pSU->getEventHolder()->onChangeSUCaches( mask ) ;

         suUnlock( suID, EXCLUSIVE ) ;
      }

      PD_TRACE_EXIT ( SDB__SDB_DMSCB_CHGSUCACHES_CSLIST ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DISPATCHDICTJOB, "_SDB_DMSCB::dispatchDictJob" )
   BOOLEAN _SDB_DMSCB::dispatchDictJob( dmsDictJob &job )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_DISPATCHDICTJOB ) ;
      BOOLEAN foundJob = FALSE ;

      if ( _dictWaitQue.size() > 0 )
      {
         BOOLEAN result = _dictWaitQue.try_pop( job ) ;
         if ( result )
         {
            if ( pmdGetTickSpanTime( job._createTime ) > OSS_ONE_SEC * 5 )
            {
               foundJob = TRUE ;
            }
            else
            {
               _dictWaitQue.push( job ) ;
            }
         }
      }

      PD_TRACE_EXIT( SDB__SDB_DMSCB_DISPATCHDICTJOB ) ;
      return foundJob ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_PUSHDICTJOB, "_SDB_DMSCB::pushDictJob" )
   void _SDB_DMSCB::pushDictJob( dmsDictJob job )
   {
      job._createTime = pmdGetDBTick() ;
      _dictWaitQue.push( job ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_AQUIRE_CSMUTEX, "_SDB_DMSCB::aquireCSMutex" )
   void _SDB_DMSCB::aquireCSMutex( const CHAR *pCSName )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_AQUIRE_CSMUTEX ) ;
      UINT32 pos = ossHash( pCSName ) % DMS_CS_MUTEX_BUCKET_SIZE ;
      _vecCSMutex[ pos ]->get() ;
      PD_TRACE_EXIT ( SDB__SDB_DMSCB_AQUIRE_CSMUTEX ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_RELEASE_CSMUTEX, "_SDB_DMSCB::releaseCSMutex" )
   void _SDB_DMSCB::releaseCSMutex( const CHAR *pCSName )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_RELEASE_CSMUTEX ) ;
      UINT32 pos = ossHash( pCSName ) % DMS_CS_MUTEX_BUCKET_SIZE ;
      _vecCSMutex[ pos ]->release() ;
      PD_TRACE_EXIT ( SDB__SDB_DMSCB_RELEASE_CSMUTEX ) ;
   }

   void _SDB_DMSCB::setIxmKeySorterCreator( dmsIxmKeySorterCreator* creator )
   {
      _ixmKeySorterCreator = creator ;
   }

   dmsIxmKeySorterCreator* _SDB_DMSCB::getIxmKeySorterCreator()
   {
      return _ixmKeySorterCreator ;
   }

   dmsIxmKeySorter* _SDB_DMSCB::createIxmKeySorter( INT64 bufSize, const _dmsIxmKeyComparer& comparer )
   {
      SDB_ASSERT( NULL != _ixmKeySorterCreator, "_ixmKeySorterCreator can't be NULL" ) ;

      return _ixmKeySorterCreator->createSorter( bufSize, comparer ) ;
   }

   void _SDB_DMSCB::releaseIxmKeySorter( dmsIxmKeySorter* sorter )
   {
      SDB_ASSERT( NULL != _ixmKeySorterCreator, "_ixmKeySorterCreator can't be NULL" ) ;

      if ( NULL != sorter )
      {
         _ixmKeySorterCreator->releaseSorter( sorter ) ;
      }
   }

   /*
      _dmsCSMutexScope implement
   */
   _dmsCSMutexScope::_dmsCSMutexScope( SDB_DMSCB *pDMSCB, const CHAR *pName )
   {
      _pDMSCB = pDMSCB ;
      _pName = pName ;

      _pDMSCB->aquireCSMutex( _pName ) ;
   }

   _dmsCSMutexScope::~_dmsCSMutexScope()
   {
      _pDMSCB->releaseCSMutex( _pName ) ;
   }

   /*
      get global SDB_DMSCB
   */
   SDB_DMSCB* sdbGetDMSCB ()
   {
      static SDB_DMSCB s_dmsCB ;
      return &s_dmsCB ;
   }
}

