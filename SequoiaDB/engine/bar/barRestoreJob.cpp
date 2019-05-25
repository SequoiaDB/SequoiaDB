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

   Source File Name = barRestoreJob.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/11/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "barRestoreJob.hpp"

namespace engine
{
   
   /*
      _barRestoreJob implement
   */
   _barRestoreJob::_barRestoreJob( barRSBaseLogger * pRSLogger )
   {
      _rsLogger = pRSLogger ;
   }

   _barRestoreJob::~_barRestoreJob ()
   {
      _rsLogger = NULL ;
   }

   RTN_JOB_TYPE _barRestoreJob::type() const
   {
      return RTN_JOB_RESTORE ;
   }

   const CHAR *_barRestoreJob::name() const
   {
      return "Restore" ;
   }

   BOOLEAN _barRestoreJob::muteXOn( const _rtnBaseJob * pOther )
   {
      return FALSE ;
   }

   INT32 _barRestoreJob::doit ()
   {
      return _rsLogger->restore( eduCB() ) ;
   }

   INT32 startRestoreJob( EDUID * pEDUID, barRSBaseLogger * pRSLogger )
   {
      INT32 rc                = SDB_OK ;
      barRestoreJob * pJob    = NULL ;

      pJob = SDB_OSS_NEW barRestoreJob ( pRSLogger ) ;
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


