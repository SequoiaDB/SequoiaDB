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
      INT32 rc = SDB_OK ;
      BOOLEAN bPop = FALSE ;
      MsgHeader *pHeader = NULL ;
      NET_HANDLE handle = NET_INVALID_HANDLE ;
      pmdEDUMemTypes memType = PMD_EDU_MEM_NONE ;

      while( !eduCB()->isForced() )
      {
         bPop = _pTaskAdapter->pop( OSS_ONE_SEC, &pHeader,
                                    handle, memType ) ;

         if ( bPop )
         {
            SDB_ASSERT( PMD_EDU_MEM_ALLOC == memType,
                        "Mem type must be PMD_EDU_MEM_ALLOC" ) ;

            rc = _pSessionMgr->dispatchMsg( handle, pHeader,
                                            memType, TRUE ) ;

            if ( rc )
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

