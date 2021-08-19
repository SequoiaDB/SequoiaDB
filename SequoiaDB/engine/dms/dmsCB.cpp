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
#include "dmsRBSSUMgr.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "dpsOp2Record.hpp"
#include "rtn.hpp"
#include "ossLatch.hpp"
#include "rtnExtDataHandler.hpp"
#include "rtnRecover.hpp"

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

   #define DMS_CSCB_STATUS_NONE              ( 0 )
   #define DMS_CSCB_STATUS_DELETING          ( 1 )
   #define DMS_CSCB_STATUS_RENAMING          ( 2 )

   /*
      _SDB_DMSCB implement
   */

   _SDB_DMSCB::_SDB_DMSCB()
   :_mutex( MON_LATCH_SDB_DMSCB_MUTEX ),
    _stateMtx( MON_LATCH_DMSCB_STATEMTX ),
    _writeCounter(0),
    _dmsCBState(DMS_STATE_NORMAL),
    _logicalSUID(0),
    _nullCSUniqueIDCnt( 0 ),
    _tempSUMgr( this ),
    _statSUMgr( this ),
    _localSUMgr( this ),
    _ixmKeySorterCreator( NULL )
   {
      for ( UINT32 i = 0 ; i< DMS_MAX_CS_NUM ; ++i )
      {
         _cscbVec.push_back ( NULL ) ;
         _tmpCscbVec.push_back ( NULL ) ;
         _tmpCscbStatusVec.push_back( DMS_CSCB_STATUS_NONE ) ;
         // free in desctructor
         _latchVec.push_back ( new(std::nothrow) ossRWMutex() ) ;
         _freeList.push( i ) ;
      }

      for ( UINT32 i = 0 ; i < DMS_CS_MUTEX_BUCKET_SIZE ; ++i )
      {
         _vecCSMutex.push_back( new( std::nothrow ) ossSpinRecursiveXLatch() ) ;
      }

      _blockEvent.signal() ;
   }

   _SDB_DMSCB::~_SDB_DMSCB()
   {
      // make sure dms control block is finalized
      fini() ;
   }

   INT32 _SDB_DMSCB::init ()
   {
      INT32 rc = SDB_OK ;

      if ( pmdGetKRCB()->isRestore() )
      {
         goto done ;
      }

      // 1. load all
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

      // 2. init temp cs mgr
      rc = _tempSUMgr.init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init temp cb, rc: %d", rc ) ;

      // 3. init stat cs cb
      if ( SDB_ROLE_DATA == pmdGetDBRole() ||
           SDB_ROLE_STANDALONE == pmdGetDBRole() )
      {
         rc = _statSUMgr.init() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to init stat cb, rc: %d", rc ) ;

         // Register statistics SU manager
         // which is not registered in loading phase
         if ( _statSUMgr.initialized() )
         {
            _registerHandler( &_statSUMgr ) ;
         }
      }

      rc = _localSUMgr.init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init local su manager, rc: %d",
                   rc ) ;

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
      _localSUMgr.fini() ;
      _tempSUMgr.fini() ;

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
         if ( _vecCSMutex[ i ] )
         {
            SDB_OSS_DEL _vecCSMutex[ i ] ;
            _vecCSMutex[ i ] = NULL ;
         }
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

            /// update cache info
            dmsStorageInfo *pInfo = su->_getStorageInfo() ;
            utilCacheUnit *pCache = su->cacheUnit() ;

            pInfo->_overflowRatio = optCB->getOverFlowRatio() ;
            pInfo->_extentThreshold = optCB->getExtendThreshold() << 20 ;
            pInfo->_enableSparse = optCB->sparseFile() ;
            pInfo->_cacheMergeSize = optCB->getCacheMergeSize() ;
            pInfo->_pageAllocTimeout = optCB->getPageAllocTimeout() ;
            pInfo->_logWriteMod = optCB->logWriteMod() ;

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
      utilCSUniqueID csUniqueID = su->CSUniqueID() ;

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

      // We get from front and return to back so that suID is not reused
      // immediately.
      suID = _freeList.front() ;
      su->_setCSID( suID ) ;
      su->_setLogicalCSID( _logicalSUID++ ) ;
      _freeList.pop() ;
      _cscbNameMap[cscb->_name] = suID ;
      _cscbVec[suID] = cscb ;
      if ( UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
      {
         _cscbIDMap[csUniqueID] = suID ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB__CSCBNMINST, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _SDB_DMSCB::_CSCBNameLookup ( const CHAR *pName,
                                       SDB_DMS_CSCB **cscb,
                                       dmsStorageUnitID *pSuID,
                                       BOOLEAN exceptDeleting )
   {
      return _CSCBLookup( pName, UTIL_UNIQUEID_NULL,
                          cscb, pSuID, exceptDeleting ) ;
   }

   INT32 _SDB_DMSCB::_CSCBIdLookup ( utilCSUniqueID csUniqueID,
                                     SDB_DMS_CSCB **cscb,
                                     dmsStorageUnitID *pSuID,
                                     BOOLEAN exceptDeleting )
   {
      return _CSCBLookup( NULL, csUniqueID, cscb, pSuID, exceptDeleting ) ;
   }

   // look up by name or unique id
   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBLOOK, "_SDB_DMSCB::_CSCBLookup" )
   INT32 _SDB_DMSCB::_CSCBLookup ( const CHAR *pName,
                                   utilCSUniqueID csUniqueID,
                                   SDB_DMS_CSCB **cscb,
                                   dmsStorageUnitID *pSuID,
                                   BOOLEAN exceptDeleting )
   {
      SDB_ASSERT( cscb, "cscb can't be null!" ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB__CSCBLOOK ) ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;

      if ( UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
      {
         CSCB_ID_MAP_CONST_ITER it = _cscbIDMap.find( csUniqueID ) ;
         if ( it != _cscbIDMap.end() )
         {
            suID = it->second ;
         }
      }
      else if ( pName )
      {
         CSCB_MAP_CONST_ITER it = _cscbNameMap.find( pName ) ;
         if ( it != _cscbNameMap.end() )
         {
            suID = it->second ;
         }
      }

      if ( DMS_INVALID_SUID == suID )
      {
         rc = SDB_DMS_CS_NOTEXIST ;
         goto error;
      }

      if ( pSuID )
      {
         *pSuID = suID ;
      }

      if ( _cscbVec[ suID ] )
      {
         *cscb = _cscbVec[ suID ] ;
         goto done ;
      }
      else if ( _tmpCscbVec[ suID ] )
      {
         if ( !exceptDeleting )
         {
            *cscb = _tmpCscbVec[ suID ] ;
            goto done ;
         }

         BYTE disabledStatus = _tmpCscbStatusVec[ suID ] ;
         switch ( disabledStatus )
         {
            case DMS_CSCB_STATUS_DELETING :
               rc = SDB_DMS_CS_DELETING ;
               break ;
            case DMS_CSCB_STATUS_RENAMING :
               rc = SDB_DMS_CS_RENAMING ;
               break ;
            default :
               rc =  SDB_SYS ;
               SDB_ASSERT( FALSE, "This is impossible in this case" ) ;
               break ;
         }
         goto error ;
      }
      else
      {
         /// This is impossible in this case
         SDB_ASSERT( FALSE, "This is impossible in this case" ) ;
         rc = SDB_DMS_CS_NOTEXIST ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB__CSCBLOOK, rc ) ;
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

   INT32 _SDB_DMSCB::_CSCBIdLookupAndLock ( utilCSUniqueID csUniqueID,
                                            dmsStorageUnitID &suID,
                                            SDB_DMS_CSCB **cscb,
                                            OSS_LATCH_MODE lockType,
                                            INT32 millisec )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( cscb, "cscb can't be null!" );

      rc = _CSCBIdLookup( csUniqueID, cscb, &suID, TRUE ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__MVCSCB2TMP, "_SDB_DMSCB::_moveCSCB2TmpList" )
   INT32 _SDB_DMSCB::_moveCSCB2TmpList( const CHAR *pName, BYTE status )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB__MVCSCB2TMP ) ;

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
      // now let's lock the collectionspace, if we can't lock it, let's return
      // false. we shouldn't wait forever
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

      // there is a small timing hole before getting the latch, so we have
      // to get current suID again to verify
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
      SDB_ASSERT ( pCSCB->_name, "cs name can't be null" ) ;

      _tmpCscbVec[ suID ] = pCSCB ;
      _cscbVec[suID] = NULL ;
      _tmpCscbStatusVec[ suID ] = status ;

      _mutex.release () ;
      metaLock = FALSE ;

   done :
      if ( metaLock )
      {
         _mutex.release() ;
         metaLock = FALSE ;
      }
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB__MVCSCB2TMP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__RESTORECSCBFRTMP, "_SDB_DMSCB::_restoreCSCBFromTmpList" )
   INT32 _SDB_DMSCB::_restoreCSCBFromTmpList( const CHAR *pName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB__RESTORECSCBFRTMP ) ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      SDB_DMS_CSCB *pCSCB = NULL ;

      ossScopedLock _lock( &_mutex, EXCLUSIVE ) ;
      rc = _CSCBNameLookup( pName, &pCSCB, &suID, FALSE ) ;
      if ( rc )
      {
         SDB_ASSERT( FALSE, "Impossible in the case" ) ;
         goto error ;
      }

      if ( _tmpCscbVec[suID] != pCSCB )
      {
         SDB_ASSERT( FALSE, "Impossible in this case" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else
      {
         _tmpCscbVec[ suID ] = NULL ;
         _cscbVec[ suID ] = pCSCB ;
         _tmpCscbStatusVec[ suID ] = DMS_CSCB_STATUS_NONE ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB__RESTORECSCBFRTMP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBRENAME, "_SDB_DMSCB::_CSCBRename" )
   INT32 _SDB_DMSCB::_CSCBRename( const CHAR *pName,
                                  const CHAR *pNewName,
                                  _pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB__CSCBRENAME ) ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      UINT32 csLID = ~0 ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      BOOLEAN isReserved = FALSE ;
      BOOLEAN isLocked = FALSE ;
      UINT32 logRecSize = 0 ;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;
      SDB_DMS_CSCB *pCSCB = NULL ;
      IDmsExtDataHandler *extHandler = NULL ;

      // reserved log-size
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

      _mutex.get() ;
      isLocked = TRUE ;

      /// check old name and new name
      rc = _CSCBNameLookup( pName, &pCSCB, &suID, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _CSCBNameLookup( pNewName, &pCSCB, NULL, TRUE ) ;
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

      SDB_ASSERT ( pCSCB->_su, "su can't be null" ) ;

      /// rename cs file
      rc = pCSCB->_su->renameCS( pNewName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rename collection space[%s] to [%s] failed, "
                 "rc: %d", pName, pNewName, rc ) ;
         goto error ;
      }

      extHandler = pCSCB->_su->data()->getExtDataHandler() ;
      if ( extHandler )
      {
         rc = extHandler->onRenameCS( pName, pNewName, cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "External operation on rename cs failed, "
                                   "rc: %d", rc ) ;
      }

      /// rename in map
      // 1) erase map must before reset the name, because map'key is CBCB's name
      _cscbNameMap.erase( pName ) ;
      // 2) rename the CSCB's name
      ossStrncpy( pCSCB->_name, pNewName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
      pCSCB->_name[ DMS_COLLECTION_SPACE_NAME_SZ ] = 0 ;
      // 3) insert new name to map
      _cscbNameMap[ pCSCB->_name ] = suID ;

      /// write log
      csLID = pCSCB->_su->LogicalCSID() ;
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

      // Release the mutex first, since event handler needs the mutex
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
         isLocked = FALSE ;
      }
      if ( isReserved )
      {
         pTransCB->releaseLogSpace( logRecSize, cb ) ;
         isReserved = FALSE ;
      }
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB__CSCBRENAME, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _SDB_DMSCB::_CSCBRenameP1( const CHAR *pName,
                                    const CHAR *pNewName,
                                    _pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB )
   {
      return _moveCSCB2TmpList( pName, DMS_CSCB_STATUS_RENAMING ) ;
   }

   INT32 _SDB_DMSCB::_CSCBRenameP1Cancel( const CHAR *pName,
                                          const CHAR *pNewName,
                                          _pmdEDUCB *cb,
                                          SDB_DPSCB *dpsCB )
   {
      return _restoreCSCBFromTmpList( pName ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBRENAMEP2, "_SDB_DMSCB::_CSCBRenameP2" )
   INT32 _SDB_DMSCB::_CSCBRenameP2( const CHAR *pName,
                                    const CHAR *pNewName,
                                    _pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB__CSCBRENAMEP2 );

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnitID newSuID = DMS_INVALID_SUID ;
      UINT32 csLID = ~0 ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      BOOLEAN isReserved = FALSE ;
      BOOLEAN isLocked = FALSE ;
      UINT32 logRecSize = 0 ;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;
      SDB_DMS_CSCB *pCSCB = NULL ;
      SDB_DMS_CSCB *pNewCSCB = NULL ;

      /// reserved log-size
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

      _mutex.get() ;
      isLocked = TRUE ;

      /// check old name and new name
      rc = _CSCBNameLookup( pName, &pCSCB, &suID, FALSE ) ;
      if ( rc )
      {
         SDB_ASSERT( FALSE, "Impossible in this case" ) ;
         goto error ;
      }
      rc = _CSCBNameLookup( pNewName, &pNewCSCB, &newSuID, FALSE ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         rc = SDB_OK ;
      }
      else
      {
         rc = ( SDB_OK == rc ) ? SDB_DMS_CS_EXIST : rc ;
         SDB_ASSERT( FALSE, "Impossible in this case" ) ;
         goto error ;
      }

      if ( pCSCB != _tmpCscbVec[ suID ] )
      {
         SDB_ASSERT( FALSE, "Impossible in this case" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// rename in map
      // 1) erase map must before reset the name, because map'key is CBCB's name
      _cscbNameMap.erase( pName ) ;
      // 2) rename the CSCB's name
      ossStrncpy( pCSCB->_name, pNewName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
      pCSCB->_name[ DMS_COLLECTION_SPACE_NAME_SZ ] = 0 ;
      // 3) insert new name to map
      _cscbNameMap[ pCSCB->_name ] = suID ;
      // 4) enable the cscb
      _tmpCscbVec[ suID ] = NULL ;
      _cscbVec[ suID ] = pCSCB ;
      _tmpCscbStatusVec[ suID ] = DMS_CSCB_STATUS_NONE ;

      /// write log
      SDB_ASSERT ( pCSCB->_su, "su can't be null" ) ;
      csLID = pCSCB->_su->LogicalCSID() ;

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
         isLocked = FALSE ;
      }
      if ( isReserved )
      {
         pTransCB->releaseLogSpace( logRecSize, cb ) ;
         isReserved = FALSE ;
      }
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB__CSCBRENAMEP2, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _SDB_DMSCB::_CSCBNameRemoveP1 ( const CHAR *pName,
                                         _pmdEDUCB *cb,
                                         SDB_DPSCB *dpsCB )
   {
      return _moveCSCB2TmpList( pName, DMS_CSCB_STATUS_DELETING ) ;
   }

   INT32 _SDB_DMSCB::_CSCBNameRemoveP1Cancel ( const CHAR *pName,
                                               _pmdEDUCB *cb,
                                               SDB_DPSCB *dpsCB )
   {
      return _restoreCSCBFromTmpList( pName ) ;
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
      BOOLEAN isReserved = FALSE ;
      BOOLEAN isLocked = FALSE ;
      UINT32 logRecSize = 0 ;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;
      utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;

      if ( NULL != dpsCB )
      {
         // reserved log-size
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
      if ( pCSCB != _tmpCscbVec[suID] )
      {
         SDB_ASSERT( FALSE, "Impossible in this case" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      SDB_ASSERT ( pCSCB->_su, "su can't be null" ) ;

      // get unique id from su. Because if the cl is in _cscbIDMap, and
      // we don't erase it, it may cause core dump.
      csUniqueID = pCSCB->_su->CSUniqueID() ;

      _tmpCscbVec[ suID ] = NULL ;
      _tmpCscbStatusVec[ suID ] = DMS_CSCB_STATUS_NONE ;
      _cscbNameMap.erase( pName ) ;
      if ( UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
      {
         _cscbIDMap.erase( csUniqueID ) ;
      }
      _freeList.push( suID ) ;

      // log here
      csLID = pCSCB->_su->LogicalCSID() ;
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

         _freeList.push( suID ) ;
         if ( _cscbVec[suID] )
         {
            SDB_OSS_DEL _cscbVec[suID] ;
            _cscbVec[suID] = NULL ;
         }
         if ( _tmpCscbVec[suID] )
         {
            SDB_OSS_DEL _tmpCscbVec[suID] ;
            _tmpCscbVec[suID] = NULL ;
         }
         _tmpCscbStatusVec[ suID ] = DMS_CSCB_STATUS_NONE ;
      }
      _cscbNameMap.clear() ;
      _cscbIDMap.clear() ;
      PD_TRACE_EXIT ( SDB__SDB_DMSCB__CSCBNMMAPCLN );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__GETCSLIST, "_SDB_DMSCB::_getCSList" )
   INT32 _SDB_DMSCB::_getCSList( ossPoolVector< ossPoolString > &csNameVec )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB__GETCSLIST ) ;

      INT32 rc = SDB_OK ;

      ossScopedLock lock( &_mutex, SHARED ) ;
      for ( CSCB_MAP_CONST_ITER itr = _cscbNameMap.begin();
            itr != _cscbNameMap.end(); ++itr )
      {
         try
         {
            csNameVec.push_back( ossPoolString( itr->first ) ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Get collectionspaces list occur exception: %s",
                    e.what() ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB__GETCSLIST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_WRITABLE, "_SDB_DMSCB::writable" )
   INT32 _SDB_DMSCB::writable( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_WRITABLE );
      BOOLEAN hasBlock = FALSE ;

      BOOLEAN locked = FALSE ;

      if ( cb && cb->getLockItem(SDB_LOCK_DMS)->getMode() >= DMS_LOCK_WRITE )
      {
         cb->getLockItem(SDB_LOCK_DMS)->incCount() ;
         _stateMtx.get () ;
         ++_writeCounter ;
         _stateMtx.release() ;
         // already writable
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

            if ( !hasBlock )
            {
               cb->setBlock( EDU_BLOCK_DMS, "Waiting for dms writable" ) ;
               hasBlock = TRUE ;
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
      if ( hasBlock )
      {
         cb->unsetBlock() ;
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
   INT32 _SDB_DMSCB::blockWrite( _pmdEDUCB *cb, SDB_DB_STATUS byStatus,
                                 INT32 timeout )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_BLOCKWRITE );
      INT32 timeSpent = 0 ; // milliseconds
      BOOLEAN hasBlock = FALSE ;

      if ( cb && SDB_DB_NORMAL == byStatus &&
           DMS_LOCK_WHOLE == cb->getLockItem( SDB_LOCK_DMS )->getMode() )
      {
         cb->getLockItem( SDB_LOCK_DMS )->incCount() ;
         goto done ;
      }

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
         if ( cb && cb->isInterrupted() )
         {
            _dmsCBState = DMS_STATE_NORMAL ;
            PMD_SET_DB_STATUS( SDB_DB_NORMAL ) ;
            _blockEvent.signal() ;
            rc = SDB_APP_INTERRUPT ;
            break ;
         }
         else if ( timeout != -1 && timeSpent >= timeout )
         {
            _dmsCBState = DMS_STATE_NORMAL ;
            PMD_SET_DB_STATUS( SDB_DB_NORMAL ) ;
            _blockEvent.signal() ;
            rc = SDB_TIMEOUT ;
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
               cb->getLockItem( SDB_LOCK_DMS )->incCount() ;
            }
            goto done;
         }
         else
         {
            if ( cb )
            {
               hasBlock = TRUE ;
               cb->setBlock( EDU_BLOCK_DMS, "" ) ;
               cb->printInfo( EDU_INFO_DOING,
                              "Waiting to block dms write(WriteCounter:%u)",
                              _writeCounter ) ;
            }
            _stateMtx.release();
            ossSleepmillis( DMS_CHANGESTATE_WAIT_LOOP ) ;
            timeSpent += DMS_CHANGESTATE_WAIT_LOOP ;
         }
      }

   done:
      if ( hasBlock )
      {
         cb->unsetBlock() ;
      }
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB_BLOCKWRITE, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_UNBLOCKWRITE, "_SDB_DMSCB::unblockWrite" )
   void _SDB_DMSCB::unblockWrite( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_UNBLOCKWRITE );

      SDB_ASSERT( ( cb->getLockItem( SDB_LOCK_DMS )->lockCount() > 0 &&
                    DMS_LOCK_WHOLE ==
                    cb->getLockItem( SDB_LOCK_DMS )->getMode() ),
                  "The edu's lock mode or lock count is invalid" ) ;

      if ( cb &&
           cb->getLockItem( SDB_LOCK_DMS )->decCount() > 0 )
      {
         goto done ;
      }

      _stateMtx.get() ;
      _dmsCBState = DMS_STATE_NORMAL ;
      PMD_SET_DB_STATUS( SDB_DB_NORMAL ) ;
      _stateMtx.release() ;
      if ( cb )
      {
         cb->getLockItem(SDB_LOCK_DMS)->setMode( DMS_LOCK_NONE ) ;
      }

   done:
      _blockEvent.signalAll() ;
      PD_TRACE_EXIT ( SDB__SDB_DMSCB_UNBLOCKWRITE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_REGFULLSYNC, "_SDB_DMSCB::registerFullSync" )
   INT32 _SDB_DMSCB::registerFullSync( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_REGFULLSYNC );
      BOOLEAN hasBlock = FALSE ;

   retry:
      /// Full-sync can't blockWrite, because create/drop index when
      /// full-sync need to writable in async thread tasks
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
            if ( !hasBlock )
            {
               cb->setBlock( EDU_BLOCK_DMS, "Waiting for dms fullsync" ) ;
               hasBlock = TRUE ;
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
         cb->getLockItem( SDB_LOCK_DMS )->setMode( DMS_LOCK_WHOLE ) ;
         cb->getLockItem( SDB_LOCK_DMS )->incCount() ;

         _stateMtx.release() ;
      }

      if ( hasBlock )
      {
         cb->unsetBlock() ;
      }
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB_REGFULLSYNC, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_FULLSYNCDOWN, "_SDB_DMSCB::fullSyncDown" )
   void _SDB_DMSCB::fullSyncDown( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_FULLSYNCDOWN ) ;

      SDB_ASSERT( ( cb->getLockItem( SDB_LOCK_DMS )->lockCount() > 0 &&
                    DMS_LOCK_WHOLE ==
                    cb->getLockItem( SDB_LOCK_DMS )->getMode() ),
                  "The edu's lock mode or lock count is invalid" ) ;

      ossScopedLock lock( &_stateMtx ) ;
      if ( 0 == cb->getLockItem( SDB_LOCK_DMS )->decCount() )
      {
         cb->getLockItem( SDB_LOCK_DMS )->setMode( DMS_LOCK_NONE ) ;
         _dmsCBState = DMS_STATE_NORMAL ;
      }
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

   INT32 _SDB_DMSCB::idToSUAndLock ( utilCSUniqueID csUniqueID,
                                     dmsStorageUnitID &suID,
                                     _dmsStorageUnit **su,
                                     OSS_LATCH_MODE lockType,
                                     INT32 millisec )
   {
      INT32 rc = SDB_OK ;
      SDB_DMS_CSCB *cscb = NULL ;

      SDB_ASSERT( su, "su can't be null!" ) ;
      if ( ! UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
      {
         return SDB_INVALIDARG ;
      }

      ossScopedLock _lock( &_mutex, SHARED ) ;
      rc = _CSCBIdLookupAndLock( csUniqueID, suID, &cscb,
                                 lockType, millisec ) ;
      if ( SDB_OK == rc )
      {
         *su = cscb->_su ;
      }

      return rc ;
   }

   INT32 _SDB_DMSCB::nameToSUAndLock ( const CHAR *pName,
                                       dmsStorageUnitID &suID,
                                       _dmsStorageUnit **su,
                                       OSS_LATCH_MODE lockType,
                                       INT32 millisec )
   {
      INT32 rc = SDB_OK ;
      SDB_DMS_CSCB *cscb = NULL ;
      SDB_ASSERT( su, "su can't be null!" ) ;
      if ( !pName )
      {
         return SDB_INVALIDARG ;
      }

      ossScopedLock _lock( &_mutex, SHARED ) ;
      rc = _CSCBNameLookupAndLock( pName, suID, &cscb,
                                   lockType, millisec ) ;
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
                   "Failed to get collection space [%s] in %d, rc: %d",
                   pCSName, lockType, rc ) ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CHGCSUID, "_SDB_DMSCB::changeCSUniqueID" )
   INT32 _SDB_DMSCB::changeCSUniqueID( _dmsStorageUnit* su,
                                       utilCSUniqueID csUniqueID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_CHGCSUID ) ;

      SDB_DMS_CSCB* cscb = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageInfo* suInfo = NULL ;
      const CHAR* csname = su->CSName() ;
      utilCSUniqueID orgUniqueID = su->CSUniqueID() ;

      if ( orgUniqueID == csUniqueID )
      {
         goto done ;
      }

      // change unique id in storage unit
      suInfo = su->_getStorageInfo() ;
      suInfo->_csUniqueID = csUniqueID ;

      su->_pDataSu->updateCSUniqueIDFromInfo() ;
      su->_pIndexSu->updateCSUniqueIDFromInfo() ;
      su->_pLobSu->updateCSUniqueIDFromInfo() ;

      PD_LOG ( PDEVENT,
               "Change cs[%s] unique id from [%u] to [%u]",
               csname, orgUniqueID, csUniqueID ) ;

      // get su id
      rc = _CSCBNameLookup( csname, &cscb, &suID ) ;
      if ( rc || DMS_INVALID_SUID == suID )
      {
         PD_LOG( PDERROR,
                 "Failed to look up cs[%s], suID: %d, rc: %d",
                 csname, suID, rc ) ;
         goto error ;
      }

      // change map
      if ( UTIL_IS_VALID_CSUNIQUEID( orgUniqueID ) )
      {
         _cscbIDMap.erase( orgUniqueID ) ;
      }
      if ( UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
      {
         _cscbIDMap[ csUniqueID ] = suID ;
      }

      if ( orgUniqueID != UTIL_UNIQUEID_NULL &&
           csUniqueID  == UTIL_UNIQUEID_NULL )
      {
         _nullCSUniqueIDCntInc() ;
      }
      if ( orgUniqueID == UTIL_UNIQUEID_NULL &&
           csUniqueID  != UTIL_UNIQUEID_NULL )
      {
         _nullCSUniqueIDCntDec() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_CHGCSUID, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // input: clInfoObj
   // [
   //    { "Name": "bar1", "UniqueID": 2667174690817 } ,
   //    { "Name": "bar2", "UniqueID": 2667174690818 }
   // ]
   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CHGUID, "_SDB_DMSCB::changeUniqueID" )
   INT32 _SDB_DMSCB::changeUniqueID( const CHAR* csname,
                                     utilCSUniqueID csUniqueID,
                                     const BSONObj& clInfoObj,
                                     pmdEDUCB* cb,
                                     SDB_DPSCB* dpsCB,
                                     BOOLEAN isLoadCS )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_CHGUID ) ;

      BOOLEAN isReserved = FALSE ;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;
      UINT32 logRecSize = 0 ;
      dpsTransCB* pTransCB = pmdGetKRCB()->getTransCB();
      SDB_DMS_CSCB* cscb = NULL ;
      SDB_DMS_CSCB *tmpCSCB = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnitID suTmpID = DMS_INVALID_SUID ;
      dmsStorageUnit* su = NULL ;
      dmsStorageUnit* suTmp = NULL ;
      BOOLEAN isMetaLocked = FALSE ;

      _mutex.get_shared () ;
      rc = _CSCBNameLookup( csname, &cscb, &suID, TRUE ) ;
      _mutex.release_shared () ;

      if ( rc )
      {
         goto error ;
      }
      su = cscb->_su ;

      // reserved log-size
      if ( dpsCB )
      {
         rc = dpsAddUniqueID2Record( csname, csUniqueID, clInfoObj, record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build record:%d", rc ) ;

         rc = dpsCB->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = pTransCB->reservedLogSpace( logRecSize, cb );
         PD_RC_CHECK( rc, PDERROR,
                     "Failed to reserved log space(length=%u)",
                     logRecSize );
         isReserved = TRUE ;
      }

      // change cl unique id
      rc = nameToSUAndLock( csname, suTmpID, &suTmp, SHARED ) ;
      if ( rc )
      {
         goto error ;
      }
      else if ( suTmpID != suID )
      {
         suUnlock ( suTmpID ) ;
         rc = SDB_DMS_CS_NOTEXIST ;
         goto error ;
      }

      su->data()->changeCLUniqueID( utilBson2ClNameId( clInfoObj ),
                                    csUniqueID, isLoadCS ) ;

      suUnlock ( suID ) ;

      // get meta lock
      _mutex.get() ;
      isMetaLocked = TRUE ;

      // there is a small timing hole before getting the mutex, so we have
      // to get current suID again to verify
      rc = _CSCBNameLookup( csname, &tmpCSCB, &suTmpID, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }
      else if ( suTmpID != suID )
      {
         rc = SDB_DMS_CS_NOTEXIST ;
         goto error ;
      }

      // change cs unique id
      rc = changeCSUniqueID( su, csUniqueID ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to change cs unique id, rc: %d",
                    rc ) ;

      // write dps
      if ( dpsCB )
      {
         info.setInfoEx( cscb->_su->LogicalCSID(), ~0, DMS_INVALID_EXTENT, cb );
         rc = dpsCB->prepare ( info ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to insert cscrt into log, rc: %d",
                       rc ) ;

         _mutex.release() ;
         isMetaLocked = FALSE ;

         dpsCB->writeData( info ) ;
      }

   done :
      if ( rc == SDB_OK )
      {
         PD_LOG( PDEVENT,
                 "Change unique id, cs name: %s, cs unique id: %u, cl info: %s",
                 csname, csUniqueID, clInfoObj.toString().c_str() ) ;
      }
      if ( isMetaLocked )
      {
         _mutex.release () ;
      }
      if ( isReserved )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_CHGUID, rc ) ;
      return rc ;
   error :
      goto done ;
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
      utilCSUniqueID csUniqueID = su->CSUniqueID() ;

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
         // reserved log-size
         rc = dpsCSCrt2Record( pName, csUniqueID, pageSize,
                               lobPageSz, type, record ) ;
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
         rc = SDB_DMS_CS_EXIST ;
         goto error ;
      }
      else if ( rc != SDB_DMS_CS_NOTEXIST )
      {
         goto error ;
      }

      rc = _CSCBIdLookup( csUniqueID, &cscb ) ;
      if ( SDB_OK == rc )
      {
         rc = SDB_DMS_CS_UNIQUEID_CONFLICT ;
         PD_LOG ( PDERROR,
                  "CS unique id[%u] already exists[name: %s], rc: %d",
                  csUniqueID, cscb->_name, rc ) ;
         goto error ;
      }
      else if ( rc != SDB_DMS_CS_NOTEXIST )
      {
         goto error ;
      }

      rc = _CSCBNameInsert ( pName, topSequence, su, suID ) ;
      // write dps
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

      // statistics SU manager might not be initialized
      // 1. during dmsCB initialization (will be registered later)
      // 2. in CATALOG node
      if ( _statSUMgr.initialized() )
      {
         su->regEventHandler( &_statSUMgr ) ;
      }
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
      if ( SDB_OK == rc &&
           !dmsIsSysCSName( pName ) &&
           csUniqueID == UTIL_UNIQUEID_NULL )
      {
         _nullCSUniqueIDCntInc() ;
      }
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
                                          SDB_DPSCB * dpsCB, BOOLEAN removeFile,
                                          BOOLEAN onlyEmpty )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_DELCS ) ;

      UINT32 csLID = ~0 ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      BOOLEAN isTransLocked = FALSE ;
      SDB_DMS_CSCB* pCSCB = NULL ;

      // get cs cb
      _mutex.get_shared() ;
      rc = _CSCBNameLookup( pName, &pCSCB, NULL, TRUE ) ;
      _mutex.release_shared() ;
      if ( rc )
      {
         goto error ;
      }

      SDB_ASSERT ( pCSCB->_su, "su can't be null" ) ;

      // check cs is empty or not
      if ( onlyEmpty && 0 != pCSCB->_su->data()->getCollectionNum() )
      {
         rc = SDB_DMS_CS_NOT_EMPTY ;
         goto error ;
      }

      // lock transaction, standalone need lock trans here
      csLID = pCSCB->_su->LogicalCSID() ;
      if ( cb && cb->getTransExecutor()->useTransLock() )
      {
         dpsTransRetInfo lockConflict ;
         rc = pTransCB->transLockTryX( cb, csLID, DMS_INVALID_MBID,
                                       NULL, &lockConflict ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR,
                     "Failed to lock collection-space, rc:%d"OSS_NEWLINE
                     "Conflict( representative ):"OSS_NEWLINE
                     "   EDUID:  %llu"OSS_NEWLINE
                     "   TID:    %u"OSS_NEWLINE
                     "   LockId: %s"OSS_NEWLINE
                     "   Mode:   %s"OSS_NEWLINE,
                     rc,
                     lockConflict._eduID,
                     lockConflict._tid,
                     lockConflict._lockID.toString().c_str(),
                     lockModeToString( lockConflict._lockType ) ) ;
            goto error ;
         }
         isTransLocked = TRUE ;
      }

      // drop phase 1
      rc = _delCollectionSpaceP1( pName, cb, dpsCB, removeFile ) ;
      if ( rc )
      {
         goto error ;
      }

      // re-check cs is empty or not in lock
      if ( onlyEmpty && 0 != pCSCB->_su->data()->getCollectionNum() )
      {
         // it is not empty after phase 1, cancel deleting
         _delCollectionSpaceP1Cancel( pName, cb, dpsCB ) ;
         rc = SDB_DMS_CS_NOT_EMPTY ;
         goto error ;
      }

      // drop phase 2
      rc = _delCollectionSpaceP2( pName, cb, dpsCB, removeFile ) ;
      if ( rc )
      {
         _delCollectionSpaceP1Cancel( pName, cb, dpsCB ) ;
         goto error ;
      }

   done:
      if ( isTransLocked )
      {
         pTransCB->transLockRelease( cb, csLID ) ;
         isTransLocked = FALSE ;
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_DELCS, rc ) ;
      return rc ;
   error:
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__DELCSP1, "_SDB_DMSCB::_delCollectionSpaceP1" )
   INT32 _SDB_DMSCB::_delCollectionSpaceP1 ( const CHAR *pName, _pmdEDUCB *cb,
                                             SDB_DPSCB *dpsCB,
                                             BOOLEAN removeFile )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB__DELCSP1 ) ;
      IDmsExtDataHandler *extHandler = NULL ;
      SDB_DMS_CSCB *pCSCB = NULL ;

      if ( !pName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _CSCBNameRemoveP1( pName, cb, dpsCB ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to drop cs[%s], rc: %d",
                 pName, rc ) ;
         goto error ;
      }

      // The cscb of the cs is now in the deleting vector. Try to delete all the
      // capped collections of the text indices in it.
      _mutex.get_shared() ;
      rc = _CSCBNameLookup( pName, &pCSCB, NULL, FALSE ) ;
      _mutex.release_shared() ;
      PD_RC_CHECK( rc, PDERROR, "Find collection space[ %s ] failed, rc: %d",
                   pName, rc ) ;

      extHandler = pCSCB->_su->data()->getExtDataHandler() ;
      if ( extHandler )
      {
         rc = extHandler->onDelCS( pCSCB->_name, cb, removeFile ) ;
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            // if capped cs doesn't exist, we just ignor error
            rc = SDB_OK ;
         }
         if ( rc )
         {
            // If external operation failed, we should resume by cancel the
            // the remove.
            PD_LOG( PDERROR, "External operation on drop CS[ %s ] failed,"
                    " rc: %d", pName, rc ) ;
            INT32 rcTmp = _CSCBNameRemoveP1Cancel( pName, cb, dpsCB ) ;
            if ( rcTmp )
            {
               PD_LOG( PDERROR, "Cancel remove cs name failed, rc: %d",
                       rcTmp ) ;
            }
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB__DELCSP1, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__DELCSP1CANCEL, "_SDB_DMSCB::_delCollectionSpaceP1Cancel" )
   INT32 _SDB_DMSCB::_delCollectionSpaceP1Cancel ( const CHAR *pName, _pmdEDUCB *cb,
                                                   SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      IDmsExtDataHandler *extHandler = NULL ;
      SDB_DMS_CSCB *pCSCB = NULL ;

      PD_TRACE_ENTRY ( SDB__SDB_DMSCB__DELCSP1CANCEL ) ;
      if ( !pName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // Until now the cs is still not visible to other operations, as the cscb
      // is in the deleting vector. So abort the external operation before
      // the P1Cancel below. As once the cancel is done, the cs is exposed to
      // other operations, and parallel scenarios should be considered.
      _mutex.get_shared () ;
      rc = _CSCBNameLookup( pName, &pCSCB, NULL, FALSE ) ;
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
            rc = extHandler->abortOperation( DMS_EXTOPR_TYPE_DROPCS, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "External abort operation on drop "
                         "CS[ %s ] failed, rc: %d", pName, rc ) ;
         }
      }

      rc = _CSCBNameRemoveP1Cancel( pName, cb, dpsCB );
      PD_RC_CHECK( rc, PDERROR,
                   "failed to cancel remove cs(rc=%d)",
                   rc );

   done :
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB__DELCSP1CANCEL, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__DELCSP2, "_SDB_DMSCB::_delCollectionSpaceP2" )
   INT32 _SDB_DMSCB::_delCollectionSpaceP2 ( const CHAR *pName, _pmdEDUCB *cb,
                                             SDB_DPSCB *dpsCB,
                                             BOOLEAN removeFile )
   {
      INT32 rc = SDB_OK ;
      SDB_DMS_CSCB *pCSCB = NULL ;
      IDmsExtDataHandler *extHandler = NULL ;

      PD_TRACE_ENTRY ( SDB__SDB_DMSCB__DELCSP2 ) ;
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
         rc = extHandler->done( DMS_EXTOPR_TYPE_DROPCS, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "External operation on drop CS[ %s ] failed,"
                      " rc: %d", pName, rc ) ;
      }

      if ( removeFile )
      {
         pCSCB->_su->getEventHolder()->onDropCS( DMS_EVENT_MASK_ALL,
                                                 cb, dpsCB ) ;
         // if remove file failed, we can do nothing
         rc = pCSCB->_su->remove() ;
      }
      else
      {
         pCSCB->_su->getEventHolder()->onUnloadCS( DMS_EVENT_MASK_ALL,
                                                   cb, dpsCB ) ;
         pCSCB->_su->close() ;
      }

      SDB_OSS_DEL pCSCB ;
      PD_RC_CHECK( rc, PDERROR,
                   "remove failed(rc=%d)", rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB__DELCSP2, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _SDB_DMSCB::dropCollectionSpaceP1 ( const CHAR *pName, _pmdEDUCB *cb,
                                             SDB_DPSCB *dpsCB )
   {
      return _delCollectionSpaceP1( pName, cb, dpsCB, TRUE ) ;
   }

   INT32 _SDB_DMSCB::dropCollectionSpaceP1Cancel ( const CHAR *pName,
                                                   _pmdEDUCB *cb,
                                                   SDB_DPSCB *dpsCB )
   {
      return _delCollectionSpaceP1Cancel( pName, cb, dpsCB ) ;
   }

   INT32 _SDB_DMSCB::dropCollectionSpaceP2 ( const CHAR *pName, _pmdEDUCB *cb,
                                             SDB_DPSCB *dpsCB )
   {
      return _delCollectionSpaceP2( pName, cb, dpsCB, TRUE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_RENAMECS, "_SDB_DMSCB::renameCollectionSpace" )
   INT32 _SDB_DMSCB::renameCollectionSpace( const CHAR *pName,
                                            const CHAR *pNewName,
                                            _pmdEDUCB *cb,
                                            SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_RENAMECS ) ;

      UINT32 csLID = ~0 ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      BOOLEAN isTransLocked = FALSE ;
      SDB_DMS_CSCB* pCSCB = NULL ;

      // get cs cb
      _mutex.get_shared() ;
      rc = _CSCBNameLookup( pName, &pCSCB, NULL, TRUE ) ;
      _mutex.release_shared() ;
      if ( rc )
      {
         goto error ;
      }

      SDB_ASSERT ( pCSCB->_su, "su can't be null" ) ;

      // lock transaction, standalone need lock trans here
      csLID = pCSCB->_su->LogicalCSID() ;
      if ( cb && cb->getTransExecutor()->useTransLock() )
      {
         dpsTransRetInfo lockConflict ;
         rc = pTransCB->transLockTryS( cb, csLID, DMS_INVALID_MBID,
                                       NULL, &lockConflict ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR,
                     "Failed to lock collection-space, rc:%d"OSS_NEWLINE
                     "Conflict( representative ):"OSS_NEWLINE
                     "   EDUID:  %llu"OSS_NEWLINE
                     "   TID:    %u"OSS_NEWLINE
                     "   LockId: %s"OSS_NEWLINE
                     "   Mode:   %s"OSS_NEWLINE,
                     rc,
                     lockConflict._eduID,
                     lockConflict._tid,
                     lockConflict._lockID.toString().c_str(),
                     lockModeToString( lockConflict._lockType ) ) ;
            goto error ;
         }
         isTransLocked = TRUE ;
      }

      rc = renameCollectionSpaceP1( pName, pNewName, cb, dpsCB ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = renameCollectionSpaceP2( pName, pNewName, cb, dpsCB ) ;
      if ( rc )
      {
         renameCollectionSpaceP1Cancel( pName, pNewName, cb, dpsCB ) ;
         goto error ;
      }

   done:
      if ( isTransLocked )
      {
         pTransCB->transLockRelease( cb, csLID ) ;
         isTransLocked = FALSE ;
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_RENAMECS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_RENAMECSP1, "_SDB_DMSCB::renameCollectionSpaceP1" )
   INT32 _SDB_DMSCB::renameCollectionSpaceP1( const CHAR *pName,
                                              const CHAR *pNewName,
                                              _pmdEDUCB *cb,
                                              SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_RENAMECSP1 ) ;

#ifdef _WINDOWS

      INT32 rcNew = SDB_OK ;
      SDB_DMS_CSCB *pCSCB = NULL ;
      BOOLEAN aquired = FALSE ;

      if ( !pName || !pNewName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      aquireCSMutex( pName ) ;
      aquired = TRUE ;

      /// check old cs and new cs
      _mutex.get_shared() ;
      rc = _CSCBNameLookup( pName, &pCSCB, NULL, TRUE ) ;
      rcNew = _CSCBNameLookup( pNewName, &pCSCB, NULL, TRUE ) ;
      _mutex.release_shared() ;

      if ( rc )
      {
         goto error ;
      }

      if ( SDB_DMS_CS_NOTEXIST == rcNew )
      {
         rcNew = SDB_OK ;
      }
      else if ( SDB_OK == rcNew )
      {
         rcNew = SDB_DMS_CS_EXIST ;
      }
      if ( rcNew )
      {
         rc = rcNew ;
         goto error ;
      }

      /// rename
      rc = _CSCBRenameP1( pName, pNewName, cb, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to rename collection space at phase 1 [%s] to [%s], "
                   "rc: %d", pName, pNewName, rc ) ;

   done:
      if ( aquired )
      {
         releaseCSMutex( pName ) ;
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_RENAMECSP1, rc ) ;
      return rc ;
   error:
      goto done ;

#else

   PD_TRACE_EXITRC( SDB__SDB_DMSCB_RENAMECSP1, rc ) ;
   return rc ;

#endif
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_RENAMECSP1C, "_SDB_DMSCB::renameCollectionSpaceP1Cancel" )
   INT32 _SDB_DMSCB::renameCollectionSpaceP1Cancel( const CHAR *pName,
                                                    const CHAR *pNewName,
                                                    _pmdEDUCB *cb,
                                                    SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_RENAMECSP1C ) ;

#ifdef _WINDOWS

      INT32 rcNew = SDB_OK ;
      SDB_DMS_CSCB *pCSCB = NULL ;
      BOOLEAN aquired = FALSE ;

      if ( !pName || !pNewName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      aquireCSMutex( pName ) ;
      aquired = TRUE ;

      /// check old cs and new cs
      _mutex.get_shared() ;
      rc = _CSCBNameLookup( pName, &pCSCB, NULL, FALSE ) ;
      rcNew = _CSCBNameLookup( pNewName, &pCSCB, NULL, FALSE ) ;
      _mutex.release_shared() ;

      if ( rc )
      {
         goto error ;
      }

      if ( SDB_DMS_CS_NOTEXIST == rcNew )
      {
         rcNew = SDB_OK ;
      }
      else if ( SDB_OK == rcNew )
      {
         rcNew = SDB_DMS_CS_EXIST ;
      }
      if ( rcNew )
      {
         rc = rcNew ;
         goto error ;
      }

      /// cancel rename
      rc = _CSCBRenameP1Cancel( pName, pNewName, cb, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to cancel rename collection space at phase 1 "
                   "[%s] to [%s], rc: %d", pName, pNewName, rc ) ;

   done:
      if ( aquired )
      {
         releaseCSMutex( pName ) ;
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_RENAMECSP1C, rc ) ;
      return rc ;
   error:
      goto done ;

#else

   PD_TRACE_EXITRC( SDB__SDB_DMSCB_RENAMECSP1C, rc ) ;
   return rc ;

#endif
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_RENAMECSP2, "_SDB_DMSCB::renameCollectionSpaceP2" )
   INT32 _SDB_DMSCB::renameCollectionSpaceP2( const CHAR *pName,
                                              const CHAR *pNewName,
                                              _pmdEDUCB *cb,
                                              SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_RENAMECSP2 ) ;

      BOOLEAN aquired = FALSE ;

#ifdef _WINDOWS

      INT32 rcNew = SDB_OK ;
      SDB_DMS_CSCB *pCSCB = NULL ;
      SDB_DMS_CSCB *pNewCSCB = NULL ;
      IDmsExtDataHandler *extHandler = NULL ;

      if ( !pName || !pNewName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      aquireCSMutex( pName ) ;
      aquired = TRUE ;

      /// check old cs and new cs
      _mutex.get_shared() ;
      rc = _CSCBNameLookup( pName, &pCSCB, NULL, FALSE ) ;
      rcNew = _CSCBNameLookup( pNewName, &pNewCSCB, NULL, FALSE ) ;
      _mutex.release_shared() ;

      if ( rc )
      {
         goto error ;
      }

      if ( SDB_DMS_CS_NOTEXIST == rcNew )
      {
         rcNew = SDB_OK ;
      }
      else if ( SDB_OK == rcNew )
      {
         rcNew = SDB_DMS_CS_EXIST ;
      }
      if ( rcNew )
      {
         rc = rcNew ;
         goto error ;
      }

      SDB_ASSERT ( pCSCB->_su, "su can't be null" ) ;

      /// rename su
      rc = pCSCB->_su->renameCS( pNewName ) ;
      PD_RC_CHECK( rc, PDERROR, "Rename collection space[%s] to [%s] failed, "
                   "rc: %d", pName, pNewName, rc ) ;

      extHandler = pCSCB->_su->data()->getExtDataHandler() ;
      if ( extHandler )
      {
         rc = extHandler->onRenameCS( pName, pNewName, cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "External operation on rename cs failed, "
                      "rc: %d", rc ) ;
      }

      pCSCB->_su->getEventHolder()->onRenameCS( DMS_EVENT_MASK_ALL, pName,
                                                pNewName, cb, dpsCB ) ;

      /// rename map
      rc = _CSCBRenameP2( pName, pNewName, cb, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to rename collection space at phase 2 [%s] to [%s], "
                   "rc: %d", pName, pNewName, rc ) ;

#else

      rc = _CSCBRename( pName, pNewName, cb, dpsCB ) ;
      if ( rc )
      {
         goto error ;
      }

#endif

      PD_LOG( PDEVENT, "Rename collection space[%s] to [%s] succeed",
              pName, pNewName ) ;

   done:
      if ( aquired )
      {
         releaseCSMutex( pName ) ;
         aquired = FALSE ;
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_RENAMECSP2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPCLSIMPLE, "_SDB_DMSCB::dumpInfo" )
   INT32 _SDB_DMSCB::dumpInfo( MON_CL_SIM_LIST &collectionList,
                               BOOLEAN sys )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_DUMPCLSIMPLE );
      INT32 rc = SDB_OK ;
      CSCB_MAP_CONST_ITER it ;

      ossScopedLock _lock(&_mutex, SHARED) ;

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
         rc = su->dumpInfo ( collectionList, sys ) ;
         if ( rc )
         {
            goto error ;
         }
      } // for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_DUMPCLSIMPLE, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPCSSIMPLE, "_SDB_DMSCB::dumpInfo" )
   INT32 _SDB_DMSCB::dumpInfo( MON_CS_SIM_LIST &csList,
                               BOOLEAN sys, BOOLEAN dumpCL, BOOLEAN dumpIdx )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_DUMPCSSIMPLE );

      ossPoolVector< ossPoolString > csNameVec ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;

      rc = _getCSList( csNameVec ) ;
      if ( rc )
      {
         goto error ;
      }

      for ( ossPoolVector< ossPoolString >::iterator itr = csNameVec.begin();
            itr != csNameVec.end(); ++itr )
      {
         // As we do not take the cs metadata mutex here, so cs may have been
         // dropped after we get the names.
         rc = nameToSUAndLock( itr->c_str(), suID, &su ) ;
         if ( rc )
         {
            if ( SDB_DMS_CS_NOTEXIST != rc )
            {
               PD_LOG( PDERROR, "Failed to lock collectionspace[%s], rc: %d",
                       itr->c_str(), rc ) ;
            }
            continue ;
         }

         SDB_ASSERT ( su, "storage unit pointer can't be NULL" ) ;
         if ( ( !sys && dmsIsSysCSName(su->CSName()) ) ||
              ( ossStrcmp ( su->CSName(), SDB_DMSTEMP_NAME ) == 0 ) )
         {
            suUnlock( suID ) ;
            continue ;
         }

         monCSSimple cs ;
         rc = su->dumpInfo( cs, sys, dumpCL, dumpIdx ) ;
         try
         {
            csList.insert ( cs ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Dump collectionspaces occur exception: %s",
                    e.what() ) ;
            rc = SDB_OOM ;
         }

         suUnlock( suID ) ;

         if ( rc )
         {
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__SDB_DMSCB_DUMPCSSIMPLE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPINFO, "_SDB_DMSCB::dumpInfo" )
   INT32 _SDB_DMSCB::dumpInfo ( MON_CL_LIST &collectionList, BOOLEAN sys )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_DUMPINFO );

      INT32 rc = SDB_OK ;
      CSCB_MAP_CONST_ITER it ;

      ossScopedLock _lock(&_mutex, SHARED) ;

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
         rc = su->dumpInfo ( collectionList, sys ) ;
         if ( rc )
         {
            goto error ;
         }
      } // for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_DUMPINFO, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPINFO2, "_SDB_DMSCB::dumpInfo" )
   INT32 _SDB_DMSCB::dumpInfo ( MON_CS_LIST &csList, BOOLEAN sys )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_DUMPINFO2 );

      INT32 rc = SDB_OK ;
      CSCB_MAP_CONST_ITER it ;

      ossScopedLock _lock(&_mutex, SHARED) ;

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
         // do not dump temp cs
         else if ( dmsIsSysCSName(cscb->_name) &&
                   0 == ossStrcmp(cscb->_name, SDB_DMSTEMP_NAME ) )
         {
            continue ;
         }
         monCollectionSpace cs ;
         rc = su->dumpInfo ( cs, sys ) ;
         try
         {
            csList.insert ( cs ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Dump collectionspaces occur exception: %s",
                    e.what() ) ;
            rc = SDB_OOM ;
         }

         if ( rc )
         {
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_DUMPINFO2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPINFO3, "_SDB_DMSCB::dumpInfo" )
   INT32 _SDB_DMSCB::dumpInfo ( MON_SU_LIST &storageUnitList, BOOLEAN sys )
   {
      PD_TRACE_ENTRY ( SDB__SDB_DMSCB_DUMPINFO3 );

      INT32 rc = SDB_OK ;
      CSCB_MAP_CONST_ITER it ;

      ossScopedLock _lock(&_mutex, SHARED) ;

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

         try
         {
            storageUnitList.insert( storageUnit ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Dump storages occur exception: %s",
                    e.what() ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      } // for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_DUMPINFO3, rc ) ;
      return rc ;
   error:
      goto done ;
   }

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
         /// push back
         vecCS.push_back( monCSName( cscb->_name ) ) ;
      }
   }

   UINT32 _SDB_DMSCB::nullCSUniqueIDCnt() const
   {
      return _nullCSUniqueIDCnt ;
   }

   void _SDB_DMSCB::_nullCSUniqueIDCntInc()
   {
      _nullCSUniqueIDCnt++ ;
   }

   void _SDB_DMSCB::_nullCSUniqueIDCntDec()
   {
      _nullCSUniqueIDCnt-- ;
   }

   void _SDB_DMSCB::_registerHandler ( _IDmsEventHandler *pHandler)
   {
      ossScopedLock lock( &_mutex, SHARED ) ;

      for ( CSCB_ITERATOR iter = _cscbVec.begin() ;
            iter != _cscbVec.end();
            ++ iter )
      {
         if ( NULL == (*iter) )
         {
            continue ;
         }
         _dmsStorageUnit * su = (*iter)->_su ;
         SDB_ASSERT( su, "su is invalid" ) ;
         su->regEventHandler( pHandler ) ;
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

   dmsLocalSUMgr* _SDB_DMSCB::getLocalSUMgr()
   {
      return &_localSUMgr ;
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
         // Make sure main-collection plans are invalidated
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

   BOOLEAN _SDB_DMSCB::dispatchDictJob( dmsDictJob &job )
   {
      return _dictWaitQue.try_pop( job ) ;
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

   INT32 _SDB_DMSCB::createIxmKeySorter( INT64 bufSize,
                                         const _dmsIxmKeyComparer& comparer,
                                         dmsIxmKeySorter** ppSorter )
   {
      SDB_ASSERT( NULL != _ixmKeySorterCreator, "_ixmKeySorterCreator can't be NULL" ) ;

      return _ixmKeySorterCreator->createSorter( bufSize, comparer, ppSorter ) ;
   }

   void _SDB_DMSCB::releaseIxmKeySorter( dmsIxmKeySorter* pSorter )
   {
      SDB_ASSERT( NULL != _ixmKeySorterCreator, "_ixmKeySorterCreator can't be NULL" ) ;

      if ( NULL != pSorter )
      {
         _ixmKeySorterCreator->releaseSorter( pSorter ) ;
      }
   }

   INT32 _SDB_DMSCB::getMaxDMSLSN( DPS_LSN_OFFSET &maxLsn )
   {
      INT32 rc = SDB_OK ;
      MON_CS_SIM_LIST csList ;
      MON_CS_SIM_LIST::iterator it ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dumpInfo( csList, TRUE ) ;

      for ( it = csList.begin() ; it != csList.end() ; ++it )
      {
         const monCSSimple &csInfo = *it ;

         if ( 0 == ossStrcmp( csInfo._name, SDB_DMSTEMP_NAME ) )
         {
            continue ;
         }

         dmsStorageUnit *su = NULL ;
         suID = DMS_INVALID_SUID ;
         rc = nameToSUAndLock( csInfo._name, suID, &su ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to lock collectionspace[%s], rc: %d",
                    csInfo._name, rc ) ;
            goto error ;
         }

         DPS_LSN_OFFSET tmpMaxLsn = DPS_INVALID_LSN_OFFSET ;
         rtnRecoverUnit recoverUnit ;
         rc = recoverUnit.init( su ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to init recover unit:rc=%d", rc ) ;

         tmpMaxLsn = recoverUnit.getMaxValidLsn() ;
         if ( DPS_INVALID_LSN_OFFSET != tmpMaxLsn )
         {
            if ( DPS_INVALID_LSN_OFFSET == maxLsn || maxLsn < tmpMaxLsn )
            {
               maxLsn = tmpMaxLsn ;
            }
         }

         if ( DMS_INVALID_SUID != suID )
         {
            suUnlock( suID ) ;
            suID = DMS_INVALID_SUID ;
         }
      }

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         suUnlock( suID ) ;
         suID = DMS_INVALID_SUID ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_FIXTRANSMBSTATS, "_SDB_DMSCB::fixTransMBStats" )
   void _SDB_DMSCB::fixTransMBStats ()
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_FIXTRANSMBSTATS ) ;
      MON_CS_SIM_LIST monCSList ;
      dumpInfo( monCSList, TRUE, FALSE, FALSE ) ;
      for ( MON_CS_SIM_LIST::const_iterator csIter = monCSList.begin() ;
            csIter != monCSList.end() ;
            csIter ++ )
      {
         INT32 rc = SDB_OK ;
         dmsStorageUnit * su = NULL ;
         const monCSSimple & monCS = (*csIter) ;
         dmsEventSUItem suItem( monCS._name, monCS._suID, monCS._logicalID ) ;

         rc = verifySUAndLock( &suItem, &su, SHARED, OSS_ONE_SEC ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDDEBUG, "Failed to get storage unit [%s], rc: %d",
                    monCS._name, rc ) ;
            continue ;
         }

         su->fixTransMBStat() ;

         suUnlock( monCS._suID, SHARED ) ;
      }
      PD_TRACE_EXIT( SDB__SDB_DMSCB_FIXTRANSMBSTATS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CLEARALLCRUDCB, "_SDB_DMSCB::clearAllCRUDCB" )
   void _SDB_DMSCB::clearAllCRUDCB ()
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_CLEARALLCRUDCB ) ;

      MON_CS_SIM_LIST monCSList ;
      dumpInfo( monCSList, TRUE, FALSE, FALSE ) ;
      for ( MON_CS_SIM_LIST::const_iterator csIter = monCSList.begin() ;
            csIter != monCSList.end() ;
            csIter ++ )
      {
         INT32 rc = SDB_OK ;
         dmsStorageUnit * su = NULL ;
         const monCSSimple & monCS = (*csIter) ;
         dmsEventSUItem suItem( monCS._name, monCS._suID, monCS._logicalID ) ;

         rc = verifySUAndLock( &suItem, &su, SHARED, OSS_ONE_SEC ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDDEBUG, "Failed to get storage unit [%s], rc: %d",
                    monCS._name, rc ) ;
            continue ;
         }

         su->clearMBCRUDCB() ;

         suUnlock( monCS._suID, SHARED ) ;
      }

      PD_TRACE_EXIT( SDB__SDB_DMSCB_CLEARALLCRUDCB ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CLEARSUCRUDCB, "_SDB_DMSCB::clearSUCRUDCB" )
   INT32 _SDB_DMSCB::clearSUCRUDCB ( const CHAR * collectionSpace )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__SDB_DMSCB_CLEARSUCRUDCB ) ;

      SDB_ASSERT( NULL != collectionSpace, "collection space is invalid" ) ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnit * su = NULL ;

      rc = nameToSUAndLock( collectionSpace, suID, &su, SHARED, OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get storage unit [%s], rc: %d",
                   collectionSpace, rc ) ;

      su->clearMBCRUDCB() ;

   done :
      if ( DMS_INVALID_SUID != suID )
      {
         suUnlock( suID ) ;
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_CLEARSUCRUDCB, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CLEARMBCRUDCB, "_SDB_DMSCB::clearMBCRUDCB" )
   INT32 _SDB_DMSCB::clearMBCRUDCB ( const CHAR * collection )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__SDB_DMSCB_CLEARMBCRUDCB ) ;

      SDB_ASSERT( NULL != collection, "collection is invalid" ) ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnit * su = NULL ;
      CHAR collectionSpace[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;
      CHAR clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = { 0 } ;
      dmsMBContext * mbContext = NULL ;

      rc = rtnResolveCollectionName( collection,
                                     ossStrlen( collection ),
                                     collectionSpace,
                                     DMS_COLLECTION_SPACE_NAME_SZ,
                                     clShortName,
                                     DMS_COLLECTION_NAME_SZ ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to resolve collection name [%s], "
                    "rc: %d", collection, rc ) ;

      rc = nameToSUAndLock( collectionSpace, suID, &su, SHARED, OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get storage unit [%s], rc: %d",
                   collectionSpace, rc ) ;

      rc = su->data()->getMBContext( &mbContext, clShortName, -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get mb context [%s], rc: %d",
                   collection, rc ) ;

      mbContext->mbStat()->_crudCB.resetOnce() ;

   done :
      if ( NULL != su && NULL != mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         suUnlock( suID ) ;
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_CLEARMBCRUDCB, rc ) ;
      return rc ;

   error :
      goto done ;
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

