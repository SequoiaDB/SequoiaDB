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

   Source File Name = pmdMemPool.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdMemPool.hpp"
#include "ossMem.hpp"
#include "pd.hpp"
#include "pmdEnv.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{
   /*
      Global define
   */
   const UINT32 PMD_MEM_ALIGMENT_SIZE  = 1024 ;

   #define PMD_CACHE_JOB_INTERVAL            ( 100 )     /// ms
   #define PMD_MIN_CACHE_JOB                 ( 2 )
   #define PMD_CACHE_JOB_TIMEOUT             ( 300 * OSS_ONE_SEC )

   /*
      _pmdBuffPool implement
   */
   _pmdBuffPool::_pmdBuffPool()
   {
      _curAgent = 0 ;
      _idleAgent = 0 ;
      _maxCacheJob = PMD_MIN_CACHE_JOB ;
      _startCtrlJob = FALSE ;
      _perfStat = FALSE ;
   }

   _pmdBuffPool::~_pmdBuffPool()
   {
   }

   void _pmdBuffPool::setMaxCacheSize( UINT64 maxCacheSize )
   {
      _maxCacheSize = maxCacheSize * 1024 * 1024 ;
      _checkAndStartJob( TRUE ) ;
   }

   void _pmdBuffPool::setMaxCacheJob( UINT32 maxCacheJob )
   {
      _maxCacheJob = maxCacheJob ;
      if ( _maxCacheJob < PMD_MIN_CACHE_JOB )
      {
         _maxCacheJob = PMD_MIN_CACHE_JOB ;
      }
   }

   void _pmdBuffPool::enablePerfStat( BOOLEAN enable )
   {
      _perfStat = enable ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDBUFFPOOL_EXITJOB, "_pmdBuffPool::exitJob" )
   void _pmdBuffPool::exitJob( BOOLEAN isControl )
   {
      PD_TRACE_ENTRY ( SDB__PMDBUFFPOOL_EXITJOB ) ;
      _unitLatch.get() ;

      --_curAgent ;
      --_idleAgent ;
      if ( isControl )
      {
         _startCtrlJob = FALSE ;
      }
      _checkAndStartJob( FALSE ) ;

      _unitLatch.release() ;

      PD_TRACE_EXIT( SDB__PMDBUFFPOOL_EXITJOB ) ;
   }

   INT32 _pmdBuffPool::init( UINT64 cacheSize )
   {
      INT32 rc = _utilCacheMgr::init( cacheSize ) ;
      if ( rc )
      {
         goto error ;
      }
      // _checkAndStartJob( TRUE ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _pmdBuffPool::fini()
   {
      _utilCacheMgr::fini() ;

      SDB_ASSERT( 0 == _unitList.size(), "List size must be 0" ) ;
      SDB_ASSERT( 0 == _curAgent && 0 == _idleAgent,
                  "Agent must be 0" ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDBUFFPOOL_CHECKSTARTJOB, "_pmdBuffPool::_checkAndStartJob" )
   void _pmdBuffPool::_checkAndStartJob( BOOLEAN needLock )
   {
      PD_TRACE_ENTRY ( SDB__PMDBUFFPOOL_CHECKSTARTJOB ) ;
      if ( needLock )
      {
         _unitLatch.get() ;
      }
      if ( maxCacheSize() > 0 && _unitList.size() > 0 &&
           FALSE == _startCtrlJob && !pmdIsQuitApp() )
      {
         if ( SDB_OK == pmdStartCacheJob( NULL, this, -1 ) )
         {
            ++_curAgent ;
            ++_idleAgent ;
            _startCtrlJob = TRUE ;
         }
      }
      if ( needLock )
      {
         _unitLatch.release() ;
      }
      PD_TRACE_EXIT( SDB__PMDBUFFPOOL_CHECKSTARTJOB ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDBUFFPOOL_REGUNIT, "_pmdBuffPool::registerUnit" )
   void _pmdBuffPool::registerUnit( _utilCacheUnit *pUnit )
   {
      PD_TRACE_ENTRY ( SDB__PMDBUFFPOOL_REGUNIT ) ;
      ossScopedLock lock( &_unitLatch ) ;

      _unitList.push_back( pUnit ) ;

      _checkAndStartJob( FALSE ) ;

      PD_TRACE_EXIT( SDB__PMDBUFFPOOL_REGUNIT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDBUFFPOOL_PUSHUNIT, "_pmdBuffPool::pushBackUnit" )
   void _pmdBuffPool::pushBackUnit( _utilCacheUnit *pUnit )
   {
      PD_TRACE_ENTRY ( SDB__PMDBUFFPOOL_PUSHUNIT ) ;

      ossScopedLock lock( &_unitLatch ) ;

      if ( !pUnit->isClosed() )
      {
         _unitList.push_back( pUnit ) ;
      }
      /// release the page cleaner
      pUnit->unlockPageCleaner() ;
      /// inc idla agent
      ++_idleAgent ;

      PD_TRACE_EXIT( SDB__PMDBUFFPOOL_PUSHUNIT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDBUFFPOOL_DISPATCHUNIT, "_pmdBuffPool::dispatchUnit" )
   _utilCacheUnit* _pmdBuffPool::dispatchUnit()
   {
      _utilCacheUnit *pUnit = NULL ;
      LIST_UNIT::iterator it ;
      BOOLEAN force = FALSE ;
      PD_TRACE_ENTRY ( SDB__PMDBUFFPOOL_DISPATCHUNIT ) ;

      ossScopedLock lock( &_unitLatch ) ;

      if ( _unitList.empty() )
      {
         return NULL ;
      }

      it = _unitList.begin() ;
      while( it != _unitList.end() )
      {
         pUnit = *it ;

         if ( pUnit->canSync( force ) || pUnit->canRecycle( force ) )
         {
            _unitList.erase( it ) ;
            /// lock the unit
            pUnit->lockPageCleaner() ;
            /// dec idle agent
            --_idleAgent ;
            break ; 
         }
         else
         {
            pUnit = NULL ;
         }
         ++it ;
      }

      PD_TRACE_EXIT( SDB__PMDBUFFPOOL_DISPATCHUNIT ) ;
      return pUnit ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDBUFFPOOL_UNREGUNIT, "_pmdBuffPool::unregUnit" )
   void _pmdBuffPool::unregUnit( _utilCacheUnit *pUnit )
   {
      LIST_UNIT::iterator it ;
      PD_TRACE_ENTRY ( SDB__PMDBUFFPOOL_UNREGUNIT ) ;

      ossScopedLock lock( &_unitLatch ) ;

      it = _unitList.begin() ;
      while( it != _unitList.end() )
      {
         if ( (*it) == pUnit )
         {
            _unitList.erase( it ) ;
            break ;
         }
         ++it ;
      }

      PD_TRACE_EXIT( SDB__PMDBUFFPOOL_UNREGUNIT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDBUFFPOOL_CHECKLOAD, "_pmdBuffPool::checkLoad" )
   void _pmdBuffPool::checkLoad()
   {
      LIST_UNIT::iterator it ;
      UINT32 readyNum = 0 ;
      BOOLEAN force = FALSE ;
      PD_TRACE_ENTRY ( SDB__PMDBUFFPOOL_CHECKLOAD ) ;

      ossScopedLock lock( &_unitLatch ) ;

      it = _unitList.begin() ;
      while( it != _unitList.end() )
      {
         utilCacheUnit *pUnit = *it ;
         ++it ;
         if ( pUnit->canSync( force ) || pUnit->canRecycle( force ) )
         {
            ++readyNum ;
         }
      }

      if ( readyNum > 0 )
      {
         /// wake up the agent
         _wakeUpEvent.signalAll() ;

         /// start agent
         while ( _curAgent < PMD_MIN_CACHE_JOB ||
                 ( readyNum / 4 > _idleAgent &&
                   _curAgent < _maxCacheJob ) )
         {
            if ( SDB_OK == pmdStartCacheJob( NULL, this,
                                             PMD_CACHE_JOB_TIMEOUT ) )
            {
               ++_curAgent ;
               ++_idleAgent ;
            }
            else
            {
               break ;
            }
         }
      }

      PD_TRACE_EXIT( SDB__PMDBUFFPOOL_CHECKLOAD ) ;
   }

   /*
      _pmdCacheJob implement
   */
   _pmdCacheJob::_pmdCacheJob( pmdBuffPool *pBuffPool, INT32 timeout )
   {
      _pBuffPool = pBuffPool ;
      _timeout = timeout ;
   }

   _pmdCacheJob::~_pmdCacheJob()
   {
   }

   RTN_JOB_TYPE _pmdCacheJob::type() const
   {
      return PMD_JOB_CACHE ;
   }

   const CHAR* _pmdCacheJob::name() const
   {
      if ( isControlJob() )
      {
         return "CACHE-JOB-D" ;
      }
      return "CACHE-JOB" ;
   }

   BOOLEAN _pmdCacheJob::isControlJob() const
   {
      return _timeout < 0 ? TRUE : FALSE ;
   }

   BOOLEAN _pmdCacheJob::muteXOn( const _rtnBaseJob *pOther )
   {
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDCACHEJOB_DOIT, "_pmdCacheJob::doit" )
   INT32 _pmdCacheJob::doit()
   {
      pmdEDUMgr *pEDUMgr = eduCB()->getEDUMgr() ;
      utilCacheUnit *pUnit = NULL ;
      UINT32 timeout = 0 ;
      PD_TRACE_ENTRY ( SDB__PMDCACHEJOB_DOIT ) ;

      pEDUMgr->activateEDU( eduCB() ) ;

      while( !eduCB()->isForced() )
      {
         if ( isControlJob() )
         {
            _pBuffPool->checkLoad() ;

            if ( _pBuffPool->canRecycle() )
            {
               _pBuffPool->recycleBlocks() ;
               eduCB()->incEventCount( 1 ) ;
            }
            else
            {
               ossSleep( PMD_CACHE_JOB_INTERVAL ) ;
            }
         }
         else
         {
            pEDUMgr->waitEDU( eduCB() ) ;

            /// sync unit
            pUnit = _pBuffPool->dispatchUnit() ;
            if ( pUnit )
            {
               timeout = 0 ;
               _doUnit( pUnit ) ;

               if ( _pBuffPool->isEnabledPerfStat() )
               {
                  pUnit->dumpStatInfo() ;
               }
               /// push back
               _pBuffPool->pushBackUnit( pUnit ) ;
            }
            else
            {
               _pBuffPool->getEvent()->reset() ;
               if ( SDB_TIMEOUT ==
                    _pBuffPool->getEvent()->wait( OSS_ONE_SEC * 5 ) )
               {
                  timeout += ( 5 * OSS_ONE_SEC ) ;
               }
               else
               {
                  timeout += 100 ;
               }

               if ( timeout >= (UINT32)_timeout )
               {
                  /// over _timeout millsecs, donothing, qiut the job
                  break ;
               }
            }
         }
      }

      _pBuffPool->exitJob( isControlJob() ) ;

      PD_TRACE_EXIT( SDB__PMDCACHEJOB_DOIT ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDCACHEJOB__DOUNIT, "_pmdCacheJob::_doUnit" )
   void _pmdCacheJob::_doUnit( _utilCacheUnit *pUnit )
   {
      PD_TRACE_ENTRY ( SDB__PMDCACHEJOB__DOUNIT ) ;

      pmdEDUMgr *pEDUMgr = eduCB()->getEDUMgr() ;
      BOOLEAN force = FALSE ;
      UINT32 step = 0 ;

      pEDUMgr->activateEDU( eduCB() ) ;

      if ( pUnit->canSync( force ) )
      {
         pUnit->syncPages( eduCB(), force ) ;
         step = 1 ;
      }
      if ( pUnit->canRecycle( force ) )
      {
         pUnit->recyclePages( force ) ;
         step = 1 ;
      }
      eduCB()->incEventCount( step ) ;

      pEDUMgr->waitEDU( eduCB() ) ;

      PD_TRACE_EXIT( SDB__PMDCACHEJOB__DOUNIT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMD_STARTCACHEJOB, "pmdStartCacheJob" )
   INT32 pmdStartCacheJob( EDUID *pEDUID, pmdBuffPool *pBuffPool,
                           INT32 timeout )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMD_STARTCACHEJOB ) ;
      pmdCacheJob *pJob = NULL ;

      pJob = SDB_OSS_NEW pmdCacheJob( pBuffPool, timeout ) ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc cache job failed" ) ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, pEDUID  ) ;
      /// neither failed or succeed, the pJob will release in job manager

   done:
      PD_TRACE_EXITRC( SDB__PMD_STARTCACHEJOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _pmdMemPool implement
   */
   _pmdMemPool::_pmdMemPool()
   :_totalMemSize( 0 )
   {
   }

   _pmdMemPool::~_pmdMemPool()
   {
   }

   INT32 _pmdMemPool::initialize()
   {
      return SDB_OK ;
   }

   INT32 _pmdMemPool::final()
   {
      if ( 0 != totalSize() )
      {
         PD_LOG( PDERROR, "MemPool has memory leak: %llu", totalSize() ) ;
      }
      return SDB_OK ;
   }

   void _pmdMemPool::clear ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDMEMPOL_ALLOC, "_pmdMemPool::alloc" )
   CHAR *_pmdMemPool::alloc( UINT32 size, UINT32 &assignSize )
   {
      PD_TRACE_ENTRY ( SDB__PMDMEMPOL_ALLOC );

      CHAR *pBuffer = (CHAR*)SDB_POOL_ALLOC2( size, &assignSize ) ;
      if ( pBuffer )
      {
         _totalMemSize.add( assignSize ) ;
      }
      else
      {
         assignSize = 0 ;
         PD_LOG ( PDERROR, "Failed to allocate memory" ) ;
      }

      PD_TRACE_EXIT ( SDB__PMDMEMPOL_ALLOC );
      return pBuffer ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDMEMPOL_RELEASE, "_pmdMemPool::release" )
   void _pmdMemPool::release( CHAR* pBuff, UINT32 size )
   {
      PD_TRACE_ENTRY ( SDB__PMDMEMPOL_RELEASE );
      if ( pBuff && size > 0 )
      {
         SDB_POOL_FREE( pBuff ) ;
         _totalMemSize.sub( size ) ;
      }
      PD_TRACE_EXIT ( SDB__PMDMEMPOL_RELEASE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDMEMPOL_REALLOC, "_pmdMemPool::realloc" )
   CHAR *_pmdMemPool::realloc ( CHAR* pBuff, UINT32 srcSize,
                                UINT32 needSize, UINT32 &assignSize )
   {
      PD_TRACE_ENTRY ( SDB__PMDMEMPOL_REALLOC );
      CHAR *p = NULL ;
      if ( srcSize >= needSize )
      {
         assignSize = srcSize ;
         p = pBuff ;
         goto done ;
      }

      release ( pBuff, srcSize );

      p = alloc ( needSize, assignSize ) ;
   done :
      PD_TRACE_EXIT ( SDB__PMDMEMPOL_REALLOC );
      return p ;
   }
}

