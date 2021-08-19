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

   Source File Name = rtnPrefetchJob.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/09/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnPrefetchJob.hpp"
#include "bps.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{

   _rtnPrefetchJob::_rtnPrefetchJob ( INT32 timeout )
   {
      _timeout = timeout ;
   }

   _rtnPrefetchJob::~_rtnPrefetchJob ()
   {
   }

   RTN_JOB_TYPE _rtnPrefetchJob::type () const
   {
      return RTN_JOB_PREFETCH ;
   }

   const CHAR* _rtnPrefetchJob::name () const
   {
      return "Job[Prefetcher]" ;
   }

   BOOLEAN _rtnPrefetchJob::muteXOn( const _rtnBaseJob * pOther )
   {
      return FALSE ;
   }

   INT32 _rtnPrefetchJob::doit ()
   {
      INT32 rc             = SDB_OK ;
      pmdKRCB *krcb        = pmdGetKRCB() ;
      SDB_BPSCB *bpscb     = krcb->getBPSCB() ;
      pmdEDUMgr *eduMgr    = krcb->getEDUMgr() ;
      ossQueue< bpsDataPref > *pQueue = bpscb->getPrefetchQueue() ;
      bpsDataPref request ;
      INT64 contextID = -1 ;
      UINT32 timeOut = 0 ;

      bpscb->_curPrefAgentNum.inc() ;

      while ( _timeout <= 0 || (INT32)timeOut <= _timeout )
      {
         eduMgr->waitEDU( eduCB() ) ;

         if ( !pQueue->timed_wait_and_pop( request, OSS_ONE_SEC ) )
         {
            timeOut += OSS_ONE_SEC ;

            if ( eduCB()->isDisconnected() )
            {
               break ;
            }
            continue ;
         }
         timeOut = 0 ;

         eduMgr->activateEDU( eduCB() ) ;
         eduCB()->incEventCount() ;

         if ( !request._context )
         {
            continue ;
         }

         // dec idle
         bpscb->_idlePrefAgentNum.dec() ;
         contextID = request._context->contextID() ;
         rc = request._context->prefetch( eduCB(), request._prefID ) ;
         if ( rc && SDB_DMS_EOC != rc )
         {
            PD_LOG( PDWARNING, "Prefetch context[%lld] failed, rc: %d",
                    contextID, rc ) ;
            rc = SDB_OK ;
         }
      }

      bpscb->_curPrefAgentNum.dec() ;
      bpscb->_idlePrefAgentNum.dec() ;
      return rc ;
   }

   INT32 startPrefetchJob( EDUID *pEDUID, INT32 timeout )
   {
      INT32 rc                = SDB_OK ;
      rtnPrefetchJob * pJob   = NULL ;

      pJob = SDB_OSS_NEW rtnPrefetchJob ( timeout ) ;
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

