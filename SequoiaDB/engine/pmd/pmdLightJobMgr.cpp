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

   Source File Name = pmdLightJobMgr.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/13/2019  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdLightJobMgr.hpp"
#include "pmdEnv.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{
   /*
      Global define
   */
   #define PMD_LJOB_CONTROL_INTERVAL         ( 500 )     /// ms
   #define PMD_LJOB_INTERVAL                 ( 1000 )    /// ms
   #define PMD_LJOB_MIN_EXE_NUM              ( 1 )
   #define PMD_LJOB_DFT_MAX_EXE_NUM          ( 10 )
   #define PMD_LJOB_IDLE_TIMEOUT             ( 300 * OSS_ONE_SEC )
   #define PMD_LJOB_PER_AGENT_POWER          ( 12 )      /// 2^12
   #define PMD_LJOB_DISPATCH_TIME            ( 100 )     /// ms

   #define PMD_LJOB_EXE_TIME_SLICE           ( 1000 )    // ms
   #define PMD_LJOB_EXE_COUNT_SLICE          ( 10 )

   /*
      _pmdLightJobMgr implement
   */
   _pmdLightJobMgr::_pmdLightJobMgr()
   {
      _curAgent = 0 ;
      _idleAgent = 0 ;
      _maxExeJob = PMD_LJOB_DFT_MAX_EXE_NUM ;
      _startCtrlJob = FALSE ;
   }

   _pmdLightJobMgr::~_pmdLightJobMgr()
   {
   }

   void _pmdLightJobMgr::setMaxExeJob( UINT64 maxExeJob )
   {
      _maxExeJob = maxExeJob ;
      if ( _maxExeJob < PMD_LJOB_MIN_EXE_NUM )
      {
         _maxExeJob = PMD_LJOB_MIN_EXE_NUM ;
      }
   }

   void _pmdLightJobMgr::exitJob( BOOLEAN isControl )
   {
      _unitLatch.get() ;

      --_curAgent ;
      --_idleAgent ;
      if ( isControl )
      {
         _startCtrlJob = FALSE ;
      }
      _checkAndStartJob( FALSE ) ;

      _unitLatch.release() ;
   }

   void _pmdLightJobMgr::_checkAndStartJob( BOOLEAN needLock )
   {
      if ( needLock )
      {
         _unitLatch.get() ;
      }
      if ( size() > 0 && FALSE == _startCtrlJob && !pmdIsQuitApp() )
      {
         if ( SDB_OK == pmdStartLightJobExe( NULL, this, -1 ) )
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
   }

   void _pmdLightJobMgr::push( utilLightJob *pJob,
                               BOOLEAN takeOver,
                               INT32 priority,
                               UINT64 expectAvgCost )
   {
      _utilLightJobMgr::push( pJob, takeOver, priority, expectAvgCost ) ;
      _wakeUpEvent.signalAll() ;

      if ( FALSE == _startCtrlJob )
      {
         _checkAndStartJob( TRUE ) ;
      }
   }

   void _pmdLightJobMgr::push( const utilLightJobInfo &job )
   {
      _utilLightJobMgr::push( job ) ;
      _wakeUpEvent.signalAll() ;

      if ( FALSE == _startCtrlJob )
      {
         _checkAndStartJob( TRUE ) ;
      }
   }

   void _pmdLightJobMgr::pushBackJob( utilLightJobInfo &job,
                                      UTIL_LJOB_DO_RESULT result )
   {
      {
         ossScopedLock lock( &_unitLatch ) ;

         if ( UTIL_LJOB_DO_CONT == result )
         {
            /// add pending job
            _pendingJobVec.push_back( job ) ;
            job.reset() ;
         }
         /// inc idle agent
         ++_idleAgent ;
      }

      job.release() ;
   }

   BOOLEAN _pmdLightJobMgr::dispatchJob( utilLightJobInfo &job )
   {
      BOOLEAN hasJob = FALSE ;

      if ( pop( job, PMD_LJOB_DISPATCH_TIME ) )
      {
         hasJob = TRUE ;
         ossScopedLock lock( &_unitLatch ) ;
         /// dec idle agent
         --_idleAgent ;
      }
      else
      {
         _wakeUpEvent.reset() ;
      }

      return hasJob ;
   }

   UINT32 _pmdLightJobMgr::processPending()
   {
      LIGHT_JOB_VEC_IT it ;
      UINT32 num = 0 ;
      UINT32 ignoreNum = 0 ;
      UINT64 curTime = 0 ;
      LIGHT_JOB_VEC tmpJobVec ;

      ossScopedLock lock( &_unitLatch ) ;

      curTime = ossGetCurrentMicroseconds() ;

      /// push pending job to que
      it = _pendingJobVec.begin() ;
      while ( it != _pendingJobVec.end() )
      {
         utilLightJobInfo &info = *it ;

         if ( curTime < info.lastDoTime() + info.lastCost() +
                        info.expectSleepTime() )
         {
            /// wait for next time
            tmpJobVec.push_back( info ) ;
            ++ignoreNum ;
         }
         else if ( info.totalCost() < UTIL_LJOB_MIN_AVG_COST *
                                      info.totalTimes() )
         {
            // this light job executes too fast, may be it is endless loop?
            if ( info.totalTimes() < PMD_LJOB_EXE_COUNT_SLICE )
            {
               _utilLightJobMgr::push( info ) ;
               ++num ;
            }
            else if ( (UINT64)( curTime - info.lastDoTime() ) >
                      ( UTIL_LJOB_MIN_AVG_COST * info.totalTimes() ) << 2 )
            {
               info.resetStat() ;
               _utilLightJobMgr::push( info ) ;
               ++num ;
            }
            else
            {
               /// wait for next time
               tmpJobVec.push_back( info ) ;
               ++ignoreNum ;
            }
         }
         else if ( size() <= ( _idleAgent << 4 ) )
         {
            // there is a small amount of light jobs to process
            info.resetStat() ;
            _utilLightJobMgr::push( info ) ;
            ++num ;
         }
         else
         {
            UINT64 expectTotalCost = info.expectAvgCost() * info.totalTimes() ;
            if ( info.totalCost() > ( expectTotalCost << 4 ) )
            {
               // this light job takes too much time to execute
               if ( (UINT64)( curTime - info.lastDoTime() ) >=
                    ( PMD_LJOB_CONTROL_INTERVAL << 1 ) )
               {
                  info.resetStat() ;
               }
               /// wait for next time
               tmpJobVec.push_back( info ) ;
               ++ignoreNum ;
            }
            else
            {
               info.resetStat() ;
               _utilLightJobMgr::push( info ) ;
               ++num ;
            }
         }
         ++it ;
      }
      _pendingJobVec.clear() ;

      if ( ignoreNum > 0 )
      {
         _pendingJobVec = tmpJobVec ;
      }
      if ( num > 0 )
      {
         _wakeUpEvent.signalAll() ;
      }

      return num ;
   }

   void _pmdLightJobMgr::checkLoad()
   {
      UINT64 readyNum = size() ;

      ossScopedLock lock( &_unitLatch ) ;

      if ( readyNum > 0 )
      {
         /// wake up the agent
         _wakeUpEvent.signalAll() ;

         /// start agent
         while ( _curAgent < PMD_LJOB_MIN_EXE_NUM ||
                 ( ( readyNum >> PMD_LJOB_PER_AGENT_POWER ) > _idleAgent &&
                   _curAgent < _maxExeJob ) )
         {
            if ( SDB_OK == pmdStartLightJobExe( NULL, this,
                                                PMD_LJOB_IDLE_TIMEOUT ) )
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
   }

   void _pmdLightJobMgr::_onFini()
   {
      for ( LIGHT_JOB_VEC_IT it = _pendingJobVec.begin() ;
            it != _pendingJobVec.end() ; ++it )
      {
         _utilLightJobMgr::push( *it ) ;
      }
   }

   /*
      _pmdLightJobExe implement
   */
   _pmdLightJobExe::_pmdLightJobExe( pmdLightJobMgr *pJobMgr, INT32 timeout )
   {
      _pJobMgr = pJobMgr ;
      _timeout = timeout ;
   }

   _pmdLightJobExe::~_pmdLightJobExe()
   {
   }

   RTN_JOB_TYPE _pmdLightJobExe::type() const
   {
      return PMD_JOB_LIGHTJOB ;
   }

   const CHAR* _pmdLightJobExe::name() const
   {
      if ( isControlJob() )
      {
         return "LIGHT-JOB-D" ;
      }
      return "LIGHT-JOB" ;
   }

   BOOLEAN _pmdLightJobExe::isControlJob() const
   {
      return _timeout < 0 ? TRUE : FALSE ;
   }

   BOOLEAN _pmdLightJobExe::isSystem() const
   {
      return isControlJob() ;
   }

   BOOLEAN _pmdLightJobExe::muteXOn( const _rtnBaseJob *pOther )
   {
      return FALSE ;
   }

   INT32 _pmdLightJobExe::doit()
   {
      pmdEDUMgr *pEDUMgr = eduCB()->getEDUMgr() ;
      utilLightJobInfo job ;
      UINT32 timeout = 0 ;
      UTIL_LJOB_DO_RESULT result = UTIL_LJOB_DO_FINISH ;
      INT64 waitTime = isControlJob() ? PMD_LJOB_CONTROL_INTERVAL :
                                        PMD_LJOB_INTERVAL ;
      UINT64 costTime = 0 ;      /// micro second
      INT32 rcTmp = 0 ;

      while( !eduCB()->isForced() )
      {
         pEDUMgr->waitEDU( eduCB() ) ;

         if ( _pJobMgr->dispatchJob( job ) )
         {
            timeout = 0 ;
            pEDUMgr->activateEDU( eduCB() ) ;
            eduCB()->incEventCount( 1 ) ;

            UINT64 sliceTime = 0 ;
            UINT32 sliceCount = 0 ;

            while ( sliceTime < PMD_LJOB_EXE_TIME_SLICE &&
                    sliceCount < PMD_LJOB_EXE_COUNT_SLICE )
            {
               rcTmp = job.doit( eduCB(), result ) ;

               sliceTime += job.lastCost() ;
               ++sliceCount ;

               if ( UTIL_LJOB_DO_CONT != result )
               {
                  if ( rcTmp )
                  {
                     PD_LOG( PDWARNING, "Do light job[%s] failed, rc: %d",
                             job.getJob()->name(), rcTmp ) ;
                  }
                  break ;
               }
               else if ( job.expectSleepTime() > 0 )
               {
                  break ;
               }
            }

            costTime += sliceTime ;

            /// push back
            _pJobMgr->pushBackJob( job, result ) ;
         }
         else
         {
            timeout += PMD_LJOB_DISPATCH_TIME ;
            _pJobMgr->processPending() ;

            UINT64 bWaitTime = ossGetCurrentMilliseconds() ;

            if ( SDB_TIMEOUT == _pJobMgr->getEvent()->wait( waitTime ) )
            {
               timeout += waitTime ;
            }
            else
            {
               UINT64 eWaitTime = ossGetCurrentMilliseconds() ;
               if ( eWaitTime >= bWaitTime )
               {
                  timeout += ( eWaitTime - bWaitTime ) ;
               }
            }

            if ( isControlJob() )
            {
               costTime += ( timeout * 1000 ) ;
            }
            else if ( timeout > (UINT32)_timeout )
            {
               /// over _timeout millsecs, donothing, quit the job
               break ;
            }
         }

         if ( isControlJob() &&
              costTime / 1000 >= PMD_LJOB_CONTROL_INTERVAL )
         {
            _pJobMgr->checkLoad() ;
            _pJobMgr->processPending() ;
            costTime = 0 ;
         }
      }

      _pJobMgr->exitJob( isControlJob() ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMD_STARTLIGHTJOB, "pmdStartLightJobExe" )
   INT32 pmdStartLightJobExe( EDUID *pEDUID, pmdLightJobMgr *pJobMgr,
                              INT32 timeout )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMD_STARTLIGHTJOB ) ;
      pmdLightJobExe *pJob = NULL ;

      pJob = SDB_OSS_NEW pmdLightJobExe( pJobMgr, timeout ) ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc cache job failed" ) ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, pEDUID  ) ;
      /// neither failed or succeed, the pJob will release in job manager

   done:
      PD_TRACE_EXITRC( SDB__PMD_STARTLIGHTJOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}

