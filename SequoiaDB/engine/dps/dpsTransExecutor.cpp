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

   Source File Name = dpsTransExecutor.cpp

   Descriptive Name = Operating System Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "dpsTransExecutor.hpp"
#include "dpsTransLockDef.hpp"
#include "dpsTransLRB.hpp"

using namespace bson ;

namespace engine
{

   /*
      _dpsTransConfItem implement
   */
   _dpsTransConfItem::_dpsTransConfItem()
   {
      reset() ;
   }

   _dpsTransConfItem::~_dpsTransConfItem()
   {
   }

   void _dpsTransConfItem::reset()
   {
      _transIsolation   = DPS_TRANS_ISOLATION_DFT ;
      _transTimeout     = DPS_TRANS_DFT_TIMEOUT * OSS_ONE_SEC ;
      _transWaitLock    = DPS_TRANS_LOCKWAIT_DFT ;
      _useRollbackSegment = TRUE ;
      _transAutoCommit  = FALSE ;
      _transAutoRollback= TRUE ;
      _transRCCount     = DPS_TRANS_RCCOUNT_DFT ;
      _transAllowLockEscalation = DPS_TRANS_ALLOWLOCKESCALATION_DFT ;
      _transMaxLockNum  = DPS_TRANS_MAXLOCKNUM_DFT ;
      _transMaxLogSpaceRatio = DPS_TRANS_MAXLOGSPACERATIO_DFT ;
      _transConfMask    = 0 ;
      _transConfVer     = 1 ;
   }

   void _dpsTransConfItem::resetConfMask()
   {
      _transConfMask = 0 ;
   }

   void _dpsTransConfItem::resetConfMask( UINT32 bitMask )
   {
      OSS_BIT_CLEAR( _transConfMask, bitMask ) ;
   }

   INT32 _dpsTransConfItem::getTransIsolation() const
   {
      return _transIsolation ;
   }

   UINT32 _dpsTransConfItem::getTransTimeout() const
   {
      return _transTimeout ;
   }

   BOOLEAN _dpsTransConfItem::isTransWaitLock() const
   {
      return _transWaitLock ;
   }

   BOOLEAN _dpsTransConfItem::useRollbackSegment() const
   {
      return _useRollbackSegment ;
   }

   BOOLEAN _dpsTransConfItem::isTransAutoCommit() const
   {
      return _transAutoCommit ;
   }

   BOOLEAN _dpsTransConfItem::isTransAutoRollback() const
   {
      return _transAutoRollback ;
   }

   BOOLEAN _dpsTransConfItem::isTransRCCount () const
   {
      return _transRCCount ;
   }

   BOOLEAN _dpsTransConfItem::isTransAllowLockEscalation() const
   {
      return _transAllowLockEscalation ;
   }

   INT32 _dpsTransConfItem::getTransMaxLockNum() const
   {
      return _transMaxLockNum ;
   }

   INT32 _dpsTransConfItem::getTransMaxLogSpaceRatio() const
   {
      return _transMaxLogSpaceRatio ;
   }

   UINT32 _dpsTransConfItem::getTransConfMask() const
   {
      return _transConfMask ;
   }

   UINT32 _dpsTransConfItem::getTransConfVer() const
   {
      return _transConfVer ;
   }

   void _dpsTransConfItem::setTransIsolation( INT32 isolation,
                                              BOOLEAN enableMask )
   {
      if ( isolation >= TRANS_ISOLATION_RU &&
           isolation <= TRANS_ISOLATION_MAX - 1 &&
           _transIsolation != isolation )
      {
         _transIsolation = isolation ;
         ++_transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_ISOLATION ;
      }
   }

   void _dpsTransConfItem::setTransTimeout( UINT32 timeout,
                                            BOOLEAN enableMask )
   {
      if ( _transTimeout != timeout )
      {
         _transTimeout = timeout ;
         ++_transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_TIMEOUT ;
      }
   }

   void _dpsTransConfItem::setTransWaitLock( BOOLEAN waitLock,
                                             BOOLEAN enableMask )
   {
      if ( _transWaitLock != waitLock )
      {
         _transWaitLock = waitLock ;
         ++_transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_WAITLOCK ;
      }
   }

   void _dpsTransConfItem::setUseRollbackSemgent( BOOLEAN use,
                                                  BOOLEAN enableMask )
   {
      if ( _useRollbackSegment != use )
      {
         _useRollbackSegment = use ;
         ++_transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_USERBS ;
      }
   }

   void _dpsTransConfItem::setTransAutoCommit( BOOLEAN autoCommit,
                                               BOOLEAN enableMask )
   {
      if ( _transAutoCommit != autoCommit )
      {
         _transAutoCommit = autoCommit ;
         ++_transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_AUTOCOMMIT ;
      }
   }

   void _dpsTransConfItem::setTransAutoRollback( BOOLEAN autoRollback,
                                                 BOOLEAN enableMask )
   {
      if ( _transAutoRollback != autoRollback )
      {
         _transAutoRollback = autoRollback ;
         ++_transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_AUTOROLLBACK ;
      }
   }

   void _dpsTransConfItem::setTransRCCount ( BOOLEAN rcCount,
                                             BOOLEAN enableMask )
   {
      if ( _transRCCount != rcCount )
      {
         _transRCCount = rcCount ;
         ++_transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_RCCOUNT ;
      }
   }

   void _dpsTransConfItem::setTransAllowLockEscalation( BOOLEAN allow,
                                                        BOOLEAN enableMask )
   {
      if ( _transAllowLockEscalation != allow )
      {
         _transAllowLockEscalation = allow ;
         ++ _transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_ALLOWLOCKESCALATION ;
      }
   }

   void _dpsTransConfItem::setTransMaxLockNum( INT32 maxNum,
                                               BOOLEAN enableMask )
   {
      if ( _transMaxLockNum != maxNum )
      {
         _transMaxLockNum = maxNum ;
         ++ _transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_MAXLOCKNUM ;
      }
   }

   void _dpsTransConfItem::setTransMaxLogSpaceRatio( INT32 maxRatio,
                                                     BOOLEAN enableMask )
   {
      if ( _transMaxLogSpaceRatio != maxRatio )
      {
         _transMaxLogSpaceRatio = maxRatio ;
         ++ _transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_MAXLOGSPACERATIO ;
      }
   }

   void _dpsTransConfItem::updateByMask( const _dpsTransConfItem &rhs )
   {
      UINT32 rhsMask = rhs.getTransConfMask() ;
      UINT32 oldTransConfVer = _transConfVer ;

      if ( rhsMask & TRANS_CONF_MASK_ISOLATION )
      {
         setTransIsolation( rhs.getTransIsolation(), TRUE ) ;
      }
      if ( rhsMask & TRANS_CONF_MASK_TIMEOUT )
      {
         setTransTimeout( rhs.getTransTimeout(), TRUE ) ;
      }
      if ( rhsMask & TRANS_CONF_MASK_USERBS )
      {
         setUseRollbackSemgent( rhs.useRollbackSegment(), TRUE ) ;
      }
      if ( rhsMask & TRANS_CONF_MASK_AUTOCOMMIT )
      {
         setTransAutoCommit( rhs.isTransAutoCommit(), TRUE ) ;
      }
      if ( rhsMask & TRANS_CONF_MASK_AUTOROLLBACK )
      {
         setTransAutoRollback( rhs.isTransAutoRollback(), TRUE ) ;
      }
      if ( rhsMask & TRANS_CONF_MASK_WAITLOCK )
      {
         setTransWaitLock( rhs.isTransWaitLock(), TRUE ) ;
      }
      if ( rhsMask & TRANS_CONF_MASK_RCCOUNT )
      {
         setTransRCCount( rhs.isTransRCCount(), TRUE ) ;
      }
      if ( rhsMask & TRANS_CONF_MASK_ALLOWLOCKESCALATION )
      {
         setTransAllowLockEscalation( rhs.isTransAllowLockEscalation(),
                                      TRUE ) ;
      }
      if ( rhsMask & TRANS_CONF_MASK_MAXLOCKNUM )
      {
         setTransMaxLockNum( rhs.getTransMaxLockNum(), TRUE ) ;
      }
      if ( rhsMask & TRANS_CONF_MASK_MAXLOGSPACERATIO )
      {
         setTransMaxLogSpaceRatio( rhs.getTransMaxLogSpaceRatio(), TRUE ) ;
      }

      if ( oldTransConfVer != _transConfVer )
      {
         _transConfVer = oldTransConfVer + 1 ;
      }
   }

   void _dpsTransConfItem::copyFrom( const _dpsTransConfItem &rhs )
   {
      *this = rhs ;
   }

   void _dpsTransConfItem::toBson( BSONObjBuilder & builder ) const
   {
      try
      {
         builder.append( FIELD_NAME_TRANSISOLATION, _transIsolation ) ;
         builder.append( FIELD_NAME_TRANS_TIMEOUT,
                         _transTimeout / OSS_ONE_SEC ) ;
         builder.appendBool( FIELD_NAME_TRANS_USE_RBS, _useRollbackSegment ) ;
         builder.appendBool( FIELD_NAME_TRANS_WAITLOCK, _transWaitLock ) ;
         builder.appendBool( FIELD_NAME_TRANS_AUTOCOMMIT, _transAutoCommit ) ;
         builder.appendBool( FIELD_NAME_TRANS_AUTOROLLBACK,
                             _transAutoRollback ) ;
         builder.appendBool( FIELD_NAME_TRANS_RCCOUNT, _transRCCount ) ;
         builder.appendBool( FIELD_NAME_TRANS_ALLOWLOCKESCALATION,
                             _transAllowLockEscalation ) ;
         builder.append( FIELD_NAME_TRANS_MAXLOCKNUM, _transMaxLockNum ) ;
         builder.append( FIELD_NAME_TRANS_MAXLOGSPACERATIO,
                         _transMaxLogSpaceRatio ) ;
      }
      catch ( std::exception &e )
      {
         /// ignore
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
      }
   }

   void _dpsTransConfItem::fromBson( const BSONObj &obj )
   {
      UINT32 oldTransConfVer = _transConfVer ;

      try
      {
         BSONObjIterator itr( obj ) ;
         while( itr.more() )
         {
            BSONElement e = itr.next() ;

            if ( 0 == ossStrcmp( e.fieldName(),
                                 FIELD_NAME_TRANSISOLATION ) )
            {
               setTransIsolation( e.numberInt(), TRUE ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_TRANS_TIMEOUT ) )
            {
               setTransTimeout( e.numberInt() * OSS_ONE_SEC, TRUE ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_TRANS_USE_RBS ) )
            {
               setUseRollbackSemgent( e.booleanSafe(), TRUE ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_TRANS_WAITLOCK ) )
            {
               setTransWaitLock( e.booleanSafe(), TRUE ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_TRANS_AUTOCOMMIT ) )
            {
               setTransAutoCommit( e.booleanSafe(), TRUE ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_TRANS_AUTOROLLBACK ) )
            {
               setTransAutoRollback( e.booleanSafe(), TRUE ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_TRANS_RCCOUNT ) )
            {
               setTransRCCount( e.booleanSafe(), TRUE ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_TRANS_ALLOWLOCKESCALATION ) )
            {
               setTransAllowLockEscalation( e.booleanSafe(), TRUE ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_TRANS_MAXLOCKNUM ) )
            {
               setTransMaxLockNum( e.numberInt(), TRUE ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_TRANS_MAXLOGSPACERATIO ) )
            {
               setTransMaxLogSpaceRatio( e.numberInt(), TRUE ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exeption: %s", e.what() ) ;
         /// ignore
      }

      if ( oldTransConfVer != _transConfVer )
      {
         _transConfVer = oldTransConfVer + 1 ;
      }
   }

   /*
      _dpsTransExecutor implement
   */
   _dpsTransExecutor::_dpsTransExecutor(monMonitorManager *monMgr)
      : _monMgr( monMgr )
   {
      for ( UINT32 i = LOCKMGR_TRANS_LOCK; i < LOCKMGR_TYPE_MAX; i++ )
      {
         _waiter[ i ]        = NULL;
         _waiterQueType[ i ] = DPS_QUE_NULL ;
         _lastLRB[ i ]       = NULL;
         _lockCount[ i ]     = 0 ;
         _leafLockCount[ i ] = 0 ;
         _isLockEscalated[ i ] = FALSE ;
         _accessingTransLRB[ i ] = NULL ;
      }
      _useTransLock     = TRUE ;
      _reservedLogSpace = 0 ;
      _usedLogSpace     = 0 ;
      _maxLogSpace      = OSS_UINT64_MAX ;
      _lockWaitStarted  = FALSE ;
      _monLock          = NULL ;
   }

   _dpsTransExecutor::~_dpsTransExecutor()
   {
   }

   void _dpsTransExecutor::clearAll()
   {
      clearMBStats() ;
      for ( UINT32 i = LOCKMGR_TRANS_LOCK; i < LOCKMGR_TYPE_MAX; i++ )
      {
         clearWaiterInfo( (LOCKMGR_TYPE)i ) ;
         clearLastLRB( (LOCKMGR_TYPE) i ) ;
         clearLockCount( (LOCKMGR_TYPE) i ) ;
         clearLock( (LOCKMGR_TYPE) i ) ;
      }
      resetLogSpace() ;
   }

   void _dpsTransExecutor::assertLocks()
   {
      if ( _lockCount[LOCKMGR_TRANS_LOCK] > 0 )
      {
         dpsTransLRB *curLRB = _lastLRB[LOCKMGR_TRANS_LOCK] ;
         while ( curLRB )
         {
            PD_LOG( PDWARNING, "lock leak [CSID: %u, CLID: %u, extID: %u, extOffset: %u]",
                    curLRB->lrbHdr->lockId.csID(),
                    curLRB->lrbHdr->lockId.clID(),
                    curLRB->lrbHdr->lockId.extentID(),
                    curLRB->lrbHdr->lockId.offset() ) ;
            curLRB = curLRB->eduLrbNext ;
         }
      }
      SDB_ASSERT( _mapCSCLLockID[LOCKMGR_TRANS_LOCK].size() == 0,
                  "Trans lock must be 0" ) ;
      SDB_ASSERT( _mapCSCLLockID[LOCKMGR_INDEX_LOCK].size() == 0,
                  "Index lock must be 0" ) ;
      SDB_ASSERT( _lockCount[LOCKMGR_TRANS_LOCK] == 0,
                  "Trans Lock must be 0" ) ;
      SDB_ASSERT( _lockCount[LOCKMGR_INDEX_LOCK] == 0,
                  "Index lock must be 0" ) ;
      SDB_ASSERT( _leafLockCount[LOCKMGR_TRANS_LOCK] == 0,
                  "Trans leaf Lock must be 0" ) ;
      SDB_ASSERT( _leafLockCount[LOCKMGR_INDEX_LOCK] == 0,
                  "Index leaf lock must be 0" ) ;
      SDB_ASSERT( _waiter[ LOCKMGR_TRANS_LOCK ] == NULL,
                  "Trans lock waiter LRB must be invalid" ) ;
      SDB_ASSERT( _waiter[ LOCKMGR_INDEX_LOCK ] == NULL,
                  "Index lock waiter LRB must be invalid" ) ;
      SDB_ASSERT( _lastLRB[ LOCKMGR_TRANS_LOCK ] == NULL,
                  "Tran lock last LRB must be invalid" ) ;
      SDB_ASSERT( _lastLRB[ LOCKMGR_INDEX_LOCK ] == NULL,
                  "Index lock last LRB must be invalid" ) ;
      SDB_ASSERT( _reservedLogSpace == 0, "Reserved log space must be 0" ) ;
      SDB_ASSERT( _usedLogSpace == 0, "Used log space must be 0" ) ;
   }

   void _dpsTransExecutor::setWaiterInfo( dpsTransLRB* waiter,
                                          DPS_TRANS_QUE_TYPE type,
                                          LOCKMGR_TYPE lockMgrType )
   {
      acquireLRBAccessingLock( lockMgrType ) ;
      _waiter[ lockMgrType ]        = waiter ;
      _waiterQueType[ lockMgrType ] = type ;
      releaseLRBAccessingLock( lockMgrType ) ;

      if ( !_lockWaitStarted )
      {
         _lockWaitStartTimer.sample() ;
         _lockWaitStarted = TRUE ;

         if ( _monMgr->isOperational( MON_CLASS_LOCK ) &&
              _monLock == NULL )
         {
            _monLock = _monMgr->registerMonitorObject<monClassLock>() ;
            if ( _monLock )
            {
               dpsTransLRB *ownerLRB = waiter->lrbHdr->ownerLRB ;
               // NOTE:
               //    The owner list could be empty this time. For example,
               // when the last owner releases the lock, it removes itself
               // from owner list. At this time a new lock requester may
               // get the bkt latch before the one be woken up. The new
               // requester will add itself into the upgrade or waiter list,
               // if the list is not empty to avoid starving the waiters.
               if ( NULL != ownerLRB )
               {
                  if ( ownerLRB->lockMode == DPS_TRANSLOCK_X )
                  {
                     _monLock->xOwnerTID = ownerLRB->dpsTxExectr->getTID() ;
                  }

                  _monLock->numOwner = 1 ;
                  while ( ownerLRB->nextLRB != NULL )
                  {
                     _monLock->numOwner++ ;
                     ownerLRB = ownerLRB->nextLRB ;
                  }
               }
               _monLock->waiterTID = getTID() ;
               _monLock->lockID = waiter->lrbHdr->lockId ;
               _monLock->lockMode = waiter->lockMode ;
            }
         }
      }
   }

   void _dpsTransExecutor::clearWaiterInfo( LOCKMGR_TYPE lockMgrType )
   {
      acquireLRBAccessingLock( lockMgrType ) ;
      _waiter[ lockMgrType ]        = NULL;
      _waiterQueType[ lockMgrType ] = DPS_QUE_NULL ;
      releaseLRBAccessingLock( lockMgrType ) ;
   }

   dpsTransLRB* _dpsTransExecutor::getWaiterLRB( LOCKMGR_TYPE lockMgrType ) const
   {
      return _waiter[ lockMgrType ] ;
   }

   DPS_TRANS_QUE_TYPE _dpsTransExecutor::getWaiterQueType( LOCKMGR_TYPE lockMgrType ) const
   {
      return _waiterQueType[ lockMgrType ]  ;
   }

   void _dpsTransExecutor::setLastLRB( dpsTransLRB* lrb, LOCKMGR_TYPE lockMgrType )
   {
      acquireLRBAccessingLock( lockMgrType ) ;
      if ( NULL == lrb )
      {
         setAccessingLRB( lockMgrType, NULL ) ;
      }

      _lastLRB[ lockMgrType ] = lrb ;
      releaseLRBAccessingLock( lockMgrType ) ;
   }

   void _dpsTransExecutor::clearLastLRB( LOCKMGR_TYPE lockMgrType )
   {
      acquireLRBAccessingLock( lockMgrType ) ;
      _lastLRB[ lockMgrType ] = NULL ;
      setAccessingLRB( lockMgrType, NULL ) ;
      releaseLRBAccessingLock( lockMgrType ) ;
   }

   dpsTransLRB * _dpsTransExecutor::getLastLRB( LOCKMGR_TYPE lockMgrType ) const
   {
      return _lastLRB[ lockMgrType ] ;
   }

   BOOLEAN _dpsTransExecutor::addLock( const dpsTransLockId &lockID,
                                       dpsTransLRB * lrb,
                                       LOCKMGR_TYPE lockMgrType )
   {
      // only add CS or CL lock into the map
      if ( ! lockID.isLeafLevel() )
      {
         ossScopedLock lock( &_mapMutex ) ;

         if ( _mapCSCLLockID[ lockMgrType ].insert(
                  std::make_pair( lockID, lrb ) ).second )
         {
            return TRUE ;
         }
      }
      return FALSE ;
   }

   BOOLEAN _dpsTransExecutor::findLock( const dpsTransLockId &lockID,
                                        dpsTransLRB * &lrb,
                                        LOCKMGR_TYPE lockMgrType,
                                        BOOLEAN needLock )
   {
      BOOLEAN hasFound = FALSE ;

      // only search the map if it is CS or CL lock
      if ( ! lockID.isLeafLevel() )
      {
         if ( needLock )
         {
            _mapMutex.get() ;
         }

         DPS_LOCKID_MAP_CIT cit = _mapCSCLLockID[ lockMgrType ].find( lockID ) ;
         if ( cit != _mapCSCLLockID[ lockMgrType ].end() )
         {
            lrb = cit->second ;
            hasFound = TRUE ;
         }

         if ( needLock )
         {
            _mapMutex.release() ;
         }
      }

      return hasFound ;
   }

   BOOLEAN _dpsTransExecutor::removeLock( const dpsTransLockId &lockID,
                                          LOCKMGR_TYPE lockMgrType )
   {
      // only do the remove if it is CS or CL lock
      if ( ! lockID.isLeafLevel() )
      {
         ossScopedLock lock( &_mapMutex ) ;

         return _mapCSCLLockID[ lockMgrType ].erase( lockID ) ? TRUE : FALSE ;
      }
      return FALSE ;
   }

   void _dpsTransExecutor::clearLock( LOCKMGR_TYPE lockMgrType )
   {
      ossScopedLock lock( &_mapMutex ) ;

      _mapCSCLLockID[ lockMgrType ].clear() ;
   }

   void _dpsTransExecutor::incLockCount( LOCKMGR_TYPE lockMgrType,
                                         BOOLEAN isLeafLevel )
   {
      _lockCount[ lockMgrType ]++ ;
      if ( isLeafLevel )
      {
         _leafLockCount[ lockMgrType ] ++ ;
      }
   }

   void _dpsTransExecutor::decLockCount( LOCKMGR_TYPE lockMgrType,
                                         BOOLEAN isLeafLevel )
   {
      SDB_ASSERT( _lockCount[ lockMgrType ] > 0, "Lock count must > 0" ) ;
      if ( _lockCount[ lockMgrType ] > 0 )
      {
         _lockCount[ lockMgrType ]-- ;
      }
      if ( isLeafLevel )
      {
         SDB_ASSERT( _leafLockCount[ lockMgrType ] > 0,
                     "Leaf lock count must > 0" ) ;
         if ( _leafLockCount[ lockMgrType ] > 0 )
         {
            _leafLockCount[ lockMgrType ] -- ;
         }
      }
   }

   void _dpsTransExecutor::clearLockCount( LOCKMGR_TYPE lockMgrType )
   {
      _lockCount[ lockMgrType ] = 0 ;
      _leafLockCount[ lockMgrType ] = 0 ;
      _isLockEscalated[ lockMgrType ] = FALSE ;
   }

   UINT32 _dpsTransExecutor::getLockCount( LOCKMGR_TYPE lockMgrType ) const
   {
      return _lockCount[ lockMgrType ] ;
   }

   UINT32 _dpsTransExecutor::getLeafLockCount( LOCKMGR_TYPE lockMgrType ) const
   {
      return _leafLockCount[ lockMgrType ] ;
   }

   BOOLEAN _dpsTransExecutor::useTransLock() const
   {
      return _useTransLock ;
   }

   void _dpsTransExecutor::setUseTransLock( BOOLEAN use )
   {
      _useTransLock = use ;
   }

   void _dpsTransExecutor::initTransConf( INT32 isolation,
                                          UINT32 timeout,
                                          BOOLEAN waitLock,
                                          BOOLEAN autoCommit,
                                          BOOLEAN autoRollback,
                                          BOOLEAN useRBS,
                                          BOOLEAN rcCount,
                                          BOOLEAN allowLockEscalation,
                                          INT32 maxLockNum,
                                          INT32 maxLogSpaceRatio,
                                          UINT64 totalLogSpace )
   {
      _transConfMask = 0 ;

      setTransIsolation( isolation, FALSE ) ;
      setTransTimeout( timeout, FALSE ) ;
      setTransWaitLock( waitLock, FALSE ) ;
      setTransAutoCommit( autoCommit, FALSE ) ;
      setTransAutoRollback( autoRollback, FALSE ) ;
      setUseRollbackSemgent( useRBS, FALSE ) ;
      setTransRCCount( rcCount, FALSE ) ;
      setTransAllowLockEscalation( allowLockEscalation, FALSE ) ;
      setTransMaxLockNum( maxLockNum, FALSE ) ;
      setTransMaxLogSpaceRatio( maxLogSpaceRatio, FALSE ) ;

      _useTransLock        = TRUE ;
      _transConfVer        = 1 ;

      updateMaxLogSpace( totalLogSpace ) ;
   }

   BOOLEAN _dpsTransExecutor::updateTransConf( INT32 isolation,
                                               UINT32 timeout,
                                               BOOLEAN waitLock,
                                               BOOLEAN autoCommit,
                                               BOOLEAN autoRollback,
                                               BOOLEAN useRBS,
                                               BOOLEAN rcCount,
                                               BOOLEAN allowLockEscalation,
                                               INT32 maxLockNum,
                                               INT32 maxLogSpaceRatio,
                                               UINT64 totalLogSpace )
   {
      UINT32 oldTransConfVer = _transConfVer ;
      BOOLEAN updateAll = FALSE ;

      /// only timeout can update in transaction
      if ( !OSS_BIT_TEST( _transConfMask, TRANS_CONF_MASK_TIMEOUT ) )
      {
         setTransTimeout( timeout, FALSE ) ;
      }

      if ( DPS_INVALID_TRANS_ID == getExecutor()->getTransID() )
      {
         if ( !OSS_BIT_TEST( _transConfMask, TRANS_CONF_MASK_ISOLATION ) )
         {
            setTransIsolation( isolation, FALSE ) ;
         }
         if ( !OSS_BIT_TEST( _transConfMask, TRANS_CONF_MASK_WAITLOCK ) )
         {
            setTransWaitLock( waitLock, FALSE ) ;
         }
         if ( !OSS_BIT_TEST( _transConfMask, TRANS_CONF_MASK_AUTOCOMMIT ) )
         {
            setTransAutoCommit( autoCommit, FALSE ) ;
         }
         if ( !OSS_BIT_TEST( _transConfMask, TRANS_CONF_MASK_AUTOROLLBACK ) )
         {
            setTransAutoRollback( autoRollback, FALSE ) ;
         }
         if ( !OSS_BIT_TEST( _transConfMask, TRANS_CONF_MASK_USERBS ) )
         {
            setUseRollbackSemgent( useRBS, FALSE ) ;
         }
         if ( !OSS_BIT_TEST( _transConfMask, TRANS_CONF_MASK_RCCOUNT ) )
         {
            setTransRCCount( rcCount, FALSE ) ;
         }
         if ( !OSS_BIT_TEST( _transConfMask,
                             TRANS_CONF_MASK_ALLOWLOCKESCALATION ) )
         {
            setTransAllowLockEscalation( allowLockEscalation, FALSE ) ;
         }
         if ( !OSS_BIT_TEST( _transConfMask,
                             TRANS_CONF_MASK_MAXLOCKNUM ) )
         {
            setTransMaxLockNum( maxLockNum, FALSE ) ;
         }
         if ( !OSS_BIT_TEST( _transConfMask,
                             TRANS_CONF_MASK_MAXLOGSPACERATIO ) )
         {
            setTransMaxLogSpaceRatio( maxLogSpaceRatio, FALSE ) ;
            updateMaxLogSpace( totalLogSpace ) ;
         }
         updateAll = TRUE ;
      }

      if ( oldTransConfVer != _transConfVer )
      {
         _transConfVer = oldTransConfVer + 1 ;
      }
      return updateAll ;
   }

   void _dpsTransExecutor::copyTransConf( const dpsTransConfItem &conf,
                                          UINT64 totalLogSpace )
   {
      copyFrom( conf ) ;
      updateMaxLogSpace( totalLogSpace ) ;
   }

   void _dpsTransExecutor::updateTransConfByMask( const dpsTransConfItem &conf,
                                                  UINT64 totalLogSpace )
   {
      updateByMask( conf ) ;
      updateMaxLogSpace( totalLogSpace ) ;
   }

   void  _dpsTransExecutor::addReservedSpace( const UINT64 len )
   {
      _reservedLogSpace += len ;
   }

   void  _dpsTransExecutor::decReservedSpace( const UINT64 len )
   {
      SDB_ASSERT( _reservedLogSpace >= len,
                  "reserved log space is not enough" ) ;
      _reservedLogSpace -= len ;
   }

   void _dpsTransExecutor::addUsedSpace( const UINT64 len )
   {
      _usedLogSpace += len ;
   }

   UINT64 _dpsTransExecutor::getReservedSpace() const
   {
      return _reservedLogSpace ;
   }

   UINT64 _dpsTransExecutor::getUsedSpace() const
   {
      return _usedLogSpace ;
   }

   UINT64 _dpsTransExecutor::getLogSpace() const
   {
      return _usedLogSpace + _reservedLogSpace ;
   }

   void  _dpsTransExecutor::resetLogSpace()
   {
      _reservedLogSpace = 0 ;
      _usedLogSpace = 0 ;
   }

   INT32 _dpsTransExecutor::checkLogSpace( UINT64 usedLen,
                                           UINT64 reservedLen ) const
   {
      INT32 rc = SDB_OK ;

      // do not calculate with reserved size
      UINT64 totalLogSpace = _usedLogSpace + usedLen ;

      PD_CHECK( totalLogSpace <= _maxLogSpace,
                SDB_DPS_TRANS_LOG_SPACE_UP_TO_LIMIT, error, PDERROR,
                "Failed to check log space for transaction, "
                "used [%llu], reserved [%llu], required used [%llu], "
                "required reserved [%llu], total [%llu] > max [%llu]",
                _usedLogSpace, _reservedLogSpace, usedLen, reservedLen,
                totalLogSpace, _maxLogSpace ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   void _dpsTransExecutor::updateMaxLogSpace( UINT64 totalLogSpace )
   {
      SDB_ASSERT( 0 < _transMaxLogSpaceRatio,
                  "max log space ratio should be > 0" ) ;

      if ( 50 <= _transMaxLogSpaceRatio )
      {
         // at most half of log space can be used
         _maxLogSpace = totalLogSpace / 2 ;
      }
      else
      {
         FLOAT64 temp = (FLOAT64)totalLogSpace / 100.0 *
                        (FLOAT64)_transMaxLogSpaceRatio ;
         _maxLogSpace = (UINT64)( OSS_ROUND( temp ) ) ;
      }

      PD_LOG( PDDEBUG, "Update max log space to [%llu], "
              "total [%llu], ratio [%d]", _maxLogSpace, totalLogSpace,
              _transMaxLogSpaceRatio ) ;
   }

   void _dpsTransExecutor::commitMBStats ()
   {
      for ( TRANS_MB_STAT_MAP_IT iter = _transMBStatMap.begin() ;
            iter != _transMBStatMap.end() ;
            ++ iter )
      {
         iter->second.commit() ;
      }
      clearMBStats() ;
   }

   void _dpsTransExecutor::rollbackMBStats ()
   {
      for ( TRANS_MB_STAT_MAP_IT iter = _transMBStatMap.begin() ;
            iter != _transMBStatMap.end() ;
            ++ iter )
      {
         iter->second.rollback() ;
      }
      clearMBStats() ;
   }

   void _dpsTransExecutor::clearMBStats ()
   {
      _transMBStatMap.clear() ;
   }

   void _dpsTransExecutor::_initMBStat ( utilCLUniqueID clUniqueID,
                                         ossAtomic64 * totalRecords,
                                         UINT64 incDelta,
                                         UINT64 decDelta )
   {
      SDB_ASSERT( NULL != totalRecords, "total records should not be NULL" ) ;
      dpsTransMBStat stat( totalRecords, incDelta, decDelta ) ;
       _transMBStatMap.insert( std::make_pair( clUniqueID, stat ) ) ;
   }

   BOOLEAN _dpsTransExecutor::incMBTotalRecords ( utilCLUniqueID clUniqueID,
                                                  ossAtomic64 * totalRecords,
                                                  UINT64 delta )
   {
      if ( !UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
      {
         return FALSE ;
      }
      TRANS_MB_STAT_MAP_IT iter = _transMBStatMap.find( clUniqueID ) ;
      if ( iter == _transMBStatMap.end() )
      {
         _initMBStat( clUniqueID, totalRecords, delta, 0 ) ;
      }
      else
      {
         iter->second.increase( delta ) ;
      }
      return TRUE ;
   }

   BOOLEAN _dpsTransExecutor::decMBTotalRecords ( utilCLUniqueID clUniqueID,
                                                  ossAtomic64 * totalRecords,
                                                  UINT64 delta )
   {
      if ( !UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
      {
         return FALSE ;
      }
      TRANS_MB_STAT_MAP_IT iter = _transMBStatMap.find( clUniqueID ) ;
      if ( iter == _transMBStatMap.end() )
      {
         _initMBStat( clUniqueID, totalRecords, 0, delta ) ;
      }
      else
      {
         iter->second.decrease( delta ) ;
      }
      return TRUE ;
   }

   BOOLEAN _dpsTransExecutor::getMBTotalRecords ( utilCLUniqueID clUniqueID,
                                                  UINT64 & totalRecords ) const
   {
      if ( !UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
      {
         return FALSE ;
      }
      TRANS_MB_STAT_MAP_CIT citer = _transMBStatMap.find( clUniqueID ) ;
      if ( citer != _transMBStatMap.end() )
      {
         totalRecords = citer->second.getTotalRecords() ;
         return TRUE ;
      }
      return FALSE ;
   }

   void _dpsTransExecutor::finishLockWait()
   {
      SDB_ASSERT( TRUE == _lockWaitStarted, "No lock wait observed" ) ;
      ossTick end ;
      end.sample() ;
      _lockWaitTime = end - _lockWaitStartTimer ;

      if ( _monLock )
      {
         _monLock->waitTime = _lockWaitTime ;
         _monMgr->removeMonitorObject( _monLock ) ;
         _monLock = NULL ;
      }
      _lockWaitStarted = FALSE ;
   }

   // protect waiter and edu's lrb
   void _dpsTransExecutor::acquireLRBAccessingLock( LOCKMGR_TYPE lockMgrType )
   {
      _accessingLRBMutex.get() ;
   }

   // protect waiter and edu's lrb
   void _dpsTransExecutor::releaseLRBAccessingLock( LOCKMGR_TYPE lockMgrType )
   {
      _accessingLRBMutex.release() ;
   }

   void _dpsTransExecutor::setAccessingLRB( LOCKMGR_TYPE lockMgrType,
                                            dpsTransLRB *LRB )
   {
      _accessingTransLRB[ lockMgrType ] = LRB ;
   }

   dpsTransLRB *_dpsTransExecutor::getAccessingLRB( LOCKMGR_TYPE lockMgrType )
   {
      return _accessingTransLRB[ lockMgrType ] ;
   }

   // get the waiting LRB and lockId if this executor is waiting for a
   // trans lock and it has opened a transaction and has associated with
   // _tmsDataTransContext
   BOOLEAN _dpsTransExecutor::getTransWaitingLRBInfo
   (
      dpsTxWaitLRB & exctrWaitInfo,
      LOCKMGR_TYPE lockMgrType
   )
   {
      BOOLEAN result = FALSE ;
      {
         acquireLRBAccessingLock( lockMgrType ) ;
         dpsTransLRB *pLRB = _waiter[ lockMgrType ] ;
         if ( pLRB )
         {
            exctrWaitInfo.pLRB   = pLRB ;
            exctrWaitInfo.lockId = pLRB->lrbHdr->lockId ;
            result = TRUE;
         }
         releaseLRBAccessingLock( lockMgrType ) ;
      }
      return result ;
   }

   DPS_TRANS_ID _dpsTransExecutor::getNormalizedTransID()
   {
      return DPS_TRANS_GET_ID( getExecutor()->getTransID() ) ;
   }

   INT32 _dpsTransExecutor::checkLockEscalation( LOCKMGR_TYPE lockMgrType,
                                                 const dpsTransLockId &lockID,
                                                 BOOLEAN &needEscalate )
   {
      INT32 rc = SDB_OK ;

      needEscalate = FALSE ;

      // NOTE: only consider lock escalation in transaction
      if ( ( LOCKMGR_TRANS_LOCK == lockMgrType ) &&
           ( lockID.isSupportEscalation() ) &&
           ( DPS_INVALID_TRANS_ID != getTransID() ) &&
           !( getTransID() & DPS_TRANSID_ROLLBACKTAG_BIT ) )
      {
         // for transaction lock, we need escalate if already acquired too
         // many record locks to limit the resource of the transaction
         INT32 maxRecordLockNum = getTransMaxLockNum() ;
         UINT32 curRecordLockNum = getLeafLockCount( lockMgrType ) ;
         BOOLEAN curLockEscalated = isLockEscalated( lockMgrType ) ;

         needEscalate = curLockEscalated ||
                        ( 0 == maxRecordLockNum ||
                          ( 0 < maxRecordLockNum &&
                            curRecordLockNum > (UINT32)maxRecordLockNum ) ) ;

         // check if lock escalation is allowed
         PD_CHECK( isTransAllowLockEscalation() || !needEscalate,
                   SDB_DPS_TRANS_LOCK_UP_TO_LIMIT, error, PDERROR,
                   "Failed to check record locks for transaction, "
                   "acquired [%u], max [%u], allow escalation [%s]",
                   curRecordLockNum, maxRecordLockNum,
                   isTransAllowLockEscalation() ? "TRUE" : "FALSE" ) ;

         // set lock escalated if needed
         if ( needEscalate )
         {
            setLockEscalated( lockMgrType, TRUE ) ;
         }
      }

   done:
      return rc ;

   error:
      goto done ;
   }

}
