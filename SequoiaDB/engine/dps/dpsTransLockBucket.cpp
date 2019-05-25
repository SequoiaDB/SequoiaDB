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

   Source File Name = dpsTransLockBucket.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "dpsTransLockBucket.hpp"
#include "dpsTransLockUnit.hpp"
#include "dpsTransLockDef.hpp"
#include "pmdDef.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"
#include "pmd.hpp"

namespace engine
{
   UINT32 dpsLockBucket::_lockTimeout = 0 ;
   ossSpinXLatch dpsLockBucket::_initMutex ;

   dpsLockBucket::dpsLockBucket()
   {
      if ( 0 == _lockTimeout )
      {
         ossScopedLock _lock( &_initMutex );
         _lockTimeout = pmdGetOptionCB()->transTimeout() * 1000 ;
      }
   }

   dpsLockBucket::~dpsLockBucket()
   {
      dpsTransLockUnitList::iterator iterLst = _lockLst.begin();
      while( iterLst != _lockLst.end() )
      {
         SDB_OSS_DEL( iterLst->second );
         _lockLst.erase( iterLst++ );
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_ACQUIRE, "dpsLockBucket::acquire" )
   INT32 dpsLockBucket::acquire( _pmdEDUCB *eduCB,
                                 const dpsTransLockId &lockId,
                                 DPS_TRANSLOCK_TYPE lockType )
   {
      PD_TRACE_ENTRY( SDB_DPSLOCKBUCKET_ACQUIRE ) ;
      INT32 rc = SDB_OK;
      dpsTransLockUnit *pLockUnit = NULL;
      {
         ossScopedLock _lock( &_lstMutex );

         dpsTransLockUnitList::iterator iterLst
                                 = _lockLst.find( lockId );
         if ( _lockLst.end() == iterLst )
         {
            pLockUnit = SDB_OSS_NEW dpsTransLockUnit();
            if ( NULL == pLockUnit )
            {
               rc = SDB_OOM;
               goto error;
            }
            _lockLst[ lockId ] = pLockUnit;
         }
         else
         {
            pLockUnit = iterLst->second ;
         }

         rc = appendToRun( eduCB, lockType, pLockUnit );
         if ( rc )
         {
            appendToWait( eduCB, lockId, pLockUnit );
            rc = SDB_OK ;
         }
         else
         {
            goto done ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get the lock, append failed(rc=%d)",
                   rc );

   waitretry:
      rc = waitLock( eduCB );

      {
         ossScopedLock _lock_2( &_lstMutex );

         if ( rc )
         {
            removeFromWait( eduCB, pLockUnit, lockId );
            goto error ;
         }

         rc = appendToRun( eduCB, lockType, pLockUnit );
         if ( rc )
         {
            goto waitretry;
         }
         removeFromWait( eduCB, pLockUnit, lockId );
      }
   done:
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_ACQUIRE );
      return rc;
   error:
      PD_LOG ( PDERROR, "Failed to get the lock(rc=%d)", rc ) ;
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_WAITLOCKX, "dpsLockBucket::waitLockX" )
   INT32 dpsLockBucket::waitLockX( _pmdEDUCB *eduCB,
                                   const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSLOCKBUCKET_WAITLOCKX ) ;
      INT32 rc = SDB_OK;
      dpsTransLockUnit *pLockUnit = NULL;
      {
         ossScopedLock _lock( &_lstMutex );
         dpsTransLockUnitList::iterator iterLst
                                 = _lockLst.find( lockId );
         if ( _lockLst.end() == iterLst )
         {
            rc = SDB_SYS;
            goto error;
         }
         pLockUnit = iterLst->second;
      }

   waitretry:
      rc = waitLock( eduCB );

      {
         ossScopedLock _lock_2( &_lstMutex );

         if ( rc )
         {
            removeFromWait( eduCB, pLockUnit, lockId );
            goto error ;
         }

         rc = appendToRun( eduCB, DPS_TRANSLOCK_X, pLockUnit );
         if ( rc )
         {
            goto waitretry;
         }
         removeFromWait( eduCB, pLockUnit, lockId );
      }
   done:
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_WAITLOCKX );
      return rc;
   error:
      PD_LOG ( PDERROR, "Failed to get the lock(rc=%d)", rc );
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_UPGRADE, "dpsLockBucket::upgrade" )
   INT32 dpsLockBucket::upgrade( _pmdEDUCB *eduCB,
                                 const dpsTransLockId &lockId,
                                 DPS_TRANSLOCK_TYPE lockType )
   {
      PD_TRACE_ENTRY( SDB_DPSLOCKBUCKET_UPGRADE ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      INT32 rc = SDB_OK;

      dpsTransLockUnit *pLockUnit = NULL;
      {
         ossScopedLock _lock( &_lstMutex );
         dpsTransLockUnitList::iterator iterLst
                                 = _lockLst.find( lockId );
         if ( _lockLst.end() == iterLst )
         {
            pLockUnit = SDB_OSS_NEW dpsTransLockUnit();
            if ( NULL == pLockUnit )
            {
               rc = SDB_OOM;
               goto error;
            }
            _lockLst[ lockId ] = pLockUnit;
         }
         else
         {
            pLockUnit = iterLst->second ;
         }

         rc = appendToRun( eduCB, lockType, pLockUnit );
         if ( rc )
         {
            appendHeadToWait( eduCB, lockId, pLockUnit );
            rc = SDB_OK ;
         }
         else
         {
            goto done ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get the lock, append failed(rc=%d)",
                   rc );
   waitretry:
      rc = waitLock( eduCB );

      {
         ossScopedLock _lock_2( &_lstMutex );

         if ( rc )
         {
            removeFromWait( eduCB, pLockUnit, lockId );
            goto error ;
         }

         rc = appendToRun( eduCB, lockType, pLockUnit );
         if ( rc )
         {
            goto waitretry;
         }
         removeFromWait( eduCB, pLockUnit, lockId );
      }
   done:
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_UPGRADE );
      return rc;
   error:
      PD_LOG ( PDERROR, "Failed to upgrade the lock(rc=%d)", rc );
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_LOCKID, "dpsLockBucket::lockId" )
   void dpsLockBucket::release( _pmdEDUCB *eduCB,
                                const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSLOCKBUCKET_LOCKID ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;

      dpsTransLockUnit *pLockUnit = NULL;
      {
         ossScopedLock _lock( &_lstMutex );
         dpsTransLockUnitList::iterator iterLst
                                 = _lockLst.find( lockId );
         if ( _lockLst.end() == iterLst )
         {
            goto done ;
         }
         pLockUnit = iterLst->second ;

         removeFromRun(  eduCB, pLockUnit );
         if ( NULL == pLockUnit->_pWaitCB
            && pLockUnit->_runList.size() == 0 )
         {
            SDB_OSS_DEL( pLockUnit );
            _lockLst.erase( iterLst );
         }
      }
   done:
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_LOCKID );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_APPENDTORUN, "dpsLockBucket::appendToRun" )
   INT32 dpsLockBucket::appendToRun( _pmdEDUCB *eduCB,
                                     DPS_TRANSLOCK_TYPE lockType,
                                     dpsTransLockUnit *pLockUnit )
   {
      PD_TRACE_ENTRY( SDB_DPSLOCKBUCKET_APPENDTORUN ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      SDB_ASSERT( pLockUnit, "pLockUnit can't be null" ) ;
      INT32 rc = SDB_OK;
      if ( !checkCompatible( eduCB, lockType, pLockUnit) )
      {
         rc = SDB_DPS_TRANS_LOCK_INCOMPATIBLE;
         goto error;
      }

      pLockUnit->_runList[eduCB->getTID()] = lockType;
   done:
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_APPENDTORUN );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_APPENDTOWAIT, "dpsLockBucket::appendToWait" )
   void dpsLockBucket::appendToWait( _pmdEDUCB *eduCB,
                                     const dpsTransLockId &lockId,
                                     dpsTransLockUnit *pLockUnit  )
   {
      PD_TRACE_ENTRY( SDB_DPSLOCKBUCKET_APPENDTOWAIT ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      SDB_ASSERT( pLockUnit, "pLockUnit can't be null" ) ;
      dpsTransCBLockInfo *pLockInfo = NULL ;
      _pmdEDUCB *pWaitCB = pLockUnit->_pWaitCB;
      if ( NULL == pWaitCB )
      {
         pLockUnit->_pWaitCB = eduCB ;
         goto done;
      }
      while( TRUE )
      {
         pLockInfo =  pWaitCB->getTransLock( lockId );
         pWaitCB = pLockInfo->getNextWaitCB();
         if ( NULL == pWaitCB )
         {
            pLockInfo->setNextWaitCB( eduCB );
            break;
         }
      }
   done:
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_APPENDTOWAIT );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_APPENDHEADTOWAIT, "dpsLockBucket::appendHeadToWait" )
   void dpsLockBucket::appendHeadToWait( _pmdEDUCB *eduCB,
                                         const dpsTransLockId &lockId,
                                         dpsTransLockUnit *pLockUnit  )
   {
      PD_TRACE_ENTRY( SDB_DPSLOCKBUCKET_APPENDHEADTOWAIT ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      SDB_ASSERT( pLockUnit, "pLockUnit can't be null" ) ;
      dpsTransCBLockInfo *pLockInfo = NULL ;

      _pmdEDUCB *pWaitCB = pLockUnit->_pWaitCB;
      pLockUnit->_pWaitCB = eduCB;
      if ( pWaitCB != NULL )
      {
         pLockInfo = eduCB->getTransLock( lockId );
         pLockInfo->setNextWaitCB( pWaitCB );
      }
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_APPENDHEADTOWAIT );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_REMOVEFROMRUN, "dpsLockBucket::removeFromRun" )
   void dpsLockBucket::removeFromRun( _pmdEDUCB *eduCB,
                                      dpsTransLockUnit *pLockUnit  )
   {
      PD_TRACE_ENTRY( SDB_DPSLOCKBUCKET_REMOVEFROMRUN ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      SDB_ASSERT( pLockUnit, "pLockUnit can't be null" ) ;

      dpsTransLockRunList::iterator iterLst;
      pLockUnit->_runList.erase( eduCB->getTID() );

      if ( pLockUnit->_pWaitCB != NULL )
      {
         wakeUp( pLockUnit->_pWaitCB );
      }
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_REMOVEFROMRUN );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_REMOVEFROMWAIT, "dpsLockBucket::removeFromWait" )
   void dpsLockBucket::removeFromWait( _pmdEDUCB *eduCB,
                                       dpsTransLockUnit *pLockUnit,
                                       const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY ( SDB_DPSLOCKBUCKET_REMOVEFROMWAIT ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      SDB_ASSERT( pLockUnit, "pLockUnit can't be null" ) ;
      dpsTransCBLockInfo *pLockInfo = NULL ;
      UINT32 id = eduCB->getTID() ;
      _pmdEDUCB *pWaitCB = pLockUnit->_pWaitCB;
      SDB_ASSERT( pWaitCB, "waitCB can't be NULL" );

      if ( pWaitCB->getTID() == id )
      {
         pLockInfo = pWaitCB->getTransLock( lockId );
         if ( pLockInfo && pLockInfo->getNextWaitCB() )
         {
            pLockUnit->_pWaitCB = pLockInfo->getNextWaitCB();

            wakeUp( pLockUnit->_pWaitCB );
         }
         else
         {
            pLockUnit->_pWaitCB = NULL;
         }
         goto done ;
      }
      while( pWaitCB )
      {
         pLockInfo = pWaitCB->getTransLock( lockId );
         pWaitCB = pLockInfo->getNextWaitCB();
         if ( pWaitCB && pWaitCB->getTID() == id )
         {
            dpsTransCBLockInfo *pLockInfoTmp
                        = pWaitCB->getTransLock( lockId );
            if ( pLockInfoTmp )
            {
               pLockInfo->setNextWaitCB( pLockInfoTmp->getNextWaitCB() );
            }
            else
            {
               PD_LOG( PDWARNING, "Failed to get lock-info, "
                       "dead-lock may occured" );
               pLockInfo->setNextWaitCB( NULL );
            }
            break;
         }
      }
   done:
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_REMOVEFROMWAIT );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_WAITLOCK, "dpsLockBucket::waitLock" )
   INT32 dpsLockBucket::waitLock( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY ( SDB_DPSLOCKBUCKET_WAITLOCK ) ;
      INT32 rc = SDB_OK ;
      pmdEDUEvent event;

      if ( !eduCB->waitEvent( event, _lockTimeout ) )
      {
         rc = SDB_TIMEOUT ;
         goto error ;
      }

      if ( event._eventType != PMD_EDU_EVENT_LOCKWAKEUP )
      {
         rc = SDB_INTERRUPT ;
         eduCB->postEvent( event );
         goto error;
      }

   done:
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_WAITLOCK );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_WAKEUP, "dpsLockBucket::wakeUp" )
   void dpsLockBucket::wakeUp( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB_DPSLOCKBUCKET_WAKEUP ) ;
      eduCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_LOCKWAKEUP,
                                     PMD_EDU_MEM_NONE, NULL ));
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_WAKEUP );
   }

   BOOLEAN dpsLockBucket::isLockCompatible( DPS_TRANSLOCK_TYPE first,
                                            DPS_TRANSLOCK_TYPE second )
   {
      if (( first | second ) == DPS_TRANSLOCK_X )
      {
         return FALSE;
      }
      return TRUE;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_CHECKCOMPATIBLE, "dpsLockBucket::checkCompatible" )
   BOOLEAN dpsLockBucket::checkCompatible( _pmdEDUCB *eduCB,
                                           DPS_TRANSLOCK_TYPE lockType,
                                           dpsTransLockUnit *pLockUnit )
   {
      PD_TRACE_ENTRY( SDB_DPSLOCKBUCKET_CHECKCOMPATIBLE ) ;
      BOOLEAN isCompatible = TRUE;
      dpsTransLockRunList::iterator iterLst
                              = pLockUnit->_runList.begin() ;
      while( iterLst != pLockUnit->_runList.end() )
      {
         if ( iterLst->second == lockType &&
              DPS_TRANSLOCK_X != lockType )
         {
            break ;
         }
         if ( iterLst->first != eduCB->getTID() )
         {
            isCompatible = isLockCompatible( iterLst->second, lockType );
            if ( !isCompatible )
            {
               PD_LOG( PDERROR, "Lock conflicts!"
                       "(myTID:%d, myLockType:%d, curTID:%d, curLockType=%d)",
                       eduCB->getTID(), lockType, iterLst->first,
                       iterLst->second );
               break ;
            }
         }
         ++iterLst ;
      }
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_CHECKCOMPATIBLE );
      return isCompatible;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_TEST, "dpsLockBucket::test" )
   INT32 dpsLockBucket::test( _pmdEDUCB *eduCB,
                              const dpsTransLockId &lockId,
                              DPS_TRANSLOCK_TYPE lockType )
   {
      PD_TRACE_ENTRY( SDB_DPSLOCKBUCKET_TEST ) ;
      INT32 rc = SDB_OK;
      dpsTransLockUnit *pLockUnit = NULL;
      {
         ossScopedLock _lock( &_lstMutex );

         dpsTransLockUnitList::iterator iterLst
                                 = _lockLst.find( lockId );
         if ( _lockLst.end() == iterLst )
         {
            goto done;
         }
         else
         {
            pLockUnit = iterLst->second ;
         }
         if ( !checkCompatible( eduCB, lockType, pLockUnit) )
         {
            rc = SDB_DPS_TRANS_LOCK_INCOMPATIBLE ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_TEST );
      return rc;
   error:
      PD_LOG ( PDINFO, "Failed to test the lock(rc=%d)", rc );
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_TRYACQUIRE, "dpsLockBucket::tryAcquire" )
   INT32 dpsLockBucket::tryAcquire( _pmdEDUCB *eduCB,
                                    const dpsTransLockId &lockId,
                                    DPS_TRANSLOCK_TYPE lockType )
   {
      PD_TRACE_ENTRY( SDB_DPSLOCKBUCKET_TRYACQUIRE ) ;
      INT32 rc = SDB_OK;
      dpsTransLockUnit *pLockUnit = NULL;
      {
         ossScopedLock _lock( &_lstMutex );

         dpsTransLockUnitList::iterator iterLst
                                 = _lockLst.find( lockId );
         if ( _lockLst.end() == iterLst )
         {
            pLockUnit = SDB_OSS_NEW dpsTransLockUnit();
            if ( NULL == pLockUnit )
            {
               rc = SDB_OOM;
               goto error;
            }
            _lockLst[ lockId ] = pLockUnit;
         }
         else
         {
            pLockUnit = iterLst->second ;
         }

         rc = appendToRun( eduCB, lockType, pLockUnit );
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get the lock, append failed(rc=%d)",
                   rc );
   done:
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_TRYACQUIRE );
      return rc;
   error:
      PD_LOG ( PDERROR, "Failed to get the lock(rc=%d)", rc );
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_TRYACQUIREORAPPEND, "dpsLockBucket::tryAcquireOrAppend" )
   INT32 dpsLockBucket::tryAcquireOrAppend( _pmdEDUCB *eduCB,
                                            const dpsTransLockId &lockId,
                                            DPS_TRANSLOCK_TYPE lockType,
                                            BOOLEAN appendHead )
   {
      PD_TRACE_ENTRY( SDB_DPSLOCKBUCKET_TRYACQUIREORAPPEND ) ;
      INT32 rc = SDB_OK;
      dpsTransLockUnit *pLockUnit = NULL;
      {
         ossScopedLock _lock( &_lstMutex );

         dpsTransLockUnitList::iterator iterLst
                                 = _lockLst.find( lockId );
         if ( _lockLst.end() == iterLst )
         {
            pLockUnit = SDB_OSS_NEW dpsTransLockUnit();
            if ( NULL == pLockUnit )
            {
               rc = SDB_OOM;
               goto error;
            }
            _lockLst[ lockId ] = pLockUnit;
         }
         else
         {
            pLockUnit = iterLst->second ;
         }

         rc = appendToRun( eduCB, lockType, pLockUnit );

         if ( rc )
         {
            if ( appendHead )
            {
               appendHeadToWait( eduCB, lockId, pLockUnit );
            }
            else
            {
               appendToWait( eduCB, lockId, pLockUnit );
            }
            rc = SDB_DPS_TRANS_APPEND_TO_WAIT;
            goto done;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get the lock, append failed(rc=%d)",
                   rc );
   done:
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_TRYACQUIREORAPPEND );
      return rc;
   error:
      PD_LOG ( PDERROR, "Failed to get the lock(rc=%d)", rc );
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSLOCKBUCKET_HASWAIT, "dpsLockBucket::hasWait" )
   BOOLEAN dpsLockBucket::hasWait( const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSLOCKBUCKET_HASWAIT ) ;
      BOOLEAN result = FALSE;
      dpsTransLockUnit *pLockUnit = NULL;
      {
         ossScopedLock _lock( &_lstMutex );

         dpsTransLockUnitList::iterator iterLst
                                 = _lockLst.find( lockId );
         if ( iterLst != _lockLst.end())
         {
            pLockUnit = iterLst->second;
            if ( pLockUnit->_pWaitCB != NULL )
            {
               result = TRUE;
            }
         }
      }
      PD_TRACE_EXIT ( SDB_DPSLOCKBUCKET_HASWAIT );
      return result;
   }
}
