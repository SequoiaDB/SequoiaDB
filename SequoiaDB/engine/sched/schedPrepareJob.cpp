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

   Source File Name = schedPrepareJob.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/28/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "schedPrepareJob.hpp"
#include "schedTaskAdapterBase.hpp"
#include "pd.hpp"

namespace engine
{

   /*
      _schedPrepareJob implement
   */
   _schedPrepareJob::_schedPrepareJob( _schedTaskAdapterBase *pTaskAdapter )
   {
      _pTaskAdapter = pTaskAdapter ;
   }

   _schedPrepareJob::~_schedPrepareJob()
   {
   }

   RTN_JOB_TYPE _schedPrepareJob::type() const
   {
      return RTN_JOB_SCHED_PREPARE ;
   }

   const CHAR* _schedPrepareJob::name() const
   {
      return "SCHED-PREPARE" ;
   }

   BOOLEAN _schedPrepareJob::muteXOn( const _rtnBaseJob *pOther )
   {
      return FALSE ;
   }

   INT32 _schedPrepareJob::doit()
   {
      UINT32 count = 0 ;

      while( !eduCB()->isForced() )
      {
         count = _pTaskAdapter->prepare( OSS_ONE_SEC ) ;
         eduCB()->incEventCount( count ) ;
      }

      return SDB_OK ;
   }

   /*
      Global function implement
   */
   INT32 schedStartPrepareJob( _schedTaskAdapterBase *pTaskAdapter,
                               EDUID *pEduID )
   {
      INT32 rc = SDB_OK ;
      _schedPrepareJob *pJob = NULL ;

      pJob = SDB_OSS_NEW _schedPrepareJob( pTaskAdapter ) ;
      if ( !pJob )
      {
         PD_LOG( PDERROR, "Allocate sched-prepare job failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_REUSE,
                                     pEduID, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start sched-prepare job failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

