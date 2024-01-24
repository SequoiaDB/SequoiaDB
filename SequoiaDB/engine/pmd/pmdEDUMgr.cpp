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

   Source File Name = pmdEDUMgr.cpp

   Descriptive Name = Process MoDel Engine Dispatchable Unit Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for thread pooling.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "core.hpp"
#include "pd.hpp"
#include "pmd.hpp"
#include "pmdEDUMgr.hpp"
#include "ossEDU.hpp"
#include "oss.hpp"
#include "utilMemListPool.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{

   ossAtomic64 __deadCheckAdjustTime( 0 ) ;

   void pmdUpdateDeadCheckWaitTime( INT64 timeout )
   {
      __deadCheckAdjustTime.swap( timeout ) ;
   }

   /*
      Local Define
   */
   #define PMD_EDU_IDLE_MAX_TIME             ( 1800 * OSS_ONE_SEC )  /// 30 mins
   #define PMD_EDU_IDLE_LOW_TIME             ( 10 * OSS_ONE_SEC ) /// 10 seconds
   #define PMD_EDU_IDLE_LOW_SIZE             ( 10 )

   #define PMD_FORCE_IO_INTERVAL             ( 200 )
   #define PMD_FORCE_EDU_INTERVAL            ( 100 )

   #define PMD_STOP_MIN_TIMEOUT              ( 60000 )   /// 1 min

   #define PMD_EDU_DUMP_WAIT_TIME            ( 500 ) // ms

   /*
      _pmdThreadPoolAssist define
   */
   class _pmdThreadPoolAssist
   {
      public:
         _pmdThreadPoolAssist()
         {
            if ( SDB_OK == _memPool.init() )
            {
               utilSetThreadMemPool( &_memPool ) ;
               _hasReg = TRUE ;
            }
            else
            {
               _hasReg = FALSE ;
            }
         }

         ~_pmdThreadPoolAssist()
         {
            if ( _hasReg )
            {
               utilSetThreadMemPool( NULL ) ;
            }
         }

         utilMemListPool* getRegisterMemPool()
         {
            return _hasReg ? &_memPool : NULL ;
         }

      private:
         utilMemListPool      _memPool ;
         BOOLEAN              _hasReg ;
   } ;
   typedef _pmdThreadPoolAssist pmdThreadPoolAssist ;

   /*
      _pmdEDUMgr implement
   */
   _pmdEDUMgr::_pmdEDUMgr() :
   _isDestroyed(FALSE),
   _isQuiesced(FALSE)
   {
      _pResource = NULL ;
      _pMonitorThd = NULL ;
      _pDeadCheckThd = NULL ;

      _EDUIDBase = 1 ;
   }

   _pmdEDUMgr::~_pmdEDUMgr()
   {
      reset () ;
   }

   void _pmdEDUMgr::addIOService( IIOService *pIOService )
   {
      ossScopedLock lock( &_latch, EXCLUSIVE ) ;
      _vecIOServices.push_back( pIOService ) ;
   }

   void _pmdEDUMgr::delIOSerivce( IIOService *pIOService )
   {
      ossScopedLock lock( &_latch, EXCLUSIVE ) ;

      VEC_IOSERVICE::iterator it = _vecIOServices.begin() ;
      while( it != _vecIOServices.end() )
      {
         if ( (*it) == pIOService )
         {
            _vecIOServices.erase( it ) ;
            break ;
         }
         ++it ;
      }
   }

   INT32 _pmdEDUMgr::init( IResource *pResource )
   {
      INT32 rc = SDB_OK ;

      if ( !pResource )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      _pResource = pResource ;

      /// create monitor thread
      try
      {
         _pMonitorThd = new boost::thread( boost::bind( &pmdEDUMgr::monitor,
                                                        this )
                                          ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDSEVERE, "Failed to create monitor thread: %s",
                  e.what() ) ;
         delete _pMonitorThd ;
         _pMonitorThd = NULL ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _pmdEDUMgr::reset( INT64 timeout )
   {
      BOOLEAN normalStop = TRUE ;

      if ( timeout > 0 && timeout < PMD_STOP_MIN_TIMEOUT )
      {
         timeout = PMD_STOP_MIN_TIMEOUT ;
      }

      if ( timeout > 0 )
      {
         normalStop = destroyAll( timeout ) ;
         if ( !normalStop )
         {
            /// check again
            normalStop = destroyAll( PMD_STOP_MIN_TIMEOUT ) ;
         }
      }
      else
      {
         normalStop = destroyAll( timeout ) ;
      }

      return normalStop ;
   }

   INT32 _pmdEDUMgr::startDeadCheck( INT64 timeout )
   {
      INT32 rc = SDB_OK ;

      /// create dead check thread
      try
      {
         _pDeadCheckThd = new boost::thread( boost::bind( &pmdEDUMgr::deadCheck,
                                                          this,
                                                          timeout )
                                          ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDSEVERE, "Failed to create dead check thread: %s",
                  e.what() ) ;
         delete _pDeadCheckThd ;
         _pDeadCheckThd = NULL ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _pmdEDUMgr::stopDeadCheck()
   {
      _deadCheckEvent.signal() ;

      if ( _pDeadCheckThd )
      {
         _pDeadCheckThd->join() ;
         delete _pDeadCheckThd ;
         _pDeadCheckThd = NULL ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR_DUMPINFO, "_pmdEDUMgr::dumpInfo" )
   INT32 _pmdEDUMgr::dumpInfo ( set<monEDUSimple> &info )
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR_DUMPINFO ) ;
      MAP_EDUCB_IT it ;
      INT32 rc = SDB_OK ;
      pmdEDUCB *cb = NULL ;
      monEDUSimple simple ;

      ossScopedLock lock( &_latch, SHARED ) ;

      try
      {
         for ( it = _mapRuns.begin () ; it != _mapRuns.end () ; ++it )
         {
            cb = it->second ;
            // If tid hasn't been set, maybe the thread hasn't started yet,
            // just igore it
            if ( cb->_hasSetTid )
            {
               cb->dumpInfo( simple ) ;
               info.insert(simple) ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Dump session information occur exception: %s",
                 e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__PMDEDUMGR_DUMPINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR_DUMPINFO2, "_pmdEDUMgr::dumpInfo" )
   INT32 _pmdEDUMgr::dumpInfo ( set<monEDUFull> &info )
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR_DUMPINFO2 ) ;
      MAP_EDUCB_IT it ;
      INT32 rc = SDB_OK ;
      pmdEDUCB *cb = NULL ;
      monEDUFull full ;

      ossScopedLock lock( &_latch, SHARED ) ;

      try
      {
         for ( it = _mapRuns.begin () ; it != _mapRuns.end () ; ++it )
         {
            cb = it->second ;
            // If tid hasn't been set, maybe the thread hasn't started yet,
            // just igore it
            if ( cb->_hasSetTid )
            {
               cb->dumpInfo( full ) ;
               info.insert(full) ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Dump session information occur exception: %s",
                 e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__PMDEDUMGR_DUMPINFO2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

#if defined( SDB_ENGINE )
   INT32 _pmdEDUMgr::beginDumpEDUTrans( EDUID eduID,
                                        pmdTransExecutor **executor,
                                        monTransInfo &transInfo,
                                        BOOLEAN bDumpTransInfo )
   {
      INT32 rc = SDB_OK ;
      MAP_EDUCB_IT it ;
      pmdEDUCB *cb = NULL ;
      BOOLEAN isNeedWaitDumpCount = FALSE ;

      while ( TRUE )
      {
         if ( isNeedWaitDumpCount )
         {
            _dumpCountEvent.wait( PMD_EDU_DUMP_WAIT_TIME ) ;
            isNeedWaitDumpCount = FALSE ;
         }

         ossScopedLock lock( &_latch, SHARED ) ;
         it = _mapRuns.find( eduID ) ;
         if ( _mapRuns.end() ==  it )
         {
            rc = SDB_PMD_SESSION_NOT_EXIST ;
            goto error ;
         }

         cb = it->second ;

         if ( PMD_EDU_DESTROY == cb->getStatus()
              || PMD_EDU_CREATING == cb->getStatus() )
         {
            // cb is destroying or creating, return error to skip this cb
            rc = SDB_PMD_SESSION_NOT_EXIST ;
            goto error ;
         }

         if ( DPS_INVALID_TRANS_ID == cb->getTransID() )
         {
            rc = SDB_DPS_TRANS_NO_TRANS ;
            goto error ;
         }

         if ( NULL == cb->getTransExecutor() )
         {
            rc = SDB_DPS_TRANS_NO_TRANS ;
            goto error ;
         }

         if ( !cb->checkAndSwapDumpTransCount( 0, 1 ) )
         {
            // other snapshot_trans is running
            isNeedWaitDumpCount = TRUE ;
            continue ;
         }

         if ( bDumpTransInfo )
         {
            cb->dumpTransInfo( transInfo ) ;
         }
         *executor = cb->getTransExecutor() ;
         break ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdEDUMgr::endDumpEDUTrans( EDUID eduID )
   {
      INT32 rc = SDB_OK ;
      MAP_EDUCB_IT it ;
      pmdEDUCB *cb = NULL ;

      ossScopedLock lock( &_latch, SHARED ) ;

      it = _mapRuns.find( eduID ) ;
      if ( _mapRuns.end() ==  it )
      {
         SDB_ASSERT( FALSE, "Shoule still in the run map!" ) ;
         rc = SDB_PMD_SESSION_NOT_EXIST ;
         goto error ;
      }

      cb = it->second ;
      if ( cb->checkAndSwapDumpTransCount( 1, 0 ) )
      {
         _dumpCountEvent.signalAll() ;
      }
      else
      {
         SDB_ASSERT( FALSE, "dump trans count must be 1" ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

#endif //SDB_ENGINE

   UINT32 _pmdEDUMgr::dumpAbnormalEDU()
   {
      UINT32 count = 0 ;
      MAP_EDUCB_IT it ;
      pmdEDUCB *cb = NULL ;

      ossScopedLock lock( &_latch, SHARED ) ;

      it = _mapRuns.begin() ;
      while( it != _mapRuns.end() )
      {
         cb = it->second ;
         ++it ;

         PD_LOG( PDERROR, "Run EDU[ID:%llu, TID:%u, Type:%s, Name:%s] is "
                 "abnormal", cb->getID(), cb->getTID(),
                 getEDUName( cb->getType() ), cb->getName() ) ;
         ++count ;
      }

      it = _mapIdles.begin() ;
      while( it != _mapIdles.end() )
      {
         cb = it->second ;
         ++it ;

         PD_LOG( PDERROR, "Idle EDU[ID:%llu, TID:%u, Type:%s, Name:%s] is "
                 "abnormal", cb->getID(), cb->getTID(),
                 getEDUName( cb->getType() ), cb->getName() ) ;
         ++count ;
      }

      return count ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR_DESTROYALL, "_pmdEDUMgr::destroyAll" )
   BOOLEAN _pmdEDUMgr::destroyAll( INT64 timeout )
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR_DESTROYALL ) ;

      BOOLEAN normalStop = TRUE ;
      INT64 itemTimeout = timeout > 0 ? ( timeout / 3 ) : 0x7FFFFFFF ;
      INT64 lastTimeout = 0 ;
      INT64 timeoutCounter = 0 ;

      setDestroyed( TRUE ) ;
      setQuiesced( TRUE ) ;
      _monitorEvent.signal() ;

      /// join monitor
      if ( _pMonitorThd )
      {
         _pMonitorThd->join() ;
         delete _pMonitorThd ;
         _pMonitorThd = NULL ;
      }

      //stop all ioservice
      timeoutCounter = 0 ;
      while ( countIOService() > 0 )
      {
         forceIOSerivce() ;
         ossSleepmillis( PMD_FORCE_IO_INTERVAL ) ;
         timeoutCounter += PMD_FORCE_IO_INTERVAL ;

         if ( timeoutCounter >= itemTimeout + lastTimeout )
         {
            normalStop = FALSE ;
            break ;
         }
      }
      /// fix lastTimeout value
      if ( timeoutCounter < itemTimeout + lastTimeout )
      {
         lastTimeout = itemTimeout + lastTimeout - timeoutCounter ;
      }

      //stop all user edus
      timeoutCounter = 0 ;
      UINT32 timeCounter = 0 ;
      UINT32 eduCount = _getEDUCount( EDU_USER ) ;
      while ( eduCount != 0 )
      {
         if ( 0 == timeCounter % 50 )
         {
            _forceEDUs( EDU_USER ) ;
         }
         ++timeCounter ;
         ossSleepmillis ( PMD_FORCE_EDU_INTERVAL ) ;
         timeoutCounter += PMD_FORCE_EDU_INTERVAL ;
         if ( timeoutCounter > itemTimeout + lastTimeout )
         {
            normalStop = FALSE ;
            break ;
         }
         eduCount = _getEDUCount( EDU_USER ) ;
      }
      /// fix lastTimeout value
      if ( timeoutCounter < itemTimeout + lastTimeout )
      {
         lastTimeout = itemTimeout + lastTimeout - timeoutCounter ;
      }

      //stop all system edus
      timeoutCounter = 0 ;
      timeCounter = 0 ;
      eduCount = _getEDUCount( EDU_ALL ) ;
      while ( eduCount != 0 )
      {
         if ( 0 == timeCounter % 50 )
         {
            _forceEDUs( EDU_ALL ) ;
         }

         ++timeCounter ;
         ossSleepmillis ( PMD_FORCE_EDU_INTERVAL ) ;
         timeoutCounter += PMD_FORCE_EDU_INTERVAL ;
         if ( timeoutCounter > itemTimeout + lastTimeout )
         {
            normalStop = FALSE ;
            break ;
         }
         eduCount = _getEDUCount( EDU_ALL ) ;
      }

      if ( normalStop )
      {
         while ( TRUE )
         {
            INT32 rc = _eduExitMutex.lock_w( PMD_FORCE_EDU_INTERVAL ) ;
            if ( SDB_OK == rc )
            {
               // got w lock, all EDUs have been released
               _eduExitMutex.release_w() ;
               break ;
            }
            else if ( SDB_TIMEOUT != rc )
            {
               // not timeout error
               normalStop = FALSE ;
               break ;
            }
            // retry
            timeoutCounter += PMD_FORCE_EDU_INTERVAL ;
            if ( timeoutCounter > ( timeout > 0 ? timeout : 0x7FFFFFFF ) )
            {
               normalStop = FALSE ;
               break ;
            }
         }
      }

      PD_TRACE_EXIT ( SDB__PMDEDUMGR_DESTROYALL ) ;
      return normalStop ;
   }

   BOOLEAN _pmdEDUMgr::_isSystemEDU( pmdEDUCB *cb )
   {
      pmdEPFactory &factory = pmdGetEPFactory() ;
      const pmdEPItem *pItem = NULL ;

      pItem = factory.getItem( cb->getType() ) ;
      if ( pItem && pItem->isSystem() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR_FORCEUSREDU, "_pmdEDUMgr::forceUserEDU" )
   INT32 _pmdEDUMgr::forceUserEDU( EDUID eduID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR_FORCEUSREDU ) ;
      MAP_EDUCB_IT it ;
      pmdEDUCB *cb = NULL ;
      BOOLEAN isSystem = FALSE ;

      ossScopedLock lock( &_latch, SHARED ) ;

      if ( ( it = _mapRuns.find( eduID ) ) != _mapRuns.end() )
      {
         cb = it->second ;
         isSystem = ( _isSystemEDU( cb ) || cb->isLocked() ) ;
      }
      else if ( ( it = _mapIdles.find( eduID ) ) != _mapIdles.end() )
      {
         cb = it->second ;
      }
      else
      {
         rc = SDB_PMD_SESSION_NOT_EXIST ;
         goto error ;
      }

      if ( isSystem )
      {
         PD_LOG ( PDERROR, "System EDU %d can't be forced", eduID ) ;
         rc = SDB_PMD_FORCE_SYSTEM_EDU ;
         goto error ;
      }

      cb->force() ;

   done :
      PD_TRACE_EXITRC ( SDB__PMDEDUMGR_FORCEUSREDU, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _pmdEDUMgr::interruptUserEDU( EDUID eduID )
   {
      INT32 rc = SDB_OK ;
      MAP_EDUCB_IT it ;
      pmdEDUCB *cb = NULL ;
      BOOLEAN isSystem = FALSE ;

      ossScopedLock lock( &_latch, SHARED ) ;

      if ( ( it = _mapRuns.find( eduID ) ) != _mapRuns.end() )
      {
         cb = it->second ;
         isSystem = ( _isSystemEDU( cb ) || cb->isLocked() ) ;
      }
      else if ( ( it = _mapIdles.find( eduID ) ) != _mapIdles.end() )
      {
         cb = it->second ;
      }
      else
      {
         rc = SDB_PMD_SESSION_NOT_EXIST ;
         goto error ;
      }

      if ( isSystem )
      {
         PD_LOG ( PDERROR, "System EDU %d can't be interrupt", eduID ) ;
         rc = SDB_PMD_FORCE_SYSTEM_EDU ;
         goto error ;
      }

      cb->interrupt( TRUE ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdEDUMgr::disconnectUserEDU( EDUID eduID )
   {
      INT32 rc = SDB_OK ;
      MAP_EDUCB_IT it ;
      pmdEDUCB *cb = NULL ;
      BOOLEAN isSystem = FALSE ;

      ossScopedLock lock( &_latch, SHARED ) ;

      if ( ( it = _mapRuns.find( eduID ) ) != _mapRuns.end() )
      {
         cb = it->second ;
         isSystem = ( _isSystemEDU( cb ) || cb->isLocked() ) ;
      }
      else if ( ( it = _mapIdles.find( eduID ) ) != _mapIdles.end() )
      {
         cb = it->second ;
      }
      else
      {
         rc = SDB_PMD_SESSION_NOT_EXIST ;
         goto error ;
      }

      if ( isSystem )
      {
         PD_LOG ( PDERROR, "System EDU %d can't be disconnect", eduID ) ;
         rc = SDB_PMD_FORCE_SYSTEM_EDU ;
         goto error ;
      }

      cb->disconnect() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _pmdEDUMgr::forceIOSerivce()
   {
      VEC_IOSERVICE::iterator it ;
      ossScopedLock lock( &_latch, EXCLUSIVE ) ;

      it = _vecIOServices.begin() ;
      while( it != _vecIOServices.end() )
      {
         (*it)->stop() ;
         ++it ;
      }
   }

   UINT32 _pmdEDUMgr::countIOService()
   {
      _latch.get_shared() ;
      UINT32 count = _vecIOServices.size() ;
      _latch.release_shared() ;

      return count ;
   }

   UINT32 _pmdEDUMgr::size()
   {
      _latch.get_shared() ;
      UINT32 count = _mapRuns.size() +  _mapIdles.size() ;
      _latch.release_shared() ;
      return count ;
   }

   UINT32 _pmdEDUMgr::sizeRun()
   {
      _latch.get_shared() ;
      UINT32 count = _mapRuns.size() ;
      _latch.release_shared() ;
      return count ;
   }

   UINT32 _pmdEDUMgr::sizeIdle()
   {
      _latch.get_shared() ;
      UINT32 count = _mapIdles.size() ;
      _latch.release_shared() ;
      return count ;
   }

   UINT32 _pmdEDUMgr::sizeSystem ()
   {
      _latch.get_shared() ;
      UINT32 count = _mapSystemEdu.size() ;
      _latch.release_shared() ;
      return count ;
   }

   UINT32 _pmdEDUMgr::sizeByType( INT32 type )
   {
      UINT32 count = 0 ;
      MAP_EDUCB_IT it ;
      _latch.get_shared() ;

      it = _mapRuns.begin() ;
      while( it != _mapRuns.end() )
      {
         if ( it->second->getType() == type )
         {
            ++count ;
         }
         ++it ;
      }
      _latch.release_shared() ;

      return count ;
   }

   UINT32 _pmdEDUMgr::sizeWithSession()
   {
      UINT32 count = 0 ;
      MAP_EDUCB_IT it ;
      _latch.get_shared() ;

      it = _mapRuns.begin() ;
      while( it != _mapRuns.end() )
      {
         if ( NULL != it->second->getSession() )
         {
            ++count ;
         }
         ++it ;
      }
      _latch.release_shared() ;

      return count ;
   }

   void _pmdEDUMgr::sizeInfo( UINT32 &runSize,
                              UINT32 &idleSize,
                              UINT32 &sysSize )
   {
      _latch.get_shared() ;
      runSize = _mapRuns.size() ;
      idleSize = _mapIdles.size() ;
      sysSize = _mapSystemEdu.size() ;
      _latch.release_shared() ;
   }

   EDUID _pmdEDUMgr::getSystemEDU( INT32 eduType )
   {
      EDUID eduID = PMD_INVALID_EDUID ;

      _latch.get_shared() ;
      MAP_SYSTEMEDU_IT it = _mapSystemEdu.find( eduType ) ;
      if ( it != _mapSystemEdu.end() )
      {
         eduID = it->second ;
      }
      _latch.release_shared() ;

      return eduID ;
   }

   BOOLEAN _pmdEDUMgr::isSystemEDU( EDUID eduID )
   {
      BOOLEAN isSystem = FALSE ;
      MAP_SYSTEMEDU_IT it ;

      _latch.get_shared() ;
      for ( it = _mapSystemEdu.begin() ; it != _mapSystemEdu.end() ; ++it )
      {
         if ( eduID == it->second )
         {
            isSystem = TRUE ;
            break ;
         }
      }
      _latch.release_shared() ;

      return isSystem ;
   }

   BOOLEAN _pmdEDUMgr::isQuiesced()
   {
      ossScopedLock lock( &_latch, SHARED ) ;
      return _isQuiesced ;
   }

   BOOLEAN _pmdEDUMgr::isDestroyed()
   {
      ossScopedLock lock( &_latch, SHARED ) ;
      return _isDestroyed ;
   }

   void _pmdEDUMgr::setDestroyed( BOOLEAN b )
   {
      ossScopedLock lock( &_latch, EXCLUSIVE ) ;
      _isDestroyed = b ;
   }

   void _pmdEDUMgr::setQuiesced( BOOLEAN b )
   {
      ossScopedLock lock( &_latch, EXCLUSIVE ) ;
      _isQuiesced = b ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR__FORCEEDUS, "_pmdEDUMgr::_forceEDUs" )
   void _pmdEDUMgr::_forceEDUs( INT32 property )
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR__FORCEEDUS ) ;
      MAP_EDUCB_IT it ;
      pmdEDUCB *cb = NULL ;

      ossScopedLock lock( &_latch, EXCLUSIVE ) ;

      for( it = _mapRuns.begin () ; it != _mapRuns.end () ; ++it )
      {
         cb = it->second ;

         if ( ( ( EDU_SYSTEM & property ) && _isSystemEDU( cb ) ) ||
              ( ( EDU_USER & property ) && !_isSystemEDU( cb ) ) )
         {
            cb->force() ;
            PD_LOG ( PDDEBUG, "Force edu[ID:%lld]", it->first ) ;
         }
      }

      for ( it = _mapIdles.begin () ; it != _mapIdles.end () ; ++it )
      {
         cb = it->second ;

         if ( EDU_USER & property )
         {
            cb->force () ;
         }
      }

      PD_TRACE_EXIT ( SDB__PMDEDUMGR__FORCEEDUS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR__GETEDUCNT, "_pmdEDUMgr::_getEDUCount" )
   UINT32 _pmdEDUMgr::_getEDUCount( INT32 property )
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR__GETEDUCNT ) ;
      UINT32 eduCount = 0 ;
      MAP_EDUCB_IT it ;
      pmdEDUCB *cb = NULL ;

      ossScopedLock lock( &_latch, EXCLUSIVE ) ;

      for( it = _mapRuns.begin () ; it != _mapRuns.end () ; ++it )
      {
         cb = it->second ;

         if ( ( ( EDU_SYSTEM & property ) && _isSystemEDU( cb ) ) ||
              ( ( EDU_USER & property ) && !_isSystemEDU( cb ) ) )
         {
            ++eduCount ;
         }
      }

      for ( it = _mapIdles.begin () ; it != _mapIdles.end () ; ++it )
      {
         if ( EDU_USER & property )
         {
            ++eduCount ;
         }
      }

      PD_TRACE1 ( SDB__PMDEDUMGR__GETEDUCNT, PD_PACK_UINT(eduCount) ) ;
      PD_TRACE_EXIT ( SDB__PMDEDUMGR__GETEDUCNT ) ;
      return eduCount ;
   }

   UINT32 _pmdEDUMgr::_interruptWritingEDUs( BOOLEAN withUserTrans,
                                             INT32 interruptRC )
   {
      MAP_EDUCB_IT it ;
      pmdEDUCB *cb = NULL ;
      UINT32 count = 0 ;

      _latch.get() ;
      for ( it = _mapRuns.begin () ; it != _mapRuns.end () ; ++it )
      {
         cb = it->second ;
         if ( cb->isWritingDB() )
         {
            ++count ;
            cb->interrupt( TRUE, interruptRC ) ;
            PD_LOG ( PDDEBUG, "Interrupt edu[ID:%lld]", it->first ) ;
         }
#if defined ( SDB_ENGINE )
         else if ( withUserTrans &&
                   !_isSystemEDU( cb ) &&
                   cb->isTransaction() )
         {
            ++count ;
            cb->interrupt( TRUE, interruptRC ) ;
            PD_LOG ( PDDEBUG, "Interrupt edu[ID:%lld]", it->first ) ;
         }
#endif
      }
      _latch.release() ;

      if ( count > 0 )
      {
         PD_LOG( PDEVENT, "Interrupt %d writing edus", count ) ;
      }
      return count ;
   }

   INT32 _pmdEDUMgr::_getWritingEDUs( INT32 eduTypeFilter,
                                      UINT64 idThreshold,
                                      EDU_BLOCK_TYPE excludeBlockType,
                                      PMD_EDU_PROCESS_LIST& writingEDUList )
   {
      pmdEDUCB *self = pmdGetThreadEDUCB() ;
      INT32 rc = SDB_OK ;

      ossScopedLock _lock( &_latch, SHARED ) ;

      for ( MAP_EDUCB_IT it = _mapRuns.begin () ; it != _mapRuns.end () ;
            ++ it )
      {
         pmdEDUCB *cb = it->second ;
         if ( self == cb )
         {
            continue ;
         }
         else if ( cb->isWritingDB() )
         {
            UINT64 opID = cb->getWritingID() ;

            if ( -1 != eduTypeFilter && eduTypeFilter != cb->getType() )
            {
               continue ;
            }
            else if ( 0 != idThreshold && opID > idThreshold )
            {
               continue ;
            }
            else if ( EDU_BLOCK_ALL == excludeBlockType && cb->isBlocked() )
            {
               continue ;
            }
            else if ( EDU_BLOCK_NONE != excludeBlockType &&
                      OSS_BIT_TEST( cb->getBlockType(), excludeBlockType ) )
            {
               continue ;
            }
            else
            {
               CHAR processName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
               ossStrncpy( processName,
                           cb->getCurProcessName(),
                           DMS_COLLECTION_FULL_NAME_SZ ) ;
               if ( 0 == processName[ 0 ] )
               {
                  continue ;
               }
               else if ( 0 != idThreshold && opID != cb->getWritingID() )
               {
                  // if the writingID has been changed, just discard it
                  continue ;
               }

               try
               {
                  pmdEDUProcessInfo info ;
                  info._opID = opID ;
                  info._eduID = cb->getID() ;
                  info._processName.assign( processName ) ;
                  writingEDUList.push_back( info ) ;
               }
               catch ( exception &e )
               {
                  rc = ossException2RC( &e ) ;
                  PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
               }
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _pmdEDUMgr::_hasWritingEDU( INT32 eduTypeFilter,
                                       UINT64 idThreshold,
                                       EDU_BLOCK_TYPE excludeBlockType )
   {
      BOOLEAN hasWriting = FALSE ;

      pmdEDUCB *self = pmdGetThreadEDUCB() ;

      ossScopedLock _lock( &_latch, SHARED ) ;

      for ( MAP_EDUCB_IT it = _mapRuns.begin () ;
            it != _mapRuns.end () ;
            ++ it )
      {
         pmdEDUCB *cb = it->second ;
         if ( self == cb )
         {
            continue ;
         }
         else if ( cb->isWritingDB() )
         {
            UINT64 opID = cb->getWritingID() ;

            if ( -1 != eduTypeFilter && eduTypeFilter != cb->getType() )
            {
               continue ;
            }
            else if ( 0 != idThreshold && opID > idThreshold )
            {
               continue ;
            }
            else if ( EDU_BLOCK_ALL == excludeBlockType && cb->isBlocked() )
            {
               continue ;
            }
            else if ( EDU_BLOCK_NONE != excludeBlockType &&
                      OSS_BIT_TEST( cb->getBlockType(), excludeBlockType ) )
            {
               continue ;
            }

            PD_LOG ( PDDEBUG, "Session [%lld] TID [%u] writing ID [%llu] "
                     "is writing", cb->getID(), cb->getTID(), opID ) ;
            hasWriting = TRUE ;
            break ;
         }
      }

      return hasWriting ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR_PSTEDUPST, "_pmdEDUMgr::postEDUPost" )
   INT32 _pmdEDUMgr::postEDUPost ( EDUID eduID,
                                   pmdEDUEventTypes type,
                                   pmdEDUMemTypes dataMemType,
                                   void *pData,
                                   UINT64 usrData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR_PSTEDUPST ) ;
      pmdEDUCB* eduCB = NULL ;
      MAP_EDUCB_IT it ;

      ossScopedLock lock( &_latch, SHARED ) ;

      if( _mapRuns.end () == ( it = _mapRuns.find ( eduID ) ) )
      {
         if( _mapIdles.end () == ( it = _mapIdles.find ( eduID ) ) )
         {
            rc = SDB_PMD_SESSION_NOT_EXIST ;
            goto error ;
         }
      }

      eduCB = it->second ;
      eduCB->postEvent( pmdEDUEvent ( type,
                                      dataMemType,
                                      pData,
                                      usrData ) ) ;
   done:
      PD_TRACE_EXITRC ( SDB__PMDEDUMGR_PSTEDUPST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   UINT32 _pmdEDUMgr::interruptWritingAndTransEDUs( INT32 interruptRC )
   {
      return _interruptWritingEDUs( TRUE, interruptRC ) ;
   }

   UINT32 _pmdEDUMgr::interruptWritingEDUS( INT32 interruptRC )
   {
      return _interruptWritingEDUs( FALSE, interruptRC ) ;
   }

   BOOLEAN _pmdEDUMgr::hasWritingEDU( INT32 eduTypeFilter,
                                      UINT64 idThreshold,
                                      EDU_BLOCK_TYPE excludeBlockType )
   {
      return _hasWritingEDU( eduTypeFilter, idThreshold, excludeBlockType ) ;
   }

   INT32 _pmdEDUMgr::getWritingEDUs( INT32 eduTypeFilter,
                                     UINT64 idThreshold,
                                     EDU_BLOCK_TYPE excludeBlockType,
                                     PMD_EDU_PROCESS_LIST& writingEDUList )
   {
      return _getWritingEDUs( eduTypeFilter, idThreshold, excludeBlockType,
                              writingEDUList ) ;
   }

   void _pmdEDUMgr::resetIOService()
   {
      VEC_IOSERVICE::iterator itIO ;
      IIOService *pIOService = NULL ;

      ossScopedLock lock( &_latch, SHARED ) ;

      itIO = _vecIOServices.begin() ;
      while( itIO != _vecIOServices.end() )
      {
         pIOService = *itIO ;
         pIOService->resetMon() ;
         ++itIO ;
      }
   }

   void _pmdEDUMgr::resetMon( EDUID eduID )
   {
      MAP_EDUCB_IT it ;
      pmdEDUCB *cb = NULL ;

      ossScopedLock lock( &_latch, SHARED ) ;

      for ( it = _mapRuns.begin () ; it != _mapRuns.end () ; ++it )
      {
         cb = it->second ;
         if ( PMD_INVALID_EDUID == eduID )
         {
            cb->resetMon() ;
         }
         else if ( eduID == it->first )
         {
            cb->resetMon() ;
            goto done ;
         }
      }

      for ( it = _mapIdles.begin () ; it != _mapIdles.end () ; ++it )
      {
         cb = it->second ;
         if ( PMD_INVALID_EDUID == eduID )
         {
            cb->resetMon() ;
         }
         else if ( eduID == it->first )
         {
            cb->resetMon() ;
            goto done ;
         }
      }

   done:
      return ;
   }

   UINT32 _pmdEDUMgr::_calIdleLowSize( UINT32 runSize,
                                       UINT32 idleSize,
                                       UINT32 sysSize,
                                       UINT32 poolSize )
   {
      UINT32 idleLowSize = 0 ;

      if ( poolSize > 0 && poolSize + sysSize > runSize )
      {
         poolSize += sysSize ;

         if ( poolSize - runSize > PMD_EDU_IDLE_LOW_SIZE )
         {
            idleLowSize = PMD_EDU_IDLE_LOW_SIZE ;
         }
         else
         {
            idleLowSize = poolSize - runSize ;
         }
      }
      return idleLowSize ;
   }

   UINT32 _pmdEDUMgr::calIdleLowSize( UINT32 *pRunSize,
                                      UINT32 *pIdleSize,
                                      UINT32 *pSysSize,
                                      UINT32 *pPoolSize )
   {
      UINT32 runSize = 0 ;
      UINT32 idleSize = 0 ;
      UINT32 sysSize = 0 ;
      UINT32 maxPool = pmdGetOptionCB()->getMaxPooledEDU() ;

      sizeInfo( runSize, idleSize, sysSize ) ;

      if ( pRunSize )
      {
         *pRunSize = runSize ;
      }
      if ( pIdleSize )
      {
         *pIdleSize = idleSize ;
      }
      if ( pSysSize )
      {
         *pSysSize = sysSize ;
      }
      if ( pPoolSize )
      {
         *pPoolSize = maxPool ;
      }

      return _calIdleLowSize( runSize, idleSize, sysSize, maxPool ) ;
   }

   BOOLEAN _pmdEDUMgr::forceDestory( pmdEDUCB *cb, UINT32 idleTime )
   {
      BOOLEAN destroyed = FALSE ;

      if ( idleTime <= PMD_EDU_IDLE_LOW_TIME ||
           PMD_EDU_CREATING == cb->getStatus() )
      {
         goto done ;
      }
      else
      {
         UINT32 runSize = 0 ;
         UINT32 idleSize = 0 ;
         UINT32 sysSize = 0 ;
         UINT32 maxPool = 0 ;
         UINT32 idleLowSize = 0 ;
         BOOLEAN isNeedWaitDump = FALSE ;

         idleLowSize = calIdleLowSize( &runSize, &idleSize,
                                       &sysSize, &maxPool ) ;
         if ( idleSize < idleLowSize )
         {
            goto done ;
         }
         else if ( runSize + idleSize - sysSize <= maxPool &&
                   idleTime <= PMD_EDU_IDLE_MAX_TIME )
         {
            goto done ;
         }

         /// destroy the edu
         while ( TRUE )
         {
            if ( isNeedWaitDump )
            {
               _dumpCountEvent.wait( PMD_EDU_DUMP_WAIT_TIME ) ;
               isNeedWaitDump = FALSE ;
            }

            MAP_EDUCB_IT it ;
            ossScopedLock lock( &_latch, EXCLUSIVE ) ;

            if ( _mapIdles.end() != ( it = _mapIdles.find( cb->getID() ) ) )
            {
               _mapIdles.erase( it ) ;
            }
            else if ( _mapRuns.end() != ( it = _mapRuns.find( cb->getID() ) ) )
            {
               if ( PMD_EDU_CREATING == cb->getStatus() )
               {
                  goto done ;
               }

               cb->setStatus( PMD_EDU_DESTROY ) ;
               if ( cb->getDumpTransCount() > 0 )
               {
                  isNeedWaitDump = TRUE ;
                  continue ;
               }

               _mapRuns.erase( it ) ;
            }
            else
            {
               SDB_ASSERT( FALSE, "EDU is not found" ) ;
               goto done ;
            }

            _postDestoryEDU( cb ) ;

            destroyed = TRUE ;

            break ;
         }
      }

   done:
      if ( destroyed )
      {
         pmdUndeclareEDUCB() ;
         SDB_OSS_DEL cb ;
      }
      return destroyed ;
   }

   void _pmdEDUMgr::_postDestoryEDU( pmdEDUCB *cb )
   {
      /// remove from system map
      _mapSystemEdu.erase( cb->getType() ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR_RTNEDU, "_pmdEDUMgr::returnEDU" )
   void _pmdEDUMgr::returnEDU( pmdEDUCB *cb, BOOLEAN &destroyed )
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR_RTNEDU ) ;
      BOOLEAN toDestory = FALSE ;
      BOOLEAN hasLock = FALSE ;
      pmdEPFactory &factory = pmdGetEPFactory() ;
      const pmdEPItem *pItem = NULL ;
      MAP_EDUCB_IT it ;
      BOOLEAN isNeedWaitDump = FALSE ;

      destroyed = FALSE ;

   retry:

      if ( isNeedWaitDump )
      {
         _dumpCountEvent.wait( PMD_EDU_DUMP_WAIT_TIME ) ;
         isNeedWaitDump = FALSE ;
      }

      _latch.get() ;
      hasLock = TRUE ;

      /// When cb is creating, do nothing
      if ( PMD_EDU_CREATING != cb->getStatus() )
      {
         if ( cb->isForced() || _isDestroyed )
         {
            toDestory = TRUE ;
         }
         else if ( cb->isLocked() )
         {
            toDestory = TRUE ;
         }
         else if ( 0 == pmdGetOptionCB()->getMaxPooledEDU() )
         {
            toDestory = TRUE ;
         }
         else if ( NULL == ( pItem = factory.getItem( cb->getType() ) ) ||
                   !pItem->isPoolable() )
         {
            toDestory = TRUE ;
         }

         /// In run map
         if ( ( it = _mapRuns.find( cb->getID() ) ) != _mapRuns.end() )
         {
            cb->setStatus( PMD_EDU_DESTROY ) ;
            if ( cb->getDumpTransCount() > 0 )
            {
               _latch.release() ;
               hasLock = FALSE ;
               isNeedWaitDump = TRUE ;
               goto retry ;
            }

            _mapRuns.erase( it ) ;

            /// add to idle map
            if ( !toDestory )
            {
               try
               {
                  while ( TRUE )
                  {
                     EDUID newID = _allocEDUID() ;

                     // check if has duplicated EDUID
                     if ( _mapIdles.insert( make_pair( newID, cb ) ).second )
                     {
                        cb->setID( newID ) ;
                        break ;
                     }
                  }

                  SDB_ASSERT( PMD_EDU_IDLE != cb->getStatus(),
                              "Status can't be idle" ) ;
                  cb->setStatus( PMD_EDU_IDLE ) ;
                  cb->setType( EDU_TYPE_UNKNOWN ) ;
               }
               catch( std::exception &e )
               {
                  PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
                  toDestory = TRUE ;
               }
            }
         }
         /// In idle map
         else if ( ( it = _mapIdles.find( cb->getID() ) ) != _mapIdles.end() )
         {
            if ( toDestory )
            {
               _mapIdles.erase( it ) ;
            }
         }
         /// Not found
         else
         {
            SDB_ASSERT( FALSE, "CB is not found" ) ;
            toDestory = TRUE ;
         }

         if ( toDestory )
         {
            _postDestoryEDU( cb ) ;

            _latch.release() ;
            hasLock = FALSE ;

            destroyed = TRUE ;
            pmdUndeclareEDUCB() ;
            SDB_OSS_DEL cb ;
         }
         else
         {
            cb->clear() ;
         }
      }

      if ( hasLock )
      {
         _latch.release() ;
      }
      PD_TRACE_EXIT( SDB__PMDEDUMGR_RTNEDU );
   }

   void _pmdEDUMgr::findAndRemove( pmdEDUCB *cb )
   {
      pmdEDUCB *removedCB = NULL ;
      BOOLEAN isNeedWaitDump = FALSE ;
      MAP_EDUCB_IT it ;

      SDB_ASSERT( NULL != cb, "eduCB should be valid" ) ;

   Retry:
      if ( isNeedWaitDump )
      {
         _dumpCountEvent.wait( PMD_EDU_DUMP_WAIT_TIME ) ;
         isNeedWaitDump = FALSE ;
      }

      _latch.get() ;
      EDUID eduID = cb->getID() ;
      if ( _mapIdles.end() != ( it = _mapIdles.find( eduID ) ) )
      {
         removedCB = it->second ;
         _mapIdles.erase( it ) ;
      }
      else if ( _mapRuns.end() != ( it = _mapRuns.find( eduID ) ) )
      {
         removedCB = it->second ;

         removedCB->setStatus( PMD_EDU_DESTROY ) ;
         if ( removedCB->getDumpTransCount() > 0 )
         {
            _latch.release() ;
            isNeedWaitDump = TRUE ;
            goto Retry ;
         }

         _mapRuns.erase( it ) ;
      }

      if ( removedCB )
      {
         SDB_ASSERT( removedCB == cb, "should be the same EDU" ) ;
         _postDestoryEDU( removedCB ) ;
      }

      _latch.release() ;
   }

   pmdEDUCB* _pmdEDUMgr::getFromPool( INT32 type )
   {
      pmdEDUCB *cb = NULL ;
      MAP_EDUCB_IT it ;
      UINT32 maxPool = pmdGetKRCB()->getOptionCB()->getMaxPooledEDU() ;

      ossScopedLock lock( &_latch, EXCLUSIVE ) ;

      it = _mapIdles.begin() ;
      if ( it != _mapIdles.end() )
      {
         cb = it->second ;
         // Make sure the cb is available
         if ( cb->getTID() )
         {
            try
            {
               EDUID runEDUID = cb->getID() ;
               while ( TRUE )
               {
                  // the ID is generated when
                  // - create and add into idle map
                  // - return to idle map
                  // so we can reuse the ID here

                  // but the ID checking of idle map and run map are individual,
                  // so still need to check if has duplicated EDUID in run map
                  if ( _mapRuns.insert( make_pair( runEDUID, cb ) ).second )
                  {
                     // moved to run map, erase from idle map
                     _mapIdles.erase( it ) ;
                     // new allocated EDUID
                     if ( runEDUID != cb->getID() )
                     {
                        cb->setID( runEDUID ) ;
                     }
                     break ;
                  }

                  // found duplicated one, allocate new EDUID and retry
                  runEDUID = _allocEDUID() ;
               }
            }
            catch ( std::exception &e )
            {
               cb = NULL ;
               PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
               goto error ;
            }

            cb->setType( type ) ;
            SDB_ASSERT( PMD_EDU_IDLE == cb->getStatus(), "Status must be idle" ) ;
            cb->setStatus( PMD_EDU_CREATING ) ;
         }
         else
         {
            cb = NULL ;
         }
      }

   done :
      if ( _mapIdles.size() < _calIdleLowSize( _mapRuns.size(),
                                               _mapIdles.size(),
                                               _mapSystemEdu.size(),
                                               maxPool ) )
      {
         /// Notify monitor thread
         _monitorEvent.signal() ;
      }
      return cb ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR_STARTEDU, "_pmdEDUMgr::startEDU" )
   INT32 _pmdEDUMgr::startEDU ( INT32 type,
                                void *args,
                                EDUID *pEDUID,
                                const CHAR *pInitName )
   {
      INT32     rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR_STARTEDU ) ;

      pmdEDUCB* eduCB = NULL ;
      pmdEPFactory &factory = pmdGetEPFactory() ;
      const pmdEPItem *pItem = NULL ;

      if ( isQuiesced() )
      {
         rc = SDB_QUIESCED ;
         goto done ;
      }

      pItem = factory.getItem( type ) ;
      if ( !pItem || NULL == pItem->_pFunc )
      {
         PD_LOG( PDERROR, "The edu[Type:%d] is not mapped", type ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !pItem->isSystem() && pItem->isPoolable() &&
           NULL != ( eduCB = getFromPool( type ) ) )
      {
         eduCB->clear() ;

         if ( pInitName )
         {
            eduCB->setName( pInitName ) ;
         }
         if ( pEDUID )
         {
            *pEDUID = eduCB->getID() ;
         }
         /// post resume event
         eduCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_RESUME,
                                        PMD_EDU_MEM_NONE,
                                        args ) ) ;
      }
      else
      {
         rc = createNewEDU( type, pItem->isSystem(),
                            args, pEDUID, pInitName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Create edu[Type:%d] failed, rc: %d",
                    type, rc ) ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__PMDEDUMGR_STARTEDU, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _pmdEDUMgr::createIdleEDU( EDUID *pEDUID )
   {
      INT32 rc       = SDB_OK ;
      pmdEDUCB *cb   = NULL ;
      EDUID newID = PMD_INVALID_EDUID ;
      ossEvent *pEvent = NULL ;
      pmdEventPtr ePtr ;
      INT32 result = SDB_OK ;

      if ( isQuiesced() )
      {
         rc = SDB_QUIESCED ;
         goto done ;
      }

      pEvent = SDB_OSS_NEW ossEvent() ;
      if ( !pEvent )
      {
         PD_LOG( PDERROR, "Failed to create event" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      pEvent->reset() ;
      ePtr = pmdEventPtr( pEvent ) ;

      cb = SDB_OSS_NEW pmdEDUCB ( this, EDU_TYPE_UNKNOWN ) ;
      if ( !cb )
      {
         PD_LOG( PDERROR, "Failed to allocate cb" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      cb->setName( "PoolIdle" ) ;

      // set to creating status
      cb->setStatus ( PMD_EDU_IDLE ) ;

      /// need to use ossScopedLock to prevent lock leak when exception
      {
         ossScopedLock lock( &_latch, EXCLUSIVE ) ;
         /// add to map
         try
         {
            while ( TRUE )
            {
               // allocate new EDUID
               newID = _allocEDUID() ;
               cb->setID( newID ) ;

               // check if has duplicated EDUID
               if ( _mapIdles.insert( make_pair( newID, cb ) ).second )
               {
                  break ;
               }
            }
         }
         catch ( std::exception &e )
         {
            SDB_OSS_DEL cb ;
            cb = NULL ;

            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Exception occurred: %s", e.what() ) ;
         }
      }

      // create a new thread here
      try
      {
         boost::thread agentThread ( boost::bind( &pmdEDUMgr::pmdEDUEntryPointWrapper,
                                                  this,
                                                  cb,
                                                  ePtr )
                                   ) ;
         // detach the agent so that he's all on his own
         // we only track based on CB
         agentThread.detach () ;
      }
      catch ( std::exception &e )
      {
         // if we failed to create thread, make sure to clean runqueue
         PD_LOG ( PDSEVERE, "Failed to create new edu: %s",
                  e.what() ) ;
         rc = SDB_SYS ;

         _latch.get() ;
         _mapIdles.erase( newID ) ;
         _latch.release() ;

         SDB_OSS_DEL cb ;

         goto error ;
      }

      /// Wait thread attach
      rc = pEvent->wait( -1, &result ) ;
      if ( SDB_OK == rc )
      {
         rc = result ;
      }
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to create new edu, rc: %d", rc ) ;
         goto error ;
      }

      if ( pEDUID )
      {
         *pEDUID = newID ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR_CRTNEWEDU, "_pmdEDUMgr::createNewEDU" )
   INT32 _pmdEDUMgr::createNewEDU ( INT32 type,
                                    BOOLEAN isSystem,
                                    void* arg,
                                    EDUID *pEDUID,
                                    const CHAR *pInitName )
   {
      INT32 rc       = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR_CRTNEWEDU );
      pmdEDUCB *cb   = NULL ;
      EDUID newID = PMD_INVALID_EDUID ;
      ossEvent *pEvent = NULL ;
      pmdEventPtr ePtr ;
      INT32 result = SDB_OK ;

      if ( isQuiesced() )
      {
         rc = SDB_QUIESCED ;
         goto done ;
      }

      pEvent = SDB_OSS_NEW ossEvent() ;
      if ( !pEvent )
      {
         PD_LOG( PDERROR, "Failed to create event" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      pEvent->reset() ;
      ePtr = pmdEventPtr( pEvent ) ;

      cb = SDB_OSS_NEW pmdEDUCB ( this, type ) ;
      if ( !cb )
      {
         PD_LOG( PDERROR, "Failed to allocate cb" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // set to creating status
      cb->setStatus ( PMD_EDU_CREATING ) ;
      if ( pInitName )
      {
         cb->setName( pInitName ) ;
      }

      /// need to use ossScopedLock to prevent lock leak when exception
      {
         ossScopedLock lock( &_latch, EXCLUSIVE ) ;
         try
         {
            /// add to map
            while ( TRUE )
            {
               // allocate new EDUID
               newID = _allocEDUID() ;
               cb->setID( newID ) ;

               // check if has duplicated EDUID
               if ( _mapRuns.insert( make_pair( newID, cb ) ).second )
               {
                  break ;
               }
            }

            /// add to system map
            if ( isSystem )
            {
               _mapSystemEdu[ type ] = newID ;
            }
            /// post resume event before thread start
            cb ->postEvent( pmdEDUEvent( PMD_EDU_EVENT_RESUME,
                                         PMD_EDU_MEM_NONE,
                                         arg ) ) ;
         }
         catch ( std::exception &e )
         {
            _mapRuns.erase( newID ) ;
            if ( isSystem )
            {
               _mapSystemEdu.erase( type ) ;
            }
            SDB_OSS_DEL cb ;
            cb = NULL ;

            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Exception occurred: %s", e.what() ) ;
         }
      }

      // create a new thread here
      try
      {
         boost::thread agentThread ( boost::bind( &pmdEDUMgr::pmdEDUEntryPointWrapper,
                                                  this,
                                                  cb,
                                                  ePtr )
                                    ) ;
         // detach the agent so that he's all on his own
         // we only track based on CB
         agentThread.detach () ;
      }
      catch ( std::exception &e )
      {
         // if we failed to create thread, make sure to clean runqueue
         PD_LOG ( PDSEVERE, "Failed to create new edu: %s",
                  e.what() ) ;
         rc = SDB_SYS ;

         _latch.get() ;
         _mapRuns.erase( newID ) ;
         if ( isSystem )
         {
            _mapSystemEdu.erase( type ) ;
         }
         _latch.release() ;

         SDB_OSS_DEL cb ;

         goto error ;
      }

      /// Wait thread attach
      rc = pEvent->wait( -1, &result ) ;
      if ( SDB_OK == rc )
      {
         rc = result ;
      }
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to create new edu, rc: %d", rc ) ;
         goto error ;
      }

      if ( pEDUID )
      {
         *pEDUID = newID ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__PMDEDUMGR_CRTNEWEDU, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR_WAITEDU, "_pmdEDUMgr::waitEDU" )
   INT32 _pmdEDUMgr::waitEDU( EDUID eduID )
   {
      INT32 rc        = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR_WAITEDU );
      MAP_EDUCB_IT it ;

      ossScopedLock lock( &_latch, SHARED ) ;

      it = _mapRuns.find( eduID ) ;
      if ( it != _mapRuns.end() )
      {
         rc = waitEDU( it->second ) ;
      }
      else
      {
         rc = SDB_SYS ;
      }

      PD_TRACE_EXITRC ( SDB__PMDEDUMGR_WAITEDU, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR_WAITEDU2, "_pmdEDUMgr::waitEDU" )
   INT32 _pmdEDUMgr::waitEDU( pmdEDUCB * cb )
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR_WAITEDU2 );

      INT32 rc = SDB_OK ;
      INT32 eduStatus = 0 ;

      if ( !cb )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      eduStatus = cb->getStatus() ;

      // if it's already waiting, let's do nothing
      if ( PMD_IS_EDU_WAITING ( eduStatus ) )
      {
         goto done ;
      }
      else if ( !PMD_IS_EDU_RUNNING( eduStatus ) &&
                !PMD_IS_EDU_CREATING( eduStatus ) )
      {
         // if it's not running status
         rc = SDB_EDU_INVAL_STATUS ;
         goto error ;
      }
      cb->setStatus ( PMD_EDU_WAITING ) ;

   done:
      PD_TRACE_EXITRC ( SDB__PMDEDUMGR_WAITEDU2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR_ATVEDU, "_pmdEDUMgr::activateEDU" )
   INT32 _pmdEDUMgr::activateEDU( EDUID eduID )
   {
      INT32   rc        = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR_ATVEDU );
      MAP_EDUCB_IT it ;

      ossScopedLock lock( &_latch, SHARED ) ;

      it = _mapRuns.find( eduID ) ;
      if ( it != _mapRuns.end() )
      {
         rc = activateEDU( it->second ) ;
      }
      else
      {
         rc = SDB_SYS ;
      }

      PD_TRACE_EXITRC ( SDB__PMDEDUMGR_ATVEDU, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR_ATVEDU2, "_pmdEDUMgr::activateEDU" )
   INT32 _pmdEDUMgr::activateEDU( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR_ATVEDU2 ) ;
      INT32  eduStatus = 0 ;

      if ( !cb )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      eduStatus = cb->getStatus() ;

      if ( PMD_IS_EDU_RUNNING( eduStatus ) )
      {
         goto done ;
      }
      else if ( !PMD_IS_EDU_WAITING ( eduStatus ) &&
                !PMD_IS_EDU_CREATING ( eduStatus ) )
      {
         rc = SDB_EDU_INVAL_STATUS ;
         goto error ;
      }
      cb->setStatus ( PMD_EDU_RUNNING ) ;

   done:
      PD_TRACE_EXITRC ( SDB__PMDEDUMGR_ATVEDU2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _pmdEDUMgr::lockEDU( pmdEDUCB *cb )
   {
      if ( cb )
      {
         cb->setLock( TRUE ) ;
      }
   }

   void _pmdEDUMgr::unlockEDU( pmdEDUCB *cb )
   {
      if ( cb )
      {
         cb->setLock( FALSE ) ;
      }
   }

   pmdEDUCB* _pmdEDUMgr::getEDUByID( EDUID eduID )
   {
      ossScopedLock lock( &_latch, SHARED ) ;
      return _getEDUByID( eduID ) ;
   }

   pmdEDUCB* _pmdEDUMgr::_getEDUByID( EDUID eduID )
   {
      MAP_EDUCB_IT it ;
      pmdEDUCB *cb = NULL ;

      if ( _mapRuns.end () == ( it = _mapRuns.find( eduID ) ) )
      {
         if ( _mapIdles.end () == ( it = _mapIdles.find( eduID ) ) )
         {
            goto done ;
         }
      }
      cb = it->second ;

   done:
      return cb ;
   }

   INT32 _pmdEDUMgr::getEDUTypeByID( EDUID eduID )
   {
      MAP_EDUCB_IT it ;
      pmdEDUCB *cb = NULL ;
      INT32 type = EDU_TYPE_UNKNOWN ;

      ossScopedLock lock( &_latch, SHARED ) ;

      if ( _mapRuns.end () == ( it = _mapRuns.find ( eduID ) ) )
      {
         if ( _mapIdles.end () == ( it = _mapIdles.find ( eduID ) ) )
         {
            goto done ;
         }
      }
      cb = it->second ;
      type = cb->getType() ;

   done:
      return type ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR_WAITUTIL, "_pmdEDUMgr::waitUntil" )
   INT32 _pmdEDUMgr::waitUntil( EDUID eduID,
                                EDU_STATUS status,
                                UINT32 waitPeriod,
                                UINT32 waitRound )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR_WAITUTIL ) ;
      MAP_EDUCB_IT it ;
      UINT32 round = 0 ;

      for ( round = 0; round < waitRound; ++round )
      {
         _latch.get_shared() ;
         if ( _mapRuns.end () == ( it = _mapRuns.find ( eduID ) ) )
         {
            if ( _mapIdles.end () == ( it = _mapIdles.find ( eduID ) ) )
            {
               rc = SDB_PMD_SESSION_NOT_EXIST ;
               _latch.release_shared () ;
               goto error ;
            }
         }
         if ( it->second->getStatus() == status )
         {
            _latch.release_shared () ;
            break ;
         }
         _latch.release_shared () ;
         ossSleepmillis( waitPeriod ) ;
      }

      if ( round == waitRound )
      {
         rc = SDB_TIMEOUT ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__PMDEDUMGR_WAITUTIL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR_WAITUTILBYTYPE, "_pmdEDUMgr::waitUntilByType" )
   INT32 _pmdEDUMgr::waitUntilByType( INT32 type,
                                      EDU_STATUS status,
                                      UINT32 waitPeriod,
                                      UINT32 waitRound )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR_WAITUTILBYTYPE ) ;
      EDUID eduID = getSystemEDU( type ) ;

      while ( waitRound > 0 && PMD_INVALID_EDUID == eduID )
      {
         ossSleepmillis( waitPeriod ) ;
         eduID = getSystemEDU( type ) ;
         --waitRound ;

         if ( PMD_IS_DB_DOWN() )
         {
            rc = SDB_APP_FORCED ;
            goto error ;
         }
      }

      if ( PMD_INVALID_EDUID == eduID )
      {
         rc = SDB_PMD_SESSION_NOT_EXIST ;
         goto error ;
      }

      rc = waitUntil( eduID, status, waitPeriod,
                      waitRound == 0 ? 1 : waitRound ) ;
   done:
      PD_TRACE_EXITRC ( SDB__PMDEDUMGR_WAITUTILBYTYPE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

#if defined (_LINUX)
   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUMGR_GETEDUTRDID, "_pmdEDUMgr::getEDUThreadID" )
   void _pmdEDUMgr::getEDUThreadID ( set<pthread_t> &tidList )
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUMGR_GETEDUTRDID ) ;
      MAP_EDUCB_IT it ;
      pmdEDUCB *cb = NULL ;

      try
      {
         ossScopedLock lock( &_latch, SHARED ) ;

         for ( it = _mapRuns.begin () ; it != _mapRuns.end () ; ++it )
         {
            cb = it->second ;
            tidList.insert ( cb->getThreadID() ) ;
         }

         for ( it = _mapIdles.begin () ; it != _mapIdles.end () ; ++it )
         {
            cb = it->second ;
            tidList.insert ( cb->getThreadID() ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to insert tid into set: %s", e.what() ) ;
      }
      PD_TRACE_EXIT ( SDB__PMDEDUMGR_GETEDUTRDID ) ;
   }

   void _pmdEDUMgr::killByThreadID( INT32 signo )
   {
      INT32 rc = SDB_OK ;
      MAP_EDUCB_IT it ;
      pmdEDUCB *cb = NULL ;

      try
      {
         ossScopedLock lock( &_latch, SHARED ) ;

         for ( it = _mapRuns.begin() ; it != _mapRuns.end() ; ++it )
         {
            cb = it->second ;
            // threadID was initialized to 0 in constructor, and set to real
            // thread id in pmdEDUEntryPoint
            if ( 0 == cb->getThreadID() ||
                 ossPThreadSelf() == cb->getThreadID() )
            {
               continue ;
            }
            rc = ossPThreadKill( cb->getThreadID (), signo ) ;
            if ( rc )
            {
               PD_LOG ( PDWARNING, "Failed to send signal %d to thread %llu, "
                        "errno = %d", signo, cb->getTID(),
                        ossGetLastError() ) ;
            }
         }

         for ( it = _mapIdles.begin() ; it != _mapIdles.end() ; ++it )
         {
            cb = it->second ;
            // threadID was initialized to 0 in constructor, and set to real
            // thread id in pmdEDUEntryPoint
            if ( 0 == cb->getThreadID() ||
                 ossPThreadSelf() == cb->getThreadID() )
            {
               continue ;
            }
            rc = ossPThreadKill( cb->getThreadID (), signo ) ;
            if ( rc )
            {
               PD_LOG ( PDWARNING, "Failed to send signal %d to thread %llu, "
                        "errno = %d", signo, cb->getTID(),
                        ossGetLastError() ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to pthread_kill tid: %s", e.what() ) ;
      }
   }
#endif //_LINUX

   void _pmdEDUMgr::monitor()
   {
      INT32 rc = SDB_OK ;
      EDUID eduID = PMD_INVALID_EDUID ;
      UINT32 runSize = 0 ;
      UINT32 idleSize = 0 ;
      UINT32 sysSize = 0 ;
      UINT32 poolSize = 0 ;
      UINT32 idleLowSize = 0 ;

      while( !isDestroyed() )
      {
         if ( SDB_TIMEOUT == _monitorEvent.wait( OSS_ONE_SEC * 5 ) )
         {
            continue ;
         }
         else if ( !_pResource->isActive() )
         {
            continue ;
         }

         idleLowSize = calIdleLowSize( &runSize, &idleSize,
                                       &sysSize, &poolSize ) ;
         while ( idleSize < idleLowSize )
         {
            rc = createIdleEDU( &eduID ) ;
            if ( rc )
            {
               if ( SDB_QUIESCED != rc )
               {
                  ossSleep( OSS_ONE_SEC ) ;
               }
               break ;
            }

            PD_LOG( PDDEBUG, "Create Idle edu[ID:%lld]", eduID ) ;
            idleLowSize = calIdleLowSize( &runSize, &idleSize,
                                          &sysSize, &poolSize ) ;
         }
      }
   }

   void _pmdEDUMgr::deadCheck( INT64 timeout )
   {
      INT32 rc = SDB_OK ;
      INT64 restTimeout = timeout > PMD_STOP_MIN_TIMEOUT ?
                          timeout : PMD_STOP_MIN_TIMEOUT ;
      INT64 adjustTime = 0 ;

      __deadCheckAdjustTime.swap( 0 ) ;

      while( TRUE )
      {
         rc = _deadCheckEvent.wait( OSS_ONE_SEC ) ;
         if ( SDB_OK == rc )
         {
            /// has safety quit
            break ;
         }

         restTimeout -= OSS_ONE_SEC ;
         adjustTime = __deadCheckAdjustTime.swap( 0 ) ;
         if ( adjustTime > 0 && restTimeout < adjustTime )
         {
            restTimeout = adjustTime ;
         }

         if ( restTimeout <= 0 )
         {
            /// dump abnormal edus
            dumpAbnormalEDU() ;
            /// panic the process
            PD_LOG( PDSEVERE, "The programme is dead over %d secs, "
                    "panic itself", timeout / OSS_ONE_SEC ) ;
            ossPanic() ;

            break ;
         }
      }
   }

   ///  edu entry point functions
   INT32 _pmdEDUMgr::pmdEDUEntryPointWrapper( pmdEDUCB *cb, pmdEventPtr ePtr )
   {
      INT32 rc = SDB_OK ;
      UINT32 tid = ossGetCurrentThreadID() ;
      BOOLEAN quitWithException = FALSE ;

      // save kernel thread id ( Linux ), or thread handle ( windows )
#if defined (_WINDOWS)
      HANDLE      tHdl = NULL ;
      BOOLEAN     isHdlCreated = FALSE ;
      if ( DuplicateHandle( GetCurrentProcess(), GetCurrentThread(),
                            GetCurrentProcess(), &tHdl, 0, false,
                            DUPLICATE_SAME_ACCESS ) )
      {
         isHdlCreated = TRUE ;
      }
#elif defined (_LINUX )
      OSSTID tHdl = ossGetCurrentThreadID() ;
      cb->setThreadID ( ossPThreadSelf() ) ;
#endif
      cb->setThreadHdl( tHdl ) ;
      cb->setTID( tid ) ;

      pmdThreadPoolAssist _thdPoolAssit ;
      cb->_pMemPool = _thdPoolAssit.getRegisterMemPool() ;

      // protect exit of current EDU
      ossScopedRWLock scopeLock( &_eduExitMutex, SHARED ) ;

      try
      {
         // register TLS, this must happen at very beginning of each thread
         rc = pmdEDUEntryPoint( pmdDeclareEDUCB( cb ),
                                ePtr,
                                quitWithException ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         // notify
         ePtr->signal( rc ) ;

         findAndRemove( cb ) ;
         EDUID eduID = cb->getID() ;
         SDB_OSS_DEL cb ;

         if ( quitWithException )
         {
            PD_LOG( PDSEVERE, "Thread[EDUID:%llu, TID:%u] occur "
                    "exception: %s. Restart DB", eduID, tid,
                    e.what() ) ;
            PMD_RESTART_DB( rc ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Thread[EDUID:%llu, TID:%u] occur "
                    "exception: %s", eduID, tid, e.what() ) ;
         }
      }

      // undeclare must happen after all TLS access
      pmdUndeclareEDUCB() ;

   #if defined (_WINDOWS)
      // close handle
      if ( isHdlCreated )
      {
         CloseHandle( tHdl ) ;
      }
   #endif

      return rc ;
   }

   // main entry point for all EDUs
   // it will call individual main function for each EDU type
   // entry points are defined in getEntryFuncByType
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDEDUENTPNT, "pmdEDUEntryPoint" )
   INT32 _pmdEDUMgr::pmdEDUEntryPoint( pmdEDUCB *cb,
                                       pmdEventPtr ePtr,
                                       BOOLEAN &quitWithException )
   {
      INT32       rc             = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDEDUENTPNT );

      pmdEPFactory &factory      = pmdGetEPFactory() ;
      const pmdEPItem *pItem     = NULL ;
      UINT32 idleTime            = 0 ;

      EDUID       myEDUID        = cb->getID() ;
      INT32       eduType        = cb->getType() ;
      pmdKRCB    *krcb           = pmdGetKRCB() ;
      pmdEDUMgr  *eduMgr         = ( pmdEDUMgr* )krcb->getExecutorMgr() ;
      IContextMgr *pCtxMgr       = krcb->getContextMgr() ;
      pmdEDUEvent event ;
      BOOLEAN     eduDestroyed   = FALSE ;
      CHAR        eduName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      PD_LOG ( PDEVENT, "Start thread[%u] for EDU[ID:%lld, type:%s, Name:%s]",
               cb->getTID(), cb->getID(), getEDUName( eduType ), cb->getName() ) ;

      // copy name
      ossStrncpy( eduName, cb->getName(), OSS_MAX_PATHSIZE ) ;

      /// Notify ok
      ePtr->signal() ;
      quitWithException = TRUE ;

      while( !eduDestroyed )
      {
         if ( !cb->waitEvent( event, OSS_ONE_SEC, FALSE ) )
         {
            idleTime += OSS_ONE_SEC ;

            if ( eduMgr->forceDestory( cb, idleTime ) )
            {
               utilClearThreadMemPool() ;
               break ;
            }
            continue ;
         }

         idleTime = 0 ;
         eduType = cb->getType() ;
         ossStrcpy( eduName, "PoolIdle" ) ;

         if ( cb->_pMemPool )
         {
            cb->_pMemPool->setName( getEDUName( eduType ) ) ;
         }

         // update edu ID
         myEDUID = cb->getID() ;

         if ( PMD_EDU_EVENT_RESUME == event._eventType )
         {
            PD_LOG( PDINFO, "Resume thread[%d] for EDU[ID:%lld, Type:%s]",
                    ossGetCurrentThreadID(), myEDUID, getEDUName( eduType ) ) ;

            pItem = factory.getItem( eduType ) ;
            if ( !pItem || !pItem->_pFunc )
            {
               PD_LOG( PDSEVERE, "EDU[type=%d] entry point func is NULL. "
                       "Shutdown DB", eduType ) ;
               rc = SDB_SYS ;
               PMD_SHUTDOWN_DB( SDB_SYS ) ;
            }

            eduMgr->waitEDU( cb ) ;
            pdInitCurAuditMask( pdGetAuditMask() ) ;

            *(cb->getMonConfigCB()) = *(krcb->getMonCB()) ;
            cb->initMonAppCB() ;
            cb->initConf() ;

            if ( cb->getOperator()->getGlobalID().isInvalid() &&
                 0 != pmdGetNodeID().columns.nodeID )
            {
               cb->initOperator() ;
            }

            rc = pItem->_pFunc( cb, event._Data ) ;
            // copy name
            ossStrncpy( eduName, cb->getName(), OSS_MAX_PATHSIZE ) ;

            if ( PMD_IS_DB_UP() )
            {
               if ( pItem->isSystem() )
               {
                  PD_LOG( PDSEVERE, "System EDU[ID:%lld, type:%s, Name:%s] "
                          "exit with %d. Restart DB", cb->getID(),
                          pItem->_name.c_str(), cb->getName(), rc ) ;
                  PMD_RESTART_DB( rc ) ;
               }
               else if ( SDB_OK != rc )
               {
                  PD_LOG( PDWARNING, "EDU[ID:%lld, type:%s, Name:%s] exit "
                          "with %d", cb->getID(), pItem->_name.c_str(),
                          cb->getName(), rc ) ;
               }
            }

            // set EDU status to wait
            eduMgr->waitEDU( cb ) ;
            //reset and clear
            cb->resetMon() ;
            cb->setMonQueryCB( NULL ) ;
            //delete all leak context
            if( pCtxMgr )
            {
               SINT64 contextID = -1 ;
               while ( -1 != ( contextID = cb->contextPeek() ) )
               {
                  pCtxMgr->contextDelete( contextID, NULL ) ;
                  PD_LOG ( PDWARNING, "EDU[%lld,%s] context[%d] leaked",
                           myEDUID, getEDUName( eduType ), contextID ) ;
               }
            }
            // make sure lock released
            cb->assertLocks() ;
         }
         else if ( PMD_EDU_EVENT_TERM != event._eventType )
         {
            PD_LOG( PDERROR, "Recieve the error event[type=%d] in "
                    "EDU[ID:%lld, type:%s]", event._eventType, myEDUID,
                    getEDUName( cb->getType() ) ) ;
            rc = SDB_SYS ;
         }

         // release the event data
         pmdEduEventRelease( event, cb ) ;
         event.reset() ;

         if ( pmdIsEnabledPerfStat() )
         {
            utilDumpThreadMemPoolInfo( getEDUName( eduType ), eduName ) ;
         }

         eduMgr->returnEDU( cb, eduDestroyed ) ;
         if ( !eduDestroyed )
         {
            PD_LOG( PDINFO, "Push thread[%d] for EDU[ID:%lld, NewID:%lld, Type:%s, "
                    "Name: %s] to thread pool", ossGetCurrentThreadID(),
                    myEDUID, cb->getID(), getEDUName( eduType ), eduName ) ;
         }

         utilClearThreadMemPool() ;
      }

      /// Call the thread exit hook function to release thread local variables
      if ( pmdGetEDUHook() )
      {
         PMD_ON_EDU_EXIT_FUNC pFunc = pmdGetEDUHook() ;
         pFunc() ;
      }

      PD_LOG ( PDEVENT, "Terminating thread[%d] for EDU[ID:%lld, Type:%s, "
               "Name: %s]", ossGetCurrentThreadID(), myEDUID,
               getEDUName( eduType ), eduName ) ;

      PD_TRACE_EXITRC ( SDB_PMDEDUENTPNT, rc );
      return rc ;
   }

   EDUID pmdEDUMgr::_allocEDUID()
   {
      // check if invalid ( PMD_INVALID_EDUID is for main thread )
      if ( PMD_INVALID_EDUID == _EDUIDBase )
      {
         _EDUIDBase = 1 ;
      }
      return _EDUIDBase ++ ;
   }

}

