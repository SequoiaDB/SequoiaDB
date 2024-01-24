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

   Source File Name = utilLightJobBase.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/12/2019  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilLightJobBase.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"

namespace engine
{

   /*
      _utilLightJob implement
   */
   INT32 _utilLightJob::submit( BOOLEAN takeOver,
                                INT32 priority,
                                UINT64 expectAvgCost,
                                UINT64 *pJobID )
   {
      INT32 rc = SDB_OK ;
      _utilLightJob *pJob = this ;
      utilLightJobMgr *pMgr = utilGetGlobalJobMgr() ;
      UINT64 jobID = 0 ;
      SDB_ASSERT( pMgr, "Global job manager is NULL" ) ;
      SDB_ASSERT( expectAvgCost < UTIL_LJOB_MAX_AVG_COST,
                  "Expect avgCost is more than max" ) ;

      if ( !pMgr )
      {
         PD_LOG( PDERROR, "Global job manager is NULL" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         jobID = pMgr->allocID() ;
         pJob->_jobID = jobID ;
         pMgr->push( pJob, takeOver, priority, expectAvgCost ) ;
         pJob = NULL ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // copy job ID if needed
      if ( NULL != pJobID )
      {
         *pJobID = jobID ;
      }

   done:
      if ( pJob && takeOver )
      {
         SDB_OSS_DEL pJob ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _utilLightJobInfo implement
   */
   _utilLightJobInfo::_utilLightJobInfo()
   {
      reset() ;
   }

   _utilLightJobInfo::_utilLightJobInfo( utilLightJob *pJob,
                                         BOOLEAN takeOver,
                                         INT32 priority,
                                         UINT64 expectAvgCost )
   {
      _pJob = pJob ;
      _takeOver = takeOver ;
      _priority = adjustPriority( 0 - priority ) ;
      _orgPriority = _priority ;
      _expectAvgCost = adjustAvgCost( expectAvgCost ) ;

      _lastDoTime = 0 ;
      _lastCost = 0 ;
      _totalCost = 0 ;
      _totalTimes = 0 ;
      _sleepTime = 0 ;
   }

   _utilLightJobInfo::~_utilLightJobInfo()
   {
   }

   void _utilLightJobInfo::reset()
   {
      _pJob = NULL ;
      _takeOver = FALSE ;
      _priority = UTIL_LJOB_PRI_MID ;
      _orgPriority = UTIL_LJOB_PRI_MID ;
      _expectAvgCost = UTIL_LJOB_DFT_AVG_COST ;

      _lastDoTime = 0 ;
      _lastCost = 0 ;
      _totalCost = 0 ;
      _totalTimes = 0 ;
      _sleepTime = 0 ;
   }

   bool _utilLightJobInfo::operator< ( const _utilLightJobInfo &right ) const
   {
      if ( _priority < right._priority )
      {
         return true ;
      }
      return false ;
   }

   void _utilLightJobInfo::upPriority()
   {
      _priority = adjustPriority( _priority + 1 ) ;
   }

   void _utilLightJobInfo::downPriority()
   {
      _priority = adjustPriority( _priority - 1 ) ;
   }

   void _utilLightJobInfo::restorePriority()
   {
      _priority = _orgPriority ;
   }

   INT32 _utilLightJobInfo::adjustPriority( INT32 priority )
   {
      if ( priority < UTIL_LJOB_PRI_HIGHEST )
      {
         return UTIL_LJOB_PRI_HIGHEST ;
      }
      else if ( priority > UTIL_LJOB_PRI_LOWEST )
      {
         return UTIL_LJOB_PRI_LOWEST ;
      }
      return priority ;
   }

   UINT64 _utilLightJobInfo::adjustAvgCost( UINT64 avgCost )
   {
      if ( avgCost < UTIL_LJOB_MIN_AVG_COST )
      {
         return UTIL_LJOB_MIN_AVG_COST ;
      }
      else if ( avgCost > UTIL_LJOB_MAX_AVG_COST )
      {
         return UTIL_LJOB_MAX_AVG_COST ;
      }
      return avgCost ;
   }

   void _utilLightJobInfo::release()
   {
      if ( _pJob && _takeOver )
      {
         SDB_OSS_DEL _pJob ;
      }
      _pJob = NULL ;
      _takeOver = FALSE ;
   }

   INT32 _utilLightJobInfo::doit( IExecutor *pExe,
                                  UTIL_LJOB_DO_RESULT &result )
   {
      INT32 rc = SDB_OK ;
      result = UTIL_LJOB_DO_FINISH ;
      _sleepTime = UTIL_LJOB_DFT_AVG_COST ;

      if ( _pJob )
      {
         _lastDoTime = ossGetCurrentMicroseconds() ;

         rc = _pJob->doit( pExe, result, _sleepTime ) ;

         UINT64 eTime = ossGetCurrentMicroseconds() ;

         if ( eTime >= _lastDoTime )
         {
            _lastCost = eTime - _lastDoTime ;
            _totalCost += _lastCost ;
         }
         ++_totalTimes ;
      }

      return rc ;
   }

   FLOAT64 _utilLightJobInfo::avgCost() const
   {
      if ( _totalTimes > 0 && _totalCost > 0 )
      {
         return (FLOAT64)_totalCost / _totalTimes ;
      }
      return 0.0 ;
   }

   void _utilLightJobInfo::resetStat()
   {
      _lastDoTime = 0 ;
      _lastCost = 0 ;
      _totalCost = 0 ;
      _totalTimes = 0 ;
   }

   /*
      _utilLightJobMgr define
   */
   _utilLightJobMgr::_utilLightJobMgr()
   :_id( 1 )
   {
   }

   _utilLightJobMgr::~_utilLightJobMgr()
   {
      SDB_ASSERT( size() == 0, "Not empty" ) ;
      utilLightJobInfo job ;
      while( pop( job, 0 ) )
      {
         job.release() ;
      }
   }

   UINT64 _utilLightJobMgr::allocID()
   {
      return _id.inc() ;
   }

   void _utilLightJobMgr::fini( IExecutor *pExe )
   {
      _onFini() ;

      utilLightJobInfo job ;
      UTIL_LJOB_DO_RESULT result = UTIL_LJOB_DO_FINISH ;

      while( pop( job, 0 ) )
      {
         job.doit( pExe, result ) ;
         SDB_ASSERT( result == UTIL_LJOB_DO_FINISH,
                     "Not finished" ) ;
         job.release() ;
      }
   }

   UINT64 _utilLightJobMgr::size()
   {
      return _queue.size() ;
   }

   BOOLEAN _utilLightJobMgr::isEmpty()
   {
      return _queue.empty() ;
   }

   void _utilLightJobMgr::push( utilLightJob *pJob,
                                BOOLEAN takeOver,
                                INT32 priority,
                                UINT64 expectAvgCost )
   {
      _queue.push( utilLightJobInfo( pJob, takeOver, priority,
                                     expectAvgCost ) ) ;
   }

   void _utilLightJobMgr::push( const utilLightJobInfo &job )
   {
      _queue.push( job ) ;
   }

   BOOLEAN _utilLightJobMgr::pop( utilLightJobInfo &job, INT64 millisec )
   {
      BOOLEAN ret = FALSE ;

      if ( millisec < 0 )
      {
         _queue.wait_and_pop( job ) ;
         ret = TRUE ;
      }
      else if ( 0 == millisec )
      {
         ret = _queue.try_pop( job ) ;
      }
      else
      {
         ret = _queue.timed_wait_and_pop( job, millisec ) ;
      }

      return ret ;
   }

   /*
      Global var
   */
   static _utilLightJobMgr* g_pJobMgr = NULL ;

   utilLightJobMgr* utilGetGlobalJobMgr()
   {
      return g_pJobMgr ;
   }

   void utilSetGlobalJobMgr( utilLightJobMgr *pJobMgr )
   {
      if ( NULL == g_pJobMgr )
      {
         g_pJobMgr = pJobMgr ;
      }
      else if ( NULL == pJobMgr )
      {
         SDB_ASSERT( g_pJobMgr->size() == 0,
                     "Total size must be 0" ) ;
         g_pJobMgr = pJobMgr ;
      }
      else
      {
         SDB_ASSERT( FALSE, "Job manager is already valid" ) ;
      }
   }

}


