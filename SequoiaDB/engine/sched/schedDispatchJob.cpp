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

   Source File Name = schedDispatchJob.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/28/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "schedDispatchJob.hpp"
#include "schedTaskAdapterBase.hpp"
#include "pmdAsyncSession.hpp"
#include "pd.hpp"

namespace engine
{

   /*
      _schedDispatchJob implement
   */
   _schedDispatchJob::_schedDispatchJob( _schedTaskAdapterBase *pTaskAdapter,
                                         _pmdAsycSessionMgr *pSessionMgr )
   {
      _pTaskAdapter = pTaskAdapter ;
      _pSessionMgr = pSessionMgr ;
   }

   _schedDispatchJob::~_schedDispatchJob()
   {
   }

   RTN_JOB_TYPE _schedDispatchJob::type() const
   {
      return RTN_JOB_SCHED_DISPATCH ;
   }

   const CHAR* _schedDispatchJob::name() const
   {
      return "SCHED-DISPATCH" ;
   }

   BOOLEAN _schedDispatchJob::muteXOn( const _rtnBaseJob *pOther )
   {
      return FALSE ;
   }

   INT32 _schedDispatchJob::doit()
   {
      BOOLEAN bPop = FALSE ;
      MsgHeader *pHeader = NULL ;
      NET_HANDLE handle = NET_INVALID_HANDLE ;
      pmdEDUMemTypes memType = PMD_EDU_MEM_NONE ;
      BOOLEAN hasDispatched = FALSE ;

      while( !eduCB()->isForced() )
      {
         bPop = _pTaskAdapter->pop( OSS_ONE_SEC, &pHeader,
                                    handle, memType ) ;

         if ( bPop )
         {
            _pSessionMgr->dispatchMsg( handle, pHeader,memType,
                                       TRUE,&hasDispatched ) ;
            if ( !hasDispatched )
            {
               SDB_OSS_FREE( ( CHAR* )pHeader ) ;
            }

            eduCB()->incEventCount() ;
         }
      }

      return SDB_OK ;
   }

   /*
      Gloable Functions implement
   */
   INT32 schedStartDispatchJob( _schedTaskAdapterBase *pTaskAdapter,
                                _pmdAsycSessionMgr *pSessionMgr,
                                EDUID *pEduID )
   {
      INT32 rc = SDB_OK ;
      _schedDispatchJob *pJob = NULL ;

      pJob = SDB_OSS_NEW _schedDispatchJob( pTaskAdapter, pSessionMgr ) ;
      if ( !pJob )
      {
         PD_LOG( PDERROR, "Allocate sched-dispatch job failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_REUSE,
                                     pEduID, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start sched-dispatch job failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

