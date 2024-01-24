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

   Source File Name = optPlanClearJob.cpp

   Descriptive Name = Optimizer Cached Plan Clear Job

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains background job to clear
   cached access plans.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/09/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "optPlanClearJob.hpp"
#include "pmd.hpp"
#include "optTrace.hpp"
#include "rtnCB.hpp"
#include "optAPM.hpp"

namespace engine
{

   #define OPT_PLANCLEARJOB_WAIT_INTERVAL ( OSS_ONE_SEC )
   // try clear expired cached plan for each 10 minutes
   #define OPT_PLANCLEARJOB_EXPIRE_INTERVAL ( OSS_ONE_SEC * 60 * 10 )

   /*
    *  _optPlanClearJob implement
    */
   _optPlanClearJob::_optPlanClearJob ()
   {
   }

   _optPlanClearJob::~_optPlanClearJob ()
   {
   }

   INT32 _optPlanClearJob::doit ()
   {
      pmdEDUCB *cb = eduCB() ;
      pmdEDUMgr *pEduMgr = pmdGetKRCB()->getEDUMgr() ;
      optAccessPlanManager *apm = sdbGetRTNCB()->getAPM() ;
      optCachedPlanMonitor *monitor = apm->getPlanMonitor() ;
      ossEvent *clearEvent = monitor->getClearEvent() ;

      UINT64 lastClearTick = pmdGetDBTick() ;

      while ( !PMD_IS_DB_DOWN() &&
              !cb->isForced() )
      {
         INT32 rc = SDB_OK ;

         /*
          * Before any one is found in the queue, the status of this thread is
          * wait. Once found, it will be changed to running.
          */
         pEduMgr->waitEDU( cb->getID() ) ;
         rc = clearEvent->wait( OPT_PLANCLEARJOB_WAIT_INTERVAL ) ;
         pEduMgr->activateEDU( cb ) ;

         // The database is shutting down
         if ( PMD_IS_DB_DOWN() ||
              cb->isForced() )
         {
            break ;
         }

         // No signals
         if ( SDB_TIMEOUT == rc )
         {
            if ( pmdGetTickSpanTime( lastClearTick ) >
                             OPT_PLANCLEARJOB_EXPIRE_INTERVAL )
            {
               // timeout to clear expired plans
               PD_LOG( PDDEBUG, "optPlanClearJob: start clearing "
                       "expired cached plans" ) ;
               monitor->clearExpiredCachedPlans() ;

               lastClearTick = pmdGetDBTick() ;
            }
            continue ;
         }

         // Got signal to clear cached plans
         PD_LOG( PDDEBUG, "optPlanClearJob: start clearing cached plans" ) ;
         monitor->clearCachedPlans() ;

         // Clear for too frequent signals
         clearEvent->reset() ;
         lastClearTick = pmdGetDBTick() ;
      } // End while

      PD_LOG( PDDEBUG, "optPlanClearJob: end job" ) ;

      return SDB_OK ;
   }

   INT32 startPlanClearJob ( EDUID *pEDUID )
   {
      INT32 rc = SDB_OK ;
      optPlanClearJob *pJob = NULL ;

      pJob = SDB_OSS_NEW optPlanClearJob() ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate failed" ) ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, pEDUID ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}
